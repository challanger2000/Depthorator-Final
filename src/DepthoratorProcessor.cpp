#include "DepthoratorProcessor.h"
#include "DepthoratorIDs.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace Depthorator {
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {
constexpr double kPi = 3.14159265358979323846;

struct QueueCursor {
    IParamValueQueue* queue {nullptr};
    int32 pointIndex {0};
    int32 pointCount {0};
    int32 nextOffset {0};
    ParamValue nextValue {0.0};
    bool hasNext {false};
};

bool loadNextPoint(QueueCursor& cursor) {
    if (!cursor.queue || cursor.pointIndex >= cursor.pointCount) {
        cursor.hasNext = false;
        return false;
    }
    int32 offset = 0;
    ParamValue value = 0.0;
    if (cursor.queue->getPoint(cursor.pointIndex, offset, value) != kResultTrue) {
        cursor.hasNext = false;
        return false;
    }
    cursor.nextOffset = std::max<int32>(0, offset);
    cursor.nextValue = std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
    ++cursor.pointIndex;
    cursor.hasNext = true;
    return true;
}

bool isContinuousParameter(std::size_t index) {
    return index != 0u && index != 10u && index != 11u;
}
}

Processor::Processor() { setControllerClass(kControllerUID); }

tresult PLUGIN_API Processor::initialize(FUnknown* context) {
    auto r = AudioEffect::initialize(context);
    if (r != kResultOk) return r;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                  SpeakerArrangement* outputs, int32 numOuts) {
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    return kResultFalse;
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 size) {
    return (size == kSample32 || size == kSample64) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
    sampleRate_ = (std::isfinite(setup.sampleRate) && setup.sampleRate > 1000.0) ? setup.sampleRate : 44100.0;
    // 8 ms one-pole smoothing for continuous user parameters. Delay time itself
    // retains the dedicated dual-read-head crossfade below.
    parameterSmoothCoeff_ = std::exp(-1.0 / (0.008 * sampleRate_));
    resetDSP();
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Processor::setActive(TBool state) {
    if (state) resetDSP();
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API Processor::setProcessing(TBool state) {
    if (state) resetDSP();
    return AudioEffect::setProcessing(state);
}

void Processor::syncSmoothedToTargets() {
    smoothed_ = values_;
}

void Processor::resetDSP() {
    const auto n = static_cast<std::size_t>(std::ceil(sampleRate_ * 5.0)) + 8u;
    delayL_.assign(n, 0.0);
    delayR_.assign(n, 0.0);
    writePos_ = 0;
    feedbackLP_L_ = feedbackLP_R_ = 0.0;
    duckEnvelope_ = 0.0;
    activeDelaySamples_ = oldDelaySamples_ = targetDelaySamples_ = 0.0;
    timeCrossfade_ = 1.0;
    delayTimeInitialized_ = false;
    syncSmoothedToTargets();
    reverb_.prepare(sampleRate_);
    reverb_.reset();
}

double Processor::currentDelaySeconds(const ProcessData& data) const {
    const double time = values_[0];
    if (values_[10] < 0.5) return 0.020 + time * 1.980;

    double tempo = 120.0;
    if (data.processContext &&
        (data.processContext->state & ProcessContext::kTempoValid) &&
        std::isfinite(data.processContext->tempo))
        tempo = std::clamp(static_cast<double>(data.processContext->tempo), 20.0, 400.0);

    static constexpr double beats[] = {
        0.125, 0.0833333333333333, 0.1875,
        0.25, 0.166666666666667, 0.375,
        0.5, 0.333333333333333, 0.75,
        1.0, 0.666666666666667, 1.5,
        2.0, 1.33333333333333, 3.0, 4.0
    };
    const int index = std::clamp(static_cast<int>(std::round(time * 15.0)), 0, 15);
    return (60.0 / tempo) * beats[index];
}

template <typename Sample>
void Processor::processBlock(Sample** in, Sample** out, int32 numSamples, int32 channels, ProcessData& data) {
    if (delayL_.empty()) resetDSP();
    const auto bufferSize = delayL_.size();

    std::array<QueueCursor, 12> cursors {};
    if (data.inputParameterChanges) {
        const auto count = data.inputParameterChanges->getParameterCount();
        for (int32 q = 0; q < count; ++q) {
            auto* queue = data.inputParameterChanges->getParameterData(q);
            if (!queue) continue;
            const auto id = queue->getParameterId();
            if (id < kTime || id > kMode) continue;
            auto& cursor = cursors[static_cast<std::size_t>(id - kTime)];
            cursor.queue = queue;
            cursor.pointCount = queue->getPointCount();
            loadNextPoint(cursor);
        }
    }

    const double duckAttack = std::exp(-1.0 / (0.006 * sampleRate_));
    const double duckRelease = std::exp(-1.0 / (0.240 * sampleRate_));
    const double crossfadeStep = 1.0 / std::max(1.0, 0.025 * sampleRate_);

    auto readTap = [&](const std::vector<double>& buffer, double delaySamples) {
        const double ds = std::clamp(delaySamples, 1.0, static_cast<double>(bufferSize - 3));
        const auto di = static_cast<std::size_t>(ds);
        const double frac = ds - static_cast<double>(di);
        const auto p0 = (writePos_ + bufferSize - di) % bufferSize;
        const auto p1 = (p0 + bufferSize - 1u) % bufferSize;
        return buffer[p0] * (1.0 - frac) + buffer[p1] * frac;
    };

    for (int32 sample = 0; sample < numSamples; ++sample) {
        bool delayLawChanged = false;

        for (std::size_t p = 0; p < cursors.size(); ++p) {
            auto& cursor = cursors[p];
            while (cursor.hasNext && cursor.nextOffset <= sample) {
                values_[p] = cursor.nextValue;
                if (p == 0u || p == 10u) delayLawChanged = true;
                loadNextPoint(cursor);
            }

            if (isContinuousParameter(p)) {
                smoothed_[p] = parameterSmoothCoeff_ * smoothed_[p] +
                               (1.0 - parameterSmoothCoeff_) * values_[p];
            } else {
                smoothed_[p] = values_[p];
            }
        }

        double requestedDelay = std::clamp(currentDelaySeconds(data) * sampleRate_,
                                           1.0, static_cast<double>(bufferSize - 3));
        if (!delayTimeInitialized_) {
            activeDelaySamples_ = oldDelaySamples_ = targetDelaySamples_ = requestedDelay;
            timeCrossfade_ = 1.0;
            delayTimeInitialized_ = true;
        } else if ((delayLawChanged || std::abs(requestedDelay - targetDelaySamples_) > 0.5) &&
                   std::abs(requestedDelay - targetDelaySamples_) > 0.5) {
            activeDelaySamples_ = (timeCrossfade_ < 0.5) ? oldDelaySamples_ : targetDelaySamples_;
            oldDelaySamples_ = activeDelaySamples_;
            targetDelaySamples_ = requestedDelay;
            timeCrossfade_ = 0.0;
        }

        const double feedback = std::min(0.94, smoothed_[1] * 0.94);
        const double depth = smoothed_[2];
        const double curve = smoothed_[3];
        const double width = smoothed_[7];
        const double duck = smoothed_[8];
        const double mix = smoothed_[9];
        const int mode = std::clamp(static_cast<int>(std::round(values_[11] * 2.0)), 0, 2);

        const double depthAmount = std::pow(depth, 1.15);
        const double progressionRate = 0.10 + 0.90 * std::pow(curve, 1.35);
        const double targetCutoff = 18500.0 * std::pow(0.12, depthAmount) + 900.0 * depthAmount;
        const double cutoff = 20000.0 + (targetCutoff - 20000.0) * (depthAmount * progressionRate);
        const double lpA = std::exp(-2.0 * kPi * cutoff / sampleRate_);

        reverb_.setParameters(smoothed_[4], smoothed_[5], smoothed_[6]);

        const double dryGain = std::cos(mix * 0.5 * kPi);
        const double wetGain = std::sin(mix * 0.5 * kPi);
        const double repeatToRoom = 0.10 + depthAmount * (0.25 + 0.65 * progressionRate);
        const double roomOutput = 0.22 + depthAmount * 0.58;
        const double directEcho = 1.0 - depthAmount * 0.15;
        const double roomIntoFeedback = depthAmount * progressionRate * 0.18;

        double inL = in[0] ? static_cast<double>(in[0][sample]) : 0.0;
        double inR = channels > 1 && in[1] ? static_cast<double>(in[1][sample]) : inL;
        if (!std::isfinite(inL)) inL = 0.0;
        if (!std::isfinite(inR)) inR = 0.0;

        const double detector = std::max(std::abs(inL), std::abs(inR));
        const double coeff = detector > duckEnvelope_ ? duckAttack : duckRelease;
        duckEnvelope_ = coeff * duckEnvelope_ + (1.0 - coeff) * detector;
        const double duckActivity = duckEnvelope_ / (duckEnvelope_ + 0.030);
        const double duckGain = 1.0 - duck * 0.94 * duckActivity;

        double wetL = 0.0;
        double wetR = 0.0;
        if (timeCrossfade_ < 1.0) {
            const double x = std::clamp(timeCrossfade_, 0.0, 1.0);
            const double y = x * x * (3.0 - 2.0 * x);
            wetL = readTap(delayL_, oldDelaySamples_) * (1.0 - y) +
                   readTap(delayL_, targetDelaySamples_) * y;
            wetR = readTap(delayR_, oldDelaySamples_) * (1.0 - y) +
                   readTap(delayR_, targetDelaySamples_) * y;
            timeCrossfade_ = std::min(1.0, timeCrossfade_ + crossfadeStep);
            if (timeCrossfade_ >= 1.0) activeDelaySamples_ = targetDelaySamples_;
        } else {
            wetL = readTap(delayL_, activeDelaySamples_);
            wetR = readTap(delayR_, activeDelaySamples_);
        }

        if (mode == 0) {
            const double mono = 0.5 * (wetL + wetR);
            wetL = wetR = mono;
        }

        double revL = 0.0;
        double revR = 0.0;
        reverb_.process(inL * 0.18 + wetL * repeatToRoom,
                        inR * 0.18 + wetR * repeatToRoom,
                        revL, revR);

        feedbackLP_L_ = (1.0 - lpA) * wetL + lpA * feedbackLP_L_;
        feedbackLP_R_ = (1.0 - lpA) * wetR + lpA * feedbackLP_R_;

        const double fbL = feedbackLP_L_ + revL * roomIntoFeedback;
        const double fbR = feedbackLP_R_ + revR * roomIntoFeedback;

        if (mode == 2) {
            const double inputMono = 0.5 * (inL + inR);
            delayL_[writePos_] = inputMono + fbR * feedback;
            delayR_[writePos_] = fbL * feedback;
        } else {
            delayL_[writePos_] = inL + fbL * feedback;
            delayR_[writePos_] = inR + fbR * feedback;
        }

        const double preWidthL = (wetL * directEcho + revL * roomOutput) * duckGain;
        const double preWidthR = (wetR * directEcho + revR * roomOutput) * duckGain;
        const double fxMid = 0.5 * (preWidthL + preWidthR);
        const double fxSide = 0.5 * (preWidthL - preWidthR) * width;
        double fxL = fxMid + fxSide;
        double fxR = fxMid - fxSide;

        if (!std::isfinite(fxL)) fxL = 0.0;
        if (!std::isfinite(fxR)) fxR = 0.0;

        out[0][sample] = static_cast<Sample>(inL * dryGain + fxL * wetGain);
        if (channels > 1 && out[1])
            out[1][sample] = static_cast<Sample>(inR * dryGain + fxR * wetGain);

        writePos_ = (writePos_ + 1u) % bufferSize;
    }
}

tresult PLUGIN_API Processor::process(ProcessData& data) {
    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0) return kResultOk;
    const auto channels = std::min<int32>(2, std::min(data.inputs[0].numChannels, data.outputs[0].numChannels));
    if (channels <= 0) return kResultOk;

    if (data.symbolicSampleSize == kSample32)
        processBlock(data.inputs[0].channelBuffers32, data.outputs[0].channelBuffers32,
                     data.numSamples, channels, data);
    else if (data.symbolicSampleSize == kSample64)
        processBlock(data.inputs[0].channelBuffers64, data.outputs[0].channelBuffers64,
                     data.numSamples, channels, data);
    else
        return kResultFalse;

    bool silent = true;
    if (data.symbolicSampleSize == kSample32) {
        for (int32 c = 0; c < channels && silent; ++c)
            if (data.outputs[0].channelBuffers32[c])
                for (int32 i = 0; i < data.numSamples; ++i)
                    if (data.outputs[0].channelBuffers32[c][i] != 0.f) { silent = false; break; }
    } else {
        for (int32 c = 0; c < channels && silent; ++c)
            if (data.outputs[0].channelBuffers64[c])
                for (int32 i = 0; i < data.numSamples; ++i)
                    if (data.outputs[0].channelBuffers64[c][i] != 0.0) { silent = false; break; }
    }
    data.outputs[0].silenceFlags = silent ? ((Steinberg::uint64{1} << channels) - 1) : 0;
    return kResultOk;
}

tresult PLUGIN_API Processor::setState(IBStream* state) {
    if (!state) return kInvalidArgument;
    IBStreamer streamer(state, kLittleEndian);
    for (auto& value : values_) {
        double x = 0.0;
        if (!streamer.readDouble(x) || !std::isfinite(x)) return kResultFalse;
        value = std::clamp(x, 0.0, 1.0);
    }
    syncSmoothedToTargets();
    delayTimeInitialized_ = false;
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state) {
    if (!state) return kInvalidArgument;
    IBStreamer streamer(state, kLittleEndian);
    for (const auto value : values_)
        if (!streamer.writeDouble(value)) return kResultFalse;
    return kResultOk;
}

} // namespace Depthorator
