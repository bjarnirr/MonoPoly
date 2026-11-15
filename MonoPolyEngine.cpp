#include "MonoPolyEngine.h"
#include <algorithm>
#include <cmath>

namespace monopoly {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr float kTwoPi = 6.2831853071795864769f;
inline float clampf(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }
inline double midiHz(float note) { return 440.0 * std::pow(2.0, (double(note) - 69.0) / 12.0); }
}

void MonoPolyEngine::Env::gateOn() { stage = Stage::Attack; }
void MonoPolyEngine::Env::gateOff() {
    if (stage != Stage::Off) { releaseStart = value; stage = Stage::Release; }
}
float MonoPolyEngine::Env::tick(const EnvelopeParams& p, float sr) {
    const float a = std::max(0.0005f, p.attack);
    const float d = std::max(0.0005f, p.decay);
    const float r = std::max(0.0005f, p.release);
    switch (stage) {
        case Stage::Off: value = 0.0f; break;
        case Stage::Attack:
            value += 1.0f / (a * sr);
            if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
            break;
        case Stage::Decay:
            value -= (1.0f - clampf(p.sustain, 0.0f, 1.0f)) / (d * sr);
            if (value <= p.sustain) { value = p.sustain; stage = Stage::Sustain; }
            break;
        case Stage::Sustain: value = clampf(p.sustain, 0.0f, 1.0f); break;
        case Stage::Release:
            value -= releaseStart / (r * sr);
            if (value <= 0.00001f) { value = 0.0f; stage = Stage::Off; }
            break;
    }
    return value;
}

MonoPolyEngine::MonoPolyEngine() { prepare(48000.0); }
void MonoPolyEngine::prepare(double sampleRate) {
    sr_ = std::max(8000.0, sampleRate);
    reset();
}
void MonoPolyEngine::reset() {
    for (auto& o : oscs_) o.phase = 0.0;
    notes_ = {};
    age_ = 0;
    ampEnv_ = {};
    filterEnv_ = {};
    filterState_ = {};
    mg1Phase_ = mg2Phase_ = 0.0;
    lastAmp_ = 0.0f;
    lastCutoff_ = 9000.0f;
}
void MonoPolyEngine::noteOn(int midiNote, float velocity) {
    velocity = clampf(velocity, 0.0f, 1.0f);
    int slot = -1;
    for (int i = 0; i < 4; ++i) if (notes_[i].note == midiNote) { slot = i; break; }
    if (slot < 0) {
        for (int i = 0; i < 4; ++i) if (notes_[i].note < 0) { slot = i; break; }
    }
    if (slot < 0) {
        slot = 0;
        for (int i = 1; i < 4; ++i) if (notes_[i].age < notes_[slot].age) slot = i;
    }
    notes_[slot] = {midiNote, velocity, ++age_};
    ampEnv_.gateOn();
    filterEnv_.gateOn();
}
void MonoPolyEngine::noteOff(int midiNote) {
    for (auto& n : notes_) if (n.note == midiNote) n.note = -1;
    bool any = false;
    for (const auto& n : notes_) any |= n.note >= 0;
    if (!any) { ampEnv_.gateOff(); filterEnv_.gateOff(); }
}
void MonoPolyEngine::allNotesOff() {
    for (auto& n : notes_) n.note = -1;
    ampEnv_.gateOff();
    filterEnv_.gateOff();
}
float MonoPolyEngine::randomBipolar() {
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    return (float(rng_ & 0x00ffffffu) / 8388607.5f) - 1.0f;
}
float MonoPolyEngine::lfo(LfoWave shape, double phase) {
    const float p = float(phase - std::floor(phase));
    switch (shape) {
        case LfoWave::Sine: return std::sin(kTwoPi * p);
        case LfoWave::Triangle: return 1.0f - 4.0f * std::fabs(p - 0.5f);
        case LfoWave::Ramp: return 2.0f * p - 1.0f;
        case LfoWave::Square: return p < 0.5f ? 1.0f : -1.0f;
    }
    return 0.0f;
}
float MonoPolyEngine::oscillator(Waveform shape, double phase, float pulseWidth) {
    const float p = float(phase - std::floor(phase));
    switch (shape) {
        case Waveform::Triangle: return 1.0f - 4.0f * std::fabs(p - 0.5f);
        case Waveform::Saw: return 2.0f * p - 1.0f;
        case Waveform::Pulse: return p < 0.5f ? 1.0f : -1.0f;
        case Waveform::PWM: return p < clampf(pulseWidth, 0.05f, 0.95f) ? 1.0f : -1.0f;
    }
    return 0.0f;
}
int MonoPolyEngine::noteForOsc(int oscIndex, VoiceMode mode) const {
    if (mode == VoiceMode::Mono) {
        int newest = -1;
        for (int i = 0; i < 4; ++i)
            if (notes_[i].note >= 0 && (newest < 0 || notes_[i].age > notes_[newest].age)) newest = i;
        return newest;
    }
    if (mode == VoiceMode::Two) {
        // Two oscillator pairs follow the two most recently held notes.
        int first = -1, second = -1;
        for (int i = 0; i < 4; ++i) if (notes_[i].note >= 0) {
            if (first < 0 || notes_[i].age > notes_[first].age) { second = first; first = i; }
            else if (second < 0 || notes_[i].age > notes_[second].age) second = i;
        }
        const int chosen = (oscIndex < 2) ? first : second;
        return chosen;
    }
    // Four-oscillator allocation: each oscillator follows a different held note,
    // ordered by age (oldest to newest).
    std::array<int,4> order{{-1,-1,-1,-1}};
    int count = 0;
    for (int i = 0; i < 4; ++i) if (notes_[i].note >= 0) {
        int pos = count;
        while (pos > 0 && notes_[order[pos-1]].age > notes_[i].age) {
            order[pos] = order[pos-1]; --pos;
        }
        order[pos] = i; ++count;
    }
    return oscIndex < count ? order[oscIndex] : -1;
}
float MonoPolyEngine::filterSample(float input, float cutoff, float resonance) {
    // Four cascaded one-pole stages with feedback. A compact, stable approximation;
    // not a component-level SSM2044 model.
    const float fc = clampf(cutoff, 20.0f, float(sr_ * 0.42));
    const float g = clampf(1.0f - std::exp(-kTwoPi * fc / float(sr_)), 0.0001f, 0.95f);
    const float fb = clampf(resonance, 0.0f, 0.95f) * 3.65f;
    float x = std::tanh(input - fb * filterState_[3]);
    for (int i = 0; i < 4; ++i) {
        filterState_[i] += g * (x - filterState_[i]);
        if (std::fabs(filterState_[i]) < 1.0e-20f) filterState_[i] = 0.0f;
        x = std::tanh(filterState_[i] * 1.15f);
    }
    return filterState_[3];
}
void MonoPolyEngine::process(float* left, float* right, int frames, const Parameters& p) {
    if (!left || !right || frames <= 0) return;
    const float sr = float(sr_);
    const float modeCount = float(static_cast<uint8_t>(p.voiceMode));
    (void)modeCount;
    const float ampSmooth = 1.0f - std::exp(-1.0f / (0.005f * sr));
    const float cutSmooth = 1.0f - std::exp(-1.0f / (0.010f * sr));
    for (int s = 0; s < frames; ++s) {
        const float mg1 = lfo(p.mg1Wave, mg1Phase_);
        const float mg2 = lfo(p.mg2Wave, mg2Phase_);
        mg1Phase_ += std::max(0.01f, p.mg1RateHz) / sr;
        mg2Phase_ += std::max(0.01f, p.mg2RateHz) / sr;
        if (mg1Phase_ >= 1.0) mg1Phase_ -= std::floor(mg1Phase_);
        if (mg2Phase_ >= 1.0) mg2Phase_ -= std::floor(mg2Phase_);

        float mix = p.noiseLevel * randomBipolar() * 0.18f;
        for (int o = 0; o < 4; ++o) {
            const int ni = noteForOsc(o, p.voiceMode);
            if (ni < 0) continue;
            const auto& op = p.osc[o];
            const float midi = float(notes_[ni].note + op.octave * 12) + op.tuneCents / 100.0f
                             + mg1 * p.pitchModDepth * 2.0f;
            const double hz = midiHz(midi);
            const double inc = hz / sr_;
            const float pw = clampf(op.pulseWidth + mg2 * p.pwmModDepth * 0.45f, 0.05f, 0.95f);
            mix += oscillator(op.waveform, oscs_[o].phase, pw) * clampf(op.level, 0.0f, 1.0f) * notes_[ni].velocity * 0.25f;
            oscs_[o].phase += inc;
            if (oscs_[o].phase >= 1.0) oscs_[o].phase -= std::floor(oscs_[o].phase);
        }

        const float fe = filterEnv_.tick(p.filterEnv, sr);
        const float ae = ampEnv_.tick(p.ampEnv, sr);
        const float keyNote = notes_[0].note >= 0 ? float(notes_[0].note) : 60.0f;
        const float keyOffset = (keyNote - 60.0f) * p.filterKeyTrack * 0.5f;
        const float cutoffTarget = p.cutoffHz * std::pow(2.0f, (fe * p.filterEnvAmount * 4.0f + keyOffset / 12.0f + mg2 * p.filterModDepth * 2.0f));
        lastCutoff_ += cutSmooth * (cutoffTarget - lastCutoff_);
        lastAmp_ += ampSmooth * (ae - lastAmp_);
        const float out = filterSample(mix, lastCutoff_, p.resonance) * lastAmp_ * clampf(p.masterLevel, 0.0f, 1.0f);
        left[s] = out; if (right) right[s] = out;
    }
}
} // namespace monopoly
