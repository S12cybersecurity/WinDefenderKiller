#pragma once
#include <windows.h>
#include <winternl.h>
#include <iostream>
#include <string>
#include <psapi.h>
#include <shlwapi.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")
#include "driverBytes.h"

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_IMAGE_ALREADY_LOADED ((NTSTATUS)0xC000010E)
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034L)
#define SE_LOAD_DRIVER_NAME TEXT("SeLoadDriverPrivilege")

typedef NTSTATUS(NTAPI* pNtLoadDriver)(PUNICODE_STRING DriverServiceName);
typedef NTSTATUS(NTAPI* pNtUnloadDriver)(PUNICODE_STRING DriverServiceName);
typedef VOID(NTAPI* pRtlInitUnicodeString)(PUNICODE_STRING DestinationString, PCWSTR SourceString);

std::string GetFileNameFromPath(const std::string& path) {
    size_t lastSlash = path.find_last_of("\\/");
    if (std::string::npos == lastSlash) return path;
    return path.substr(lastSlash + 1);
}

std::wstring StringToWString(const std::string& str) {
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

std::string GetTempDriverPath(const std::string& driverName) {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    return std::string(tempPath) + driverName + ".sys";
}




bool ExtractDriverToTemp(const std::string& outputPath) {
    std::cout << "[*] Extracting embedded driver to: " << outputPath << std::endl;

    HANDLE hFile = CreateFileA(
        outputPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] Failed to create temp file. Error: " << GetLastError() << std::endl;
        return false;
    }

    DWORD bytesWritten;
    bool success = WriteFile(hFile, g_DriverData, g_DriverData_size, &bytesWritten, NULL);
    CloseHandle(hFile);

    if (!success || bytesWritten != g_DriverData_size) {
        std::cerr << "[-] Failed to write driver data. Error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "[+] Driver extracted successfully (" << bytesWritten << " bytes)" << std::endl;
    return true;
}

bool DeleteTempDriver(const std::string& driverPath) {
    if (PathFileExistsA(driverPath.c_str())) {
        if (DeleteFileA(driverPath.c_str())) {
            std::cout << "[+] Temporary driver file deleted." << std::endl;
            return true;
        }
        else {
            std::cerr << "[-] Failed to delete temp driver. Error: " << GetLastError() << std::endl;
            return false;
        }
    }
    return true;
}

bool EnableLoadDriverPrivilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) return false;
    LUID luid;
    if (!LookupPrivilegeValue(nullptr, SE_LOAD_DRIVER_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool res = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
    CloseHandle(hToken);
    return res && (GetLastError() != ERROR_NOT_ALL_ASSIGNED);
}

bool IsDriverLoaded(const std::string& driverFileName) {
    LPVOID drivers[1024];
    DWORD cbNeeded;
    if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded)) {
        int numDrivers = cbNeeded / sizeof(drivers[0]);
        for (int i = 0; i < numDrivers; i++) {
            char name[MAX_PATH];
            if (GetDeviceDriverBaseNameA(drivers[i], name, sizeof(name))) {
                if (_stricmp(name, driverFileName.c_str()) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CleanupService(const std::string& driverName) {
    // 1. First, try to unload the driver from kernel memory using NtUnloadDriver
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    auto NtUnload = (pNtUnloadDriver)GetProcAddress(hNtdll, "NtUnloadDriver");
    auto RtlInit = (pRtlInitUnicodeString)GetProcAddress(hNtdll, "RtlInitUnicodeString");

    if (NtUnload && RtlInit) {
        std::wstring regPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + StringToWString(driverName);
        UNICODE_STRING uStr;
        RtlInit(&uStr, regPath.c_str());

        NTSTATUS status = NtUnload(&uStr);
        if (status == STATUS_SUCCESS) {
            std::cout << "[*] Driver unloaded from kernel." << std::endl;
        }
        else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            std::cout << "[*] Driver was not in kernel memory." << std::endl;
        }
        else {
            std::cout << "[*] NtUnloadDriver returned: 0x" << std::hex << status << std::dec << std::endl;
        }
        Sleep(300); // Give kernel time to unload
    }

    // 2. Try to stop and delete the service from SCM
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm) {
        SC_HANDLE svc = OpenServiceA(scm, driverName.c_str(), SERVICE_ALL_ACCESS);
        if (svc) {
            SERVICE_STATUS status;
            ControlService(svc, SERVICE_CONTROL_STOP, &status);
            DeleteService(svc);
            CloseServiceHandle(svc);
            std::cout << "[*] SCM service deleted." << std::endl;
        }
        CloseServiceHandle(scm);
    }

    // 3. Delete registry key
    std::string regSubKey = "SYSTEM\\CurrentControlSet\\Services\\" + driverName;
    if (RegDeleteTreeA(HKEY_LOCAL_MACHINE, regSubKey.c_str()) == ERROR_SUCCESS) {
        std::cout << "[*] Registry key deleted." << std::endl;
    }

    Sleep(200); // Give the system time to process
}


bool UnloadDriverNT(const std::string& driverName) {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    auto NtUnload = (pNtUnloadDriver)GetProcAddress(hNtdll, "NtUnloadDriver");
    auto RtlInit = (pRtlInitUnicodeString)GetProcAddress(hNtdll, "RtlInitUnicodeString");

    if (!NtUnload || !RtlInit) return false;

    std::wstring regPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + StringToWString(driverName);
    UNICODE_STRING uStr;
    RtlInit(&uStr, regPath.c_str());

    NTSTATUS status = NtUnload(&uStr);

    if (status == STATUS_SUCCESS) {
        std::cout << "[+] Driver unloaded successfully." << std::endl;
        return true;
    }
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
        std::cout << "[*] Driver was not loaded." << std::endl;
        return true;
    }
    else {
        std::cout << "[-] Error unloading driver: 0x" << std::hex << status << std::dec << std::endl;
        return false;
    }
}

bool SetupDriverService(const std::string& driverPath, const std::string& driverName) {
    HKEY hKey;
    std::string regSubKey = "SYSTEM\\CurrentControlSet\\Services\\" + driverName;

    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, regSubKey.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }

    std::string ntPath = "\\??\\" + driverPath;
    DWORD type = 1, start = 3, errorControl = 1;

    RegSetValueExA(hKey, "Type", 0, REG_DWORD, (BYTE*)&type, 4);
    RegSetValueExA(hKey, "Start", 0, REG_DWORD, (BYTE*)&start, 4);
    RegSetValueExA(hKey, "ErrorControl", 0, REG_DWORD, (BYTE*)&errorControl, 4);
    RegSetValueExA(hKey, "ImagePath", 0, REG_SZ, (BYTE*)ntPath.c_str(), (DWORD)ntPath.length() + 1);
    RegCloseKey(hKey);

    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (scm) {
        SC_HANDLE svc = CreateServiceA(scm, driverName.c_str(), driverName.c_str(), SERVICE_ALL_ACCESS,
            SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, driverPath.c_str(), NULL, NULL, NULL, NULL, NULL);
        if (svc) {
            CloseServiceHandle(svc);
        }
        else if (GetLastError() == ERROR_SERVICE_EXISTS) {
            std::cout << "[!] Service already exists in SCM (conflict)." << std::endl;
        }
        CloseServiceHandle(scm);
    }
    return true;
}


int LoadDriver() {
    std::string driverName = "EmbeddedDriverService";

    if (!EnableLoadDriverPrivilege()) {
        std::cerr << "[-] Error: Administrator privileges required." << std::endl;
        return 1;
    }

    // 1. Extract driver to temp location
    std::string tempDriverPath = GetTempDriverPath(driverName);
    std::string driverFile = GetFileNameFromPath(tempDriverPath);

    if (!ExtractDriverToTemp(tempDriverPath)) {
        std::cerr << "[-] Failed to extract driver." << std::endl;
        return 1;
    }

    // 2. Check if already loaded
    if (IsDriverLoaded(driverFile)) {
        std::cout << "[!] WARNING: Driver is already in memory. Attempting to unload..." << std::endl;
        if (!UnloadDriverNT(driverName)) {
            std::cerr << "[-] Could not unload. Try manually: sc stop " << driverName << std::endl;
            DeleteTempDriver(tempDriverPath);
            return 1;
        }
        Sleep(500);
    }

    // 3. Complete cleanup
    std::cout << "[*] Cleaning up previous remnants of: " << driverName << "..." << std::endl;
    CleanupService(driverName);

    // 4. Prepare Registry and SCM
    if (!SetupDriverService(tempDriverPath, driverName)) {
        std::cerr << "[-] Error configuring registry." << std::endl;
        DeleteTempDriver(tempDriverPath);
        return 1;
    }
    std::cout << "[+] Registry and SCM configured." << std::endl;

    // 5. Load via Native API
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    auto NtLoad = (pNtLoadDriver)GetProcAddress(hNtdll, "NtLoadDriver");
    auto RtlInit = (pRtlInitUnicodeString)GetProcAddress(hNtdll, "RtlInitUnicodeString");

    if (!NtLoad || !RtlInit) {
        std::cerr << "[-] Could not obtain ntdll.dll functions" << std::endl;
        DeleteTempDriver(tempDriverPath);
        return 1;
    }

    std::wstring regPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + StringToWString(driverName);
    UNICODE_STRING uStr;
    RtlInit(&uStr, regPath.c_str());

    std::cout << "[*] Executing NtLoadDriver..." << std::endl;
    NTSTATUS status = NtLoad(&uStr);

    if (status == STATUS_SUCCESS) {
        std::cout << "[+] STATUS_SUCCESS: Driver loaded successfully." << std::endl;
    }
    else if (status == STATUS_IMAGE_ALREADY_LOADED) {
        std::cout << "[!] STATUS_IMAGE_ALREADY_LOADED: Driver was already in memory." << std::endl;
    }
    else {
        std::cerr << "[-] NtLoadDriver error: 0x" << std::hex << status << std::dec << std::endl;

        if (status == 0xC0000035) {
            std::cerr << "[!] STATUS_OBJECT_NAME_COLLISION: Service/object name already exists." << std::endl;
            std::cerr << "[!] Run manually: sc delete " << driverName << std::endl;
        }

        DeleteTempDriver(tempDriverPath);
        return 1;
    }

    // 6. Final verification
    Sleep(500);
    if (IsDriverLoaded(driverFile)) {
        std::cout << "[+] CONFIRMED: Driver active in kernel." << std::endl;
    }
    else {
        std::cout << "[-] Driver does not appear in memory." << std::endl;
    }

    std::cout << "\n[*] Note: Temporary file remains at: " << tempDriverPath << std::endl;
    std::cout << "[*] You can delete it after unloading the driver." << std::endl;
    std::cout << "[OK] Use: sc query " << driverName << std::endl;
    std::cout << "========================================" << std::endl;

    // Optional: Delete temp file immediately (uncomment if desired)
    // Note: The driver must remain on disk while loaded
    // DeleteTempDriver(tempDriverPath);

    return 0;
}