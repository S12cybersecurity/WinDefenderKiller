#include <windows.h>
#include <stdio.h>
#include <iostream>

using namespace std;

bool isUserAdmin() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        return false;
    }

    if (!CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
        FreeSid(AdministratorsGroup);
        return false;
    }

    FreeSid(AdministratorsGroup);
    return isAdmin != FALSE;
}

// Enable defender via registry
int main(int argc, char* argv[]) {
    HKEY key;
    DWORD enable = 0; // Set to 0 to enable Windows Defender again

    if (!isUserAdmin()) {
        cout << "Please run this program as an administrator." << endl;
        return -1;
    }

    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows Defender", 0, KEY_ALL_ACCESS, &key);
    if (res == ERROR_SUCCESS) {
        // Set the "DisableAntiSpyware" value back to 0 (enabled)
        RegSetValueEx(key, "DisableAntiSpyware", 0, REG_DWORD, (const BYTE*)&enable, sizeof(enable));

        // Delete the "Real-Time Protection" subkey to revert the changes
        RegDeleteKey(key, "Real-Time Protection");

        RegCloseKey(key);
    }

    cout << "Windows Defender has been enabled." << endl;
    cout << "Please restart your computer to take effect." << endl;
    getchar();
    return 0;
}
