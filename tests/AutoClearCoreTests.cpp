#include "AutoClearCore.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { \
    std::fprintf(stderr, "FAILED at line %d: %s\\n", __LINE__, #x); \
    return EXIT_FAILURE; \
} } while(false)

static double singleNote(int sample, int start, double sampleRate, bool sustained = false) {
    if (sample < start) return 0.0;
    const double elapsed = (sample-start) / sampleRate;
    if (!sustained && elapsed >= 0.150) return 0.0;
    const double gain = sustained ? 0.70 : 0.70 * std::min(1.0, elapsed / 0.004)
                                * std::max(0.0, 1.0 - elapsed / 0.150);
    return gain * std::sin(2.0 * 3.141592653589793 * 220.0 * elapsed);
}

int main() {
    for (const double rate : {44100.0, 48000.0, 96000.0}) {
        Depthorator::AutoClearCore off, on, firstOnly, sustained;
        off.prepare(rate); on.prepare(rate); firstOnly.prepare(rate); sustained.prepare(rate);
        off.configure(false); on.configure(true);
        firstOnly.configure(true); sustained.configure(true);
        const int n = static_cast<int>(rate * 3.0);
        const int firstStart = static_cast<int>(rate * 0.100);
        const int secondStart = static_cast<int>(rate * 1.500);
        int duckEvents = 0;
        int beforeSecondChanged = 0;
        int secondDuckedSamples = 0;
        int recoveredFrames = 0;
        for (int i = 0; i < n; ++i) {
            const double first = singleNote(i, firstStart, rate);
            const double second = singleNote(i, secondStart, rate);
            const double both = first + second;
            const auto offFrame = off.process(both, both);
            const auto onFrame = on.process(both, both);
            const auto firstFrame = firstOnly.process(first, first);
            const auto sustainedFrame = sustained.process(singleNote(i, firstStart, rate, true),
                                                         singleNote(i, firstStart, rate, true));
            // EXACT output-gain parity for every sample of a lone first note.
            CHECK(offFrame.wetGain == 1.0 && !offFrame.beganDucking);
            CHECK(firstFrame.wetGain == 1.0 && !firstFrame.beganDucking);
            CHECK(sustainedFrame.wetGain == 1.0 && !sustainedFrame.beganDucking);
            if (i < secondStart) {
                if (onFrame.wetGain != 1.0 || onFrame.beganDucking)
                    ++beforeSecondChanged;
            } else {
                duckEvents += onFrame.beganDucking ? 1 : 0;
                if (onFrame.wetGain < 0.99) ++secondDuckedSamples;
                if (i > secondStart + static_cast<int>(0.55 * rate) &&
                    onFrame.wetGain == 1.0) ++recoveredFrames;
            }
            CHECK(std::isfinite(onFrame.wetGain));
            CHECK(onFrame.wetGain >= 0.25 && onFrame.wetGain <= 1.0);
        }
        CHECK(beforeSecondChanged == 0);
        CHECK(duckEvents == 1);
        CHECK(secondDuckedSamples > 0);
        CHECK(recoveredFrames > 0);

        // Turning OFF may never retain a reduced wet gain.
        on.configure(false);
        CHECK(on.process(0.8,0.8).wetGain == 1.0);
        on.configure(true);
        CHECK(on.process(0.8,0.8).wetGain == 1.0);
        const auto bad=on.process(std::numeric_limits<double>::quiet_NaN(),
                                  std::numeric_limits<double>::infinity());
        CHECK(std::isfinite(bad.wetGain));
        std::printf("AUTO_CLEAR_PARITY_PASS rate=%.0f first_note_unity=1 next_note_ducks=1 no_tail_kill=1\\n",rate);
    }
    return EXIT_SUCCESS;
}
