#include "AutoClearCore.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if (!(x)) { \
    std::fprintf(stderr, "FAILED line %d: %s\\n",__LINE__,#x); \
    return EXIT_FAILURE; \
} } while (false)

static double note(int frame,int start,double rate,bool sustained=false) {
    if(frame<start) return 0.0;
    const double t=(frame-start)/rate;
    if(!sustained && t>=0.15) return 0.0;
    const double gain=sustained ? 0.7 :
        0.7*std::min(1.0,t/0.004)*std::max(0.0,1.0-t/0.15);
    return gain*std::sin(2.0*3.141592653589793*220.0*t);
}

int main() {
    for(const double rate : {44100.0,48000.0,96000.0}) {
        Depthorator::AutoClearCore off,on,firstOnly,sustained;
        off.prepare(rate);on.prepare(rate);firstOnly.prepare(rate);sustained.prepare(rate);
        off.configure(false);on.configure(true);
        firstOnly.configure(true);sustained.configure(true);
        const int one=static_cast<int>(rate*0.1);
        const int two=static_cast<int>(rate*1.5);
        const int frames=static_cast<int>(rate*2.5);
        int preSecondDifferences=0,secondTriggers=0,clears=0,muteFrames=0,zeroWet=0,recovered=0;
        for(int i=0;i<frames;++i){
            const double a=note(i,one,rate),b=note(i,two,rate);
            const double s=note(i,one,rate,true);
            const auto fOff=off.process(a+b,a+b);
            const auto fOn=on.process(a+b,a+b);
            const auto fFirst=firstOnly.process(a,a);
            const auto fSustain=sustained.process(s,s);
            CHECK(fOff.wetGain==1.0&&!fOff.clearOld&&!fOff.startedFullClear);
            CHECK(fFirst.wetGain==1.0&&!fFirst.clearOld&&!fFirst.startedFullClear);
            CHECK(fSustain.wetGain==1.0&&!fSustain.clearOld&&!fSustain.startedFullClear);
            CHECK(std::isfinite(fOn.wetGain)&&fOn.wetGain>=0.0&&fOn.wetGain<=1.0);
            if(i<two){
                if(fOn.wetGain!=1.0||fOn.clearOld||fOn.startedFullClear)++preSecondDifferences;
            }else{
                secondTriggers+=fOn.startedFullClear?1:0;
                clears+=fOn.clearOld?1:0;
                muteFrames+=fOn.muteFeedback?1:0;
                zeroWet+=fOn.wetGain==0.0?1:0;
                if(i>two+static_cast<int>(rate*0.55)&&fOn.wetGain==1.0)++recovered;
                if(fOn.clearOld) CHECK(fOn.muteFeedback);
            }
        }
        CHECK(preSecondDifferences==0);
        CHECK(secondTriggers==1);
        CHECK(clears==1);
        CHECK(muteFrames>0);
        CHECK(zeroWet==1);
        CHECK(recovered>0);
        on.configure(false);
        CHECK(on.process(0.0,0.0).wetGain==1.0);
        on.configure(true);
        CHECK(on.process(0.8,0.8).wetGain==1.0);
        const auto hostile=on.process(std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::infinity());
        CHECK(std::isfinite(hostile.wetGain));
        std::printf("AUTO_CLEAR_NOTE_CONTRACT_PASS rate=%.0f first_note=unity second_note=clear old_tail=removed new_note=preserved\\n",rate);
    }
    return EXIT_SUCCESS;
}
