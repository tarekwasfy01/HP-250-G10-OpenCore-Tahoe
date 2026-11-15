#include "../kext/HP250Dmic/IpcReplay.hpp"
#include "../kext/HP250Dmic/IpcReply.hpp"
#include <cassert>
#include <cstdio>
#include <set>

int main() {
    assert(hpIpcReplyHeaderValid(0x80010000,12,0x10000000)); // DAI generic OK/error
    assert(hpIpcReplyHeaderValid(0x60010000,20,0x60010000)); // PCM extended
    assert(!hpIpcReplyHeaderValid(0x80010000,8,0x10000000));
    assert(!hpIpcReplyHeaderValid(0x80010000,13,0x10000000));
    assert(!hpIpcReplyHeaderValid(0x80010000,20,0x10000000));
    assert(!hpIpcReplyHeaderValid(0x80010000,12,0x70000000));
    assert(!hpIpcReplyHeaderValid(0x80010000,0x2004,0x80010000));
    std::set<unsigned> nodes;
    bool params = false, configured = false;
    for (const auto &m : kIpc_setup) {
        assert(m.words >= 3 && m.words <= 64 && m.data[0] == m.words * 4);
        assert(m.data[1] == m.cmd);
        if (m.cmd == 0x30100000 || m.cmd == 0x30200000 || m.cmd == 0x30010000)
            assert(nodes.insert(m.data[2]).second);
        if (m.cmd == 0x30030000) {
            assert(nodes.count(m.data[2]) && nodes.count(m.data[3]));
        }
        if (m.cmd == 0x80010000) {
            assert(m.data[2] == 2 && m.data[3] == 0); // DMIC0
            assert(m.data[17] == 48000 && m.data[19] == 0x00200020);
            assert(m.data[21] == 1 && m.data[31] == 0x10000 && m.data[32] == 1);
            configured = true;
        }
        if (m.patch == kPatchPcmParams) {
            assert(nodes.count(m.data[2]) && m.words == 27);
            assert(m.data[6] == 84 && m.data[9] * 4096 == m.data[10]);
            assert(m.data[10] == 65536 && m.data[17] == 48000);
            assert((m.data[18] >> 16) == 2 && m.data[19] == 0x40004);
            assert(m.data[20] % 8 == 0 && m.data[10] % m.data[20] == 0);
            params = true;
        }
    }
    assert(nodes.size() == 4 && params && configured);
    for (const auto &m : kIpc_teardown) assert(nodes.erase(m.data[2]) == 1);
    assert(nodes.empty());
    puts("Actual IPC data: graph IDs, stereo PCM/page table/period geometry, PDM0 pair, teardown completeness passed.");
}
