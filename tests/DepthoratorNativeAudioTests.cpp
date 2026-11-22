#include "DepthoratorProcessor.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/utility/memoryibstream.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

#define REQUIRE_TRUE(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "NATIVE_AUDIO_FAILED line=%d expression=%s\\n", __LINE__, #condition); \
    return false; \
} } while (false)

using namespace Steinberg;
using namespace Steinberg::Vst;

static double dryNote(int sample, int onset, int rate) {
    const double time = static_cast<double>(sample - onset)/rate;
    if (time < 0.0 || time >= 0.150) return 0.0;
    const double attack = std::min(1.0, time / 0.004);
    return 0.67 * attack * (1.0 - time / 0.150) *
           std::sin(2.0 * 3.14159265358979323846 * 220.0 * time);
}

static bool applyState(Depthorator::Processor& processor, bool enabled) {
    Steinberg::ResizableMemoryIBStream state;
    IBStreamer values(&state, kLittleEndian);
    const std::array<double, 13> params{{
        0.375, 0.76, 0.20, 0.50, 0.65, 0.45, 0.50, 0.90,
        0.0, 0.75, 0.0, 0.50, enabled ? 1.0 : 0.0
    }};
    for (auto value:params) REQUIRE_TRUE(values.writeDouble(value));
    state.rewind();
    REQUIRE_TRUE(processor.setState(&state) == kResultOk);
    return true;
}

template <typename Sample>
static bool run(int rate) {
    constexpr int blockSize = 256;
    const int note1 = static_cast<int>(rate * 0.100);
    const int note2 = static_cast<int>(rate * 1.500);
    const int frames = static_cast<int>(rate * 2.700);
    Depthorator::Processor off, on;

    REQUIRE_TRUE(off.initialize(nullptr) == kResultOk);
    REQUIRE_TRUE(on.initialize(nullptr) == kResultOk);
    ProcessSetup setup{};
    setup.processMode = kOffline;
    setup.symbolicSampleSize = std::is_same<Sample, float>::value ? kSample32 : kSample64;
    setup.maxSamplesPerBlock = blockSize;
    setup.sampleRate = rate;
    REQUIRE_TRUE(off.setupProcessing(setup) == kResultOk);
    REQUIRE_TRUE(on.setupProcessing(setup) == kResultOk);
    REQUIRE_TRUE(off.setActive(true) == kResultOk);
    REQUIRE_TRUE(on.setActive(true) == kResultOk);
    REQUIRE_TRUE(applyState(off,false));
    REQUIRE_TRUE(applyState(on,true));
    REQUIRE_TRUE(off.setProcessing(true) == kResultOk);
    REQUIRE_TRUE(on.setProcessing(true) == kResultOk);

    std::array<Sample, blockSize> dryL{}, dryR{};
    std::array<Sample, blockSize> offL{}, offR{}, onL{}, onR{};
    Sample* inputs[2] {dryL.data(),dryR.data()};
    Sample* offOutputs[2] {offL.data(),offR.data()};
    Sample* onOutputs[2] {onL.data(),onR.data()};

    AudioBusBuffers inputBus{};
    inputBus.numChannels = 2;
    AudioBusBuffers offBus{},onBus{};
    offBus.numChannels = onBus.numChannels = 2;
    if constexpr (std::is_same<Sample,float>::value) {
        inputBus.channelBuffers32 = inputs;
        offBus.channelBuffers32 = offOutputs;
        onBus.channelBuffers32 = onOutputs;
    } else {
        inputBus.channelBuffers64 = inputs;
        offBus.channelBuffers64 = offOutputs;
        onBus.channelBuffers64 = onOutputs;
    }

    ProcessData offData{},onData{};
    for (auto* d : {&offData,&onData}) {
        d->processMode = kOffline;
        d->symbolicSampleSize = setup.symbolicSampleSize;
        d->numInputs = d->numOutputs = 1;
        d->inputs = &inputBus;
    }
    offData.outputs = &offBus;
    onData.outputs = &onBus;

    long firstDeltas = 0, duckDeltas = 0, lateDeltas = 0;
    double preDuckPeak = 0.0, duckRmsDelta = 0.0;
    for (int start=0;start<frames;start+=blockSize) {
        const int n = std::min(blockSize,frames-start);
        for (int j=0;j<n;++j) {
            const double dry = dryNote(start+j,note1,rate)
                             + dryNote(start+j,note2,rate);
            dryL[j] = static_cast<Sample>(dry);
            dryR[j] = static_cast<Sample>(dry);
        }
        offData.numSamples = onData.numSamples = n;
        REQUIRE_TRUE(off.process(offData) == kResultOk);
        REQUIRE_TRUE(on.process(onData) == kResultOk);
        for (int j=0;j<n;++j) {
            const int i = start+j;
            const auto a = static_cast<double>(offL[j]);
            const auto b = static_cast<double>(onL[j]);
            REQUIRE_TRUE(std::isfinite(a) && std::isfinite(b));
            const double delta = std::abs(a-b);
            if (i<note2) {
                if (offL[j] != onL[j] || offR[j] != onR[j]) ++firstDeltas;
                preDuckPeak = std::max(preDuckPeak,delta);
            } else if (i < note2 + static_cast<int>(rate * 0.340)) {
                if (delta>1.0e-7) ++duckDeltas;
                duckRmsDelta+=delta*delta;
            } else if (i > note2 + static_cast<int>(rate * 0.450)) {
                if (offL[j] != onL[j] || offR[j] != onR[j]) ++lateDeltas;
            }
        }
    }
    REQUIRE_TRUE(firstDeltas == 0);
    REQUIRE_TRUE(preDuckPeak == 0.0);
    REQUIRE_TRUE(duckDeltas > 50);
    REQUIRE_TRUE(duckRmsDelta > 1.0e-8);
    REQUIRE_TRUE(lateDeltas == 0);
    std::printf("VST3_NATIVE_DRY_WET_PARITY_PASS rate=%d bits=%d first_sample_deltas=%ld duck_deltas=%ld recovered=%s\\n",
        rate,static_cast<int>(sizeof(Sample)*8),firstDeltas,duckDeltas,lateDeltas==0?"true":"false");
    REQUIRE_TRUE(off.setProcessing(false) == kResultOk);
    REQUIRE_TRUE(on.setProcessing(false) == kResultOk);
    REQUIRE_TRUE(off.setActive(false) == kResultOk);
    REQUIRE_TRUE(on.setActive(false) == kResultOk);
    REQUIRE_TRUE(off.terminate() == kResultOk);
    REQUIRE_TRUE(on.terminate() == kResultOk);
    return true;
}

int main() {
    if (!run<float>(44100)) return EXIT_FAILURE;
    if (!run<double>(48000)) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
