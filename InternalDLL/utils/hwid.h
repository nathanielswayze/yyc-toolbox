#include <iostream>
#include <string>
#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "iphlpapi.lib")

namespace HWID {
    std::string getCPUSerial() {
        int cpuInfo[4] = { 0 };
        __cpuid(cpuInfo, 0);  // Get vendor ID

        std::ostringstream oss;
        for (int i = 0; i < 4; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(8) << cpuInfo[i];
        }
        return oss.str();
    }

    std::string getVolumeSerialNumber() {
        DWORD serialNumber = 0;
        GetVolumeInformationA(
            "C:\\",              // Root path
            nullptr,             // Volume name buffer
            0,                   // Buffer size
            &serialNumber,       // Serial number
            nullptr,             // Max component length
            nullptr,             // File system flags
            nullptr,             // File system name buffer
            0                    // File system name buffer size
        );

        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(8) << serialNumber;
        return oss.str();
    }

    std::string getMACAddress() {
        IP_ADAPTER_INFO adapterInfo[16];  // Buffer to hold adapter info
        DWORD bufferSize = sizeof(adapterInfo);

        if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
            std::ostringstream oss;
            for (unsigned char byte : adapterInfo->Address) {
                oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
            }
            return oss.str();
        }
        return "00:00:00:00:00:00";  // Default if MAC cannot be retrieved
    }

    std::string generateHWID() {
        std::string cpuSerial = getCPUSerial();
        std::string volumeSerial = getVolumeSerialNumber();
        std::string macAddress = getMACAddress();

        std::string concatenated = cpuSerial + volumeSerial + macAddress;

        // Simple hashing to create a compact HWID
        std::hash<std::string> hasher;
        size_t hash = hasher(concatenated);

        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << hash;
        return oss.str();
    }
}