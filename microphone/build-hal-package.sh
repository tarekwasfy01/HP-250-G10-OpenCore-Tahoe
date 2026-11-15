#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
ROOT="$PWD"
OUT="${1:-$ROOT/../packages/HP250Dmic-HAL-0.1.0-25G332.pkg}"
[[ ! -e "$OUT" ]] || { echo 'Output already exists; choose a new path.'; exit 1; }
/usr/bin/codesign --verify --strict "$ROOT/dist/HP250Dmic.driver"
WORK=$(mktemp -d /private/tmp/HP250Dmic-pkg.XXXXXX)
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/root/Library/Audio/Plug-Ins/HAL" "$WORK/scripts"
/usr/bin/ditto "$ROOT/dist/HP250Dmic.driver" "$WORK/root/Library/Audio/Plug-Ins/HAL/HP250Dmic.driver"
cp "$ROOT/pkg-scripts/preinstall" "$ROOT/pkg-scripts/postinstall" "$WORK/scripts/"
chmod 755 "$WORK/scripts/"*
/usr/bin/pkgbuild --analyze --root "$WORK/root" "$WORK/components.plist"
/usr/bin/python3 - "$WORK/components.plist" <<'PY'
import plistlib,sys
p=sys.argv[1]
with open(p,'rb') as f:items=plistlib.load(f)
for item in items:
    item['BundleIsRelocatable']=False
    item['BundleIsVersionChecked']=False
with open(p,'wb') as f:plistlib.dump(items,f)
PY
/usr/bin/pkgbuild --root "$WORK/root" --component-plist "$WORK/components.plist" --scripts "$WORK/scripts" --identifier com.tarekwasfy.HP250Dmic.HAL --version 0.1.0 --install-location / "$OUT"
