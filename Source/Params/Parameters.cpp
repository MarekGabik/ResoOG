#include "Parameters.h"

using namespace juce;

namespace
{
const StringArray waveNames { "Sine", "Triangle", "Sharktooth", "Saw", "Square" };

ParamDef F (const char* id, const char* name, float mn, float mx, float df, Unit u, const char* tip,
            float skew = 1.0f, bool sym = false, bool log = false)
{
    ParamDef d { id, name, Kind::Float, mn, mx, df, u, skew, sym, log, {}, tip };
    return d;
}
ParamDef LogF (const char* id, const char* name, float mn, float mx, float df, Unit u, const char* tip)
{
    return F (id, name, mn, mx, df, u, tip, 1.0f, false, true);
}
ParamDef B (const char* id, const char* name, bool df, const char* tip)
{
    ParamDef d { id, name, Kind::Bool, 0.0f, 1.0f, df ? 1.0f : 0.0f, Unit::Toggle, 1.0f, false, false, {}, tip };
    return d;
}
ParamDef C (const char* id, const char* name, StringArray choices, int df, const char* tip)
{
    ParamDef d { id, name, Kind::Choice, 0.0f, (float) (choices.size() - 1), (float) df, Unit::Choice, 1.0f, false, false, choices, tip };
    return d;
}
}

const StringArray& Params::lfoDivisions()
{
    static const StringArray d { "8 bar", "4 bar", "2 bar", "1 bar", "1/2 D", "1/2", "1/2 T", "1/4 D", "1/4", "1/4 T",
                                 "1/8 D", "1/8", "1/8 T", "1/16 D", "1/16", "1/16 T", "1/32", "1/32 T", "1/64" };
    return d;
}

double Params::divisionInBeats (int i)
{
    static const double beats[] = { 32.0, 16.0, 8.0, 4.0, 3.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0,
                                    0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125, 1.0 / 12.0, 0.0625 };
    return beats[jlimit (0, 18, i)];
}

const std::vector<ParamDef>& Params::layerDefs()
{
    static const std::vector<ParamDef> defs = []
    {
        std::vector<ParamDef> d;
        // NOISE
        d.push_back (F ("noise_color", "Noise Color", -1.0f, 1.0f, 0.0f, Unit::Colour,
                        "Noise colour: left = darker (red, pink), centre = white, right = brighter (blue, violet)"));
        // DUAL OSCILLATOR
        d.push_back (F ("osc_freq", "Frequency", -7.0f, 7.0f, 0.0f, Unit::Semitones, "Pitch of both oscillators, +-7 semitones"));
        d.push_back (F ("osc_detune", "Osc 2 Detune", -7.0f, 7.0f, 0.07f, Unit::Semitones,
                        "Detune of oscillator 2 against oscillator 1. Fine around the centre: small values give slow beating", 0.35f, true));
        d.push_back (F ("osc_glide", "Glide", 0.0f, 10.0f, 0.0f, Unit::Seconds, "Time the pitch slides from note to note", 0.25f));
        d.push_back (C ("osc2_oct", "Osc 2 Octave", { "+0", "+1", "+2" }, 0, "Octave of oscillator 2 above oscillator 1"));
        d.push_back (B ("osc_sync", "Hard Sync", false, "Oscillator 2 restarts with every cycle of oscillator 1. Detune osc 2 for the classic sync sweep"));
        d.push_back (B ("osc_keyreset", "Key Reset", true, "Oscillators restart at the same phase on every note: identical, punchy attacks"));
        d.push_back (F ("osc2_phase", "Osc 2 Phase", 0.0f, 360.0f, 0.0f, Unit::Degrees, "Phase of oscillator 2 against oscillator 1 (180 deg on a saw removes odd harmonics)"));
        d.push_back (F ("osc_wave", "Waveshape", 0.0f, 4.0f, 3.0f, Unit::Wave, "Sine - Triangle - Sharktooth - Saw - Square, morphs continuously in between"));
        d.push_back (F ("osc_duty", "Duty Cycle", 0.0f, 100.0f, 50.0f, Unit::Percent, "Symmetry of every waveshape (pulse width on the square)"));
        // MIXER
        d.push_back (F ("mix_osc1", "Osc 1 Level", 0.0f, 10.0f, 7.0f, Unit::Scale10, "Oscillator 1 level. Above 7 the mixer starts to saturate"));
        d.push_back (F ("mix_osc2", "Osc 2 Level", 0.0f, 10.0f, 6.0f, Unit::Scale10, "Oscillator 2 level. Above 7 the mixer starts to saturate"));
        d.push_back (F ("mix_noise", "Noise Level", 0.0f, 10.0f, 0.0f, Unit::Scale10, "Noise level"));
        d.push_back (F ("mix_sub", "Sub Level", 0.0f, 10.0f, 5.0f, Unit::Scale10, "Sub oscillator level (goes to the sub filter)"));
        // LOW PASS
        d.push_back (LogF ("lpf_cutoff", "LPF Cutoff", 16.0f, 20000.0f, 900.0f, Unit::Hz, "Cutoff of the 24 dB ladder low pass"));
        d.push_back (F ("lpf_res", "LPF Resonance", 0.0f, 10.0f, 2.0f, Unit::Scale10, "Ladder resonance; self-oscillates near 10"));
        d.push_back (F ("lpf_eg", "LPF EG Amount", -10.0f, 10.0f, 4.0f, Unit::Bipolar10, "How far the filter envelope moves the cutoff (10 = 7 octaves)"));
        // HIGH PASS
        d.push_back (LogF ("hpf_cutoff", "HPF Cutoff", 16.0f, 20000.0f, 16.0f, Unit::Hz, "Cutoff of the 12 dB high pass"));
        d.push_back (F ("hpf_res", "HPF Resonance", 0.0f, 10.0f, 0.0f, Unit::Scale10, "High pass resonance"));
        d.push_back (F ("hpf_eg", "HPF EG Amount", -10.0f, 10.0f, 0.0f, Unit::Bipolar10, "How far the filter envelope moves the high pass"));
        // FILTERS
        d.push_back (C ("flt_order", "Filter Order", { "Serial", "Parallel", "HPF Noise" }, 0,
                        "Serial: LPF then HPF. Parallel: both summed. HPF Noise: oscillators to LPF, noise to HPF"));
        d.push_back (F ("xover", "Osc Crossover", -1.0f, 1.0f, 0.0f, Unit::Crossover,
                        "Left: low pass on the sub. Right: high pass on the oscillators. Keeps the low end clean and mono"));
        d.push_back (B ("xover_on", "Crossover On", false, "Crossover on: sub and oscillators split by frequency, lows stay mono"));
        // VOICING
        d.push_back (C ("legato", "Legato Mode", { "Re-Trig", "Legato", "Add" }, 0,
                        "Re-Trig: envelopes restart on every note. Legato: overlapping notes only glide. Add: restart from the current level"));
        d.push_back (F ("spread", "Dual Osc Spread", 0.0f, 10.0f, 0.0f, Unit::Scale10, "Pans oscillator 1 left and oscillator 2 right"));
        d.push_back (B ("accent", "Accent", false, "Notes with velocity above 96 open the envelopes harder"));
        // SUB OSC
        d.push_back (F ("sub_phase", "Sub Phase", 0.0f, 360.0f, 0.0f, Unit::Degrees, "Phase of the sub against oscillator 1 (turn it if the low end thins out)"));
        d.push_back (C ("sub_wave", "Sub Waveshape", { "Sine", "Saw", "Square" }, 0, "Sub waveshape"));
        d.push_back (C ("sub_offset", "Sub Freq Offset", { "-1 Octave", "-1 Linked", "-2 Octave" }, 0,
                        "-1 Linked also follows the FREQUENCY knob and its modulation"));
        // SUB FILTER
        d.push_back (LogF ("subf_cutoff", "Sub Cutoff", 16.0f, 8000.0f, 2000.0f, Unit::Hz, "Cutoff of the sub filter"));
        d.push_back (F ("subf_res", "Sub Resonance", 0.0f, 10.0f, 0.0f, Unit::Scale10, "Sub filter resonance"));
        d.push_back (F ("subf_eg", "Sub EG Amount", -10.0f, 10.0f, 0.0f, Unit::Bipolar10, "How far the filter envelope moves the sub filter"));
        d.push_back (C ("subf_mode", "Sub Filter Mode", { "High", "Band", "Low" }, 2, "Sub filter type"));

        // CNTRL: LFO 1-3
        for (int i = 1; i <= 3; ++i)
        {
            static std::vector<String> keep;   // ids and names must outlive the table
            auto s = [] (String v) { keep.push_back (v); return keep.back().toRawUTF8(); };
            keep.reserve (64);
            const String n (i), p = "lfo" + n + "_", N = "LFO " + n + " ";
            d.push_back (LogF (s (p + "rate"), s (N + "Rate"), 0.05f, 50.0f, i == 1 ? 4.0f : (i == 2 ? 0.6f : 0.15f), Unit::Hz, "LFO speed"));
            d.push_back (B (s (p + "sync"), s (N + "Sync"), false, "Lock the LFO to the song tempo"));
            d.push_back (C (s (p + "div"), s (N + "Division"), Params::lfoDivisions(), 11, "LFO length when synced"));
            d.push_back (B (s (p + "kbreset"), s (N + "KB Reset"), false, "Restart the LFO on every note"));
            d.push_back (C (s (p + "wave"), s (N + "Waveshape"), { "Sine", "Triangle", "Ramp Up", "Ramp Down", "Square" }, 0, "LFO waveshape"));
            d.push_back (F (s (p + "phase"), s (N + "Phase"), 0.0f, 360.0f, 0.0f, Unit::Degrees, "Where the LFO starts on a key reset"));
        }
        // Filter and amp envelopes
        d.push_back (LogF ("fenv_a", "Filter Attack", 0.001f, 10.0f, 0.001f, Unit::Seconds, "Filter envelope attack"));
        d.push_back (LogF ("fenv_d", "Filter Decay", 0.001f, 30.0f, 0.35f, Unit::Seconds, "Filter envelope decay"));
        d.push_back (F ("fenv_s", "Filter Sustain", 0.0f, 100.0f, 20.0f, Unit::Percent, "Filter envelope sustain level"));
        d.push_back (LogF ("fenv_r", "Filter Release", 0.001f, 30.0f, 0.2f, Unit::Seconds, "Filter envelope release"));
        d.push_back (LogF ("aenv_a", "Amp Attack", 0.001f, 10.0f, 0.002f, Unit::Seconds, "Amplifier attack"));
        d.push_back (LogF ("aenv_d", "Amp Decay", 0.001f, 30.0f, 0.6f, Unit::Seconds, "Amplifier decay"));
        d.push_back (F ("aenv_s", "Amp Sustain", 0.0f, 100.0f, 80.0f, Unit::Percent, "Amplifier sustain level"));
        d.push_back (LogF ("aenv_r", "Amp Release", 0.001f, 30.0f, 0.06f, Unit::Seconds, "Amplifier release"));
        // Mod envelope DAHDSR
        d.push_back (F ("menv_delay", "Mod Delay", 0.0f, 2.0f, 0.0f, Unit::Seconds, "Wait before the attack starts", 0.4f));
        d.push_back (LogF ("menv_a", "Mod Attack", 0.001f, 10.0f, 0.01f, Unit::Seconds, "Mod envelope attack"));
        d.push_back (F ("menv_hold", "Mod Hold", 0.0f, 2.0f, 0.0f, Unit::Seconds, "Time at the peak before the decay", 0.4f));
        d.push_back (LogF ("menv_d", "Mod Decay", 0.001f, 30.0f, 0.5f, Unit::Seconds, "Mod envelope decay"));
        d.push_back (F ("menv_s", "Mod Sustain", 0.0f, 100.0f, 0.0f, Unit::Percent, "Mod envelope sustain level"));
        d.push_back (LogF ("menv_r", "Mod Release", 0.001f, 30.0f, 0.3f, Unit::Seconds, "Mod envelope release"));
        // Random 1-2
        for (int i = 1; i <= 2; ++i)
        {
            static std::vector<String> keep;
            auto s = [] (String v) { keep.push_back (v); return keep.back().toRawUTF8(); };
            keep.reserve (32);
            const String n (i), p = "rnd" + n + "_", N = "Random " + n + " ";
            d.push_back (LogF (s (p + "rate"), s (N + "Rate"), 0.05f, 50.0f, i == 1 ? 4.0f : 1.0f, Unit::Hz, "How often a new random value is picked"));
            d.push_back (B (s (p + "sync"), s (N + "Sync"), false, "Lock the random clock to the song tempo"));
            d.push_back (C (s (p + "div"), s (N + "Division"), Params::lfoDivisions(), 14, "Random clock when synced"));
            d.push_back (F (s (p + "slew"), s (N + "Slew"), 0.0f, 10.0f, 0.0f, Unit::Scale10, "Smooths the jumps between random values"));
            d.push_back (C (s (p + "mode"), s (N + "Mode"), { "S+H", "Noise", "Perlin" }, i == 1 ? 0 : 2,
                            "S+H: stepped values. Noise: fast random. Perlin: smooth organic drift"));
        }
        return d;
    }();
    return defs;
}

const std::vector<ParamDef>& Params::globalDefs()
{
    static const std::vector<ParamDef> defs = []
    {
        const StringArray satTypes { "Off", "Tube", "Tape", "Drive" };
        std::vector<ParamDef> d;
        // Synth 1 effects
        d.push_back (F ("fx1_sat", "S1 Saturation", 0.0f, 10.0f, 3.0f, Unit::Scale10, "Saturation drive of synth 1"));
        d.push_back (C ("fx1_sattype", "S1 Saturation Type", satTypes, 0, "Tube: warm, even harmonics. Tape: soft and round. Drive: transistor bite"));
        d.push_back (F ("dly_time", "Delay Time", 0.001f, 1.4f, 0.35f, Unit::Seconds, "Delay time (when not synced)", 0.5f));
        d.push_back (B ("dly_sync", "Delay Sync", true, "Delay time in note values"));
        d.push_back (C ("dly_div", "Delay Division", Params::lfoDivisions(), 10, "Delay time when synced"));
        d.push_back (F ("dly_fb", "Delay Feedback", 0.0f, 10.0f, 3.5f, Unit::Scale10, "Number of repeats"));
        d.push_back (LogF ("dly_hpf", "Delay HPF", 40.0f, 2000.0f, 300.0f, Unit::Hz, "High pass inside the delay: repeats stay out of the sub"));
        d.push_back (F ("dly_mix", "Delay Mix", 0.0f, 100.0f, 0.0f, Unit::Percent, "Dry / wet"));
        d.push_back (B ("dly_stereo", "Delay Stereo", true, "Ping-pong repeats between left and right"));
        // Synth 2 effects
        d.push_back (F ("fx2_sat", "S2 Saturation", 0.0f, 10.0f, 3.0f, Unit::Scale10, "Saturation drive of synth 2"));
        d.push_back (C ("fx2_sattype", "S2 Saturation Type", satTypes, 0, "Tube: warm, even harmonics. Tape: soft and round. Drive: transistor bite"));
        d.push_back (LogF ("cho_rate", "Chorus Rate", 0.05f, 20.0f, 0.8f, Unit::Hz, "Chorus speed"));
        d.push_back (B ("cho_sync", "Chorus Sync", false, "Chorus speed in note values"));
        d.push_back (C ("cho_div", "Chorus Division", Params::lfoDivisions(), 3, "Chorus cycle when synced"));
        d.push_back (F ("cho_depth", "Chorus Depth", 0.0f, 10.0f, 4.0f, Unit::Scale10, "Chorus depth"));
        d.push_back (LogF ("cho_hpf", "Chorus HPF", 40.0f, 2000.0f, 250.0f, Unit::Hz, "Only frequencies above this are chorused: the sub stays solid"));
        d.push_back (F ("cho_mix", "Chorus Mix", 0.0f, 100.0f, 0.0f, Unit::Percent, "Dry / wet"));
        d.push_back (B ("cho_expand", "Chorus Expand", false, "Wider stereo image"));
        // Summing
        d.push_back (F ("sum_lvl1", "Synth 1 Level", -60.0f, 6.0f, 0.0f, Unit::Decibels, "Synth 1 volume", 2.5f));
        d.push_back (F ("sum_lvl2", "Synth 2 Level", -60.0f, 6.0f, -3.0f, Unit::Decibels, "Synth 2 volume", 2.5f));
        d.push_back (F ("sum_pan1", "Synth 1 Pan", -1.0f, 1.0f, 0.0f, Unit::Pan, "Synth 1 position left / right"));
        d.push_back (F ("sum_pan2", "Synth 2 Pan", -1.0f, 1.0f, 0.0f, Unit::Pan, "Synth 2 position left / right"));
        d.push_back (B ("sum_mute1", "Synth 1 Mute", false, "Mute synth 1"));
        d.push_back (B ("sum_mute2", "Synth 2 Mute", true, "Mute synth 2 (also saves CPU)"));
        d.push_back (F ("master_vol", "Volume", -60.0f, 12.0f, 0.0f, Unit::Decibels, "Output volume after the compressor", 2.5f));
        // Compressor
        d.push_back (B ("comp_on", "Compressor", false, "Compressor on / off"));
        d.push_back (B ("comp_fet", "FET", true, "FET character: feedback detection and a little grit, like a classic limiter"));
        d.push_back (LogF ("comp_attack", "Comp Attack", 0.01f, 1000.0f, 3.0f, Unit::Millis, "How fast the compressor grabs"));
        d.push_back (F ("comp_ratio", "Comp Ratio", 1.0f, 20.0f, 4.0f, Unit::Ratio, "Compression ratio (20:1 = limiting)", 0.4f));
        d.push_back (F ("comp_thresh", "Comp Threshold", -30.0f, 0.0f, -12.0f, Unit::Decibels, "Level where compression starts (with automatic make-up gain)"));
        d.push_back (F ("comp_mix", "Comp Mix", 0.0f, 100.0f, 100.0f, Unit::Percent, "Parallel compression: blend of dry and compressed"));
        // Global performance
        d.push_back (C ("voice_mode", "Voice Mode", { "Layered", "Duophonic" }, 0,
                        "Layered: both synths play the same note. Duophonic: synth 1 plays the lowest, synth 2 the highest held note"));
        d.push_back (C ("oversampling", "Oversampling", { "1x", "2x", "4x" }, 1, "Oscillators and filters run at a higher rate: less aliasing, more CPU"));
        d.push_back (F ("pb_range", "Pitch Bend Range", 0.0f, 24.0f, 2.0f, Unit::Semitones, "Pitch bend range in semitones"));
        d.push_back (B ("glide_legato", "Glide On Legato", false, "Glide only between overlapping notes"));
        return d;
    }();
    return defs;
}

const std::vector<ParamDef>& Params::modSlotDefs()
{
    static const std::vector<ParamDef> defs = []
    {
        std::vector<ParamDef> d;
        d.push_back (B ("on", "On", true, "Modulation on / off"));
        d.push_back (C ("src", "Source", Mod::sourceNames(), 0, "Modulation source"));
        ParamDef dst { "dst", "Destination", Kind::Float, 0.0f, (float) Mod::maxDestCode, 0.0f, Unit::Plain, 1.0f, false, false, {}, "Modulated parameter" };
        d.push_back (dst);
        d.push_back (F ("amt", "Amount", -1.0f, 1.0f, 0.25f, Unit::Pan, "Depth (negative = inverted)"));
        d.push_back (B ("bi", "Bipolar", true, "Bipolar: swings around the knob position. Unipolar: only one direction"));
        d.push_back (C ("ctl", "Controller", Mod::sourceNames(), 0, "Second source that scales this modulation (like a VCA)"));
        d.push_back (F ("ctlamt", "Controller Amount", -1.0f, 1.0f, 1.0f, Unit::Pan, "How much the controller scales the modulation"));
        d.push_back (C ("fn", "Function", Mod::functionNames(), 0, "Shapes the modulation signal"));
        d.push_back (F ("fnamt", "Function Amount", 0.0f, 100.0f, 50.0f, Unit::Percent, "Function strength"));
        return d;
    }();
    return defs;
}

String Params::layerID (int layer, const String& suffix) { return "s" + String (layer + 1) + "_" + suffix; }
String Params::modID (int slot, const String& suffix)    { return "m" + String (slot + 1).paddedLeft ('0', 2) + "_" + suffix; }

const ParamDef* Params::findDef (const String& id)
{
    if (id.length() > 3 && (id.startsWith ("s1_") || id.startsWith ("s2_")))
    {
        const auto s = id.substring (3);
        for (auto& d : layerDefs()) if (s == d.id) return &d;
        return nullptr;
    }
    if (id.length() > 4 && id[0] == 'm' && id[3] == '_')
    {
        const auto s = id.substring (4);
        for (auto& d : modSlotDefs()) if (s == d.id) return &d;
        return nullptr;
    }
    for (auto& d : globalDefs()) if (id == d.id) return &d;
    return nullptr;
}

//==============================================================================
static NormalisableRange<float> makeRange (const ParamDef& d)
{
    if (d.logarithmic)
    {
        const float mn = d.min, mx = d.max;
        return NormalisableRange<float> (mn, mx,
            [] (float a, float b, float x) { return a * std::pow (b / a, x); },
            [] (float a, float b, float v) { return std::log (jlimit (a, b, v) / a) / std::log (b / a); },
            [] (float a, float b, float v) { return jlimit (a, b, v); });
    }
    return NormalisableRange<float> (d.min, d.max, 0.0f, d.skew, d.symmetric);
}

static void addParam (std::vector<std::unique_ptr<RangedAudioParameter>>& list, const ParamDef& d, const String& id, const String& name)
{
    const int version = 1;
    if (d.kind == Kind::Bool)
    {
        list.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, version }, name, d.def > 0.5f));
    }
    else if (d.kind == Kind::Choice)
    {
        list.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, version }, name, d.choices, (int) d.def));
    }
    else if (String (d.id) == "dst")
    {
        list.push_back (std::make_unique<AudioParameterInt> (ParameterID { id, version }, name, 0, Mod::maxDestCode, 0,
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return Mod::destName (v); })
                                         .withAutomatable (false)));
    }
    else
    {
        const ParamDef* def = &d;
        list.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, version }, name, makeRange (d), d.def,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([def] (float v, int) { return Params::format (*def, v); })
                .withValueFromStringFunction ([def] (const String& t) { return Params::parse (*def, t, def->def); })));
    }
}

AudioProcessorValueTreeState::ParameterLayout Params::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> list;
    for (int l = 0; l < numLayers; ++l)
        for (auto& d : layerDefs())
            addParam (list, d, layerID (l, d.id), "S" + String (l + 1) + " " + d.name);
    for (auto& d : globalDefs())
        addParam (list, d, d.id, d.name);
    for (int s = 0; s < numModSlots; ++s)
        for (auto& d : modSlotDefs())
            addParam (list, d, modID (s, d.id), "Mod " + String (s + 1) + " " + d.name);
    return { list.begin(), list.end() };
}

//==============================================================================
String Params::noteName (double midi, bool withCents)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int n = roundToInt (midi);
    String s = String (names[((n % 12) + 12) % 12]) + String (n / 12 - 1);
    if (withCents)
    {
        const int cents = roundToInt ((midi - n) * 100.0);
        if (cents != 0) s << (cents > 0 ? "+" : "") << cents;
    }
    return s;
}

static String trimZeros (double v, int decimals)
{
    return String (v, decimals);
}

String Params::format (const ParamDef& d, float v, bool withUnit)
{
    auto u = [withUnit] (const char* s) { return withUnit ? String (s) : String(); };
    switch (d.unit)
    {
        case Unit::Plain:     return trimZeros (v, 1);
        case Unit::Scale10:   return String (v, 1);
        case Unit::Bipolar10: return (v > 0.04f ? "+" : "") + String (std::abs (v) < 0.05f ? 0.0f : v, 1);
        case Unit::Semitones: return (v > 0.004f ? "+" : "") + String (std::abs (v) < 0.005f ? 0.0f : v, 2) + u (" st");
        case Unit::Hz:
            if (v >= 1000.0f) return String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + u (" kHz");
            return String (v, v < 10.0f ? 2 : (v < 100.0f ? 1 : 0)) + u (" Hz");
        case Unit::Seconds:
            if (v < 0.0005f) return d.min <= 0.0f ? String ("off") : String ("0 ms");
            if (v < 0.01f) return String (v * 1000.0f, 1) + u (" ms");
            if (v < 1.0f) return String (roundToInt (v * 1000.0f)) + u (" ms");
            return String (v, 2) + u (" s");
        case Unit::Millis:
            if (v < 1.0f) return String (v, 2) + u (" ms");
            if (v < 100.0f) return String (v, 1) + u (" ms");
            if (v < 1000.0f) return String (roundToInt (v)) + u (" ms");
            return String (v / 1000.0f, 2) + u (" s");
        case Unit::Percent:   return String (roundToInt (v)) + u (" %");
        case Unit::Degrees:   return String (roundToInt (v)) + (withUnit ? CharPointer_UTF8 ("\xc2\xb0") : "");
        case Unit::Decibels:
            if (v <= d.min + 0.01f && d.min <= -59.0f) return "-inf" + u (" dB");
            return String (v, 1) + u (" dB");
        case Unit::Ratio:     return String (v, v < 10.0f ? 1 : 0) + ":1";
        case Unit::Pan:
            if (d.min < -0.5f && String (d.id).contains ("pan"))
            {
                const int p = roundToInt (v * 100.0f);
                return p == 0 ? String ("C") : (p < 0 ? "L " + String (-p) : "R " + String (p));
            }
            return (v > 0.0049f ? "+" : "") + String (roundToInt (v * 100.0f)) + u (" %");
        case Unit::Colour:
        {
            static const char* n[] = { "Red", "Pink", "White", "Blue", "Violet" };
            const int i = jlimit (0, 4, roundToInt ((v + 1.0f) * 2.0f));
            return String (n[i]) + (withUnit ? String (" ") + (v > 0.005f ? "+" : "") + String (v * 6.0f, 1) + " dB/oct" : String());
        }
        case Unit::Wave:
        {
            const int i = jlimit (0, 4, (int) std::floor (v + 0.0001f));
            const float f = v - (float) i;
            if (f < 0.04f || i == 4) return waveNames[i];
            if (f > 0.96f) return waveNames[i + 1];
            return waveNames[i].substring (0, 5) + ">" + waveNames[i + 1].substring (0, 5) + " " + String (roundToInt (f * 100.0f)) + "%";
        }
        case Unit::Crossover:
            if (v < -0.01f) return "Sub LP " + format (ParamDef { "", "", Kind::Float, 16, 20000, 0, Unit::Hz }, 20000.0f * std::pow (40.0f / 20000.0f, -v));
            if (v > 0.01f) return "VCO HP " + format (ParamDef { "", "", Kind::Float, 16, 20000, 0, Unit::Hz }, 16.0f * std::pow (500.0f / 16.0f, v));
            return "Neutral";
        case Unit::Choice:    return d.choices[jlimit (0, d.choices.size() - 1, roundToInt (v))];
        case Unit::Toggle:    return v > 0.5f ? "On" : "Off";
    }
    return String (v);
}

static double parseNote (const String& t, bool& ok)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    auto s = t.trim().toUpperCase().removeCharacters (" ");
    ok = false;
    if (s.isEmpty() || ! (s[0] >= 'A' && s[0] <= 'G')) return 0.0;
    int len = (s.length() > 1 && (s[1] == '#' || s[1] == 'B')) ? 2 : 1;
    auto name = s.substring (0, len).replace ("B", len == 2 ? "b" : "B");
    int pc = -1;
    for (int i = 0; i < 12; ++i) if (name == names[i]) pc = i;
    if (pc < 0 && len == 2 && name.endsWith ("b"))
    {
        for (int i = 0; i < 12; ++i) if (name.substring (0, 1) == names[i]) pc = (i + 11) % 12;
    }
    if (pc < 0) return 0.0;
    auto rest = s.substring (len);
    int sign = 1, oct = 0, cents = 0, i = 0;
    if (rest.startsWith ("-")) { sign = -1; ++i; }
    if (i >= rest.length() || ! CharacterFunctions::isDigit (rest[i])) return 0.0;
    while (i < rest.length() && CharacterFunctions::isDigit (rest[i])) oct = oct * 10 + (rest[i++] - '0');
    if (i < rest.length()) cents = rest.substring (i).getIntValue();
    ok = true;
    return 440.0 * std::pow (2.0, ((sign * oct + 1) * 12 + pc - 69 + cents / 100.0) / 12.0);
}

float Params::parse (const ParamDef& d, const String& text, float fallback)
{
    auto t = text.trim();
    auto lower = t.toLowerCase();
    if (t.isEmpty()) return fallback;
    const double num = t.retainCharacters ("0123456789.-+").getDoubleValue();
    const bool hasDigit = t.containsAnyOf ("0123456789");
    auto clampV = [&d] (double v) { return (float) jlimit ((double) d.min, (double) d.max, v); };

    switch (d.unit)
    {
        case Unit::Hz:
        {
            bool ok = false;
            const double note = parseNote (t, ok);
            if (ok) return clampV (note);
            if (! hasDigit) return fallback;
            return clampV (lower.containsChar ('k') ? num * 1000.0 : num);
        }
        case Unit::Seconds:
            if (! hasDigit) return d.min <= 0.0f && lower.startsWith ("off") ? 0.0f : fallback;
            if (lower.contains ("ms")) return clampV (num / 1000.0);
            if (lower.endsWith ("s")) return clampV (num);
            return clampV (num >= 1.0 ? num / 1000.0 : num);   // bare numbers >= 1 are milliseconds
        case Unit::Millis:
            if (lower.endsWith ("s") && ! lower.contains ("ms")) return clampV (num * 1000.0);
            return hasDigit ? clampV (num) : fallback;
        case Unit::Decibels:
            if (lower.contains ("inf")) return d.min;
            return hasDigit ? clampV (num) : fallback;
        case Unit::Ratio:
            return hasDigit ? clampV (t.upToFirstOccurrenceOf (":", false, false).getDoubleValue()) : fallback;
        case Unit::Pan:
            if (String (d.id).contains ("pan"))
            {
                if (lower == "c") return 0.0f;
                if (lower.startsWith ("l")) return clampV (-std::abs (num) / 100.0);
                if (lower.startsWith ("r")) return clampV (std::abs (num) / 100.0);
            }
            return hasDigit ? clampV (num / 100.0) : fallback;
        case Unit::Colour:
        {
            static const char* n[] = { "red", "pink", "white", "blue", "violet" };
            for (int i = 0; i < 5; ++i) if (lower.startsWith (n[i])) return (float) i / 2.0f - 1.0f;
            return hasDigit ? clampV (num / 6.0) : fallback;   // dB/oct
        }
        case Unit::Wave:
            for (int i = 0; i < 5; ++i) if (lower.startsWith (waveNames[i].toLowerCase().substring (0, 3))) return (float) i;
            return hasDigit ? clampV (num) : fallback;
        case Unit::Choice:
            for (int i = 0; i < d.choices.size(); ++i)
                if (d.choices[i].equalsIgnoreCase (t)) return (float) i;
            for (int i = 0; i < d.choices.size(); ++i)
                if (d.choices[i].startsWithIgnoreCase (t)) return (float) i;
            return hasDigit ? clampV (num) : fallback;
        case Unit::Toggle:
            return lower == "on" || lower == "1" || lower == "true" ? 1.0f : 0.0f;
        case Unit::Crossover:
        case Unit::Plain: case Unit::Scale10: case Unit::Bipolar10: case Unit::Semitones:
        case Unit::Percent: case Unit::Degrees:
            return hasDigit ? clampV (num) : fallback;
    }
    return fallback;
}

//==============================================================================
const StringArray& Mod::sourceNames()
{
    static const StringArray s = []
    {
        StringArray a { "None", "Velocity", "Keyboard Tracking", "Mod Wheel", "Pressure", "Pitch Bend", "Release Velocity",
                        "MPE Timbre", "Constant" };
        for (int l = 1; l <= 2; ++l)
            for (auto n : { "Filter Env", "Amp Env", "Mod Env", "LFO 1", "LFO 2", "LFO 3", "Random 1", "Random 2", "Accent" })
                a.add ("S" + String (l) + " " + n);
        return a;
    }();
    return s;
}

String Mod::shortSourceName (int src)
{
    auto n = sourceNames()[jlimit (0, numSources - 1, src)];
    return src >= FirstLayerSource ? n.substring (3) : n;
}

Colour Mod::sourceColour (int src)
{
    static const uint32 global[] = { 0xff5e5f68, 0xffffb86b, 0xffc9cdd8, 0xff7fd4ff, 0xff3ddba0, 0xff8f8cff, 0xffe57bff, 0xff2fd1c0, 0xff9a9ba4 };
    static const uint32 layer[] = { 0xffff9a4d, 0xffa5e844, 0xffff74b8, 0xff38d6f0, 0xff4aa8ff, 0xffb98bff, 0xffffe066, 0xff5ee0c8, 0xffff6b81 };
    if (src < FirstLayerSource) return Colour (global[jlimit (0, 8, src)]);
    return Colour (layer[(src - FirstLayerSource) % perLayerSources]);
}

bool Mod::isBipolar (int src)
{
    if (src == PitchBend) return true;
    if (src < FirstLayerSource) return false;
    const int s = (src - FirstLayerSource) % perLayerSources;
    return s >= Lfo1 && s <= Random2;
}

int Mod::destCode (const String& id)
{
    auto find = [] (const std::vector<ParamDef>& defs, const String& s) -> int
    {
        for (size_t i = 0; i < defs.size(); ++i) if (s == defs[i].id) return (int) i;
        return -1;
    };
    if (id.startsWith ("s1_") || id.startsWith ("s2_"))
    {
        const int i = find (Params::layerDefs(), id.substring (3));
        return i < 0 ? 0 : (id[1] == '1' ? 100 : 200) + i;
    }
    const int i = find (Params::globalDefs(), id);
    return i < 0 ? 0 : 300 + i;
}

String Mod::destParamID (int code)
{
    const int block = code / 100, i = code % 100;
    if (block == 1 || block == 2)
        return i < (int) Params::layerDefs().size() ? Params::layerID (block - 1, Params::layerDefs()[(size_t) i].id) : String();
    if (block == 3)
        return i < (int) Params::globalDefs().size() ? String (Params::globalDefs()[(size_t) i].id) : String();
    return {};
}

String Mod::destName (int code)
{
    const int block = code / 100, i = code % 100;
    if (block == 1 || block == 2)
        return i < (int) Params::layerDefs().size() ? "S" + String (block) + " " + Params::layerDefs()[(size_t) i].name : String ("-");
    if (block == 3)
        return i < (int) Params::globalDefs().size() ? String (Params::globalDefs()[(size_t) i].name) : String ("-");
    return "None";
}

Array<int> Mod::allDestinations()
{
    Array<int> a;
    for (int b = 1; b <= 2; ++b)
        for (size_t i = 0; i < Params::layerDefs().size(); ++i)
            if (Params::layerDefs()[i].kind == Kind::Float) a.add (b * 100 + (int) i);
    for (size_t i = 0; i < Params::globalDefs().size(); ++i)
        if (Params::globalDefs()[i].kind == Kind::Float) a.add (300 + (int) i);
    return a;
}

const StringArray& Mod::functionNames()
{
    static const StringArray f { "None", "Scale", "Offset", "Low Clip", "High Clip", "Exponential", "Norm Exponential",
                                 "Low Pass", "High Pass", "Slew", "Low Slew", "High Slew", "Sample & Hold", "Bounce" };
    return f;
}
