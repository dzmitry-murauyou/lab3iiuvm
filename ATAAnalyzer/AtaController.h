#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "hexioctrl.h"

struct AtaDeviceInfo {
    bool isPresent;
    int channel;        // 0 - Primary (0x1F0), 1 - Secondary (0x170)
    int driveSelect;    // 0 - Master (DH=0xE0), 1 - Slave (DH=0xF0)
    
    std::string model;
    std::string serialNumber;
    std::string firmwareRev;
    
    bool lbaSupported;
    ULONGLONG totalSectors;
    double sizeInGB;
    
    WORD rotationRate;
    std::string deviceType;
};

class AtaController {
public:
    AtaController();
    ~AtaController();

    bool Initialize();
    void Cleanup();

    // Детализация последней ошибки инициализации драйвера (для вывода в GUI)
    const std::string& GetLastErrorText() const { return m_lastError; }
    
    //программный сброс (SRST) через регистр DC
    void SoftwareReset(int channel);

    std::vector<AtaDeviceInfo> ScanAllChannels(std::string& debugLog);
    bool ReadIdentifyData(USHORT basePort, USHORT controlPort, int driveSelect, AtaDeviceInfo& info, std::string& log);

private:
    HexIOWrapper m_hexIO;
    bool m_initialized;
    std::string m_lastError;

    // Удаляет ранее установленную службу hexio, если она ссылается не на
    // текущий HexIO.sys (служба могла остаться от прошлых запусков из другой папки)
    void RemoveStaleService(const char* expectedSysPath);

    BYTE InByte(USHORT port);
    void OutByte(USHORT port, BYTE value);
    WORD InWord(USHORT port);

    void SwapStringBytes(char* str, int length);
    // Ждем снятия maskClear и установки maskSet в регистре AS/SR
    bool WaitForStatus(USHORT statusPort, BYTE maskClear, BYTE maskSet, DWORD timeoutMs = 2000);
};