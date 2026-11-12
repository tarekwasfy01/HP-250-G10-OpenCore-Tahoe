#!/usr/bin/env python3
"""Validate paths, AML checksums and scrubbed identity in the public EFI template."""
from pathlib import Path
import plistlib
import struct

ROOT = Path(__file__).resolve().parents[1]
OC = ROOT / "EFI" / "OC"


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def within(base, relative):
    path = (base / relative).resolve()
    require(path.is_relative_to(base.resolve()), "Path escapes its EFI directory")
    return path


def main():
    config = plistlib.loads((OC / "config.plist").read_bytes())
    generic = config["PlatformInfo"]["Generic"]
    require(generic["SystemSerialNumber"] == "", "Public serial must be empty")
    require(generic["MLB"] == "", "Public MLB must be empty")
    require(generic["SystemUUID"] == "00000000-0000-0000-0000-000000000000", "Public UUID must be zero")
    require(generic["ROM"] == bytes(6), "Public ROM must be zero")
    require(generic["SystemProductName"] == "MacBookPro16,2", "Unexpected SMBIOS model")
    require(config["UEFI"]["Output"]["Resolution"] == "1920x1080", "Unexpected GOP resolution")
    require((ROOT / "EFI/BOOT/BOOTx64.efi").is_file(), "Missing BOOTx64.efi")
    require((OC / "OpenCore.efi").is_file(), "Missing OpenCore.efi")
    for data in config["NVRAM"]["Add"].values():
        require(not any(k.lower().startswith("bluetooth") for k in data), "Private Bluetooth state in NVRAM")
    for entry in config["ACPI"]["Add"]:
        if entry["Enabled"]:
            path = within(OC / "ACPI", entry["Path"])
            data = path.read_bytes()
            require(len(data) >= 36 and data[:4] == b"SSDT", "Invalid SSDT: " + entry["Path"])
            require(struct.unpack_from("<I", data, 4)[0] == len(data), "Invalid AML length")
            require(sum(data) % 256 == 0, "Invalid AML checksum")
    bundle_ids = []
    metadata = []
    for entry in config["Kernel"]["Add"]:
        if entry["Enabled"]:
            bundle = within(OC / "Kexts", entry["BundlePath"])
            info = plistlib.loads(within(bundle, entry["PlistPath"]).read_bytes())
            if entry["ExecutablePath"]:
                require(within(bundle, entry["ExecutablePath"]).is_file(), "Missing kext executable")
            bundle_ids.append(info["CFBundleIdentifier"])
            metadata.append(info)
    require(len(bundle_ids) == len(set(bundle_ids)), "Duplicate active bundle ID")
    loaded = set()
    for info in metadata:
        for dependency in info.get("OSBundleLibraries", {}):
            if dependency in bundle_ids:
                require(dependency in loaded, "Kext dependency appears after its consumer")
        loaded.add(info["CFBundleIdentifier"])
    active = {e["BundlePath"] for e in config["Kernel"]["Add"] if e["Enabled"]}
    require(not ({"WhateverXe.kext", "WhateverGreen.kext"} <= active), "Conflicting graphics plugins")
    for driver in config["UEFI"]["Drivers"]:
        if driver["Enabled"]:
            require(within(OC / "Drivers", driver["Path"]).is_file(), "Missing UEFI driver")
    allowed = {"config.plist"}
    for path in ROOT.rglob("*"):
        if ".git" in path.parts or not path.is_file():
            continue
        name = path.name.lower()
        require(not name.startswith(("opencore-", "panic-", "._")), "Diagnostic/private file: " + path.name)
        if name.startswith("config") and name.endswith(".plist"):
            require(path.name in allowed and path.parent == OC, "Extra config file")
    print("PASS: scrubbed public identity, active EFI paths, SSDTs and kext dependencies.")


if __name__ == "__main__":
    main()
