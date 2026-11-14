# HP 250 G10 graphics installer tools

Target: **macOS Tahoe 26.7.2 / build 25G332**, x86_64, matching HP A7A1 NootedGreen EFI. This is the confirmed NG system stack, not the WhateverXe Graphics package. Copying the EFI alone does not install these components.

## Two packages

1. **HP250-Graphics-Installer-Tools.pkg** is public. It installs only the builder, verification manifest, installer helpers and license notices into `/Library/Application Support/HP250Graphics/Tools`. It does **not** activate graphics or install Apple binaries.
2. The builder creates **HP250-NootedGreen-SystemStack-25G332.pkg** locally from files you already have. The complete private package installs the eleven targeted TGL/HookCase/Metal/compiler directories. It is intentionally not uploaded to this repository: its Apple-origin binaries are not licensed by the repository's MIT license or by merely including notices.

These packages are not Developer ID Installer signed. No Gatekeeper or signature enforcement is disabled by the tooling. A working Python 3.9+ (Xcode/Command Line Tools) is required by both the builder and the full installer's scripts. The builder calls Apple's pkgbuild. Never run an unreviewed package as root.

## Create the full package from the working reference system

After installing the public tools package:

```sh
python3 "/Library/Application Support/HP250Graphics/Tools/build_local.py" --source-root / --output "$HOME/Desktop/HP250-NootedGreen-SystemStack-25G332.pkg"
```

Alternatively, run `python3 installer/build_local.py --source-root / --output /your/new/output.pkg` from this checkout. Every local source file must match `manifest.json`; different files are rejected. Do not use `sudo` just to build unless your local source permissions require it. Building changes no live graphics files.

## Full package behavior

- Accepts only the currently running startup volume and build **25G332**.
- Verifies all source hashes and all eleven bundle code signatures.
- Backs up previous targeted files, `/Library/KernelCollections`, and Preboot `KernelCollections` before writing the new stack. Saved file contents are verified; original ownership and modes are recorded.
- Writes only the enumerated graphics components. EFI partitions, identities, audio, WLAN, Bluetooth, battery, cursor overlay and energy settings are left alone.
- Registers HookCase and the two NG TGL kexts with kmutil. Exit 28 is accepted only when it explicitly requests reboot; the three exact bundle IDs and UUIDs must then appear in the Auxiliary KC.
- Checks installed file hashes. On installation/registration failure, attempts to restore saved files and kernel collections. Power loss or a kernel panic cannot be made transactional.
- Keeps private backups under `/Library/Application Support/HP250Graphics/Backups/<timestamp>` and installs licensing notices. No automatic reboot.

**The shared macOS stack affects every EFI booting this macOS.** A different bootloader does not isolate `/Library`. Keep an external recovery route and a matching known-good EFI before installing. This package does not restore or create the EFI, and does not make the internal microphone or system sleep work.

After success, restart manually through the matching NootedGreen EFI. A local build or passed checks do not qualify a new macOS build or another laptop. An installer error must be investigated before rebooting.

## Restore

For a failed installation the installer attempts rollback automatically. To restore a completed backup on the same running **25G332** installation:

```sh
sudo python3 "/Library/Application Support/HP250Graphics/Backups/TIMESTAMP/RESTORE.py" restore "/Library/Application Support/HP250Graphics/Backups/TIMESTAMP"
```

This helper targets the running startup volume only. It is **not** an offline Recovery-volume restore tool. It verifies saved content before deleting any targeted files and never restores an EFI. Do not delete the original private recovery backups created during the earlier hardware rollout.

## Licensing

Read `Licenses/COMPONENTS.md` and the included upstream licenses. Public tools contain no Apple/Intel driver binaries, macOS kernel collections, private serials, network settings or audio recordings. The public source documents how to package your local verified copy; it does not grant new redistribution rights.
