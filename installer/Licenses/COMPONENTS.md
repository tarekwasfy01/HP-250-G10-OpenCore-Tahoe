# Local graphics package: component rights and provenance

The full package is generated privately from a locally installed or locally supplied verified stack. No Apple binaries are included in the public builder/tools package. Merely bundling license texts does not grant redistribution rights for Apple proprietary components.

- AppleIntelTGLGraphics, Framebuffer, GraphicsShared, GL/Metal/VA/VAME bundles and compiler payloads: Apple/Intel-origin proprietary code. Keep your legitimate source and applicable original terms. Not covered by this repository license.
- HookCase: Stephen Michaud, MIT; included license.
- libxedrv, LLVM bridge and local compatibility changes: source lineage JulioNectar/Hackintosh-XPS13Plus-IrisXe-Tahoe, commit 24450b476d7ca025b83c85d7b8362caf3234d2f2. See included upstream LICENSE and NOTICE. Built against the locally installed 25G332 compiler libraries. The binary hash manifest identifies the exact validated local payload.
- NootedGreen and Lilu: accompanying EFI dependencies, not installed by this package; upstream licenses included as context. Matching HP custom NootedGreen is required.
- Installer helpers: original repository MIT license; do not relicense third-party payloads.

The package does not install Wi-Fi, battery, Bluetooth, audio EFI kexts, SMBIOS identifiers, software cursor, sleep settings, or other applications. It does not include a macOS installer, recovery image, or prebuilt kernel collection.
