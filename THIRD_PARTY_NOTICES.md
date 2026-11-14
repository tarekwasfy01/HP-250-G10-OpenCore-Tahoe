# Third-party components

This repository preserves the active components of one HP 250 G10 setup. It does not claim authorship of the included drivers or bootloader. The root MIT license applies only to original documentation and helper code.

| Component | Upstream project / source |
| --- | --- |
| OpenCore, OpenRuntime, ResetNvramEntry | [acidanthera/OpenCorePkg](https://github.com/acidanthera/OpenCorePkg/tree/1.0.8) |
| OpenHfsPlus | [OpenCorePkg Legacy OpenHfsPlus](https://github.com/acidanthera/OpenCorePkg/tree/1.0.8/Legacy) |
| Lilu | [acidanthera/Lilu](https://github.com/acidanthera/Lilu) |
| WhateverXe | [b00t0x/WhateverXe 0.7.0](https://github.com/b00t0x/WhateverXe/tree/0.7.0) |
| AMFIPass Mod | [b00t0x/WhateverXe-Graphics](https://github.com/b00t0x/WhateverXe-Graphics), derived from AMFIPass by Dhinak G |
| VirtualSMC | [acidanthera/VirtualSMC](https://github.com/acidanthera/VirtualSMC) |
| NVMeFix | [acidanthera/NVMeFix](https://github.com/acidanthera/NVMeFix) |
| VoodooI2C, VoodooGPIO and VoodooI2CHID | [VoodooI2C/VoodooI2C](https://github.com/VoodooI2C/VoodooI2C) |
| VoodooInput | [acidanthera/VoodooInput](https://github.com/acidanthera/VoodooInput) |
| VoodooPS2Controller | [acidanthera/VoodooPS2](https://github.com/acidanthera/VoodooPS2) |
| AirPort_RTW89, front-end and Realtek firmware | [FranciumOS-org/AirPort_RTW89](https://github.com/FranciumOS-org/AirPort_RTW89); bundled license files are retained under its Resources directory |
| IOSkywalkFamily and IO80211FamilyLegacy compatibility payloads | Apple-origin Wi-Fi components distributed for compatibility through [Dortania/OpenCore-Legacy-Patcher](https://github.com/dortania/OpenCore-Legacy-Patcher); not covered by this repository's MIT license |
| Generated HP ACPI tables | Generated from the reference laptop's firmware using [lzhoang2801/OpCore-Simplify](https://github.com/lzhoang2801/OpCore-Simplify) and OpenCore-compatible SSDT patterns |

Upstream license texts are provided in `licenses/` where available. Existing notices in bundled files remain applicable. Upstream source links and the embedded versions in `docs/snapshot.json` identify the corresponding projects; this repository is not their official distribution channel.

No macOS installer, recovery image, TGL system graphics package or full firmware DSDT is included. Apple-origin components and Realtek firmware remain subject to their respective rights and terms; the original documentation license does not grant additional rights to them.

## NootedGreen HP profile (14 November 2026)

The current snapshot replaces WhateverXe with a custom NootedGreen build and
WhateverGreen. NootedGreen source lineage: [dssoftx/Hackintosh-X1Carbon-G11-IrisXe-Tahoe](https://github.com/dssoftx/Hackintosh-X1Carbon-G11-IrisXe-Tahoe),
reviewed revisions in `docs/nootedgreen-build.json`; local changes in
`docs/nootedgreen-hp.patch`. Its upstream license is reproduced verbatim in
`licenses/NootedGreen.txt`. HookCase's MIT license is in `licenses/HookCase.txt`;
HookCase and OS-specific system graphics payloads are not included in this EFI.
WhateverGreen upstream: https://github.com/acidanthera/WhateverGreen;
license in `licenses/WhateverGreen.txt`. Existing license files for previously
included components remain as historical attribution.
