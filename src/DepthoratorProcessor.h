#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "DepthoratorReverb.h"
#include "AutoClearCore.h"
#include <array>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace Depthorator {

class Processor : public Steinberg::Vst::AudioEffect {
public:
    Processor();
    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor()); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;

private:
    void resetDSP();
    double currentDelaySeconds(const Steinberg::Vst::ProcessData& data) const;

    template <typename Sample>
    void processBlock(Sample** inputs, Sample** outputs, Steinberg::int32 startSample,
                      Steinberg::int32 numSamples, Steinberg::int32 channels,
                      const Steinberg::Vst::ProcessData& data);

    std::array<double, 16> values_ {{
        0.375, 0.42, 0.0, 0.5, 0.74, 0.48, 0.55, 0.75, 0.25, 0.35, 1.0, 0.5,
        0.0, 1.0, 0.5, 38.0 / 498.0
    }};

    double sampleRate_ {44100.0};
    std::vector<double> delayL_;
    std::vector<double> delayR_;
    // Lazy invalidation makes a full 5-second delay clear O(1) on the audio
    // thread, while allowing the incoming note's latest attack to survive.
    std::vector<std::uint64_t> delayFrame_;
    std::uint64_t frameSerial_ {0};
    std::uint64_t liveAfterFrame_ {0};
    AutoClearCore autoClear_;
    std::size_t writePos_ {0};
    double feedbackLP_L_ {0.0};
    double feedbackLP_R_ {0.0};
    double duckEnvelope_ {0.0};

    // Delay-time changes use two simultaneous read heads and a short
    // peak-safe smooth crossfade. This avoids clicks and avoids the pitch swoop
    // produced by simply slewing one delay read head through the buffer.
    double activeDelaySamples_ {0.0};
    double oldDelaySamples_ {0.0};
    double targetDelaySamples_ {0.0};
    double timeCrossfade_ {1.0};
    bool delayTimeInitialized_ {false};

    ReverbEngine reverb_;
};

} // namespace Depthorator
