#include "HP250DmicUserClient.hpp"
#include "HP250DmicShared.h"
#include <IOKit/IOLib.h>
#include <sys/kauth.h>

#define super IOUserClient
OSDefineMetaClassAndStructors(HP250DmicUserClient, IOUserClient)

const IOExternalMethodDispatch HP250DmicUserClient::sMethods[kHP250DmicMethodCount] = {
    // function       scalarIn structIn scalarOut structOut
    { &HP250DmicUserClient::sGetInfo,     0, 0, 6, 0 },
    { &HP250DmicUserClient::sStart,       0, 0, 0, 0 },
    { &HP250DmicUserClient::sStop,        0, 0, 0, 0 },
    { &HP250DmicUserClient::sGetPosition, 0, 0, 3, 0 },
};

bool HP250DmicUserClient::initWithTask(task_t owningTask, void *securityToken, UInt32 type, OSDictionary *properties) {
    if (type != kHP250DmicUserClientType) return false;
    bool privileged = IOUserClient::clientHasPrivilege(owningTask, kIOClientPrivilegeLocalUser) == kIOReturnSuccess ||
                      IOUserClient::clientHasPrivilege(owningTask, kIOClientPrivilegeAdministrator) == kIOReturnSuccess;
    if (!privileged && kauth_getuid() != 202 && kauth_getuid() != 0) return false;
    if (!super::initWithTask(owningTask, securityToken, type, properties)) return false;
    task = owningTask;
    return true;
}

bool HP250DmicUserClient::start(IOService *provider) {
    owner = OSDynamicCast(HP250Dmic, provider);
    if (!owner || !super::start(provider)) return false;
    owner->retain();
    return true;
}

void HP250DmicUserClient::stop(IOService *provider) {
    if (started && owner) { owner->clientStop(); started = false; }
    super::stop(provider);
}

IOReturn HP250DmicUserClient::clientClose() {
    if (started && owner) { owner->clientStop(); started = false; }
    OSSafeReleaseNULL(owner);
    if (!isInactive()) terminate();
    return kIOReturnSuccess;
}

void HP250DmicUserClient::free() {
    OSSafeReleaseNULL(owner);
    super::free();
}

IOReturn HP250DmicUserClient::clientDied() {
    return clientClose();
}

IOReturn HP250DmicUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments *args,
                                           IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) {
    if (selector >= kHP250DmicMethodCount) return kIOReturnUnsupported;
    dispatch = const_cast<IOExternalMethodDispatch *>(&sMethods[selector]);
    return super::externalMethod(selector, args, dispatch, this, reference);
}

IOReturn HP250DmicUserClient::sGetInfo(OSObject *target, void *, IOExternalMethodArguments *args) {
    auto self = OSDynamicCast(HP250DmicUserClient, target);
    if (!self || !self->owner) return kIOReturnNotAttached;
    args->scalarOutput[0] = kHP250DmicInterfaceVersion;
    args->scalarOutput[1] = self->owner->stateBits();
    args->scalarOutput[2] = HP250Dmic::kRate;
    args->scalarOutput[3] = HP250Dmic::kChannels;
    args->scalarOutput[4] = HP250Dmic::kRingBytes;
    args->scalarOutput[5] = HP250Dmic::kFrameBytes;
    return kIOReturnSuccess;
}

IOReturn HP250DmicUserClient::sStart(OSObject *target, void *, IOExternalMethodArguments *) {
    auto self = OSDynamicCast(HP250DmicUserClient, target);
    if (!self || !self->owner) return kIOReturnNotAttached;
    if (self->started) return kIOReturnSuccess;
    IOReturn r = self->owner->clientStart();
    if (r == kIOReturnSuccess) self->started = true;
    return r;
}

IOReturn HP250DmicUserClient::sStop(OSObject *target, void *, IOExternalMethodArguments *) {
    auto self = OSDynamicCast(HP250DmicUserClient, target);
    if (!self || !self->owner) return kIOReturnNotAttached;
    if (!self->started) return kIOReturnSuccess;
    self->started = false;
    return self->owner->clientStop();
}

IOReturn HP250DmicUserClient::sGetPosition(OSObject *target, void *, IOExternalMethodArguments *args) {
    auto self = OSDynamicCast(HP250DmicUserClient, target);
    if (!self || !self->owner) return kIOReturnNotAttached;
    uint64_t pos = 0, host = 0, gen = 0;
    IOReturn r = self->owner->readPosition(&pos, &host, &gen);
    args->scalarOutput[0] = pos;
    args->scalarOutput[1] = host;
    args->scalarOutput[2] = gen;
    return r;
}

IOReturn HP250DmicUserClient::clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) {
    if (type != kHP250DmicRingMemoryType || !owner) return kIOReturnBadArgument;
    IOMemoryDescriptor *md = owner->ringMemory();      // retained; IOUserClient releases it
    if (!md) return kIOReturnNoMemory;
    *options = kIOMapReadOnly;
    *memory = md;
    return kIOReturnSuccess;
}
