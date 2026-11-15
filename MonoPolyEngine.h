#pragma once
#include <array>
#include <cstdint>

namespace monopoly {

enum class Waveform : uint8_t { Triangle, Saw, Pulse, PWM };
enum class VoiceMode : uint8_t { Mono = 1, Two = 2, Four = 4 };
enum class LfoWave : uint8_t { Sine, Triangle, Ramp, Square };

struct EnvelopeParams {
    float attack = 0.01f;
    float decay = 0.25f;
    float sustain = 0.7f;
    float release = 0.35f;
};

struct OscParams {
    Waveform waveform = Waveform::Saw;
    int octave = 0;       // -2..+1 relative to 8'
    float tuneCents = 0.0f;
    float level = 0.7f;
    float pulseWidth = 0.5f;
};

struct Parameters {
    std::array<OscParams, 4> osc{};
    VoiceMode voiceMode = VoiceMode::Mono;
    float noiseLevel = 0.0f;
    float masterLevel = 0.7f;
    float cutoffHz = 9000.0f;
    float resonance = 0.15f;
    float filterEnvAmount = 0.25f;
    float filterKeyTrack = 0.0f;
    EnvelopeParams filterEnv{0.01f, 0.3f, 0.0f, 0.3f};
    EnvelopeParams ampEnv{0.01f, 0.25f, 0.75f, 0.4f};
    LfoWave mg1Wave = LfoWave::Triangle;
    LfoWave mg2Wave = LfoWave::Sine;
    float mg1RateHz = 5.0f;
    float mg2RateHz = 0.7f;
    float pitchModDepth = 0.0f;
    float filterModDepth = 0.0f;
    float pwmModDepth = 0.0f;
};

class MonoPolyEngine {
public:
    MonoPolyEngine();
    void prepare(double sampleRate);
    void reset();
    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();
    void process(float* left, float* right, int frames, const Parameters& p);

private:
    struct Env {
        enum class Stage : uint8_t { Off, Attack, Decay, Sustain, Release };
        Stage stage = Stage::Off;
        float value = 0.0f;
        float releaseStart = 0.0f;
        void gateOn();
        void gateOff();
        float tick(const EnvelopeParams& p, float sr);
    };
    struct Note { int note = -1; float velocity = 0.0f; uint64_t age = 0; };
    struct Osc { double phase = 0.0; };

    double sr_ = 48000.0;
    std::array<Osc, 4> oscs_{};
    std::array<Note, 4> notes_{};
    uint64_t age_ = 0;
    Env ampEnv_{}, filterEnv_{};
    std::array<float, 4> filterState_{};
    double mg1Phase_ = 0.0, mg2Phase_ = 0.0;
    uint32_t rng_ = 0x6d2b79f5u;
    float lastAmp_ = 0.0f;
    float lastCutoff_ = 9000.0f;

    float randomBipolar();
    static float lfo(LfoWave shape, double phase);
    static float oscillator(Waveform shape, double phase, float pulseWidth);
    float filterSample(float input, float cutoff, float resonance);
    int noteForOsc(int oscIndex, VoiceMode mode) const;
};

} // namespace monopoly
