#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Depthorator {

// Per-sample audio-onset clear controller. Based on the observed behavior of
// coolvegan/qdelay-autoclear (2026-11-22), not a copy of its GPL source.
// All work is bounded and allocation-free in the audio callback.
class AutoClearCore {
public:
    struct Frame {
        double wetGain {1.0};
        bool clearOld {false};
        std::uint32_t keepRecentFrames {0};
    };

    void prepare(double sampleRate) noexcept {
        rate_ = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
        fastUp_ = 1.0 - std::exp(-1.0 / (0.001 * rate_));
        fastDown_ = 1.0 - std::exp(-1.0 / (0.050 * rate_));
        slowUp_ = 1.0 - std::exp(-1.0 / (0.060 * rate_));
        slowDown_ = 1.0 - std::exp(-1.0 / (0.400 * rate_));
        refractoryFrames_ = static_cast<std::uint32_t>(rate_ * 0.090);
        reset();
    }

    void reset() noexcept {
        phase_ = Phase::Idle;
        gain_ = 1.0;
        fast_ = slow_ = 0.0;
        sinceOnset_ = 0;
        fadeStep_ = 0.0;
        keepFrames_ = 0;
    }

    void configure(bool enabled, double amount, double thresholdDb,
                   double fadeMilliseconds) noexcept {
        enabled_ = enabled;
        amount_ = std::clamp(std::isfinite(amount) ? amount : 0.0, 0.0, 1.0);
        thresholdDb = std::clamp(std::isfinite(thresholdDb) ? thresholdDb : -30.0, -60.0, 0.0);
        threshold_ = std::pow(10.0, thresholdDb / 20.0);
        fadeMs_ = std::clamp(std::isfinite(fadeMilliseconds) ? fadeMilliseconds : 40.0, 2.0, 500.0);
        if (!enabled_ || amount_ == 0.0) {
            phase_ = Phase::Idle;
            gain_ = 1.0;
            fast_ = slow_ = 0.0;
            sinceOnset_ = 0;
        }
    }

    Frame process(double left, double right) noexcept {
        if (!enabled_ || amount_ == 0.0) return {1.0, false, 0};

        const double peak = std::max(std::abs(std::isfinite(left) ? left : 0.0),
                                     std::abs(std::isfinite(right) ? right : 0.0));
        fast_ += (peak > fast_ ? fastUp_ : fastDown_) * (peak - fast_);
        slow_ += (peak > slow_ ? slowUp_ : slowDown_) * (peak - slow_);

        if (sinceOnset_ > refractoryFrames_ &&
            peak > threshold_ && fast_ > slow_ * 2.2 &&
            phase_ != Phase::FadeOut) {
            startFade();
        }
        if (sinceOnset_ < 0x3fffffffu) ++sinceOnset_;

        bool clear = false;
        if (phase_ == Phase::FadeOut) {
            gain_ = std::max(floor_, gain_ - fadeStep_);
            if (gain_ <= floor_) {
                gain_ = floor_;
                clear = amount_ >= 0.999999;
                phase_ = Phase::FadeIn;
                fadeStep_ = (1.0 - gain_) /
                    std::max(1.0, fadeMs_ * rate_ * 0.001);
            }
        } else if (phase_ == Phase::FadeIn) {
            gain_ = std::min(1.0, gain_ + fadeStep_);
            if (gain_ >= 1.0) phase_ = Phase::Idle;
        } else {
            gain_ = 1.0;
        }
        return {gain_, clear, keepFrames_};
    }

private:
    enum class Phase { Idle, FadeOut, FadeIn };

    void startFade() noexcept {
        sinceOnset_ = 0;
        floor_ = 1.0 - amount_;
        // Full kill needs a short fade to preserve the incoming note's first repeat.
        const double downMs = amount_ >= 0.999999 ? std::min(fadeMs_, 12.0) : fadeMs_;
        const double frames = std::max(1.0, downMs * rate_ * 0.001);
        keepFrames_ = static_cast<std::uint32_t>(frames) + 64u;
        fadeStep_ = std::max(0.0, (gain_ - floor_) / frames);
        phase_ = Phase::FadeOut;
    }

    double rate_ {44100.0};
    bool enabled_ {false};
    double amount_ {1.0};
    double threshold_ {0.0316227766};
    double fadeMs_ {40.0};
    double fast_ {0.0}, slow_ {0.0};
    double fastUp_ {0.0}, fastDown_ {0.0}, slowUp_ {0.0}, slowDown_ {0.0};
    double gain_ {1.0}, floor_ {0.0}, fadeStep_ {0.0};
    std::uint32_t sinceOnset_ {0}, refractoryFrames_ {0}, keepFrames_ {0};
    Phase phase_ {Phase::Idle};
};

} // namespace Depthorator
