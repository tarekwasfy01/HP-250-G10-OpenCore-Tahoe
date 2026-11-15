// Interface shared by HP250Dmic.kext (HP250DmicUserClient) and HP250Dmic.driver (HAL plug-in).
#pragma once

#define kHP250DmicServiceClass   "HP250Dmic"
#define kHP250DmicUserClientType 0          // IOServiceOpen type
#define kHP250DmicRingMemoryType 0          // IOConnectMapMemory64 type: the 64 KB capture ring (read-only)

enum {
    kHP250DmicMethodGetInfo     = 0,   // out: version, state bits, rate, channels, ring bytes, frame bytes
    kHP250DmicMethodStart       = 1,   // out: none. Boots the DSP if needed and starts the DMIC pipeline
    kHP250DmicMethodStop        = 2,   // out: none
    kHP250DmicMethodGetPosition = 3,   // out: DMA write position in ring (bytes), mach_absolute_time, generation
    kHP250DmicMethodCount
};

enum {
    kHP250DmicStateDspUp     = 1u << 0,
    kHP250DmicStateStreaming = 1u << 1,
    kHP250DmicStateService   = 1u << 2,
};

#define kHP250DmicInterfaceVersion 1
#define kHP250DmicVersionString    "0.1.0"   // kept in sync with Info.plist by tools/set-version.sh
// Ring format: S32_LE, 4 interleaved channels, 48 kHz. ch0/ch1 = mic pair A, ch2 == ch3 = mic B.
