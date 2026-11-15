// Exercise the actual HAL conversion without opening a service or recording audio.
#include "../hal/HP250DmicDriver.cpp"
#include <cassert>
#include <cstdio>
#include <vector>
#include <climits>

int main() {
    std::vector<int32_t> data(kRingFrames * 2);
    data[(kRingFrames-1)*2] = INT32_MIN;
    data[(kRingFrames-1)*2+1] = INT32_MAX;
    data[0] = 1073741824; data[1] = -1073741824;
    data[2] = 0; data[3] = 536870912;
    gRing = data.data(); gGain = 1.f;
    AudioServerPlugInIOCycleInfo cycle{};
    cycle.mInputTime.mSampleTime = kRingFrames-1;
    float out[6]{};
    auto read = [&] {
        return DoIOOperation(gDriverRef, kObjectID_Device, kObjectID_Stream_Input,
            0, kAudioServerPlugInIOOperationReadInput, 3, &cycle, out, nullptr);
    };
    assert(read() == 0);
    assert(out[0] == -1.f && out[1] > .99999f);
    assert(out[2] == .5f && out[3] == -.5f);
    assert(out[4] == 0.f && out[5] == .25f);
    // Software input gain must clip, not wrap or read donor channels 2/3.
    gGain = 8.f; assert(read() == 0);
    assert(out[2] == 1.f && out[3] == -1.f && out[5] == 1.f);
    gRing = nullptr; assert(read() == 0);
    for (float value : out) assert(value == 0.f);
    // One guard page follows the ring under ASan: no hidden four-channel stride allowed.
    gRing = data.data(); gGain = 1.f;
    cycle.mInputTime.mSampleTime = 0;
    std::vector<float> entire(kRingFrames * 2);
    assert(DoIOOperation(gDriverRef, kObjectID_Device, kObjectID_Stream_Input, 0,
        kAudioServerPlugInIOOperationReadInput, kRingFrames, &cycle, entire.data(), nullptr) == 0);
    puts("Actual HAL: stereo ordering, signed conversion, ring wrap, clipping, unavailable-ring silence, full-ring ASan passed.");
}
