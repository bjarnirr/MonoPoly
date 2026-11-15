
#include "mpc_plugin.h"
#include "MonoPolyEngine.h"
#include <algorithm>
#include <initializer_list>
#include <utility>
#include <vector>

class MonoPolyPlugin final : public mpc::Plugin
{
public:
    enum P {
        kVoiceMode,
        kW1,kO1,kT1,kL1,kPW1,
        kW2,kO2,kT2,kL2,kPW2,
        kW3,kO3,kT3,kL3,kPW3,
        kW4,kO4,kT4,kL4,kPW4,
        kNoise,kCutoff,kReso,kFEnvAmt,kKeyTrack,
        kFA,kFD,kFS,kFR,
        kAA,kAD,kAS,kAR,
        kMG1Shape,kMG1Rate,kMG2Shape,kMG2Rate,
        kPitchMod,kFilterMod,kPWMMod,kMaster,
        kCount
    };

    static mpc::PluginInfo info() {
        mpc::PluginInfo i;
        i.name="MonoPoly"; i.vendor="MonoPoly Project";
        i.uniqueId=MPC_FOURCC('M','n','P','l');
        i.version=100; i.isInstrument=true; i.numInputs=0; i.numOutputs=2;
        return i;
    }

    MonoPolyPlugin() : mpc::Plugin(info())
    {
        using namespace mpc;
        addParam(choiceParam("voice_mode","Voice Mode",{"Mono","2 Voice","4 Voice"},0));
        const std::vector<std::string> waves={"Triangle","Saw","Pulse","PWM"};
        const char* waveIds[4]={"vco1_wave","vco2_wave","vco3_wave","vco4_wave"};
        const char* waveNames[4]={"VCO1 Wave","VCO2 Wave","VCO3 Wave","VCO4 Wave"};
        const char* octIds[4]={"vco1_oct","vco2_oct","vco3_oct","vco4_oct"};
        const char* octNames[4]={"VCO1 Oct","VCO2 Oct","VCO3 Oct","VCO4 Oct"};
        const char* tuneIds[4]={"vco1_tune","vco2_tune","vco3_tune","vco4_tune"};
        const char* tuneNames[4]={"VCO1 Tune","VCO2 Tune","VCO3 Tune","VCO4 Tune"};
        const char* levelIds[4]={"vco1_level","vco2_level","vco3_level","vco4_level"};
        const char* levelNames[4]={"VCO1 Level","VCO2 Level","VCO3 Level","VCO4 Level"};
        const char* pwIds[4]={"vco1_pw","vco2_pw","vco3_pw","vco4_pw"};
        const char* pwNames[4]={"VCO1 PulseW","VCO2 PulseW","VCO3 PulseW","VCO4 PulseW"};
        for(int n=0;n<4;++n) {
            addParam(choiceParam(waveIds[n],waveNames[n],waves,1));
            addParam(intParam(octIds[n],octNames[n],-2,1,0,"oct"));
            addParam(floatParam(tuneIds[n],tuneNames[n],-100,100,0,"ct",1));
            addParam(floatParam(levelIds[n],levelNames[n],0,100,n==0?75:55,"%",0));
            addParam(floatParam(pwIds[n],pwNames[n],5,95,50,"%",0));
        }
        addParam(floatParam("noise","Noise",0,100,0,"%",0));
        addParam(logParam("cutoff","Cutoff",20,18000,9000,"Hz",0));
        addParam(floatParam("resonance","Resonance",0,95,15,"%",0));
        addParam(floatParam("vcf_env","VCF Env",0,100,25,"%",0));
        addParam(floatParam("key_track","Key Track",0,100,0,"%",0));
        addParam(logParam("vcf_attack","VCF Attack",1,5000,10,"ms",0));
        addParam(logParam("vcf_decay","VCF Decay",1,5000,300,"ms",0));
        addParam(floatParam("vcf_sustain","VCF Sustain",0,100,0,"%",0));
        addParam(logParam("vcf_release","VCF Release",1,5000,300,"ms",0));
        addParam(logParam("vca_attack","VCA Attack",1,5000,10,"ms",0));
        addParam(logParam("vca_decay","VCA Decay",1,5000,250,"ms",0));
        addParam(floatParam("vca_sustain","VCA Sustain",0,100,75,"%",0));
        addParam(logParam("vca_release","VCA Release",1,5000,400,"ms",0));
        addParam(choiceParam("mg1_shape","MG1 Shape",{"Sine","Triangle","Ramp","Square"},1));
        addParam(logParam("mg1_rate","MG1 Rate",0.05f,30,5,"Hz",2));
        addParam(choiceParam("mg2_shape","MG2 Shape",{"Sine","Triangle","Ramp","Square"},0));
        addParam(logParam("mg2_rate","MG2 Rate",0.05f,30,0.7f,"Hz",2));
        addParam(floatParam("pitch_mod","Pitch Mod",0,100,0,"%",0));
        addParam(floatParam("filter_mod","Filter Mod",0,100,0,"%",0));
        addParam(floatParam("pwm_mod","PWM Mod",0,100,0,"%",0));
        addParam(floatParam("master","Master",0,100,70,"%",0));

        auto preset=[this](const char* name, std::initializer_list<std::pair<int,float>> changes) {
            std::vector<float> v; v.reserve(numParams());
            for(int i=0;i<numParams();++i) v.push_back(param(i).def);
            for(auto c:changes) v[(size_t)c.first]=c.second;
            addProgram(name,std::move(v));
        };
        preset("Init Mono", {});
        preset("Mono Bass", {{kVoiceMode,0},{kCutoff,850},{kReso,38},{kFEnvAmt,70},{kFA,2},{kFD,180},{kAA,2},{kAD,220},{kAS,65},{kAR,130},{kL1,90},{kL2,70},{kO2,-1},{kT2,8}});
        preset("Four Voice", {{kVoiceMode,2},{kCutoff,4200},{kReso,12},{kAS,85},{kAR,700},{kL1,85},{kL2,65},{kL3,55},{kL4,45}});
        preset("PWM Ensemble", {{kVoiceMode,2},{kW1,3},{kW2,3},{kW3,3},{kW4,3},{kPWMMod,55},{kMG2Rate,0.35f},{kCutoff,5000},{kAS,90},{kAR,900}});
    }

    void prepare(double sr,int) override { engine_.prepare(sr); }
    void reset() override { engine_.reset(); }

    void process(mpc::ProcessContext& ctx) override
    {
        monopoly::Parameters p;
        p.voiceMode = getInt(kVoiceMode)==0 ? monopoly::VoiceMode::Mono :
                      (getInt(kVoiceMode)==1 ? monopoly::VoiceMode::Two : monopoly::VoiceMode::Four);
        for(int n=0;n<4;++n) {
            const int base=1+n*5;
            p.osc[n].waveform=static_cast<monopoly::Waveform>(std::max(0,std::min(3,getInt(base))));
            p.osc[n].octave=getInt(base+1);
            p.osc[n].tuneCents=get(base+2);
            p.osc[n].level=get(base+3)*0.01f;
            p.osc[n].pulseWidth=get(base+4)*0.01f;
        }
        p.noiseLevel=get(kNoise)*0.01f;
        p.cutoffHz=get(kCutoff); p.resonance=get(kReso)*0.01f;
        p.filterEnvAmount=get(kFEnvAmt)*0.01f; p.filterKeyTrack=get(kKeyTrack)*0.01f;
        p.filterEnv={get(kFA)*0.001f,get(kFD)*0.001f,get(kFS)*0.01f,get(kFR)*0.001f};
        p.ampEnv={get(kAA)*0.001f,get(kAD)*0.001f,get(kAS)*0.01f,get(kAR)*0.001f};
        p.mg1Wave=static_cast<monopoly::LfoWave>(getInt(kMG1Shape));
        p.mg1RateHz=get(kMG1Rate);
        p.mg2Wave=static_cast<monopoly::LfoWave>(getInt(kMG2Shape));
        p.mg2RateHz=get(kMG2Rate);
        p.pitchModDepth=get(kPitchMod)*0.01f;
        p.filterModDepth=get(kFilterMod)*0.01f;
        p.pwmModDepth=get(kPWMMod)*0.01f;
        p.masterLevel=get(kMaster)*0.01f;

        float* L=ctx.outputs[0];
        float* R=ctx.numOutputs>1?ctx.outputs[1]:nullptr;
        int pos=0;
        for(int e=0;e<ctx.numMidiEvents;++e) {
            const auto& ev=ctx.midi[e];
            const int at=std::max(pos,std::min(ctx.numFrames,ev.sampleOffset));
            if(at>pos) {
                engine_.process(L+pos,R?R+pos:nullptr,at-pos,p);
                pos=at;
            }
            if(ev.isNoteOn()) engine_.noteOn(ev.note(),ev.velocity()/127.0f);
            else if(ev.isNoteOff()) engine_.noteOff(ev.note());
            else if(ev.isAllNotesOff()) engine_.allNotesOff();
        }
        if(pos<ctx.numFrames) engine_.process(L+pos,R?R+pos:nullptr,ctx.numFrames-pos,p);
    }
private:
    monopoly::MonoPolyEngine engine_;
};

MPC_PLUGIN_ENTRY(MonoPolyPlugin)
