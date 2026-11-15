#include "HP250Dmic.hpp"
#include "Regs.hpp"
#include "HP250DmicShared.h"
#include <IOKit/IOLib.h>
#include <libkern/version.h>
#include <kern/clock.h>

#define super IOService
OSDefineMetaClassAndStructors(HP250Dmic, IOService)

#include <stdarg.h>
#define LOG(fmt, ...) logLine(fmt, ##__VA_ARGS__)
static constexpr unsigned kLogLines = 128;

static constexpr uint32_t kPollMs = 2000;      // stream observation interval
static constexpr uint32_t kPollCount = 90;     // 3 minutes, then stop polling

static const char *hpContains(const char *text, const char *needle, size_t limit) {
    if (!text || !needle) return nullptr;
    for (size_t i = 0; i < limit && text[i]; ++i) {
        size_t j = 0;
        while (i + j < limit && text[i+j] && needle[j] && text[i+j] == needle[j]) ++j;
        if (!needle[j]) return text+i;
    }
    return nullptr;
}

static bool bootArg(const char *name) {
    char buf[16];
    return PE_parse_boot_argn(name, buf, sizeof(buf));
}

static IOPMPowerState gPowerStates[2] = {
    { kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 },
};

// Become a power child of the HDA driver, but only once it has attached (v0.4.4 joined the PCI device at
// load time, before AppleHDA, and the function idled before AppleHDA could initialize it). While we hold
// state 1 the HDA driver cannot enter its idle state, in which it holds the controller in reset (CRST=0)
// and the DSP loses everything. Never joins the PCI device directly.
void HP250Dmic::joinPowerParent() {
    if (pmJoined || !serviceMode) return;
    IOService *parent = nullptr;
    if (OSIterator *it = pci->getChildIterator(gIOServicePlane)) {
        while (OSObject *o = it->getNextObject()) {
            IOService *s = OSDynamicCast(IOService, o);
            if (!s || s == this || s->isInactive()) continue;
            const char *cn = s->getMetaClass()->getClassName();
            if (cn && hpContains(const_cast<char *>(cn), "HDA", 64)) { parent = s; break; }
        }
        it->release();
    }
    if (!parent) {                                    // retry from the timer (every 2 s), give up after 60 s
        if (++pmSearchTicks == 30) { LOG("PM: no HDA driver on this controller - no power hold (unsupported setup)"); pmSearchTicks = 1000; }
        return;
    }
    // Only AppleHDA idles the controller into reset while it stays the PCI device's driver; that is
    // what the power hold is for. HDAUniversal keeps the controller up by itself, and a power child
    // asking for "off" made it power the function down to D3 (audio out lost, v0.4.8 on Tahoe).
    // tgldmicpm=1 forces joining, tgldmicpm=0 never joins.
    const char *pcn = parent->getMetaClass()->getClassName();
    int pmArg = -1;
    PE_parse_boot_argn("tgldmicpm", &pmArg, sizeof(pmArg));
    bool join = pmArg == 1 || (pmArg != 0 && hpContains(const_cast<char *>(pcn), "AppleHDA", 64));
    if (!join) {
        LOG("PM: HDA driver is %s - not joining its power tree (it keeps the controller powered)%s", pcn,
            pmArg == 0 ? " [tgldmicpm=0]" : "");
        pmSearchTicks = 1000;
        return;
    }
    PMinit();
    parent->joinPMtree(this);
    if (registerPowerDriver(this, gPowerStates, 2) != kIOReturnSuccess) {
        PMstop();
        LOG("PM: registerPowerDriver failed - running without power holds");
        pmSearchTicks = 1000;
        return;
    }
    pmParent = parent; pmParent->retain();
    pmJoined = true;
    changePowerStateTo(0);                             // idle until a client records
    LOG("PM: power child of %s (power state %u)", parent->getMetaClass()->getClassName(), (unsigned)parent->getPowerState());
}

IOReturn HP250Dmic::setPowerState(unsigned long state, IOService *) {
    unsigned long old = pmState;
    pmState = state;
    if (state == 0 && old == 1 && gate && (streaming || setupDone)) gate->runAction(gatedPowerLost);
    return kIOPMAckImplied;
}

const char *HP250Dmic::platformName() const {
    switch (platformFamily) {
        case kPlatformCometLake: return "Comet Lake / Cannon Lake (cAVS 1.8)";
        case kPlatformTigerLake: return "HP 250 G10 Raptor Lake (cAVS 2.5)";
        default: return "Unknown";
    }
}

IOService *HP250Dmic::probe(IOService *provider, SInt32 *score) {
    if (bootArg("-tgldmicoff")) return nullptr;
    auto dev = OSDynamicCast(IOPCIDevice, provider);
    if (!dev) return nullptr;

    pciId = dev->configRead32(kIOPCIConfigVendorID);
    uint16_t vendor = pciId & 0xFFFF;
    uint16_t device = (pciId >> 16) & 0xFFFF;

    if (vendor != 0x8086) {
        IOLog("HP250Dmic: probe: unexpected PCI vendor %04x (id %08x), not attaching\n", vendor, pciId);
        return nullptr;
    }

    if (device != 0x51ca || dev->configRead16(0x2c) != 0x103c || dev->configRead16(0x2e) != 0x8b34)
        return nullptr;
    platformFamily = kPlatformTigerLake; // cAVS 2.5, Raptor Lake HP target
    IOLog("HP250Dmic: probe: attaching to %s (PCI 8086:%04x)\n", platformName(), device);
    return super::probe(provider, score);
}

void HP250Dmic::logLine(const char *fmt, ...) {
    char buf[200];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    IOLog("HP250Dmic: %s\n", buf);
    if (!logRing) logRing = OSArray::withCapacity(kLogLines);
    if (!logRing) return;
    if (logRing->getCount() >= kLogLines) logRing->removeObject(0);
    if (auto s = OSString::withCString(buf)) { logRing->setObject(s); s->release(); }
}

bool HP250Dmic::memoryDecoded() {
    return pci->configRead16(kIOPCIConfigCommand) & kIOPCICommandMemorySpace;
}

// Only trust BAR0 once memory decoding is on, the device is in D0 and GCAP looks sane.
bool HP250Dmic::tryInit() {
    if (ready) return true;
    if (!memoryDecoded() || !inD0()) return false;
    if (!(r32(HDA::GCTL) & 1)) {                     // CRST=0: HDA driver idle; registers read 0 until it wakes
        if (!resetLogged) { LOG("controller held in reset (GCTL 0x%08x) - HDA driver idle", r32(HDA::GCTL)); resetLogged = true; }
        return false;
    }
    uint16_t gcap = r16(HDA::GCAP);
    if (gcap == 0xFFFF || gcap == 0) { badReads++; return false; }
    if (!(r32(HDA::LLCH) & HDA::CAP_NEXT_MASK)) { badReads++; return false; }   // capability list not readable yet
    if (gcap != 0x9701) return false; // verified HP descriptor geometry
    numOut = (gcap >> 12) & 0xF; numIn = (gcap >> 8) & 0xF; numBidi = (gcap >> 3) & 0x1F;
    LOG("GCAP 0x%04x: %u in, %u out, %u bidi; HDA %u.%u; GCTL 0x%08x STATESTS 0x%04x (after %u bad reads)",
        gcap, numIn, numOut, numBidi, r8(HDA::VMAJ), r8(HDA::VMIN), r32(HDA::GCTL), r16(HDA::STATESTS), badReads);
    walkCapabilities();
    if (!ppOffset) {                                  // controller went away mid-walk (D3/reset): try again
        if (++capRetries <= 3) LOG("capability list unreadable (LLCH 0x%08x) - retrying", memoryDecoded() ? r32(HDA::LLCH) : 0);
        mlOffset = spbOffset = drsmOffset = 0;
        return false;
    }
    capInIndex = (numIn > 0 ? numIn - 1 : 0);
    int inArg = -1;
    if (PE_parse_boot_argn("tgldmicin", &inArg, sizeof(inArg)) && inArg >= 0 && (uint32_t)inArg < numIn)
        capInIndex = (uint32_t)inArg;
    capStreamTag = capInIndex + 1;
    LOG("cap: configured input stream %u, tag %u", capInIndex, capStreamTag);
    ready = true;
    snapshot(true);
    return true;
}

bool HP250Dmic::inD0() {
    uint8_t pm = 0;
    if (!pci->findPCICapability(kIOPCIPowerManagementCapability, &pm)) return true;
    return (pci->configRead16(pm + 4) & 0x3) == 0;   // PMCSR.PowerState
}

bool HP250Dmic::start(IOService *provider) {
    if (!super::start(provider)) return false;
    pci = OSDynamicCast(IOPCIDevice, provider);
    if (!pci) return false;
    pci->retain();
    readDsp = false;
    captureRequested = false;
    serviceMode = true;       // CoreAudio input, started only by a client
    bootRequested = false;    // absolutely no automatic DSP boot/self-recording
    keepRunning = false;
    LOG("HP250Dmic v%s on %s (Darwin %d.%d), dsp-reads=%s, dsp-boot=%s%s",
        kHP250DmicVersionString, provider->getName(), version_major, version_minor, readDsp ? "on" : "off",
        bootRequested ? "ON" : "off", keepRunning ? " (keep running)" : "");
    if (bootRequested) LOG("DSP boot%s scheduled %u s after the controller is ready%s", captureRequested ? " + capture" : "",
                           bootDelaySec, serviceMode ? " (service mode: DSP stays up, mic exposed to Core Audio)" : "");

    loadedAt = mach_absolute_time();
    setProperty("HP250DmicBuild", "lazy-controller-ipc3-reply-v1");
    LOG("service published without MMIO or HDA power-tree registration; hardware starts on client request");

    workLoop = IOWorkLoop::workLoop();      // own loop: boot polling must not block the provider's
    if (workLoop) {
        gate = IOCommandGate::commandGate(this);
        if (gate && workLoop->addEventSource(gate) != kIOReturnSuccess) OSSafeReleaseNULL(gate);
    }
    if (!workLoop || !gate) return false;
    publish();
    registerService();
    return true;
}

void HP250Dmic::walkCapabilities() {
    uint32_t off = r32(HDA::LLCH) & HDA::CAP_NEXT_MASK;
    for (int n = 0; off && off < bar0->getLength() && n < 10; n++) {
        uint32_t hdr = r32(off);
        uint32_t id = (hdr & HDA::CAP_ID_MASK) >> HDA::CAP_ID_SHIFT;
        switch (id) {
            case HDA::CAP_PP:   ppOffset = off; break;
            case HDA::CAP_ML:   mlOffset = off; break;
            case HDA::CAP_SPB:  spbOffset = off; break;
            case HDA::CAP_DRSM: drsmOffset = off; break;
        }
        LOG("ext cap @0x%04x id %u hdr 0x%08x", off, id, hdr);
        off = hdr & HDA::CAP_NEXT_MASK;
    }
    if (ppOffset)
        LOG("PP: PPCH 0x%08x PPCTL 0x%08x PPSTS 0x%08x", r32(ppOffset + HDA::PP_PPCH),
            r32(ppOffset + HDA::PP_PPCTL), r32(ppOffset + HDA::PP_PPSTS));
}

void HP250Dmic::snapshot(bool verbose) {
    uint32_t total = numIn + numOut + numBidi;
    if (total > 30) total = 30;
    // Decoupled descriptors (PPCTL PROCEN) feed the DSP: those are HP250Dmic's own streams, never the
    // HDA driver's link streams. Also skip our capture descriptor while we own it.
    uint32_t ours = ppOffset ? (r32(ppOffset + HDA::PP_PPCTL) & 0x3FFFFFFF) : 0;
    if (ours == 0x3FFFFFFF) ours = 0;                    // all-ones read: ignore
    if (streaming || setupDone) ours |= 1u << capInIndex;
    for (uint32_t i = 0; i < total; i++) {
        if (ours & (1u << i)) continue;
        uint32_t base = HDA::SD_BASE + i * HDA::SD_STRIDE;
        uint32_t ctl = r32(base + HDA::SD_CTL);
        if (ctl == 0xFFFFFFFF) { badReads++; return; }   // controller went away (D3/reset): skip this sample
        uint32_t tag = (ctl >> HDA::SD_CTL_TAG_SHIFT) & 0xF;
        bool run = ctl & HDA::SD_CTL_RUN;
        if (tag) tagsSeen[i] = tag;
        if (run && !(everRunning & (1u << i))) {
            everRunning |= 1u << i;
            LOG("stream %u (%s%u) started: CTL 0x%08x tag %u FMT 0x%04x CBL %u LVI %u",
                i, i < numIn ? "in" : "out", i < numIn ? i : i - numIn, ctl, tag,
                r16(base + HDA::SD_FMT), r32(base + HDA::SD_CBL), r16(base + HDA::SD_LVI));
        } else if (verbose) {
            LOG("stream %u (%s%u): CTL 0x%08x tag %u", i, i < numIn ? "in" : "out",
                i < numIn ? i : i - numIn, ctl, tag);
        }
    }
    if (dsp && verbose)
        LOG("DSP: ADSPCS 0x%08x ADSPIS 0x%08x HIPCIDR 0x%08x HIPCIDA 0x%08x",
            d32(DSP::ADSPCS), d32(DSP::ADSPIS), d32(DSP::HIPCIDR), d32(DSP::HIPCIDA));
}

void HP250Dmic::publish() {
    setProperty("HP250DmicVersion", kHP250DmicVersionString);
    setProperty("HP250DmicInterface", (unsigned long long)kHP250DmicInterfaceVersion, 32);
    setProperty("HP250DmicPhase", serviceMode ? 4ULL : captureRequested ? 3ULL : bootRequested ? 2ULL : 1ULL, 32);
    if (serviceMode) {
        setProperty("ServiceClients", startCount, 32);
        setProperty("Streaming", streaming);
        setProperty("StreamGeneration", generation, 64);
        setProperty("PowerHold", pmState == 1);
    }
    setProperty("StreamsIn", numIn, 32);
    setProperty("StreamsOut", numOut, 32);
    setProperty("UsedStreamMask", everRunning, 32);
    setProperty("PPOffset", ppOffset, 32);
    setProperty("MLOffset", mlOffset, 32);
    setProperty("Samples", samples, 32);
    setProperty("Ready", ready);
    setProperty("BadReads", badReads, 32);
    setProperty("BootState", bootState, 32);
    if (bootResult[0]) setProperty("BootResult", bootResult);
    setProperty("CaptureResult", captureResult);
    setProperty("DMAUnsafe", dmaUnsafe);
    if (captureRequested) {
        setProperty("CaptureState", captureState, 32);
        if (captureResult[0]) setProperty("CaptureResult", captureResult);
        setProperty("CaptureBytes", capBytes, 64);
        if (captureState) {
            if (auto d = OSData::withBytes(chPeak, sizeof(chPeak))) { setProperty("CapturePeak", d); d->release(); }
            if (auto d = OSData::withBytes(chMeanAbs, sizeof(chMeanAbs))) { setProperty("CaptureMeanAbs", d); d->release(); }
        }
        if (capSnap && capSnap->getLength()) setProperty("CaptureSnapshot", capSnap);
        if (envLen) if (auto d = OSData::withBytes(env, envLen * 4)) { setProperty("CaptureEnvelope", d); d->release(); }
    }
    setProperty("PlatformFamily", platformName());
    setProperty("PciVendorDeviceId", pciId, 32);
    if (logRing) { if (auto copy = OSArray::withArray(logRing)) { setProperty("Log", copy); copy->release(); } }
    if (ready && ppOffset && hda) setProperty("PPCTL", r32(ppOffset + HDA::PP_PPCTL), 32);
    if (ready && dsp) setProperty("ADSPCS", d32(DSP::ADSPCS), 32);
    if (!ready) return;
    // Suggested spare descriptors for the DSP path: highest input and highest output not used by AppleHDA.
    int spareIn = -1, spareOut = -1;
    for (int i = (int)numIn - 1; i >= 0; i--) if (!(everRunning & (1u << (uint32_t)i))) { spareIn = i; break; }
    for (int i = (int)numOut - 1; i >= 0; i--) if (!(everRunning & (1u << (numIn + (uint32_t)i)))) { spareOut = i; break; }
    setProperty("SpareInputStream", (unsigned long long)(spareIn < 0 ? 0xFF : spareIn), 32);
    setProperty("SpareOutputStream", (unsigned long long)(spareOut < 0 ? 0xFF : spareOut), 32);
}

// After the observation window: every 10 s, log changes of PCI power state, GCTL and ADSPCS,
// so we can see when (and how) the DSP gets powered down while idle. Never touches BARs in D3.
void HP250Dmic::watchdog() {
    wdTicks++;
    uint8_t pmCap = 0;
    uint16_t pm = pci->findPCICapability(kIOPCIPowerManagementCapability, &pmCap) ? (pci->configRead16(pmCap + 4) & 3) : 0;
    uint32_t gctl = 0, cs = 0;
    if (pm == 0 && memoryDecoded()) { gctl = r32(HDA::GCTL); if (dsp) cs = d32(DSP::ADSPCS); }
    if (pm != wdPm || gctl != wdGctl || cs != wdAdspcs)
        LOG("wd: t=%us D%u GCTL 0x%08x ADSPCS 0x%08x streaming %d clients %u", 180 + wdTicks * 10, pm, gctl, cs,
            streaming, startCount);
    wdPm = pm; wdGctl = gctl; wdAdspcs = cs;
}

void HP250Dmic::timerFired(OSObject *owner, IOTimerEventSource *sender) {
    auto self = OSDynamicCast(HP250Dmic, owner);
    if (!self) return;
    if (self->serviceMode && !self->pmJoined && self->pmSearchTicks < 1000) self->joinPowerParent();
    if (self->tryInit() && self->memoryDecoded() && self->inD0())
        self->snapshot(false);
    // Phase 2: one boot attempt ~20 s after the controller became usable (AppleHDA settled)
    if (self->bootRequested && self->ready && self->bootState == 0 && self->samples * (kPollMs / 1000) >= self->bootDelaySec) {
        self->bootDsp();
        self->publish();
    }
    self->samples++;
    self->publish();
    if (self->samples < kPollCount) sender->setTimeoutMS(kPollMs);
    else {
        if (self->samples == kPollCount) self->logLine("observation finished: used stream mask 0x%08x", self->everRunning);
        if (self->serviceMode) { self->watchdog(); self->publish(); sender->setTimeoutMS(10000); }
        else self->publish();
    }
}

IOReturn HP250Dmic::gatedShutdown(OSObject *o, void *, void *, void *, void *) {
    auto self = OSDynamicCast(HP250Dmic, o);
    if (!self) return kIOReturnBadArgument;
    if (self->streaming || self->setupDone) self->pipelineStop();
    self->startCount = 0;
    if (self->bootState == 1 && self->keepRunning) self->corePowerDown();
    self->gating(true);
    if (!self->dmaUnsafe) self->releaseCaptureBuffers();
    return kIOReturnSuccess;
}

void HP250Dmic::stop(IOService *provider) {
    if (gate) gate->runAction(gatedShutdown); else gatedShutdown(this, nullptr, nullptr, nullptr, nullptr);
    if (pmJoined) { PMstop(); pmJoined = false; }
    OSSafeReleaseNULL(pmParent);
    if (gate) { if (workLoop) workLoop->removeEventSource(gate); OSSafeReleaseNULL(gate); }
    if (timer) {
        timer->cancelTimeout();
        if (workLoop) workLoop->removeEventSource(timer);
        OSSafeReleaseNULL(timer);
    }
    OSSafeReleaseNULL(workLoop);
    OSSafeReleaseNULL(bar4);
    OSSafeReleaseNULL(bar0);
    hda = dsp = nullptr;
    super::stop(provider);
}

void HP250Dmic::free() {
    OSSafeReleaseNULL(capSnap);
    OSSafeReleaseNULL(logRing);
    OSSafeReleaseNULL(pci);
    super::free();
}
