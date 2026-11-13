#include "HWID.hpp"
#include <Windows.h>
#include <intrin.h>
#include <wincrypt.h>
#include <winhttp.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <Utils/Dependencies/NlohmannJson.hpp>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "winhttp.lib")

namespace Cheat
{
    namespace Auth
    {
        std::string HWID::Sha256(const std::string& input)
        {
            HCRYPTPROV hProv = 0;
            HCRYPTHASH hHash = 0;
            std::string hashHex = "";

            if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            {
                // Fallback basic hash if crypto provider unavailable
                std::hash<std::string> hasher;
                size_t val = hasher(input);
                std::stringstream ss;
                ss << std::hex << std::setfill('0') << std::setw(16) << val;
                return ss.str();
            }

            if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
            {
                if (CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.data()), static_cast<DWORD>(input.size()), 0))
                {
                    DWORD cbHash = 32;
                    BYTE rgbHash[32] = { 0 };
                    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
                    {
                        std::stringstream ss;
                        for (DWORD i = 0; i < cbHash; i++)
                        {
                            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(rgbHash[i]);
                        }
                        hashHex = ss.str();
                    }
                }
                CryptDestroyHash(hHash);
            }

            CryptReleaseContext(hProv, 0);
            return hashHex;
        }

        std::string HWID::ReadRegistryString(void* hKeyRoot, const wchar_t* subKey, const wchar_t* valueName)
        {
            HKEY hKey = nullptr;
            std::string result = "";

            if (RegOpenKeyExW(reinterpret_cast<HKEY>(hKeyRoot), subKey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
            {
                wchar_t buffer[512] = { 0 };
                DWORD bufferSize = sizeof(buffer);
                DWORD type = 0;

                if (RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS)
                {
                    int len = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 0)
                    {
                        std::string str(len - 1, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, buffer, -1, &str[0], len, nullptr, nullptr);
                        result = str;
                    }
                }
                RegCloseKey(hKey);
            }

            // Trim leading/trailing whitespace
            result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), result.end());

            return result;
        }

        std::string HWID::QueryCpuBrand()
        {
            int cpuInfo[4] = { 0 };
            __cpuid(cpuInfo, 0x80000000);
            unsigned int nExIds = static_cast<unsigned int>(cpuInfo[0]);

            char brand[0x40] = { 0 };
            if (nExIds >= 0x80000004)
            {
                __cpuid(reinterpret_cast<int*>(brand), 0x80000002);
                __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
                __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
            }

            std::string cpuName(brand);
            // Also get CPU signature
            __cpuid(cpuInfo, 1);
            std::stringstream ss;
            if (!cpuName.empty())
            {
                ss << cpuName;
            }
            else
            {
                ss << "CPU_SIG_" << std::hex << cpuInfo[0] << "_" << cpuInfo[3];
            }

            std::string res = ss.str();
            res.erase(res.begin(), std::find_if(res.begin(), res.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            res.erase(std::find_if(res.rbegin(), res.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), res.end());
            return res.empty() ? "Generic_x64_Processor" : res;
        }

        std::string HWID::QueryMotherboardSerial()
        {
            std::string mbProduct = ReadRegistryString(reinterpret_cast<void*>(HKEY_LOCAL_MACHINE), L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
            std::string mbSerial = ReadRegistryString(reinterpret_cast<void*>(HKEY_LOCAL_MACHINE), L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardSerialNumber");
            std::string mbManufacturer = ReadRegistryString(reinterpret_cast<void*>(HKEY_LOCAL_MACHINE), L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardManufacturer");
            std::string machineGuid = ReadRegistryString(reinterpret_cast<void*>(HKEY_LOCAL_MACHINE), L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid");

            std::stringstream ss;
            ss << (mbManufacturer.empty() ? "Generic" : mbManufacturer) << "_"
               << (mbProduct.empty() ? "BaseBoard" : mbProduct) << "_"
               << (mbSerial.empty() ? (machineGuid.empty() ? "MB_SERIAL_0000" : machineGuid) : mbSerial);

            return ss.str();
        }

        std::string HWID::QueryGpuName()
        {
            DISPLAY_DEVICEW dd;
            ZeroMemory(&dd, sizeof(dd));
            dd.cb = sizeof(dd);

            for (DWORD i = 0; EnumDisplayDevicesW(NULL, i, &dd, 0); ++i)
            {
                if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                {
                    int len = WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 0)
                    {
                        std::string gpuStr(len - 1, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, &gpuStr[0], len, nullptr, nullptr);
                        return gpuStr;
                    }
                }
            }

            // Fallback: take first active display adapter
            ZeroMemory(&dd, sizeof(dd));
            dd.cb = sizeof(dd);
            if (EnumDisplayDevicesW(NULL, 0, &dd, 0))
            {
                int len = WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, nullptr, 0, nullptr, nullptr);
                if (len > 0)
                {
                    std::string gpuStr(len - 1, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, &gpuStr[0], len, nullptr, nullptr);
                    return gpuStr;
                }
            }

            return "Standard_Display_Adapter";
        }

        std::string HWID::QueryPCUsername()
        {
            char username[256] = { 0 };
            DWORD len = sizeof(username);
            if (GetUserNameA(username, &len))
            {
                return std::string(username);
            }
            return "UnknownUser";
        }

        std::string HWID::QueryRamCapacity()
        {
            MEMORYSTATUSEX statex;
            statex.dwLength = sizeof(statex);
            if (GlobalMemoryStatusEx(&statex))
            {
                unsigned long long totalGb = (statex.ullTotalPhys + (1024ULL * 1024ULL * 1024ULL - 1ULL)) / (1024ULL * 1024ULL * 1024ULL);
                return std::to_string(totalGb) + " GB";
            }
            return "N/A";
        }

        HardwareInfo HWID::GetHardwareInfo()
        {
            HardwareInfo info;
            info.cpu = QueryCpuBrand();
            info.motherboard = QueryMotherboardSerial();
            info.gpu = QueryGpuName();
            info.pcUsername = QueryPCUsername();
            info.ramCapacity = QueryRamCapacity();

            std::stringstream raw;
            raw << "CPU:[" << info.cpu << "]::MB:[" << info.motherboard << "]::GPU:[" << info.gpu << "]";
            info.rawFingerprint = raw.str();
            info.hwid = Sha256(info.rawFingerprint);

            return info;
        }

        std::string HWID::GetHWID()
        {
            return GetHardwareInfo().hwid;
        }

        LocationInfo HWID::GetLocationInfo()
        {
            LocationInfo loc;
            loc.ip = "127.0.0.1";
            loc.city = "";
            loc.region = "";
            loc.country = "";
            loc.isp = "";
            loc.formattedLocation = "";

            auto FetchFromIpApi = [&loc]() -> bool {
                HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                if (!hSession) return false;

                DWORD timeoutMs = 3500;
                WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

                HINTERNET hConnect = WinHttpConnect(hSession, L"ip-api.com", INTERNET_DEFAULT_HTTP_PORT, 0);
                if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/json/?fields=status,country,regionName,city,district,zip,isp,query", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
                if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

                bool success = false;
                if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                    WinHttpReceiveResponse(hRequest, NULL))
                {
                    std::string response;
                    DWORD dwSize = 0, dwDownloaded = 0;
                    do {
                        dwSize = 0;
                        if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                            std::vector<char> buffer(dwSize + 1, 0);
                            if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                                response.append(buffer.data(), dwDownloaded);
                            }
                        }
                    } while (dwSize > 0);

                    try {
                        auto j = nlohmann::json::parse(response);
                        if (j.value("status", "") == "success") {
                            loc.country = j.value("country", "");
                            loc.region = j.value("regionName", "");
                            loc.city = j.value("city", "");
                            std::string district = j.value("district", "");
                            std::string zip = j.value("zip", "");
                            loc.ip = j.value("query", "127.0.0.1");
                            loc.isp = j.value("isp", "");

                            std::vector<std::string> parts;
                            if (!district.empty()) parts.push_back(district);
                            if (!loc.city.empty()) parts.push_back(loc.city);
                            if (!zip.empty()) parts.push_back(zip);
                            if (!loc.region.empty()) parts.push_back(loc.region);
                            if (!loc.country.empty()) parts.push_back(loc.country);

                            if (!parts.empty()) {
                                std::stringstream ss;
                                for (size_t i = 0; i < parts.size(); i++) {
                                    if (i > 0) ss << ", ";
                                    ss << parts[i];
                                }
                                loc.formattedLocation = ss.str();
                            }
                            success = !loc.formattedLocation.empty();
                        }
                    } catch (...) {}
                }
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return success;
            };

            auto FetchFromIpWhoIs = [&loc]() -> bool {
                HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                if (!hSession) return false;

                DWORD timeoutMs = 3500;
                WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

                HINTERNET hConnect = WinHttpConnect(hSession, L"ipwho.is", INTERNET_DEFAULT_HTTPS_PORT, 0);
                if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

                DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                                 SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                                 SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                                 SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));

                bool success = false;
                if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                    WinHttpReceiveResponse(hRequest, NULL))
                {
                    std::string response;
                    DWORD dwSize = 0, dwDownloaded = 0;
                    do {
                        dwSize = 0;
                        if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                            std::vector<char> buffer(dwSize + 1, 0);
                            if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                                response.append(buffer.data(), dwDownloaded);
                            }
                        }
                    } while (dwSize > 0);

                    try {
                        auto j = nlohmann::json::parse(response);
                        if (j.value("success", false)) {
                            loc.country = j.value("country", "");
                            loc.region = j.value("region", "");
                            loc.city = j.value("city", "");
                            std::string postal = j.value("postal", "");
                            loc.ip = j.value("ip", "127.0.0.1");

                            if (j.contains("connection") && j["connection"].is_object()) {
                                loc.isp = j["connection"].value("isp", "");
                            }

                            std::vector<std::string> parts;
                            if (!loc.city.empty()) parts.push_back(loc.city);
                            if (!postal.empty()) parts.push_back(postal);
                            if (!loc.region.empty()) parts.push_back(loc.region);
                            if (!loc.country.empty()) parts.push_back(loc.country);

                            if (!parts.empty()) {
                                std::stringstream ss;
                                for (size_t i = 0; i < parts.size(); i++) {
                                    if (i > 0) ss << ", ";
                                    ss << parts[i];
                                }
                                loc.formattedLocation = ss.str();
                            }
                            success = !loc.formattedLocation.empty();
                        }
                    } catch (...) {}
                }
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return success;
            };

            if (!FetchFromIpApi())
            {
                FetchFromIpWhoIs();
            }

            return loc;
        }
    }
}
