#include "stdafx.h"
#include "AtaController.h"
#include <conio.h>
#include <sstream>
#include <iomanip>

// Биты регистра состояния
#define ATA_SR_BSY  0x80  // Бит 7: Busy
#define ATA_SR_DRDY 0x40  // Бит 6: Device Ready
#define ATA_SR_DRQ  0x08  // Бит 3: Data Request (готово к обмену данными)
#define ATA_SR_ERR  0x01  // Бит 0: Error

AtaController::AtaController() {}

AtaController::~AtaController() {
    Cleanup();
}

bool AtaController::Initialize() {
    if (m_hexIO.StartUp()) {
        m_hexIO.AllowExclusiveAccess();
        return true;
    }
    return false; 
}

void AtaController::Cleanup() {
    m_hexIO.ShutDown();
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

    // 4. Запись команды IDENTIFY DEVICE (0xEC) в регистр CR (Command Register)
    BYTE cmd = 0xEC;
    OutByte(cr_srPort, cmd);  // Запись в 0x1F7 = регистр CR
    Sleep(15);

    status = InByte(asPort);

    // Обработка ATAPI (CD-ROM / ISO) — по сигнатуре в CL/CH
    if ((status & ATA_SR_ERR) || !(status & ATA_SR_DRQ)) {
        BYTE cylL = InByte(clPort);
        BYTE cylH = InByte(chPort);

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