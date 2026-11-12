# HP 250 G10 — OpenCore for macOS Tahoe

A sanitized OpenCore EFI snapshot for **one specific HP 250 15.6-inch G10 notebook with an Intel Core i5-1334U**. This is a hardware-specific reference, not a universal EFI for every HP 250 G10 variant.

## Current status

Snapshot date: **12 November 2026**. The owner reports that **macOS boots and Wi-Fi works** with the source configuration. **Graphics acceleration does not work.**

| Feature | Status |
| --- | --- |
| macOS startup | Working, reported by the owner |
| Realtek RTL8852BE-VS Wi-Fi | Working, reported by the owner |
| Intel Iris Xe Metal / hardware acceleration | **Not working** |
| OpenCore / firmware display at 1920 × 1080 | Configured; the firmware GOP mode was verified previously |
| Keyboard and touchpad | Drivers included; current behavior not separately confirmed |
| Audio, Bluetooth, camera, sleep/wake, battery reporting | Not confirmed |
| HDMI / USB-C video and USB port mapping | Not confirmed |

WhateverXe is enabled in this snapshot, but its presence **does not mean acceleration is working**. No new macOS boot was performed while preparing this repository. `ocvalidate` verifies configuration structure, not successful boot or graphics support. The exact installed macOS build and installed system graphics payloads were not re-read during this export.

## Reference hardware

| Component | Reference machine |
| --- | --- |
| Notebook | HP 250 15.6 inch G10 Notebook PC |
| Mainboard | HP 8B34 |
| BIOS | F.26 |
| CPU | Intel Core i5-1334U, Raptor Lake-U, 10 cores / 12 threads |
| Integrated GPU | Intel Iris Xe, PCI ID `8086:A7A1`, 80 EUs |
| Internal display | 1920 × 1080 |
| Wi-Fi | Realtek RTL8852BE-VS, PCI ID `10EC:B85B` |
| Keyboard | PS/2; VoodooPS2Controller is included |
| Touchpad | ELAN I2C device; VoodooI2C / VoodooI2CHID are included |
| SMBIOS model | `MacBookPro16,2` |
| Bootloader | OpenCore 1.0.8 |

The SMBIOS model is an emulated Mac identity, not the physical laptop model. Check your own device IDs, firmware, ACPI and display wiring before adapting this configuration.

## What is included

- The active `EFI/BOOT` and `EFI/OC` components from the main EFI, with machine identifiers scrubbed.
- Ten HP-specific generated SSDTs and their matching ACPI patches.
- Lilu 1.7.3, WhateverXe 0.7.0, VirtualSMC 1.3.9 and NVMeFix 1.1.4.
- The Realtek AirPort_RTW89 0.5.0 Wi-Fi stack, its front-end plugin, IOSkywalkFamily, IO80211FamilyLegacy and AMFIPass.
- VoodooI2C, VoodooI2CHID and VoodooPS2Controller input drivers.
- OpenRuntime, OpenHfsPlus and the Reset NVRAM entry driver.

See [the component inventory](docs/snapshot.json) for bundle identifiers and embedded versions. Some compatibility bundles use an internal bundle version different from their upstream archive version.

Only active payloads are distributed. The disabled WhateverGreen entry is retained in `config.plist` to preserve the source settings; its unused bundle is omitted. NootedGreen, old EFI variants, diagnostic logs, private backups, recovery images and macOS system graphics packages are not included.

## Generate your own identity before booting

The public `EFI/OC/config.plist` intentionally contains:

| `PlatformInfo → Generic` key | Public value |
| --- | --- |
| `SystemSerialNumber` | Empty |
| `MLB` | Empty |
| `SystemUUID` | `00000000-0000-0000-0000-000000000000` |
| `ROM` | Six zero bytes |

Generate your own consistent identity for `MacBookPro16,2`, for example with [GenSMBIOS](https://github.com/corpnewt/GenSMBIOS), and fill all four fields using [Dortania's PlatformInfo instructions](https://dortania.github.io/OpenCore-Install-Guide/config-laptop.plist/ice-lake.html#platforminfo). Do not reuse another person's serials, UUID or ROM. Keep your personalized copy private.

Bluetooth controller data and transient boot-device values were also excluded from the public NVRAM data. No personal identity was committed in an earlier revision of this repository: it was sanitized before the initial commit.

## Use this EFI

1. Back up your existing EFI and keep a separate way to start your current operating systems.
2. Compare your HP's hardware and firmware with the reference above. The ACPI tables here were generated from this particular laptop's firmware.
3. Generate and enter your own SMBIOS identity in a local copy.
4. Validate that copy with **`ocvalidate` from OpenCore 1.0.8**.
5. Test from a separate FAT32 USB/EFI first. Copy the `EFI` directory to the root of that partition.
6. Select that device in the firmware boot menu, then choose your macOS entry in OpenCore.

OpenCore uses a visible picker with `Timeout=0`, verbose startup and panic symbols. This repository does not automatically repartition a disk, change firmware settings or install macOS. It does not include a macOS installer or recovery image.

The source machine had UEFI Secure Boot disabled. OpenCore's separate `SecureBootModel` setting is `Disabled` in the configuration. Do not confuse these two settings.

## Graphics limitations and system-installed drivers

The firmware GOP can display 1080p without Metal acceleration. `UEFI → Output → Resolution = 1920x1080` controls the firmware/OpenCore display and is not evidence of accelerated macOS graphics.

This snapshot preserves the owner's booting configuration rather than claiming to fix Iris Xe acceleration. Its graphics boot arguments include `-allow3d -disablegfxfirmware`; it also retains the source Wi-Fi compatibility argument `-amfipassbeta` and I2C polling argument `-vi2c-force-polling`.

WhateverXe and its system graphics package are separate components. Replacing an EFI does **not** restore a kext previously changed inside macOS, its auxiliary kernel collection, or graphics bundles under `/Library`. The installed system payload versions were not verified during this export. Avoid mixing unrelated WhateverXe, NootedGreen and TGL payload versions. Refer to [WhateverXe](https://github.com/b00t0x/WhateverXe) and [WhateverXe Graphics](https://github.com/b00t0x/WhateverXe-Graphics) for their matching components and current hardware limits.

## Wi-Fi

The reference machine uses AirPort_RTW89 for the RTL8852BE-VS. The source configuration includes the legacy Wi-Fi compatibility families and blocks the built-in `com.apple.iokit.IOSkywalkFamily` with the `Exclude` strategy. AMFIPass is included; its binary in the source EFI matches the Mod build used by WhateverXe Graphics.

See [AirPort_RTW89's installation documentation](https://github.com/FranciumOS-org/AirPort_RTW89/blob/main/efi/README.md) for the driver stack. Working Wi-Fi here does not establish Bluetooth, AirDrop or Continuity support.

## Validation and privacy

Before publication, the source EFI was privately backed up and checked by file hashes. This public copy passed OpenCore 1.0.8 `ocvalidate`, active-file checks and an identity scan. The checks did not modify the laptop's boot partitions.

To check that an unpersonalized public checkout still has empty identity fields and all active paths:

```sh
python tools/validate_public.py
```

That command is intended for the public template; it will reject a personalized copy. `SHA256SUMS` records the published payload and documentation files.

Please remove serials, UUIDs, MAC/ROM values, network names and private paths before attaching any diagnostics to an issue.

## Credits and redistribution

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Included binaries retain their respective upstream licenses and notices. The repository's MIT license covers only its original documentation and helper code; it does not relicense third-party or Apple-origin components.
