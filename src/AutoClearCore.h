#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Depthorator {

// Audio-only note onset gated QDelay-style Auto Clear.
// The first note arms the detector without ever changing the audio output.
// On a later DISTINCT note, wet fades out, old buffers are invalidated, then
// wet fades in. This is ON/OFF only; timing/threshold are internal constants.
class AutoClearCore final {
public:
    struct Frame {
        double wetGain {1.0};
        bool clearOld {false};
        bool startedFullClear {false};
        bool muteFeedback {false};
    };
    void prepare(double sampleRate) noexcept {
        rate_ = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
        fastUp_ = 1.0 - std::exp(-1.0 / (0.001 * rate_));
        fastDown_ = 1.0 - std::exp(-1.0 / (0.050 * rate_));
        slowUp_ = 1.0 - std::exp(-1.0 / (0.060 * rate_));
        slowDown_ = 1.0 - std::exp(-1.0 / (0.400 * rate_));
        gateUp_ = 1.0 - std::exp(-1.0 / (0.001 * rate_));
        gateDown_ = 1.0 - std::exp(-1.0 / (0.009 * rate_));
        refractoryFrames_ = static_cast<std::uint32_t>(std::ceil(0.090 * rate_));
        rearmFrames_ = static_cast<std::uint32_t>(std::ceil(0.022 * rate_));
        fadeDownFrames_ = static_cast<std::uint32_t>(std::ceil(0.012 * rate_));
        fadeUpFrames_ = static_cast<std::uint32_t>(std::ceil(0.040 * rate_));
        reset();
    }
    void reset() noexcept {
        enabled_ = false;
        heardFirstNote_ = false;
        gateOpen_ = false;
        phase_ = Phase::Idle;
        fast_ = slow_ = gate_ = 0.0;
        wetGain_ = 1.0;
        steps_ = 0;
        sinceOnset_ = 0;
        quietFrames_ = 0;
    }
    void configure(bool enabled) noexcept {
        if (!enabled) {
            if (enabled_) reset();
        } else if (!enabled_) {
            reset();
            enabled_ = true;
        }
    }
    Frame process(double left, double right) noexcept {
        if (!enabled_) return {};
        const double peak = std::max(
            std::abs(std::isfinite(left) ? left : 0.0),
            std::abs(std::isfinite(right) ? right : 0.0));
        fast_ += (peak > fast_ ? fastUp_ : fastDown_) * (peak - fast_);
        slow_ += (peak > slow_ ? slowUp_ : slowDown_) * (peak - slow_);
        gate_ += (peak > gate_ ? gateUp_ : gateDown_) * (peak - gate_);

        bool started = false;
        if (gate_ >= kTriggerThreshold) {
            if (!gateOpen_) {
                gateOpen_ = true;
                if (!heardFirstNote_) {
                    // The FIRST note is *always* played and echoed normally.
                    heardFirstNote_ = true;
                    sinceOnset_ = 0;
                } else if (quietFrames_ >= rearmFrames_ &&
                           sinceOnset_ >= refractoryFrames_ &&
                           fast_ > slow_ * 1.7 &&
                           phase_ != Phase::FadeOut) {
                    // Require both a dry-input gap and a fresh attack.
                    // Otherwise a single sustained/oscillating note can
                    // retrigger Auto Clear and alter its first delay tail.
                    phase_ = Phase::FadeOut;
                    steps_ = 0;
                    sinceOnset_ = 0;
                    started = true;
                }
            }
            quietFrames_ = 0;
        } else if (gateOpen_) {
            if (quietFrames_ < rearmFrames_) ++quietFrames_;
            if (quietFrames_ >= rearmFrames_) gateOpen_ = false;
        }
        if (sinceOnset_ < 0x3fffffffu) ++sinceOnset_;

        bool clear = false;
        bool muteFeedback = phase_ == Phase::FadeOut;
        if (phase_ == Phase::FadeOut) {
            if (++steps_ >= fadeDownFrames_) {
                wetGain_ = 0.0;
                clear = true;
                phase_ = Phase::FadeIn;
                steps_ = 0;
            } else {
                wetGain_ = std::max(0.0, 1.0 -
                    static_cast<double>(steps_) / fadeDownFrames_);
            }
        } else if (phase_ == Phase::FadeIn) {
            if (++steps_ >= fadeUpFrames_) {
                wetGain_ = 1.0;
                phase_ = Phase::Idle;
                steps_ = 0;
            } else {
                wetGain_ = static_cast<double>(steps_) / fadeUpFrames_;
            }
        } else {
            wetGain_ = 1.0;
        }
        return {wetGain_, clear, started, muteFeedback};
    }
private:
    enum class Phase { Idle, FadeOut, FadeIn };
    static constexpr double kTriggerThreshold = 0.03162277660168379; // -30 dBFS
    double rate_ {44100.0};
    double fast_ {0.0}, slow_ {0.0}, gate_ {0.0};
    double fastUp_ {0.0}, fastDown_ {0.0};
    double slowUp_ {0.0}, slowDown_ {0.0};
    double gateUp_ {0.0}, gateDown_ {0.0};
    double wetGain_ {1.0};
    std::uint32_t sinceOnset_ {0}, quietFrames_ {0}, steps_ {0};
    std::uint32_t refractoryFrames_ {0}, rearmFrames_ {0};
    std::uint32_t fadeDownFrames_ {1}, fadeUpFrames_ {1};
    bool enabled_ {false}, heardFirstNote_ {false}, gateOpen_ {false};
    Phase phase_ {Phase::Idle};
};
} // namespace Depthorator
