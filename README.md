# WinDefenderKiller

## PermanentDisablefromRegistry
C++ Code for Permanently Disabling Windows Defender via Registry Keys

Just compile and execute it!

And when i restart it:

![image](https://github.com/S12cybersecurity/WinDefenderKiller/assets/79543461/2c410420-9ca4-4484-b0f1-cf547dfe1f7b)

## BYOVDWindowsDefenderKiller
This module implements a BYOVD (Bring Your Own Vulnerable Driver) technique to disable Windows Defender by continuously terminating its processes.

A vulnerable signed driver is loaded to gain kernel-level access, allowing the program to repeatedly kill **MsMpEng.exe** services in a loop.

### Driver
https://github.com/DeathShotXD/0xKern3lCrush-Foreverday-BYOVD-CVE-2026-0828/blob/main/drivers/0xhashes.md

## Registry + BYOVD
This repository brings together two separate techniques to illustrate a combined approach for disabling Windows Defender more effectively.

The registry-based method applies permanent configuration changes, but it is heavily detected and blocked by most antivirus solutions, including Defender itself. Because of this, running it directly is often unreliable.

To address this, the BYOVD technique can be used beforehand. By loading a vulnerable driver and gaining kernel-level access, Defender-related processes are continuously terminated, reducing its ability to detect or block further actions.

With Defender in this weakened state, the registry-based code can then be executed and disable in a permanent way the defender 

**Workflow**
- Use BYOVD to disrupt Defender at runtime (process termination loop)
- Execute the registry-based method without interference

⚠️ Disclaimer
For educational and security research purposes only. Use in controlled environments.
