#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Depthorator {

// Sidechain-style wet-only ducking driven by NEW dry-input notes.
// Never clears delay/reverb buffers or alters the feedback path. The first
// qualified note only arms the detector: its entire tail is untouched.
class AutoClearCore final {
public:
    struct Frame {
        double wetGain {1.0};
        bool beganDucking {false};
    };

    void prepare(double sampleRate) noexcept {
        rate_ = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
        gateAttack_ = 1.0 - std::exp(-1.0 / (0.001 * rate_));
        gateRelease_ = 1.0 - std::exp(-1.0 / (0.009 * rate_));
        fastRise_ = 1.0 - std::exp(-1.0 / (0.001 * rate_));
        fastFall_ = 1.0 - std::exp(-1.0 / (0.050 * rate_));
        slowRise_ = 1.0 - std::exp(-1.0 / (0.060 * rate_));
        slowFall_ = 1.0 - std::exp(-1.0 / (0.400 * rate_));
        minGap_ = static_cast<std::uint32_t>(std::ceil(rate_ * 0.022));
        minBetween_ = static_cast<std::uint32_t>(std::ceil(rate_ * 0.090));
        attackFrames_ = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(std::ceil(rate_ * 0.008)));
        holdFrames_ = static_cast<std::uint32_t>(std::ceil(rate_ * 0.065));
        releaseFrames_ = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(std::ceil(rate_ * 0.250)));
        reset();
    }

    void reset() noexcept {
        phase_ = Phase::Idle;
        enabled_ = false;
        heardFirstNote_ = false;
        gateOpen_ = false;
        gate_ = fast_ = slow_ = 0.0;
        gain_ = 1.0;
        step_ = 0.0;
        quietFrames_ = 0;
        framesSinceNote_ = 0;
        stateFrames_ = 0;
    }

    void configure(bool enabled) noexcept {
        if (!enabled) {
            // Restore absolute unity for OFF (including project recall).
            reset();
        } else if (!enabled_) {
            reset();
            enabled_ = true;
        }
    }

    Frame process(double left, double right) noexcept {
        if (!enabled_) return {1.0, false};
        const double l = std::isfinite(left) ? std::abs(left) : 0.0;
        const double r = std::isfinite(right) ? std::abs(right) : 0.0;
        const double peak = std::max(l, r);
        gate_ += (peak > gate_ ? gateAttack_ : gateRelease_) * (peak - gate_);
        fast_ += (peak > fast_ ? fastRise_ : fastFall_) * (peak - fast_);
        slow_ += (peak > slow_ ? slowRise_ : slowFall_) * (peak - slow_);

        bool beganDucking = false;
        // A quiet gap is essential without MIDI: otherwise a long first note
        // or amplitude modulation can be misidentified as a second note.
        if (gate_ >= kGateThreshold) {
            if (!gateOpen_) {
                gateOpen_ = true;
                if (!heardFirstNote_) {
                    heardFirstNote_ = true;
                    framesSinceNote_ = 0;
                } else if (quietFrames_ >= minGap_ &&
                           framesSinceNote_ >= minBetween_ &&
                           fast_ > slow_ * 1.7) {
                    startDuck();
                    beganDucking = true;
                    framesSinceNote_ = 0;
                }
            }
            quietFrames_ = 0;
        } else if (gateOpen_) {
            if (quietFrames_ < minGap_) ++quietFrames_;
            if (quietFrames_ >= minGap_) gateOpen_ = false;
        }
        if (framesSinceNote_ < 0x3fffffffu) ++framesSinceNote_;

        switch (phase_) {
        case Phase::Idle:
            gain_ = 1.0;
            break;
        case Phase::Attack:
            gain_ = std::max(kDuckedGain, gain_ - step_);
            if (++stateFrames_ >= attackFrames_ || gain_ <= kDuckedGain) {
                gain_ = kDuckedGain;
                phase_ = Phase::Hold;
                stateFrames_ = 0;
            }
            break;
        case Phase::Hold:
            if (++stateFrames_ >= holdFrames_) {
                phase_ = Phase::Release;
                stateFrames_ = 0;
                step_ = (1.0 - gain_) / static_cast<double>(releaseFrames_);
            }
            break;
        case Phase::Release:
            gain_ = std::min(1.0, gain_ + step_);
            if (++stateFrames_ >= releaseFrames_ || gain_ >= 1.0) {
                gain_ = 1.0;
                phase_ = Phase::Idle;
            }
            break;
        }
        return {gain_, beganDucking};
    }

private:
    enum class Phase { Idle, Attack, Hold, Release };
    void startDuck() noexcept {
        phase_ = Phase::Attack;
        stateFrames_ = 0;
        step_ = std::max(0.0, gain_ - kDuckedGain) / static_cast<double>(attackFrames_);
    }

    // Fixed internal tuning: only ON/OFF is user-accessible.
    static constexpr double kGateThreshold = 0.03162277660168379; // -30 dBFS
    static constexpr double kDuckedGain = 0.25; // -12.0 dB wet attenuation
    double rate_ {44100.0};
    double gate_ {0.0}, fast_ {0.0}, slow_ {0.0};
    double gateAttack_ {0.0}, gateRelease_ {0.0};
    double fastRise_ {0.0}, fastFall_ {0.0}, slowRise_ {0.0}, slowFall_ {0.0};
    double gain_ {1.0}, step_ {0.0};
    bool enabled_ {false}, heardFirstNote_ {false}, gateOpen_ {false};
    std::uint32_t quietFrames_ {0}, framesSinceNote_ {0}, stateFrames_ {0};
    std::uint32_t minGap_ {0}, minBetween_ {0};
    std::uint32_t attackFrames_ {1}, holdFrames_ {0}, releaseFrames_ {1};
    Phase phase_ {Phase::Idle};
};

} // namespace Depthorator
