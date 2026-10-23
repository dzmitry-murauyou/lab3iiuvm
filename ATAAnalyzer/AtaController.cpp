#include "stdafx.h"
#include "AtaController.h"
#include <winsvc.h>
#include <string.h>
#include <conio.h>
#include <sstream>
#include <iomanip>

// Биты регистра состояния
#define ATA_SR_BSY  0x80  // Бит 7: Busy
#define ATA_SR_DRDY 0x40  // Бит 6: Device Ready
#define ATA_SR_DRQ  0x08  // Бит 3: Data Request (готово к обмену данными)
#define ATA_SR_ERR  0x01  // Бит 0: Error

AtaController::AtaController() : m_initialized(false) {}

AtaController::~AtaController() {
    Cleanup();
}

bool AtaController::Initialize() {
    // Драйвер запускается один раз, повторные сканирования его не перезапускают.
    if (m_initialized) return true;
    m_lastError.clear();

    // HexIOWrapper ищет драйвер как <текущий каталог>\hexio.sys, а текущий
    // каталог в WinMain выставлен в папку с exe. Проверяем файл заранее,
    // иначе ошибка службы будет непонятной ("файл не найден" от SCM).
    char sysPath[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, sysPath, MAX_PATH);
    char* slash = strrchr(sysPath, '\\');
    if (slash != NULL) *(slash + 1) = '\0';
    strcat_s(sysPath, MAX_PATH, "HexIO.sys");

    if (GetFileAttributesA(sysPath) == INVALID_FILE_ATTRIBUTES) {
        m_lastError = std::string("Не найден файл драйвера рядом с exe:\r\n") + sysPath;
        return false;
    }

    // Служба hexio могла остаться от прошлых запусков с путём к уже
    // удалённой папке - тогда StartService всегда будет падать.
    RemoveStaleService(sysPath);

    if (!m_hexIO.StartUp()) return false;

    // Без разрешения на доступ к портам вызовы _inp/_outp недопустимы.
    if (!m_hexIO.AllowExclusiveAccess()) {
        m_hexIO.ShutDown();
        return false;
    }

    m_initialized = true;
    return true;
}

void AtaController::RemoveStaleService(const char* expectedSysPath) {
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm == NULL) return;

    SC_HANDLE svc = OpenServiceA(scm, "hexio",
        SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);
    if (svc == NULL) {
        CloseServiceHandle(scm);
        return;  // Службы нет - удалять нечего
    }

    // Сравниваем зарегистрированный путь к .sys с текущим
    bool stale = true;
    DWORD bytesNeeded = 0;
    QueryServiceConfigA(svc, NULL, 0, &bytesNeeded);
    if (bytesNeeded > 0) {
        std::vector<char> buf(bytesNeeded);
        QUERY_SERVICE_CONFIGA* cfg = (QUERY_SERVICE_CONFIGA*)&buf[0];
        if (QueryServiceConfigA(svc, cfg, bytesNeeded, &bytesNeeded) &&
            cfg->lpBinaryPathName != NULL &&
            lstrcmpiA(cfg->lpBinaryPathName, expectedSysPath) == 0) {
            stale = false;  // Служба уже ссылается на наш HexIO.sys
        }
    }

    if (stale) {
        SERVICE_STATUS status;
        if (QueryServiceStatus(svc, &status) && status.dwCurrentState != SERVICE_STOPPED) {
            ControlService(svc, SERVICE_CONTROL_STOP, &status);
            for (int i = 0; i < 20; ++i) {
                if (!QueryServiceStatus(svc, &status) || status.dwCurrentState == SERVICE_STOPPED) break;
                Sleep(100);
            }
        }
        DeleteService(svc);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (stale) {
        // Ждём, пока SCM окончательно уберёт удалённую службу,
        // иначе CreateService в StartUp вернёт ошибку 1072.
        for (int i = 0; i < 20; ++i) {
            SC_HANDLE m = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
            if (m == NULL) break;
            SC_HANDLE s = OpenServiceA(m, "hexio", SERVICE_QUERY_STATUS);
            if (s != NULL) CloseServiceHandle(s);
            CloseServiceHandle(m);
            if (s == NULL) break;
            Sleep(100);
        }
    }
}

void AtaController::Cleanup() {
    if (m_initialized) {
        m_hexIO.ShutDown();
        m_initialized = false;
    }
}


// Регистр DC : 0x3F6 (Primary) / 0x376 (Secondary)
void AtaController::SoftwareReset(int channel) {
    USHORT dcPort = (channel == 0) ? 0x3F6 : 0x376;  
    OutByte(dcPort, 0x04);  // Устанавливаем бит 2 (SRST = 1)
    Sleep(10);           
    OutByte(dcPort, 0x00);  // Снимаем SRST (SRST = 0)
    Sleep(2000);            
}

BYTE AtaController::InByte(USHORT port) {   //чтения 8-битных регистров
    return (BYTE)_inp(port);
}

void AtaController::OutByte(USHORT port, BYTE value) {  //для записи в 8-битные регистры
    _outp(port, value);
}

WORD AtaController::InWord(USHORT port) {
    return (WORD)_inpw(port);
}

void AtaController::SwapStringBytes(char* str, int length) {
    for (int i = 0; i < length - 1; i += 2) {
        char temp = str[i];
        str[i] = str[i + 1];
        str[i + 1] = temp;
    }
    int end = length - 1;
    while (end >= 0 && (str[end] == ' ' || str[end] == '\0' || str[end] == 0x0D || str[end] == 0x0A)) {
        str[end] = '\0';
        end--;
    }

    // Серийные номера производители часто дополняют пробелами СЛЕВА
    int start = 0;
    while (str[start] == ' ') start++;
    if (start > 0) memmove(str, str + start, strlen(str + start) + 1);
}

bool AtaController::WaitForStatus(USHORT statusPort, BYTE maskClear, BYTE maskSet, DWORD timeoutMs) {
    DWORD start = GetTickCount();
    while ((GetTickCount() - start) < timeoutMs) {
        BYTE status = InByte(statusPort);
        if ((!(status & maskClear)) && ((status & maskSet) == maskSet)) {
            return true;
        }
        Sleep(2);
    }
    return false;
}

bool AtaController::ReadIdentifyData(USHORT basePort, USHORT controlPort, int driveSelect, AtaDeviceInfo& info, std::string& log) {
    info.channel = (basePort == 0x1F0) ? 0 : 1;
    info.driveSelect = driveSelect;
    info.isPresent = false;

    USHORT drPort      = basePort + 0;  // 0x1F0/0x170
    USHORT errPort     = basePort + 1;  // 0x1F1/0x171
    USHORT scPort      = basePort + 2;  // счетчик секторов      0x1F2/0x172
    USHORT snPort      = basePort + 3;  // номер сектора         0x1F3/0x173
    USHORT clPort      = basePort + 4;  // цилиндр младший       0x1F4/0x174
    USHORT chPort      = basePort + 5;  // цилиндр старший       0x1F5/0x175
    USHORT dhPort      = basePort + 6;  // 0x1F6/0x176
    USHORT cr_srPort   = basePort + 7;  // 0x1F7/0x177
    USHORT asPort      = (basePort == 0x1F0) ? 0x3F6 : 0x376; 

    // 1. Запись в регистр DH номаер устройства (бит 4-номер устр(0 мфстер, 1 слейв), бит 6 режим адресации)
    BYTE dhValue = (driveSelect == 0) ? 0xE0 : 0xF0;
    OutByte(dhPort, dhValue);
    Sleep(15);

    BYTE status = InByte(asPort);
    
    std::ostringstream ss;
    ss << "Регистр AS (0x" << std::hex << std::uppercase << asPort << "), "
       << "канал " << info.channel 
       << (driveSelect == 0 ? " Master" : " Slave")
       << ": Status = 0x" << std::setw(2) << std::setfill('0') << (int)status << "\r\n";
    log += ss.str();

    if (status == 0xFF || status == 0x00) {
        log += "  -> Канал пуст (устройство отсутствует)\r\n";
        return false;
    }

    // 3. Ожидание снятия BSY (бит 7) И установки DRDY (бит 6) в регистре AS
    if (!WaitForStatus(asPort, ATA_SR_BSY, ATA_SR_DRDY, 500)) {
        log += "  -> Таймаут: BSY не снят или DRDY не установлен\r\n";
        return false;
    }

    // 4. Подготовка регистров и запись команды IDENTIFY DEVICE (0xEC) в CR.
    // Перед командой SC, SN, CL и CH обнуляются (для IDENTIFY не используются,
    // а сигнатуру ATAPI устройство выставит в CL/CH уже после сброса команды).
    OutByte(scPort, 0);
    OutByte(snPort, 0);
    OutByte(clPort, 0);
    OutByte(chPort, 0);

    BYTE cmd = 0xEC;
    OutByte(cr_srPort, cmd);  // Запись в 0x1F7 = регистр CR

    // Ждём окончания выполнения команды: BSY=0 и далее DRQ=1 (ATA-устройство
    // готово отдать данные) либо ERR=1 (команда отклонена - так отвечает ATAPI).
    // Одного чтения статуса недостаточно: медленное устройство ещё держит BSY.
    DWORD waitStart = GetTickCount();
    do {
        status = InByte(asPort);
        if (!(status & ATA_SR_BSY) && (status & (ATA_SR_DRQ | ATA_SR_ERR))) break;
        Sleep(2);
    } while ((GetTickCount() - waitStart) < 1000);

    // Обработка ATAPI (CD-ROM / ISO) — по сигнатуре в CL/CH
    if (status & ATA_SR_ERR) {
        BYTE err = InByte(errPort);  // Регистр ошибок ER (0x1F1/0x171)
        BYTE cylL = InByte(clPort);
        BYTE cylH = InByte(chPort);

        std::ostringstream ssErr;
        ssErr << "  -> ERR=1 после 0xEC, ER=0x" << std::hex << std::uppercase
              << std::setw(2) << std::setfill('0') << (int)err
              << ", CL=0x" << std::setw(2) << (int)cylL
              << ", CH=0x" << std::setw(2) << (int)cylH << "\r\n";
        log += ssErr.str();

        if (cylL == 0x14 && cylH == 0xEB) {
            cmd = 0xA1;  // IDENTIFY PACKET DEVICE
            OutByte(cr_srPort, cmd);
            Sleep(15);
        } else {
            return false;
        }
    }

    // 5. Ожидание готовности данных: BSY=0, DRQ=1 в регистре AS
    if (!WaitForStatus(asPort, ATA_SR_BSY, ATA_SR_DRQ, 1000)) {
        log += "  -> Таймаут: DRQ не установлен\r\n";
        return false;
    }

    // 6. Чтение 512 байт (256 слов) из регистра DR 
    WORD identifyBuffer[256];
    for (int i = 0; i < 256; ++i) {
        identifyBuffer[i] = InWord(drPort);
    }

    info.isPresent = true;

    // 7. Парсинг данных IDENTIFY DEVICE 

    // Word 10–19: серийный номер
    char serial[21] = { 0 };
    memcpy(serial, &identifyBuffer[10], 20);
    SwapStringBytes(serial, 20);
    info.serialNumber = serial;

    // Word 23–26: версия прошивки
    char firmware[9] = { 0 };
    memcpy(firmware, &identifyBuffer[23], 8);
    SwapStringBytes(firmware, 8);
    info.firmwareRev = firmware;

    // Word 27–46: модель
    char model[41] = { 0 };
    memcpy(model, &identifyBuffer[27], 40);
    SwapStringBytes(model, 40);
    info.model = model;

    // Word 49, бит 9: признак LBA
    info.lbaSupported = (identifyBuffer[49] & (1 << 9)) != 0;

    // Word 60–61 (LBA28) или Word 100–103 (LBA48): объём
    bool lba48Supported = (identifyBuffer[83] & (1 << 10)) != 0;
    if (lba48Supported) {
        info.totalSectors = ((ULONGLONG)identifyBuffer[103] << 48) |
                            ((ULONGLONG)identifyBuffer[102] << 32) |
                            ((ULONGLONG)identifyBuffer[101] << 16) |
                            ((ULONGLONG)identifyBuffer[100]);
        if (info.totalSectors == 0) {
            info.totalSectors = ((DWORD)identifyBuffer[61] << 16) | (DWORD)identifyBuffer[60];
        }
    } else {
        info.totalSectors = ((DWORD)identifyBuffer[61] << 16) | (DWORD)identifyBuffer[60];
    }
    info.sizeInGB = (double)(info.totalSectors * 512.0) / (1024.0 * 1024.0 * 1024.0);

    // Word 217: тип диска SSD или HDD
    info.rotationRate = identifyBuffer[217];
    if (cmd == 0xA1) {
        info.deviceType = "ATAPI (CD/DVD-ROM / ISO)";
    } else if (info.rotationRate == 0x0001) {
        info.deviceType = "SSD (Solid State Drive)";
    } else if (info.rotationRate > 1) {
        std::ostringstream ss_rot;
        ss_rot << "HDD (" << info.rotationRate << " RPM)";
        info.deviceType = ss_rot.str();
    } else {
        info.deviceType = "HDD / Виртуальный диск";
    }

    return true;
}

std::vector<AtaDeviceInfo> AtaController::ScanAllChannels(std::string& debugLog) {
    std::vector<AtaDeviceInfo> devices;
    debugLog = "=== Лог ответов портов ввода-вывода (Вариант А7) ===\r\n";

    struct ChannelConfig {
        USHORT base;
        USHORT control;  // Регистр DC/AS
    } channels[2] = {
        { 0x1F0, 0x3F6 },  // Primary Channel: DR=0x1F0..0x1F7, AS/DC=0x3F6
        { 0x170, 0x376 }   // Secondary Channel: DR=0x170..0x177, AS/DC=0x376
    };

    for (int ch = 0; ch < 2; ++ch) {
        for (int drv = 0; drv < 2; ++drv) {
            AtaDeviceInfo info;
            if (ReadIdentifyData(channels[ch].base, channels[ch].control, drv, info, debugLog)) {
                devices.push_back(info);
            }
        }
    }

    return devices;
}