#pragma once

#include "Primitives.h"
#include "Params/Parameters.h"

struct Transport
{
    bool playing = false;
    double ppq = 0.0;     // position in quarter notes at the start of the chunk
    double bpm = 120.0;
};

// One monophonic synth layer (SYNTH 1 or SYNTH 2 with its CNTRL page):
// dual oscillator, sub oscillator, noise, overdriving mixer, ladder LPF + HPF, sub filter, crossover,
// three envelopes, three LFOs and two random generators.
// Audio runs at the oversampled rate; modulators run once per chunk (control rate).
class SynthLayer
{
public:
    static constexpr int numSources = Mod::perLayerSources;

    void prepare (double oversampledRate, uint32_t seed);
    void setSampleRate (double oversampledRate);
    void reset();

    // Notes. legato = another note was still held (for glide and legato mode)
    void noteOn (int note, float velocity, bool legato, const float* p, bool glideOnLegatoOnly);
    void noteOff();
    void allOff();

    // Control rate: advances LFOs, random and the mod envelope by `seconds`, writes source values (-1..1 / 0..1)
    void updateModulators (const float* p, double seconds, const Transport& t, float* sourcesOut);

    // Renders n oversampled samples (adds nothing when silent). p = modulated real parameter values for this chunk.
    void render (const float* p, float* left, float* right, int n, float pitchBendSemis);

    bool isSounding() const { return amp.isActive(); }
    float getDrive() const { return driveMeter; }
    int getCurrentNote() const { return currentNote; }
    float getPitch() const { return (float) pitch; }

private:
    double fs = 96000.0;
    dsp::ShapeOsc osc1, osc2, sub;
    dsp::ColouredNoise noise;
    dsp::Ladder lpfL, lpfR;
    dsp::Svf hpfL, hpfR, subFilter, sideHp;
    dsp::LR4 xoverSub, xoverL, xoverR;
    dsp::Envelope fenv, amp, menv;
    dsp::Lfo lfo[3];
    dsp::RandomGen rnd[2];

    enum R { rFreq, rDetune, rWave, rDuty, rColour, rLvl1, rLvl2, rLvlN, rLvlS,
             rLpf, rLpfK, rLpfEg, rHpf, rHpfR, rHpfEg, rSub, rSubR, rSubEg, rSpread, rXover, numRamps };
    dsp::Ramp ramps[numRamps];
    bool rampsPrimed = false;

    double pitch = 48.0, targetPitch = 48.0;
    bool gliding = false;
    int currentNote = -1;
    float velocity = 0.0f;
    bool accented = false;
    bool pendingPhaseReset = false;
    float appliedOsc2Phase = 0.0f, appliedSubPhase = 0.0f;
    float driveMeter = 0.0f, drivePeak = 0.0f;
    float lastSources[numSources] {};
};
