#include "DepthoratorProcessor.h"
#include "DepthoratorIDs.h"
#include "LicenseStatus.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace Depthorator {
using namespace Steinberg; using namespace Steinberg::Vst;
namespace {
inline double zapTiny(double x){if(!std::isfinite(x))return 0.0;return std::abs(x)<1.0e-30?0.0:x;}
struct ParamCursor{IParamValueQueue* q{nullptr};int32 index{0};int32 count{0};int32 offset{0};ParamValue value{0.0};bool valid{false};};
bool nextPoint(ParamCursor& c){c.valid=false;if(!c.q)return false;while(c.index<c.count){int32 o=0;ParamValue v=0.0;const auto idx=c.index++;if(c.q->getPoint(idx,o,v)!=kResultTrue||!std::isfinite(v))continue;c.offset=std::max<int32>(0,o);c.value=std::clamp(v,0.0,1.0);c.valid=true;return true;}return false;}
}

Processor::Processor(){setControllerClass(kControllerUID);}
tresult PLUGIN_API Processor::initialize(FUnknown* c){auto r=AudioEffect::initialize(c);if(r!=kResultOk)return r;addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);licensed_=Licensing::isLicensed();return kResultOk;}
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* i,int32 ni,SpeakerArrangement* o,int32 no){if(ni==1&&no==1&&i[0]==SpeakerArr::kStereo&&o[0]==SpeakerArr::kStereo)return AudioEffect::setBusArrangements(i,ni,o,no);return kResultFalse;}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 s){return(s==kSample32||s==kSample64)?kResultTrue:kResultFalse;}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& s){sampleRate_=(std::isfinite(s.sampleRate)&&s.sampleRate>1000.0)?s.sampleRate:44100.0;resetDSP();demoGate_.configure(sampleRate_,licensed_);return AudioEffect::setupProcessing(s);}
tresult PLUGIN_API Processor::setActive(TBool s){if(s)resetDSP();return AudioEffect::setActive(s);}
tresult PLUGIN_API Processor::setProcessing(TBool s){if(s)resetDSP();return kResultOk;}
void Processor::resetDSP(){const auto n=static_cast<std::size_t>(std::ceil(sampleRate_*5.0))+8u;delayL_.assign(n,0.0);delayR_.assign(n,0.0);writePos_=0;feedbackLP_L_=feedbackLP_R_=0.0;duckEnvelope_=0.0;activeDelaySamples_=oldDelaySamples_=targetDelaySamples_=0.0;timeCrossfade_=1.0;delayTimeInitialized_=false;reverb_.prepare(sampleRate_);reverb_.reset();}
double Processor::currentDelaySeconds(const ProcessData& d)const{double t=values_[0];if(values_[10]<0.5)return 0.020+t*1.980;double tempo=120.0;if(d.processContext&&(d.processContext->state&ProcessContext::kTempoValid)&&std::isfinite(d.processContext->tempo))tempo=std::clamp(static_cast<double>(d.processContext->tempo),20.0,400.0);static constexpr double beats[]={0.125,0.0833333333333333,0.1875,0.25,0.166666666666667,0.375,0.5,0.333333333333333,0.75,1.0,0.666666666666667,1.5,2.0,1.33333333333333,3.0,4.0};int idx=std::clamp(static_cast<int>(std::round(t*15.0)),0,15);return(60.0/tempo)*beats[idx];}

template<typename Sample>void Processor::processBlock(Sample**in,Sample**out,int32 start,int32 n,int32 ch,const ProcessData& data){
 if(delayL_.empty()||delayR_.empty()){for(int32 j=0;j<n;++j){const int32 i=start+j;const double l=in[0]?static_cast<double>(in[0][i]):0.0;const double r=(ch>1&&in[1])?static_cast<double>(in[1][i]):l;out[0][i]=static_cast<Sample>(std::isfinite(l)?l:0.0);if(ch>1&&out[1])out[1][i]=static_cast<Sample>(std::isfinite(r)?r:0.0);}return;}const auto bs=delayL_.size();const double requestedDelay=std::clamp(currentDelaySeconds(data)*sampleRate_,1.0,static_cast<double>(bs-3));if(!delayTimeInitialized_){activeDelaySamples_=oldDelaySamples_=targetDelaySamples_=requestedDelay;timeCrossfade_=1.0;delayTimeInitialized_=true;}if(std::abs(requestedDelay-targetDelaySamples_)>0.5){activeDelaySamples_=(timeCrossfade_<0.5)?oldDelaySamples_:targetDelaySamples_;oldDelaySamples_=activeDelaySamples_;targetDelaySamples_=requestedDelay;timeCrossfade_=0.0;}const double feedback=std::min(0.94,values_[1]*0.94),depth=values_[2],curve=values_[3],width=values_[7],duck=values_[8],mix=values_[9];int mode=std::clamp(static_cast<int>(std::round(values_[11]*2.0)),0,2);const double shape=std::pow(depth,2.35-curve*1.8),cutoff=18000.0*std::pow(0.18,shape)+900.0*shape,lpA=std::exp(-2.0*3.14159265358979323846*cutoff/sampleRate_);reverb_.setParameters(values_[4],values_[5],values_[6]);const double dryGain=std::cos(mix*1.5707963267948966),wetGain=std::sin(mix*1.5707963267948966);const double repeatToRoom=0.12+shape*0.88,roomOutput=0.28+shape*0.72,directEcho=1.0-shape*0.42,roomIntoFeedback=shape*0.16;const double duckAttack=std::exp(-1.0/(0.006*sampleRate_));const double duckRelease=std::exp(-1.0/(0.240*sampleRate_));const double crossfadeStep=1.0/std::max(1.0,0.025*sampleRate_);auto readTap=[&](const std::vector<double>& b,double delaySamples){const double ds=std::clamp(delaySamples,1.0,static_cast<double>(bs-3));const auto di=static_cast<std::size_t>(ds);const double frac=ds-static_cast<double>(di);const auto p0=(writePos_+bs-di)%bs,p1=(p0+bs-1u)%bs;return b[p0]*(1.0-frac)+b[p1]*frac;};for(int32 j=0;j<n;++j){const int32 i=start+j;double inL=in[0]?static_cast<double>(in[0][i]):0.0,inR=ch>1&&in[1]?static_cast<double>(in[1][i]):inL;if(!std::isfinite(inL))inL=0.0;if(!std::isfinite(inR))inR=0.0;const double detector=std::max(std::abs(inL),std::abs(inR));const double coeff=detector>duckEnvelope_?duckAttack:duckRelease;duckEnvelope_=zapTiny(coeff*duckEnvelope_+(1.0-coeff)*detector);const double duckActivity=duckEnvelope_/(duckEnvelope_+0.030);const double duckGain=1.0-duck*0.94*duckActivity;double wetL=0.0,wetR=0.0;if(timeCrossfade_<1.0){const double x=std::clamp(timeCrossfade_,0.0,1.0);const double y=x*x*(3.0-2.0*x),a=1.0-y,b=y;wetL=readTap(delayL_,oldDelaySamples_)*a+readTap(delayL_,targetDelaySamples_)*b;wetR=readTap(delayR_,oldDelaySamples_)*a+readTap(delayR_,targetDelaySamples_)*b;timeCrossfade_=std::min(1.0,timeCrossfade_+crossfadeStep);if(timeCrossfade_>=1.0)activeDelaySamples_=targetDelaySamples_;}else{wetL=readTap(delayL_,activeDelaySamples_);wetR=readTap(delayR_,activeDelaySamples_);}if(mode==0){double m=.5*(wetL+wetR);wetL=wetR=m;}double mid=.5*(wetL+wetR),side=.5*(wetL-wetR)*width,wwL=mid+side,wwR=mid-side;double revL=0,revR=0;reverb_.process(inL*.18+wwL*repeatToRoom,inR*.18+wwR*repeatToRoom,revL,revR);feedbackLP_L_=zapTiny((1-lpA)*wetL+lpA*feedbackLP_L_);feedbackLP_R_=zapTiny((1-lpA)*wetR+lpA*feedbackLP_R_);double fbL=zapTiny(feedbackLP_L_+revL*roomIntoFeedback),fbR=zapTiny(feedbackLP_R_+revR*roomIntoFeedback);if(mode==2){const double inputMono=.5*(inL+inR);delayL_[writePos_]=zapTiny(inputMono+fbR*feedback);delayR_[writePos_]=zapTiny(fbL*feedback);}else{delayL_[writePos_]=zapTiny(inL+fbL*feedback);delayR_[writePos_]=zapTiny(inR+fbR*feedback);}double fxL=(wwL*directEcho+revL*roomOutput)*duckGain,fxR=(wwR*directEcho+revR*roomOutput)*duckGain;if(!std::isfinite(fxL))fxL=0.0;if(!std::isfinite(fxR))fxR=0.0;out[0][i]=static_cast<Sample>(inL*dryGain+fxL*wetGain);if(ch>1&&out[1])out[1][i]=static_cast<Sample>(inR*dryGain+fxR*wetGain);writePos_=(writePos_+1u)%bs;}}
tresult PLUGIN_API Processor::process(ProcessData& d){
 if(d.numInputs==0||d.numOutputs==0||d.numSamples<=0)return kResultOk;
 auto ch=std::min<int32>(2,std::min(d.inputs[0].numChannels,d.outputs[0].numChannels));if(ch<=0)return kResultOk;

 std::array<ParamCursor,12> cursors{};
 if(d.inputParameterChanges){
  const auto count=d.inputParameterChanges->getParameterCount();
  for(int32 qi=0;qi<count;++qi){
   auto*q=d.inputParameterChanges->getParameterData(qi);if(!q)continue;
   const auto id=q->getParameterId();if(id<kTime||id>kMode)continue;
   auto& cur=cursors[static_cast<std::size_t>(id-kTime)];
   cur.q=q;cur.count=q->getPointCount();nextPoint(cur);
  }
 }

 auto render=[&](int32 start,int32 count){
  if(count<=0)return;
  if(d.symbolicSampleSize==kSample32)processBlock(d.inputs[0].channelBuffers32,d.outputs[0].channelBuffers32,start,count,ch,d);
  else if(d.symbolicSampleSize==kSample64)processBlock(d.inputs[0].channelBuffers64,d.outputs[0].channelBuffers64,start,count,ch,d);
 };

 if(d.symbolicSampleSize!=kSample32&&d.symbolicSampleSize!=kSample64)return kResultFalse;

 int32 pos=0;
 while(pos<d.numSamples){
  int32 next=d.numSamples;
  for(const auto& cur:cursors)if(cur.valid)next=std::min(next,std::clamp(cur.offset,0,d.numSamples));
  if(next>pos){render(pos,next-pos);pos=next;}
  bool applied=false;
  for(std::size_t p=0;p<cursors.size();++p){
   auto& cur=cursors[p];
   while(cur.valid&&std::clamp(cur.offset,0,d.numSamples)<=pos){
    values_[p]=cur.value;applied=true;nextPoint(cur);
   }
  }
  if(!applied&&next==pos){
   // Defensive progress for malformed/non-monotonic host queues.
   ++pos;
  }
 }
 // Apply points exactly at/end-after the current block so the next block starts at host state.
 for(std::size_t p=0;p<cursors.size();++p){
  auto& cur=cursors[p];
  while(cur.valid&&cur.offset<=d.numSamples){values_[p]=cur.value;nextPoint(cur);}
 }

 if(d.symbolicSampleSize==kSample32)demoGate_.process(d.outputs[0].channelBuffers32,ch,d.numSamples);else demoGate_.process(d.outputs[0].channelBuffers64,ch,d.numSamples);

 bool silent=true;
 if(d.symbolicSampleSize==kSample32){for(int32 cc=0;cc<ch&&silent;++cc)if(d.outputs[0].channelBuffers32[cc])for(int32 i=0;i<d.numSamples;++i)if(d.outputs[0].channelBuffers32[cc][i]!=0.f){silent=false;break;}}
 else{for(int32 cc=0;cc<ch&&silent;++cc)if(d.outputs[0].channelBuffers64[cc])for(int32 i=0;i<d.numSamples;++i)if(d.outputs[0].channelBuffers64[cc][i]!=0.0){silent=false;break;}}
 d.outputs[0].silenceFlags=silent?((Steinberg::uint64{1}<<ch)-1):0;return kResultOk;
}
tresult PLUGIN_API Processor::setState(IBStream*s){if(!s)return kInvalidArgument;IBStreamer st(s,kLittleEndian);for(auto&v:values_){double x=0;if(!st.readDouble(x)||!std::isfinite(x))return kResultFalse;v=std::clamp(x,0.0,1.0);}delayTimeInitialized_=false;return kResultOk;}
tresult PLUGIN_API Processor::getState(IBStream*s){if(!s)return kInvalidArgument;IBStreamer st(s,kLittleEndian);for(auto v:values_)if(!st.writeDouble(v))return kResultFalse;return kResultOk;}
} // namespace Depthorator
