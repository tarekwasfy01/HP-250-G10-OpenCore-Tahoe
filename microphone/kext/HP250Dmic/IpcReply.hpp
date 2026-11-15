// SOF v2.2 src/ipc/ipc3/handler.c ipc_cmd(): standard replies use GLB_REPLY.
#pragma once
#include <stdint.h>
static inline bool hpIpcReplyHeaderValid(uint32_t request, uint32_t bytes, uint32_t command) {
    if (bytes < 12 || bytes > 0x2000 || (bytes & 3)) return false;
    if (command == 0x10000000) return bytes == 12; // sof_ipc_reply
    return command == request; // command-specific extended reply
}
