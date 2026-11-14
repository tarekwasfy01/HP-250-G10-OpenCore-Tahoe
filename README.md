# HP 250 G10 — OpenCore / NootedGreen for macOS Tahoe

Sanitized EFI reference for one **HP 250 15.6-inch G10 with Core i5-1334U**.
Snapshot update: **14 November 2026**. Generate your own SMBIOS identity before booting.

## Current results

The owner reports successful desktop startup and working graphics acceleration.
A local `system_profiler` report shows **Intel Iris Xe, 1536 MB dynamic VRAM,
Metal Supported**, with the compatibility device ID `0x9A49`. This is evidence
of the tested machine's current desktop, not qualification of every Metal,
video, sleep or external-display workload.

| Feature | Result |
| --- | --- |
| macOS Tahoe desktop | Working on the reference laptop, owner confirmed |
| Metal / Iris Xe acceleration | Reported working; Metal Supported observed |
| Hardware mouse cursor | Corrupted; software arrow workaround currently used |
| Realtek RTL8852BE Wi-Fi | Owner reports working |
| Keyboard / I2C touchpad | Drivers included; not comprehensively qualified |
| F2/F3 brightness keys | Working after BrightnessKeys 1.0.3, owner confirmed |
| Battery percentage | Working, owner confirmed |
| Internal audio output | Working, owner confirmed; layout 55 in this snapshot |
| Microphone | Not working |
| Realtek Bluetooth (USB 0BDA:B85D) | Discovery and pairing with Samsung Galaxy S21 work, owner confirmed |
| Sleep/wake, camera, external video | Not comprehensively tested |

## Hardware and custom graphics profile

- HP 250 G10, board **HP 8B34**, BIOS **F.26**.
- Intel **Core i5-1334U**, Raptor Lake-U, 10 cores / 12 threads.
- Intel Iris Xe **8086:A7A1 rev04**, subsystem **103C:8B34**, 80 EUs.
- 1920 × 1080 internal display; Realtek **10EC:B85B** Wi-Fi.
- OpenCore **1.0.8**, SMBIOS **MacBookPro16,2**.

This snapshot uses **NootedGreen + WhateverGreen**, not WhateverXe. The custom
NootedGreen HP profile explicitly checks device/revision/subsystem and configures
80 EUs, 10 native subslices, six L3 banks and a 1250-MHz frequency parameter.
A fingerprint-checked framebuffer patch corrects this laptop's DVMT calculation
to **60 MiB**; incompatible binary patterns are rejected. See
[build metadata](docs/nootedgreen-build.json) and [source patch](docs/nootedgreen-hp.patch).
The patch is relative to the source lineage recorded there, not a universal GPU fix.

## Required system graphics stack — EFI alone is not sufficient

The working reference installation also has a matching, separately installed
TGL graphics/HookCase stack and kernel collections. Copying this EFI alone to
an unprepared macOS installation does **not** install that stack.

The local setup uses AppleIntelTGLGraphics, AppleIntelTGLGraphicsFramebuffer,
AppleIntelGraphicsShared and associated GL/Metal/VA bundles, plus HookCase.
Those OS-specific system graphics binaries, kernel collections, recovery
images and installers are **not distributed here**. The observed macOS build
is **25G332**; other builds require separate compatibility validation.

Do not mix this graphics configuration with WhateverXe or unrelated system
payloads. Keep a working recovery boot path and a backup of your EFI and any
system graphics modifications before changing your installation.


## Brightness keys

The display brightness slider already worked. Adding BrightnessKeys 1.0.3
made F2/F3 work on the reference HP, as confirmed by the owner. It is loaded
after Lilu and uses the retained HP ACPI/XOSI configuration.

## Battery, audio and Bluetooth update

The owner confirmed battery reporting and audio output after the 14 November
OCTEST test. The exported EFI includes that successful installation state.
Microphone capture still fails. Bluetooth discovery and pairing with a Samsung
Galaxy S21 are now confirmed by the owner. Automatic reconnection, other device
profiles and sleep/wake behavior have not been qualified.

- **Battery:** SMCBatteryManager 1.3.9 with the existing VirtualSMC 1.3.9,
  plus ECEnabler 1.0.6. Charge percentage works; cycles, temperature and
  sleep/wake charging behavior have not been qualified.
- **Audio:** AppleALC 1.9.8 plus SimpleHDA 1.0.1, which restores the missing
  AppleHDA stack on Tahoe through EFI injection. `DisableLinkeditJettison` is
  enabled. Layout 55 is currently set on `PciRoot(0x0)/Pci(0x1F,0x3)`; the controller is
  Intel 8086:51CA. The codec is confirmed as ALC236 (10EC:0236, revision 0x100002) by IORegistry.
  Layout 55 is a microphone-routing trial; microphone input remains silent.
  The optional SimpleHDA HAL package was not installed. No VoodooHDA is loaded.
- **Bluetooth:** RealtekBluetoothFirmware 1.0.2 plus BlueToolFixup 2.7.2.
  The Realtek executable is unchanged; its Info.plist has one additional
  USB matching personality for the measured **0BDA:B85D**. Firmware selection
  remains based on the actual HCI version. Discovery and pairing with a Samsung Galaxy S21 are confirmed.
  Bluetooth audio, automatic reconnection and sleep/wake remain unqualified.
  No bluetoothd patch is included.

The custom NootedGreen binary also contains an opt-in native software-cursor
trial, enabled by `ngreen,hp-swcursor`. It reports the hardware cursor capability
as unavailable. This flag does not prove correct native software-cursor rendering;
the separately tested overlay arrow remains the confirmed workaround. See the
[cursor-only source diff](docs/nootedgreen-cursor-trial.patch), applied after the
[HP profile diff](docs/nootedgreen-hp.patch). It changes neither DVMT nor topology.

Replacing an EFI alone does not guarantee microphone or Bluetooth behavior on another installation.
The status table records observed limits rather than promising those features.

## Identity removed from the public EFI

The public `EFI/OC/config.plist` has blank serial and MLB values, an all-zero
SystemUUID and six zero ROM bytes. Bluetooth pairing state and transient boot
device NVRAM values are removed. Private identities and runtime dumps are not
part of this repository.

Generate your own consistent **MacBookPro16,2** identity with
[GenSMBIOS](https://github.com/corpnewt/GenSMBIOS). Set `SystemSerialNumber`, `MLB`,
`SystemUUID` and `ROM` in `PlatformInfo → Generic`; keep that customized copy private.
The local TAHOE-BOOT copy retains the owner's identity; only the public export
is sanitized.

## Installation

1. Compare your hardware IDs, BIOS, ACPI tables and display wiring with this laptop.
2. Back up your working EFI and retain a separate boot/recovery option.
3. Prepare the matching graphics stack separately; it is not supplied by this EFI.
4. Generate a unique SMBIOS identity and validate with OpenCore 1.0.8 `ocvalidate`.
5. Test the `EFI` directory from a separate FAT32 boot device before replacing an internal EFI.

`tools/validate_public.py` checks the public identity, active file paths, AML
checksums and kext dependencies. `ocvalidate` checks configuration structure;
neither check proves hardware compatibility. The firmware Secure Boot and
OpenCore SecureBootModel settings are separate; the source setup uses disabled
Secure Boot. Read and understand the included configuration before using it.

## Components and licensing

OpenCore, Lilu, NootedGreen, WhateverGreen, VirtualSMC, NVMeFix, Voodoo input
components and the Realtek Wi-Fi stack retain their respective licenses.
See [third-party notices](THIRD_PARTY_NOTICES.md) and `licenses/`. The root
license covers original repository documentation/helpers only and does not
relicense bundled third-party binaries. NootedGreen's separate license includes
restrictions: read it before redistributing or using commercially.

This project is an independent hardware-specific reference and is not affiliated
with Apple, HP, Intel or the upstream driver authors.

## CPU and sleep observations

`powermetrics` observed a 4600-MHz P-state: CPU Turbo Boost is active.
System sleep after closing the lid fails to resume reliably with this graphics
stack. The owner applied a separate macOS display-only workaround that disables
system sleep; it is not an EFI sleep fix. The laptop stays running while closed.
This workaround is not installed by copying the public EFI.

## Local system-stack .pkg and public installer tools

A verified **NootedGreen system-stack package builder** is now included under
[`installer/`](installer/README.md), with license notices, a complete source
hash manifest, exact module UUID checks, pre-install backups and failure rollback.
It targets **25G332** and does not modify the EFI.

The public [**HP250 Graphics Installer Tools.pkg**](packages/HP250-Graphics-Installer-Tools-20261114.pkg) installs the builder and documentation
only. The complete graphics .pkg is generated locally from your existing verified
files; Apple-origin graphics binaries are not redistributed in that public package.
The full local package includes TGL graphics/framebuffer, HookCase, the patched
Metal/shared bundles and the matching shader compiler/LLVM bridge. EFI-only
battery, audio, Bluetooth and brightness drivers remain in the published EFI.

See the [installer instructions and recovery limits](installer/README.md) before use.
The confirmed working laptop does not need the graphics package reinstalled now.
