#include "DepthoratorReverb.h"

#include <algorithm>
#include <cmath>

namespace Depthorator {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr std::array<double, 8> kBaseTimes {{0.0307,0.0379,0.0433,0.0499,0.0587,0.0671,0.0779,0.0893}};
constexpr std::array<double, 8> kModRates {{0.071,0.083,0.097,0.109,0.127,0.139,0.151,0.167}};
constexpr std::array<double, 4> kDiffTimes {{0.0047,0.0063,0.0089,0.0121}};
}

void ReverbEngine::prepare(double sampleRate) {
    sampleRate_ = sampleRate > 1000.0 ? sampleRate : 44100.0;
    for (std::size_t i=0;i<kLines;++i) {
        const auto n=static_cast<std::size_t>(std::ceil(kBaseTimes[i]*2.25*sampleRate_))+32u;
        buffers_[i].assign(n,0.0); writePos_[i]=0; dampState_[i]=0.0;
        modPhase_[i]=static_cast<double>(i)/static_cast<double>(kLines);
    }
    for (std::size_t i=0;i<4;++i) {
        const auto n=static_cast<std::size_t>(std::ceil(kDiffTimes[i]*2.0*sampleRate_))+8u;
        diffBuffers_[i].assign(n,0.0); diffPos_[i]=0;
    }
}

void ReverbEngine::reset() {
    for(auto& b:buffers_) std::fill(b.begin(),b.end(),0.0);
    for(auto& b:diffBuffers_) std::fill(b.begin(),b.end(),0.0);
    writePos_.fill(0); diffPos_.fill(0); dampState_.fill(0.0);
    for(std::size_t i=0;i<kLines;++i) modPhase_[i]=static_cast<double>(i)/static_cast<double>(kLines);
}

void ReverbEngine::setParameters(double size,double decay,double damping) {
    size_=std::clamp(size,0.0,1.0); decay_=std::clamp(decay,0.0,1.0); damping_=std::clamp(damping,0.0,1.0);
}

double ReverbEngine::diffuse(double input,std::size_t stage,double coefficient) {
    auto& b=diffBuffers_[stage]; auto& pos=diffPos_[stage]; if(b.empty()) return input;
    const double shapedSize=std::pow(size_,0.82);
    const double scale=0.78+shapedSize*1.02;
    const auto d=std::clamp<std::size_t>(static_cast<std::size_t>(std::round(kDiffTimes[stage]*scale*sampleRate_)),1u,b.size()-1u);
    const auto rp=(pos+b.size()-d)%b.size(); const double delayed=b[rp];
    const double output=delayed-coefficient*input; b[pos]=input+coefficient*output; pos=(pos+1u)%b.size(); return output;
}

void ReverbEngine::process(double inL,double inR,double& outL,double& outR) {
    const double mono=0.5*(inL+inR), side=0.5*(inL-inR);
    const double shapedSize=std::pow(size_,0.82);
    const double smallRoomEase=0.88+0.12*shapedSize;
    double d=mono+side*0.16;
    d=diffuse(d,0,0.68*smallRoomEase);
    d=diffuse(d,1,0.66*smallRoomEase);
    d=diffuse(d,2,0.63*smallRoomEase);
    d=diffuse(d,3,0.59*smallRoomEase);

    std::array<double,kLines> taps{}, matrix{}, gains{};
    const double sizeScale=0.70+shapedSize*1.30;
    const double decaySeconds=0.30+decay_*decay_*11.70;
    const double cutoff=18500.0*std::pow(0.14,damping_)+950.0*damping_;
    const double dampA=std::exp(-2.0*kPi*cutoff/sampleRate_);
    const double modDepthSamples=0.55+shapedSize*1.45;

    for(std::size_t i=0;i<kLines;++i) {
        auto& b=buffers_[i]; if(b.empty()) continue;
        modPhase_[i]+=kModRates[i]/sampleRate_; if(modPhase_[i]>=1.0) modPhase_[i]-=1.0;
        const double phase=2.0*kPi*modPhase_[i];
        const double modulation=(std::sin(phase)+0.28*std::sin(phase*0.47+static_cast<double>(i)*0.71))*modDepthSamples;
        const double ds=std::clamp(kBaseTimes[i]*sizeScale*sampleRate_+modulation,2.0,static_cast<double>(b.size()-3u));
        const auto di=static_cast<std::size_t>(ds); const double frac=ds-static_cast<double>(di);
        const auto p0=(writePos_[i]+b.size()-di)%b.size(), p1=(p0+b.size()-1u)%b.size();
        const double raw=b[p0]*(1.0-frac)+b[p1]*frac;
        dampState_[i]=(1.0-dampA)*raw+dampA*dampState_[i]; taps[i]=dampState_[i];
        gains[i]=std::pow(10.0,(-3.0*(ds/sampleRate_))/decaySeconds);
    }

    matrix[0]= taps[0]+taps[1]+taps[2]+taps[3]+taps[4]+taps[5]+taps[6]+taps[7];
    matrix[1]= taps[0]-taps[1]+taps[2]-taps[3]+taps[4]-taps[5]+taps[6]-taps[7];
    matrix[2]= taps[0]+taps[1]-taps[2]-taps[3]+taps[4]+taps[5]-taps[6]-taps[7];
    matrix[3]= taps[0]-taps[1]-taps[2]+taps[3]+taps[4]-taps[5]-taps[6]+taps[7];
    matrix[4]= taps[0]+taps[1]+taps[2]+taps[3]-taps[4]-taps[5]-taps[6]-taps[7];
    matrix[5]= taps[0]-taps[1]+taps[2]-taps[3]-taps[4]+taps[5]-taps[6]+taps[7];
    matrix[6]= taps[0]+taps[1]-taps[2]-taps[3]-taps[4]-taps[5]+taps[6]+taps[7];
    matrix[7]= taps[0]-taps[1]-taps[2]+taps[3]-taps[4]+taps[5]+taps[6]-taps[7];

    constexpr double norm=0.3535533905932738;
    constexpr std::array<double,8> signs {{1,-1,1,1,-1,1,-1,-1}};
    for(std::size_t i=0;i<kLines;++i) {
        auto& b=buffers_[i];
        b[writePos_[i]]=d*signs[i]*0.200+matrix[i]*norm*gains[i]*0.986;
        writePos_[i]=(writePos_[i]+1u)%b.size();
    }

    const double l=( taps[0]+taps[1]-taps[2]+taps[3]-taps[4]+taps[5]+taps[6]-taps[7]);
    const double r=(-taps[0]+taps[1]+taps[2]-taps[3]+taps[4]+taps[5]-taps[6]+taps[7]);
    outL=(l+r*0.11)*0.255;
    outR=(r+l*0.09)*0.255;
}

} // namespace Depthorator
