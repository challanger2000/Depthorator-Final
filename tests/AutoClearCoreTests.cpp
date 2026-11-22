#include "AutoClearCore.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define REQUIRE(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL %s line %d: %s\\n", __FILE__, __LINE__, #condition); \
    return EXIT_FAILURE; \
} } while (false)

int main() {
    using Depthorator::AutoClearCore;

    AutoClearCore disabled;
    disabled.prepare(48000.0);
    disabled.configure(false, 1.0, -30.0, 40.0);
    for (int i = 0; i != 12000; ++i) {
        const auto f = disabled.process(i == 6000 ? 1.0 : 0.0, 0.0);
        REQUIRE(f.wetGain == 1.0 && !f.clearOld);
    }

    AutoClearCore kill;
    kill.prepare(48000.0);
    kill.configure(true, 1.0, -30.0, 40.0);
    for (int i = 0; i < 6000; ++i) kill.process(0.0, 0.0);
    int clears = 0;
    double low = 1.0;
    std::uint32_t preserved = 0;
    for (int i = 0; i < 9000; ++i) {
        const auto f = kill.process(i == 0 ? 0.8 : 0.0, 0.0);
        low = std::min(low, f.wetGain);
        if (f.clearOld) { ++clears; preserved = f.keepRecentFrames; }
        REQUIRE(std::isfinite(f.wetGain) && f.wetGain >= 0.0 && f.wetGain <= 1.0);
    }
    REQUIRE(clears == 1);
    REQUIRE(low == 0.0);
    REQUIRE(preserved >= 64 && preserved <= 12u * 48u + 64u);
    REQUIRE(kill.process(0.0, 0.0).wetGain == 1.0);

    AutoClearCore duck;
    duck.prepare(48000.0);
    duck.configure(true, 0.5, -30.0, 40.0);
    for (int i = 0; i < 6000; ++i) duck.process(0.0, 0.0);
    int partialClears = 0;
    double minimum = 1.0;
    for (int i = 0; i < 10000; ++i) {
        const auto f = duck.process(i == 0 ? 0.8 : 0.0, 0.0);
        minimum = std::min(minimum, f.wetGain);
        partialClears += f.clearOld ? 1 : 0;
    }
    REQUIRE(partialClears == 0);
    REQUIRE(minimum >= 0.499 && minimum < 0.55);
    REQUIRE(duck.process(0.0, 0.0).wetGain == 1.0);

    AutoClearCore belowThreshold;
    belowThreshold.prepare(48000.0);
    belowThreshold.configure(true, 1.0, -6.0, 40.0);
    for (int i = 0; i < 6000; ++i) belowThreshold.process(0.0, 0.0);
    REQUIRE(belowThreshold.process(0.1, 0.0).wetGain == 1.0);

    // Invalid input is rejected rather than converted into a fake onset.
    const auto hostile = belowThreshold.process(
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity());
    REQUIRE(std::isfinite(hostile.wetGain) && !hostile.clearOld);

    kill.configure(false, 1.0, -30.0, 40.0);
    REQUIRE(kill.process(1.0, 1.0).wetGain == 1.0);
    std::puts("AUTO_CLEAR_CORE_PASS off=unity kill=one partial=duck recent=preserved hostile=finite");
    return EXIT_SUCCESS;
}
