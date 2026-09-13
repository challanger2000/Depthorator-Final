#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace Depthorator {

class ReverbEngine {
public:
    void prepare(double sampleRate);
    void reset();
    void setParameters(double size, double decay, double damping);
    void process(double inL, double inR, double& outL, double& outR);

private:
    static constexpr std::size_t kLines = 8;

    double sampleRate_ {44100.0};
    double size_ {0.7};
    double decay_ {0.5};
    double damping_ {0.5};

    std::array<std::vector<double>, kLines> buffers_;
    std::array<std::size_t, kLines> writePos_ {};
    std::array<double, kLines> dampState_ {};
    std::array<double, kLines> modPhase_ {};

    std::array<std::vector<double>, 4> diffBuffers_;
    std::array<std::size_t, 4> diffPos_ {};

    double diffuse(double input, std::size_t stage, double coefficient);
};

} // namespace Depthorator
