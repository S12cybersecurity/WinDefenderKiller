# WinDefenderKiller
Windows Defender Killer | C++ Code Disabling Permanently Windows Defender using Registry Keys

Let's compile it!

![image](https://github.com/S12cybersecurity/WinDefenderKiller/assets/79543461/7e4f97e5-0d6e-4662-9935-2b61f5fc4a32)

Command:

└─# x86_64-w64-mingw32-g++ -O2 disableWinDef.cpp -o winDefKiller -I/usr/share/mingw-w64/include -L/usr/lib -s -ffunction-sections -fdata-sections -Wno-write-strings -fno-exceptions -fmerge-all-constants -static-libstdc++ -static-libgcc -fpermissive -Wnarrowing -fexceptions

I execute it!

And when i restart it:

![image](https://github.com/S12cybersecurity/WinDefenderKiller/assets/79543461/2c410420-9ca4-4484-b0f1-cf547dfe1f7b)

If i try to download mimikatz malicious binary:

![image](https://github.com/S12cybersecurity/WinDefenderKiller/assets/79543461/2f5e78b0-33f0-4012-a1d8-84ae8e26b6e7)

Not detected by Windows Defender!
