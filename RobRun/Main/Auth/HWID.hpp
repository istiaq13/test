#pragma once

#include <string>

namespace Cheat
{
    namespace Auth
    {
        struct HardwareInfo
        {
            std::string cpu;
            std::string motherboard;
            std::string gpu;
            std::string hwid;
            std::string rawFingerprint;
            std::string pcUsername;
            std::string ramCapacity;
        };

        struct LocationInfo
        {
            std::string ip;
            std::string city;
            std::string region;
            std::string country;
            std::string isp;
            std::string formattedLocation;
        };

        class HWID
        {
        public:
            // Retrieve complete hardware information and SHA-256 HWID
            static HardwareInfo GetHardwareInfo();

            // Direct getter for 64-char hex SHA-256 HWID
            static std::string GetHWID();

            // Retrieve public IP and geolocation via lightweight HTTP lookup
            static LocationInfo GetLocationInfo();

            // SHA-256 hashing helper using Windows CryptoAPI
            static std::string Sha256(const std::string& input);

            // Hardware & PC query helpers
            static std::string QueryPCUsername();
            static std::string QueryRamCapacity();

        private:
            static std::string ReadRegistryString(void* hKeyRoot, const wchar_t* subKey, const wchar_t* valueName);
            static std::string QueryCpuBrand();
            static std::string QueryMotherboardSerial();
            static std::string QueryGpuName();
        };
    }
}
