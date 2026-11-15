# HP250Dmic 0.1.0 — HP 250 G10 internal digital microphone

**Owner-confirmed audible capture, 15 November 2026**, on HP 250 G10 / i5-1334U,
Intel 8086:51CA / subsystem 103C:8B34, Tahoe 25G332. This is a custom experimental
hardware port, not support for every Intel SST laptop.

Build marker: `lazy-controller-ipc3-reply-v1`.
Kernel executable UUID: `6C1D9A1B-2356-3B90-AA3F-0B8CE3907086` in this snapshot.

## Contents and installation

- `kext/HP250Dmic`: complete kernel source and Info.plist.
- `hal/`: CoreAudio AudioServerPlugIn source.
- `firmware/`: official Intel SOF IPC3 2.2 `sof-rpl.ri` (sof-bin alias to sof-adl.ri), source metadata and Intel license.
- `dist/HP250Dmic.driver`: matching ad-hoc-signed CoreAudio plug-in.
- `pipeline.json`: minimal PDM0 stereo IPC3 graph, without donor EQ/4-mic state.
- `licenses/`: BSD-3-Clause TGLDmic/LatSOFAudio and Intel firmware terms.
- `build.sh`: standalone local kernel/HAL build using Xcode SDK kernel headers.
- `build-hal-package.sh`: builds the HAL-only .pkg; no live installation.

The kernel binary is already in the repository's `EFI/OC/Kexts/HP250Dmic.kext`
with its Kernel/Add entry enabled. Install the separate HAL package following
[the root installation guide](../README.md). Do not duplicate the kernel extension
in /Library/Extensions. The HAL stays in macOS and serves any EFI with the matching
kernel driver; the package does not automatically set the default input.

## Build from source

On Intel macOS with Xcode command-line tools:

```sh
bash microphone/build.sh
```

Outputs: `microphone/build/HP250Dmic.kext`, `HP250Dmic.driver`, per-file manifest.
Run `bash microphone/test.sh` for the host-side startup, IPC and HAL ring tests (no recording).

This compiles/signs files only. It does not install or change your EFI. Rebuilding
may change binary UUIDs/hashes; treat a new build as a separate hardware test.
The release package uses the matching `dist` plug-in, not an arbitrary rebuild.

To reproduce the packaged release HAL (use a new output path):

```sh
bash microphone/build-hal-package.sh "$HOME/Desktop/HP250Dmic-HAL-test.pkg"
```

No kernel extension, EFI or kernel collection is installed by that package. No
microphone sample is captured during building or installation.

## Port behavior

The driver checks the physical PCI controller and HP subsystem. On load it only
publishes the service: no MMIO, automatic DSP boot or AppleHDA power-tree child.
Recording initialization is refused during the first 30 seconds after driver
load; reopen the app afterwards. Controller/DSP access then occurs on the client
command gate. The existing HDA owner must already have memory decoding and power
state D0; this build does not force a bus-master or D0i3 override.

The driver boots host core 0 using signed SOF firmware. Spare high input/output
descriptors are refused if already programmed by another owner. Stream and IPC
interrupts are not enabled on AppleHDA's shared interrupt path. Capture uses a
64-KiB ring, 8192 stereo S32 frames at 48 kHz, and Float32 conversion with software
gain/clipping in the HAL. Timeouts are finite. If DMA cannot stop, buffers remain
pinned rather than being freed under the controller. Full shutdown is then needed.

The SOF standard 12-byte `SOF_IPC_GLB_REPLY` is valid for DAI_CONFIG and other
commands; the error value remains checked. Command-specific replies are separately
validated. Reference: [SOF v2.2 ipc_cmd](https://github.com/thesofproject/sof/blob/v2.2/src/ipc/ipc3/handler.c).

User confirmation establishes audible capture with this PDM0 profile; it does not
qualify every microphone-array feature, noise processing, sleep/wake or long runs.
`PORT-STATE.json` retains provenance details and current verification scope.

## Source lineage and licenses

- TGLDmic: https://github.com/labrouss/TGLDmic, commit
  `d13dafa48fe618db743a0b6fac77ad238fdaf3a1`, BSD-3-Clause.
- Minimal stereo SOF approach: https://github.com/shubhambanekar/LatSOFAudio,
  commit `41fda77bd5321464fef5ba71a44437f993215d55`, BSD-3-Clause.
- Firmware: https://github.com/thesofproject/sof-bin, Intel license included.

Local changes cover strict HP matching, two-channel kernel/HAL geometry, Raptor
Lake firmware, minimal IPC pipeline, safe descriptor selection, reply validation,
no boot-time register/power-tree activity, client access checks and failure cleanup.
Original notices are retained. No proprietary Apple graphics binaries are included
in this microphone source/package.
