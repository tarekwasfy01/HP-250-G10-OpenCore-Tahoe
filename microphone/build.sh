#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
sdk="$(xcrun --show-sdk-path)"
kernel="$sdk/System/Library/Frameworks/Kernel.framework/Headers"
project="$PWD"
mkdir -p "$project/build/HP250Dmic.kext/Contents/MacOS" "$project/build/HP250Dmic.driver/Contents/MacOS"
python3 - <<'PY'
from pathlib import Path
p=Path('kext/HP250Dmic/kmod_info.c')
p.write_text('''#include <mach/mach_types.h>
kern_return_t HP250Dmic_start(kmod_info_t *ki, void *data) { return KERN_SUCCESS; }
kern_return_t HP250Dmic_stop(kmod_info_t *ki, void *data) { return KERN_SUCCESS; }
KMOD_EXPLICIT_DECL(com.tarekwasfy.HP250Dmic, "0.1.0", HP250Dmic_start, HP250Dmic_stop)
''')
Path('firmware-embed.s').write_text('''.section __DATA,__hp_sof_fw
.globl _kSofRplRi
.globl _kSofRplRiSize
_kSofRplRi:
.incbin "firmware/sof-rpl.ri"
_hpsof_fw_end:
.section __DATA,__hp_sof_size
_kSofRplRiSize:
.long _hpsof_fw_end - _kSofRplRi
''')
PY
common=(-arch x86_64 -target x86_64-apple-macos14.0 -isysroot "$sdk" -I"$kernel" -I"$kernel/IOKit"
    -I"$project/kext/HP250Dmic" -mkernel -nostdlib -fno-builtin -fno-common -fno-stack-protector
    -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE -DAPPLE -DNeXT -D__MACOSX__ -O2 -Wall
    -Wno-inconsistent-missing-override -Wno-deprecated-declarations)
for name in HP250Dmic DspBoot Capture HP250DmicUserClient; do
    xcrun clang++ "${common[@]}" -std=c++17 -fno-exceptions -fno-rtti \
        -c "$project/kext/HP250Dmic/$name.cpp" -o "$project/build/$name.o"
done
xcrun clang "${common[@]}" -c "$project/kext/HP250Dmic/kmod_info.c" -o "$project/build/kmod_info.o"
xcrun clang -arch x86_64 -target x86_64-apple-macos14.0 -c "$project/firmware-embed.s" -o "$project/build/Firmware.o"
xcrun clang++ -arch x86_64 -target x86_64-apple-macos14.0 -isysroot "$sdk" -nostdlib -Xlinker -kext -lcc_kext \
    "$project/build/HP250Dmic.o" "$project/build/DspBoot.o" "$project/build/Capture.o" \
    "$project/build/HP250DmicUserClient.o" "$project/build/Firmware.o" "$project/build/kmod_info.o" \
    -o "$project/build/HP250Dmic.kext/Contents/MacOS/HP250Dmic"
cp "$project/kext/HP250Dmic/Info.plist" "$project/build/HP250Dmic.kext/Contents/Info.plist"
xcrun clang++ -arch x86_64 -mmacosx-version-min=14.0 -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter \
    -bundle "$project/hal/HP250DmicDriver.cpp" -framework CoreAudio -framework CoreFoundation -framework IOKit \
    -o "$project/build/HP250Dmic.driver/Contents/MacOS/HP250Dmic"
cp "$project/hal/Info.plist" "$project/build/HP250Dmic.driver/Contents/Info.plist"
for name in HP250Dmic.kext HP250Dmic.driver; do
    mkdir -p "$project/build/$name/Contents/Resources"
    cp "$project/licenses/"* "$project/build/$name/Contents/Resources/"
    codesign --force --sign - --timestamp=none "$project/build/$name"
    codesign --verify --strict "$project/build/$name"
    plutil -lint "$project/build/$name/Contents/Info.plist"
done
python3 - <<'PY'
from pathlib import Path
import hashlib,json
b=Path('build')
manifest={}
for name in ['HP250Dmic.kext','HP250Dmic.driver']:
    manifest[name]={str(p.relative_to(b/name)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (b/name).rglob('*') if p.is_file()}
(b/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('HP-Kext und CoreAudio-Plug-in gebaut; noch kein Hardware-Funktionsnachweis.')
PY
