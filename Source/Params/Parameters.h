#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// All automatable parameters are described by one table (ParamDef). The tables are append-only:
// IDs are stable strings and must never be renamed; new parameters go to the END of their table,
// because modulation destinations are stored as table positions (see Mod::destCode).

enum class Kind { Float, Bool, Choice };

enum class Unit
{
    Plain,      // value with one decimal
    Scale10,    // Moog style 0..10
    Bipolar10,  // -10..+10
    Semitones,
    Hz,
    Seconds,    // shown as ms below one second
    Millis,
    Percent,    // stored 0..100
    Degrees,
    Decibels,
    Ratio,
    Pan,        // -1..1
    Colour,     // noise colour -1..1
    Wave,       // continuous oscillator shape 0..4
    Crossover,  // -1..1: sub low pass <- off -> VCO high pass
    Choice,
    Toggle
};

struct ParamDef
{
    const char* id;
    const char* name;
    Kind kind;
    float min, max, def;
    Unit unit;
    float skew = 1.0f;          // < 1 = finer at the start; for symmetric: finer around the centre
    bool symmetric = false;
    bool logarithmic = false;   // frequency and time style mapping
    juce::StringArray choices {};
    const char* tip = "";
};

namespace Params
{
    constexpr int numLayers = 2;
    constexpr int numModSlots = 24;

    const std::vector<ParamDef>& layerDefs();   // prefixed "s1_" / "s2_"
    const std::vector<ParamDef>& globalDefs();
    const std::vector<ParamDef>& modSlotDefs(); // prefixed "m01_" .. "m24_"

    juce::String layerID (int layer, const juce::String& suffix);   // layer 0/1 -> "s1_..."
    juce::String modID (int slot, const juce::String& suffix);      // slot 0.. -> "m01_..."

    const juce::StringArray& lfoDivisions();
    double divisionInBeats (int index);   // length in quarter notes

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Lookup of the definition behind a full parameter ID (with prefix)
    const ParamDef* findDef (const juce::String& fullID);

    juce::String format (const ParamDef&, float realValue, bool withUnit = true);
    float parse (const ParamDef&, const juce::String& text, float fallback);   // "2.5k", "A4", "120 ms", "-3"

    juce::String noteName (double midiNote, bool withCents = false);
}

// Modulation sources, destinations and functions
namespace Mod
{
    // Source list is append-only (stored as choice index)
    enum Source
    {
        None = 0, Velocity, Keyboard, ModWheel, Pressure, PitchBend, ReleaseVelocity, Timbre, Constant,
        FirstLayerSource,      // S1 Filter Env
        perLayerSources = 9    // Filter Env, Amp Env, Mod Env, LFO 1-3, Random 1-2, Accent
    };
    enum LayerSource { FilterEnv = 0, AmpEnv, ModEnv, Lfo1, Lfo2, Lfo3, Random1, Random2, Accent };
    constexpr int numSources = FirstLayerSource + 2 * perLayerSources;

    const juce::StringArray& sourceNames();
    juce::String shortSourceName (int src);           // "LFO 1", "Mod Env" (without layer)
    inline int layerSource (int layer, LayerSource s) { return FirstLayerSource + layer * perLayerSources + (int) s; }
    juce::Colour sourceColour (int src);
    bool isBipolar (int src);   // LFOs, random, pitch bend, keyboard

    // Destination codes: 0 = none, 100 + i = layer 1 table index i, 200 + i = layer 2, 300 + i = global table
    int destCode (const juce::String& fullParamID);
    juce::String destParamID (int code);
    juce::String destName (int code);
    juce::Array<int> allDestinations();   // modulatable (float) parameters
    constexpr int maxDestCode = 399;

    const juce::StringArray& functionNames();
    enum Function { FnNone = 0, Scale, Offset, LowClip, HighClip, Exponential, NormExponential,
                    LowPass, HighPass, Slew, LowSlew, HighSlew, SampleHold, Bounce };
}

// Positions in the tables above (checked by the tests against the IDs). Keep in sync, append only.
namespace LP
{
    enum : int
    {
        noise_color = 0, osc_freq, osc_detune, osc_glide, osc2_oct, osc_sync, osc_keyreset, osc2_phase, osc_wave, osc_duty,
        mix_osc1, mix_osc2, mix_noise, mix_sub,
        lpf_cutoff, lpf_res, lpf_eg, hpf_cutoff, hpf_res, hpf_eg,
        flt_order, xover, xover_on, legato, spread, accent,
        sub_phase, sub_wave, sub_offset, subf_cutoff, subf_res, subf_eg, subf_mode,
        lfo1 = 33,                // rate, sync, div, kbreset, wave, phase (6 per LFO)
        fenv_a = 51, fenv_d, fenv_s, fenv_r, aenv_a, aenv_d, aenv_s, aenv_r,
        menv_delay, menv_a, menv_hold, menv_d, menv_s, menv_r,
        rnd1 = 65,                // rate, sync, div, slew, mode (5 per random)
        count = 75
    };
    enum LfoField { rate = 0, sync, div, kbreset, wave, phase };
    enum RndField { rrate = 0, rsync, rdiv, rslew, rmode };
    inline int lfo (int i, LfoField f) { return lfo1 + 6 * i + (int) f; }
    inline int rnd (int i, RndField f) { return rnd1 + 5 * i + (int) f; }
}

namespace GP
{
    enum : int
    {
        fx1_sat = 0, fx1_sattype, dly_time, dly_sync, dly_div, dly_fb, dly_hpf, dly_mix, dly_stereo,
        fx2_sat, fx2_sattype, cho_rate, cho_sync, cho_div, cho_depth, cho_hpf, cho_mix, cho_expand,
        sum_lvl1, sum_lvl2, sum_pan1, sum_pan2, sum_mute1, sum_mute2, master_vol,
        comp_on, comp_fet, comp_attack, comp_ratio, comp_thresh, comp_mix,
        voice_mode, oversampling, pb_range, glide_legato,
        count
    };
}

namespace MP
{
    enum : int { on = 0, src, dst, amt, bi, ctl, ctlamt, fn, fnamt, count };
}

namespace Params
{
    // Crossover knob -1..1 -> filter frequencies (shared by DSP and UI)
    inline float crossoverSubLP (float v) { return 20000.0f * std::pow (40.0f / 20000.0f, -v); }
    inline float crossoverVcoHP (float v) { return 16.0f * std::pow (500.0f / 16.0f, v); }
}
