// Offline test harness for ResoOG.
// 1) Parameters: table order, IDs, formatting and parsing
// 2) Sound engine: default patch, aliasing, ladder tuning, glide, voice modes, sustain, modulation, robustness, state, presets
// 3) CPU benchmark
// 4) Editor snapshots and gestures
// 5) Loads the built VST3 through JUCE's hosting layer
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/FactoryPresets.h"
#include "DSP/Primitives.h"

using namespace juce;

static int failures = 0;
static File snapshotDir;

// Shared CI runners are slow and noisy: timing limits get 3x headroom there (functional limits stay strict)
static double timing (double limit) { return std::getenv ("CI") != nullptr ? 3.0 * limit : limit; }

static void check (bool ok, const String& what)
{
    std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << what << std::endl;
    if (! ok) ++failures;
}

static void pumpMessages (int ms = 30) { MessageManager::getInstance()->runDispatchLoopUntil (ms); }

static void setParam (AudioProcessor& p, const String& id, float realValue)
{
    if (auto* rp = dynamic_cast<ResoOGProcessor&> (p).param (id))
    {
        rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
        return;
    }
    std::cout << "  ! unknown parameter " << id << std::endl;
    ++failures;
}

static std::unique_ptr<ResoOGProcessor> makeProc (double sr = 48000.0, int block = 512)
{
    auto p = std::make_unique<ResoOGProcessor>();
    p->setPlayConfigDetails (0, 2, sr, block);
    p->prepareToPlay (sr, block);
    return p;
}

struct Ev { double time; MidiMessage msg; };

static AudioBuffer<float> render (ResoOGProcessor& p, double seconds, std::vector<Ev> events, int block = 512)
{
    const double sr = p.getSampleRate();
    const int n = (int) (seconds * sr);
    AudioBuffer<float> out (2, n);
    out.clear();
    AudioBuffer<float> buf (2, block);
    std::sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) { return a.time < b.time; });
    size_t ei = 0;
    for (int pos = 0; pos < n; pos += block)
    {
        const int len = std::min (block, n - pos);
        buf.setSize (2, len, false, false, true);
        buf.clear();
        MidiBuffer midi;
        while (ei < events.size() && (int) (events[ei].time * sr) < pos + len)
        {
            midi.addEvent (events[ei].msg, std::max (0, (int) (events[ei].time * sr) - pos));
            ++ei;
        }
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, len);
    }
    return out;
}

static std::vector<Ev> note (int n, double on, double off, float vel = 0.8f)
{
    return { { on, MidiMessage::noteOn (1, n, vel) }, { off, MidiMessage::noteOff (1, n) } };
}

static bool allFinite (const AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}

static double peakDb (const AudioBuffer<float>& b, int start = 0, int end = -1)
{
    if (end < 0) end = b.getNumSamples();
    float pk = 0.0f;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = start; i < end; ++i) pk = std::max (pk, std::abs (b.getSample (c, i)));
    return 20.0 * std::log10 (pk + 1e-12);
}

static double rmsDb (const AudioBuffer<float>& b, int start, int end)
{
    double s = 0.0;
    int count = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = start; i < end; ++i, ++count) s += (double) b.getSample (c, i) * b.getSample (c, i);
    return 10.0 * std::log10 (s / std::max (1, count) + 1e-20);
}

// magnitude spectrum (dB) of channel 0, Blackman-Harris window
static std::vector<double> spectrum (const AudioBuffer<float>& b, int start, int order)
{
    const int N = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) N * 2, 0.0f);
    for (int i = 0; i < N; ++i)
    {
        const double w = 0.35875 - 0.48829 * std::cos (2 * MathConstants<double>::pi * i / N) + 0.14128 * std::cos (4 * MathConstants<double>::pi * i / N)
                         - 0.01168 * std::cos (6 * MathConstants<double>::pi * i / N);
        data[(size_t) i] = (float) (b.getSample (0, start + i) * w);
    }
    fft.performFrequencyOnlyForwardTransform (data.data());
    std::vector<double> db ((size_t) N / 2);
    for (int i = 0; i < N / 2; ++i) db[(size_t) i] = 20.0 * std::log10 (data[(size_t) i] + 1e-12);
    return db;
}

//==============================================================================
static void testParameters()
{
    std::cout << "Parameters" << std::endl;
    const StringArray layerOrder { "noise_color", "osc_freq", "osc_detune", "osc_glide", "osc2_oct", "osc_sync", "osc_keyreset", "osc2_phase", "osc_wave", "osc_duty",
        "mix_osc1", "mix_osc2", "mix_noise", "mix_sub", "lpf_cutoff", "lpf_res", "lpf_eg", "hpf_cutoff", "hpf_res", "hpf_eg",
        "flt_order", "xover", "xover_on", "legato", "spread", "accent", "sub_phase", "sub_wave", "sub_offset", "subf_cutoff", "subf_res", "subf_eg", "subf_mode" };
    bool ok = (int) Params::layerDefs().size() == LP::count;
    for (int i = 0; i < layerOrder.size(); ++i) ok = ok && layerOrder[i] == Params::layerDefs()[(size_t) i].id;
    ok = ok && String (Params::layerDefs()[(size_t) LP::lfo (2, LP::phase)].id) == "lfo3_phase";
    ok = ok && String (Params::layerDefs()[(size_t) LP::fenv_a].id) == "fenv_a" && String (Params::layerDefs()[(size_t) LP::menv_r].id) == "menv_r";
    ok = ok && String (Params::layerDefs()[(size_t) LP::rnd (1, LP::rmode)].id) == "rnd2_mode";
    check (ok, "layer table matches the LP index constants (" + String ((int) Params::layerDefs().size()) + " per layer)");
    const StringArray globalOrder { "fx1_sat", "fx1_sattype", "dly_time", "dly_sync", "dly_div", "dly_fb", "dly_hpf", "dly_mix", "dly_stereo",
        "fx2_sat", "fx2_sattype", "cho_rate", "cho_sync", "cho_div", "cho_depth", "cho_hpf", "cho_mix", "cho_expand",
        "sum_lvl1", "sum_lvl2", "sum_pan1", "sum_pan2", "sum_mute1", "sum_mute2", "master_vol",
        "comp_on", "comp_fet", "comp_attack", "comp_ratio", "comp_thresh", "comp_mix", "voice_mode", "oversampling", "pb_range", "glide_legato" };
    ok = (int) Params::globalDefs().size() == GP::count;
    for (int i = 0; i < globalOrder.size() && ok; ++i) ok = globalOrder[i] == Params::globalDefs()[(size_t) i].id;
    check (ok, "global table matches the GP index constants (" + String ((int) Params::globalDefs().size()) + ")");

    ResoOGProcessor p;
    StringArray ids;
    for (auto* prm : p.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (prm)) ids.addIfNotAlreadyThere (rp->paramID);
    const int expected = 2 * LP::count + GP::count + Params::numModSlots * MP::count;
    check (p.getParameters().size() == expected && ids.size() == expected,
           String (p.getParameters().size()) + " parameters with unique IDs (expected " + String (expected) + ")");

    auto def = [] (const char* id) { return *Params::findDef (id); };
    check (std::abs (Params::parse (def ("s1_lpf_cutoff"), "2.5k", 0) - 2500.0f) < 0.1f, "parse \"2.5k\" = 2500 Hz");
    check (std::abs (Params::parse (def ("s1_lpf_cutoff"), "A4", 0) - 440.0f) < 0.1f, "parse note \"A4\" = 440 Hz");
    check (std::abs (Params::parse (def ("s1_lpf_cutoff"), "C#3+20", 0) - 140.20f) < 0.1f, "parse \"C#3+20\" (cents)");
    check (std::abs (Params::parse (def ("s1_fenv_d"), "120 ms", 0) - 0.12f) < 1e-4f, "parse \"120 ms\" = 0.12 s");
    check (std::abs (Params::parse (def ("sum_pan1"), "L 30", 0) + 0.3f) < 1e-4f, "parse pan \"L 30\"");
    check (Params::format (def ("s1_lpf_cutoff"), 1200.0f) == "1.20 kHz" && Params::format (def ("s1_fenv_d"), 0.048f) == "48 ms",
           "format 1.20 kHz / 48 ms");
    check (Mod::destParamID (Mod::destCode ("s2_lpf_cutoff")) == "s2_lpf_cutoff" && Mod::destParamID (Mod::destCode ("sum_pan1")) == "sum_pan1",
           "modulation destination codes round-trip");
}

static void testDefaultPatch()
{
    std::cout << "Default patch" << std::endl;
    auto p = makeProc();
    auto out = render (*p, 2.0, note (36, 0.05, 1.0));
    const double sr = 48000.0;
    check (allFinite (out), "output is finite");
    const double pk = peakDb (out);
    check (pk > -18.0 && pk < -3.0, "note C1 plays at a sensible level (peak " + String (pk, 1) + " dBFS)");
    const double tail = rmsDb (out, (int) (1.8 * sr), (int) (2.0 * sr));
    check (tail < -90.0, "release decays to silence (" + String (tail, 1) + " dB)");
    const double before = rmsDb (out, 0, (int) (0.04 * sr));
    check (before < -120.0, "silent before the first note");
}

static void quietOscPatch (ResoOGProcessor& p, float wave)
{
    setParam (p, "s1_osc_wave", wave);
    setParam (p, "s1_mix_osc1", 4.0f);
    setParam (p, "s1_mix_osc2", 0.0f);
    setParam (p, "s1_mix_sub", 0.0f);
    setParam (p, "s1_lpf_cutoff", 20000.0f);
    setParam (p, "s1_lpf_res", 0.0f);
    setParam (p, "s1_lpf_eg", 0.0f);
    setParam (p, "s1_aenv_s", 100.0f);
    setParam (p, "s1_osc_keyreset", 0.0f);
}

static double aliasDb (ResoOGProcessor& p, int midi)
{
    const double sr = p.getSampleRate();
    auto out = render (*&p, 1.2, note (midi, 0.0, 1.19));
    const auto db = spectrum (out, (int) (0.4 * sr), 15);
    const double binHz = sr / 32768.0;
    const double f0 = 440.0 * std::pow (2.0, (midi - 69) / 12.0);
    const double fund = db[(size_t) std::lround (f0 / binHz)];
    double worst = -300.0;
    for (size_t i = (size_t) (100.0 / binHz); i < (size_t) (10000.0 / binHz); ++i)
    {
        const double f = i * binHz;
        const double h = f / f0;
        if (std::abs (h - std::round (h)) * f0 < 6.0 * binHz) continue;   // harmonic (window main lobe)
        worst = std::max (worst, db[i]);
    }
    return fund - worst;
}

static void testAliasing()
{
    std::cout << "Aliasing (saw, B6 = 1976 Hz, 48 kHz, strongest alias below 10 kHz vs fundamental)" << std::endl;
    double res[3];
    for (int os = 0; os < 3; ++os)
    {
        auto p = makeProc();
        quietOscPatch (*p, 3.0f);
        setParam (*p, "oversampling", (float) os);
        res[os] = aliasDb (*p, 95);
        std::cout << "    " << (1 << os) << "x: " << String (res[os], 1) << " dB" << std::endl;
    }
    check (res[0] > 70.0, "1x: aliases " + String (res[0], 1) + " dB below the fundamental (> 70)");
    check (res[1] > 85.0, "2x: aliases " + String (res[1], 1) + " dB below the fundamental (> 85)");
    check (res[2] > 85.0, "4x: aliases " + String (res[2], 1) + " dB below the fundamental (> 85)");

    // brightness of the bare oscillator: 5th harmonic of B6 (9.9 kHz) against an ideal saw (1/5 = -14.0 dB)
    for (double fs : { 48000.0, 96000.0 })
    {
        ::dsp::ShapeOsc o;
        o.setShape (3.0f, 0.5f);
        const double f0 = 440.0 * std::pow (2.0, 26.0 / 12.0);
        const int n = (int) fs;
        AudioBuffer<float> b (1, n);
        for (int i = 0; i < n; ++i) { bool w; double x; b.setSample (0, i, o.tick (f0 / fs, w, x)); }
        const auto db = spectrum (b, n / 4, 15);
        const double binHz = fs / 32768.0;
        auto peakAt = [&] (double f) { double m = -300; for (int d = -4; d <= 4; ++d) m = std::max (m, db[(size_t) (std::lround (f / binHz) + d)]); return m; };
        const double rel = peakAt (5.0 * f0) - peakAt (f0) + 13.98;
        check (rel > (fs < 50000.0 ? -3.0 : -1.0), String (fs / 1000.0, 0) + " kHz oscillator: 5th harmonic at 9.9 kHz " + String (rel, 2) + " dB against an ideal saw");
    }

    {
        auto c5 = makeProc();
        quietOscPatch (*c5, 3.0f);
        const double typical = aliasDb (*c5, 72);
        check (typical > 90.0, "2x saw C5 (523 Hz, typical lead): aliases " + String (typical, 1) + " dB below (> 90)");
    }

    auto p = makeProc();
    quietOscPatch (*p, 4.0f);
    setParam (*p, "s1_osc_duty", 25.0f);
    const double sq = aliasDb (*p, 95);
    check (sq > 85.0, "2x pulse 25 %: aliases " + String (sq, 1) + " dB below (> 85)");

    auto s = makeProc();
    quietOscPatch (*s, 3.0f);
    setParam (*s, "s1_mix_osc1", 0.0f);
    setParam (*s, "s1_mix_osc2", 4.0f);
    setParam (*s, "s1_osc_sync", 1.0f);
    setParam (*s, "s1_osc_detune", 4.3f);
    const double sy = aliasDb (*s, 83);
    check (sy > 85.0, "2x hard sync (osc 2 +4.3 st, B5): aliases " + String (sy, 1) + " dB below (> 85)");
}

static void testMixerDrive()
{
    std::cout << "Mixer drive (CP-3 style)" << std::endl;
    auto thd = [] (float level)
    {
        auto p = makeProc();
        quietOscPatch (*p, 0.0f);
        setParam (*p, "s1_mix_osc1", level);
        auto out = render (*p, 1.0, note (45, 0.0, 0.99));
        const auto db = spectrum (out, 12000, 15);
        const double binHz = 48000.0 / 32768.0;
        auto at = [&] (double f) { double m = -300; for (int d = -3; d <= 3; ++d) m = std::max (m, db[(size_t) (std::lround (f / binHz) + d)]); return m; };
        return at (330.0) - at (110.0);   // 3rd harmonic of A1 relative to the fundamental
    };
    const double clean = thd (7.0f), driven = thd (10.0f);
    check (clean < -45.0, "level 7 is clean: 3rd harmonic " + String (clean, 1) + " dB");
    check (driven > clean + 15.0, "level 10 saturates: 3rd harmonic " + String (driven, 1) + " dB");
}

static void testLadderTuning()
{
    std::cout << "Ladder self-oscillation tuning" << std::endl;
    for (float fc : { 220.0f, 1000.0f, 3000.0f })
    {
        auto p = makeProc();
        setParam (*p, "s1_mix_osc1", 0.0f);
        setParam (*p, "s1_mix_osc2", 0.0f);
        setParam (*p, "s1_mix_sub", 0.0f);
        setParam (*p, "s1_mix_noise", 0.3f);
        setParam (*p, "s1_lpf_cutoff", fc);
        setParam (*p, "s1_lpf_res", 10.0f);
        setParam (*p, "s1_lpf_eg", 0.0f);
        setParam (*p, "s1_aenv_s", 100.0f);
        auto out = render (*p, 1.0, note (60, 0.0, 0.99));
        const auto db = spectrum (out, 16000, 14);
        size_t best = 10;
        for (size_t i = 10; i < db.size(); ++i) if (db[i] > db[best]) best = i;
        const double binHz = 48000.0 / 16384.0;
        // parabolic interpolation of the peak
        const double a = db[best - 1], b = db[best], c = db[best + 1];
        const double f = (best + 0.5 * (a - c) / (a - 2 * b + c)) * binHz;
        const double cents = 1200.0 * std::log2 (f / fc);
        check (std::abs (cents) < 60.0, "cutoff " + String (fc) + " Hz oscillates at " + String (f, 1) + " Hz (" + String (cents, 0) + " cents)");
    }
}

static void testVoices()
{
    std::cout << "Voices, glide, sustain" << std::endl;
    {
        auto p = makeProc();
        setParam (*p, "s1_osc_glide", 0.2f);
        render (*p, 0.3, { { 0.0, MidiMessage::noteOn (1, 36, 0.8f) } });
        render (*p, 0.02, { { 0.0, MidiMessage::noteOn (1, 48, 0.8f) } });
        const float mid = p->liveNote[0].load();
        render (*p, 0.8, {});
        const float end = p->liveNote[0].load();
        check (mid > 36.5f && mid < 47.5f && std::abs (end - 48.0f) < 0.05f,
               "glide 200 ms: " + String (mid, 2) + " after 20 ms, " + String (end, 2) + " after 0.8 s");
    }
    {
        auto p = makeProc();
        setParam (*p, "voice_mode", 1.0f);
        setParam (*p, "sum_mute2", 0.0f);
        render (*p, 0.2, { { 0.0, MidiMessage::noteOn (1, 60, 0.8f) }, { 0.05, MidiMessage::noteOn (1, 36, 0.8f) } });
        check (std::abs (p->liveNote[0].load() - 36.0f) < 0.01f && std::abs (p->liveNote[1].load() - 60.0f) < 0.01f,
               "duophonic: synth 1 plays the lowest (36), synth 2 the highest (60) held note");
    }
    {
        auto p = makeProc();
        render (*p, 0.2, { { 0.0, MidiMessage::noteOn (1, 40, 0.8f) }, { 0.05, MidiMessage::noteOn (1, 43, 0.8f) },
                           { 0.1, MidiMessage::noteOff (1, 43) } });
        check (std::abs (p->liveNote[0].load() - 40.0f) < 0.01f, "last-note priority: releasing the top note returns to the held one");
    }
    {
        auto p = makeProc();
        auto out = render (*p, 1.5, { { 0.0, MidiMessage::controllerEvent (1, 64, 127) }, { 0.0, MidiMessage::noteOn (1, 36, 0.8f) },
                                      { 0.1, MidiMessage::noteOff (1, 36) }, { 1.0, MidiMessage::controllerEvent (1, 64, 0) } });
        const double held = rmsDb (out, (int) (0.8 * 48000), (int) (0.95 * 48000));
        const double after = rmsDb (out, (int) (1.4 * 48000), (int) (1.5 * 48000));
        check (held > -40.0 && after < -80.0, "sustain pedal holds the note (" + String (held, 1) + " dB) and releases it (" + String (after, 1) + " dB)");
    }
    {
        // Re-Trig restarts the amp envelope without a click: largest sample-to-sample jump stays small
        auto p = makeProc();
        quietOscPatch (*p, 0.0f);
        setParam (*p, "s1_osc_keyreset", 1.0f);
        auto out = render (*p, 0.6, { { 0.0, MidiMessage::noteOn (1, 36, 0.8f) }, { 0.3, MidiMessage::noteOn (1, 38, 0.8f) } });
        float maxJump = 0.0f;
        for (int i = (int) (0.29 * 48000); i < (int) (0.33 * 48000); ++i)
            maxJump = std::max (maxJump, std::abs (out.getSample (0, i) - out.getSample (0, i - 1)));
        check (maxJump < 0.05f, "Re-Trig on a sine: no click at the retrigger (max step " + String (maxJump, 4) + ")");
    }
}

static void testModulation()
{
    std::cout << "Modulation" << std::endl;
    auto frames = [] (const AudioBuffer<float>& b)
    {
        Array<double> f;
        for (int s = 4800; s + 2400 < b.getNumSamples(); s += 2400) f.add (rmsDb (b, s, s + 2400));
        double mn = 1e9, mx = -1e9;
        for (auto v : f) { mn = std::min (mn, v); mx = std::max (mx, v); }
        return mx - mn;
    };
    auto base = makeProc();
    setParam (*base, "s1_mix_sub", 0.0f);
    setParam (*base, "s1_aenv_s", 100.0f);
    setParam (*base, "s1_lpf_eg", 0.0f);
    setParam (*base, "s1_lpf_cutoff", 300.0f);
    const double flat = frames (render (*base, 2.0, note (36, 0.0, 1.99)));

    auto p = makeProc();
    p->editorOpen = true;
    setParam (*p, "s1_mix_sub", 0.0f);
    setParam (*p, "s1_aenv_s", 100.0f);
    setParam (*p, "s1_lpf_eg", 0.0f);
    setParam (*p, "s1_lpf_cutoff", 300.0f);
    setParam (*p, "s1_lfo1_rate", 2.0f);
    const int slot = p->addModulation (Mod::layerSource (0, Mod::Lfo1), "s1_lpf_cutoff", 0.4f);
    check (slot == 0, "modulation slot created");
    const double wobble = frames (render (*p, 2.0, note (36, 0.0, 1.99)));
    check (wobble > flat + 15.0, "LFO 1 -> cutoff moves the level (" + String (flat, 1) + " dB -> " + String (wobble, 1) + " dB swing)");
    check (p->liveDest[Mod::destCode ("s1_lpf_cutoff")].load() >= 0.0f, "live modulated value is published for the UI");

    // every function keeps the output finite
    bool finite = true;
    for (int fn = 0; fn < Mod::functionNames().size(); ++fn)
    {
        setParam (*p, Params::modID (0, "fn"), (float) fn);
        setParam (*p, Params::modID (0, "fnamt"), 70.0f);
        finite = finite && allFinite (render (*p, 0.3, note (40, 0.0, 0.25)));
    }
    check (finite, "all " + String (Mod::functionNames().size()) + " modulation functions stay finite");

    // controller as VCA: mod wheel at 0 mutes the routing
    setParam (*p, Params::modID (0, "fn"), 0.0f);
    setParam (*p, Params::modID (0, "ctl"), (float) Mod::ModWheel);
    const double gated = frames (render (*p, 2.0, note (36, 0.0, 1.99)));
    check (gated < flat + 2.0, "controller (mod wheel at 0) scales the modulation to zero (" + String (gated, 1) + " dB swing)");
    p->removeModulation (0);
    check ((int) p->getParamReal (Params::modID (0, "src")) == 0, "modulation removed");
}

static void testRobustness()
{
    std::cout << "Robustness" << std::endl;
    bool ok = true;
    double worst = -200.0;
    for (double sr : { 44100.0, 96000.0, 192000.0 })
        for (int block : { 1, 7, 32, 33, 480, 4096 })
        {
            auto p = makeProc (sr, block);
            p->loadFactoryPreset (2);
            setParam (*p, "sum_mute2", 0.0f);
            auto out = render (*p, 0.5, { { 0.0, MidiMessage::noteOn (1, 36, 1.0f) }, { 0.1, MidiMessage::noteOn (1, 48, 0.5f) },
                                          { 0.2, MidiMessage::pitchWheel (1, 16000) }, { 0.3, MidiMessage::allNotesOff (1) } }, block);
            ok = ok && allFinite (out);
            worst = std::max (worst, peakDb (out));
        }
    check (ok && worst < 12.0, "44.1-192 kHz, blocks 1..4096: finite output (peak " + String (worst, 1) + " dBFS)");

    auto p = makeProc();
    AudioBuffer<float> empty (2, 0);
    MidiBuffer m;
    p->processBlock (empty, m);
    check (true, "zero-length block accepted");

    // extreme settings
    for (auto id : { "s1_lpf_res", "s1_hpf_res", "s1_subf_res", "s1_mix_osc1", "s1_mix_osc2", "s1_mix_noise", "s1_mix_sub", "fx1_sat", "dly_fb" })
        setParam (*p, id, 10.0f);
    setParam (*p, "fx1_sattype", 3.0f);
    setParam (*p, "dly_mix", 100.0f);
    setParam (*p, "oversampling", 2.0f);
    auto out = render (*p, 2.0, note (24, 0.0, 1.0, 1.0f));
    check (allFinite (out) && peakDb (out) < 12.0, "everything at maximum stays finite and bounded (" + String (peakDb (out), 1) + " dBFS)");
}

static void testState()
{
    std::cout << "State" << std::endl;
    auto a = makeProc();
    a->loadFactoryPreset (5);
    setParam (*a, "s2_lpf_cutoff", 777.0f);
    a->addModulation (Mod::layerSource (1, Mod::Random2), "sum_pan2", -0.4f);
    MemoryBlock mb;
    a->getStateInformation (mb);
    auto b = makeProc();
    b->setStateInformation (mb.getData(), (int) mb.getSize());
    int diff = 0;
    for (int i = 0; i < a->getParameters().size(); ++i)
        if (std::abs (a->getParameters()[i]->getValue() - b->getParameters()[i]->getValue()) > 1e-6f) ++diff;
    check (diff == 0 && b->getPresetName() == a->getPresetName(), "save / restore: all parameters and the preset name identical (" + String (diff) + " differ)");
}

static void testSilentChange()
{
    std::cout << "Silent preset change" << std::endl;
    auto p = makeProc();
    p->loadFactoryPreset (9);   // dub stab with long delay
    auto a = render (*p, 1.0, { { 0.0, MidiMessage::noteOn (1, 48, 1.0f) } });
    p->changeSilently ([&p] { p->loadFactoryPreset (3); });
    auto b = render (*p, 0.01, {});          // fade out happens here
    pumpMessages (40);                       // preset applied on the message thread
    auto c = render (*p, 0.3, {});
    float maxStep = 0.0f;
    for (int i = 1; i < b.getNumSamples(); ++i) maxStep = std::max (maxStep, std::abs (b.getSample (0, i) - b.getSample (0, i - 1)));
    check (p->getPresetName() == "Sub Foundation", "preset applied after the fade");
    check (rmsDb (c, 0, c.getNumSamples()) < -100.0, "old notes and delay tails are cleared (" + String (rmsDb (c, 0, c.getNumSamples()), 1) + " dB)");
    check (maxStep < 0.05f, "fade out without a click (max step " + String (maxStep, 4) + ")");
}

static void testPresets()
{
    std::cout << "Factory presets" << std::endl;
    const auto& all = factoryPresets();
    bool idsOk = true;
    for (auto& pr : all)
    {
        ResoOGProcessor p;
        for (auto& v : pr.values) if (p.param (v.first) == nullptr) { idsOk = false; std::cout << "    unknown id " << v.first << " in " << pr.name << std::endl; }
        for (auto& m : pr.mods) if (Mod::destCode (m.dest) == 0) { idsOk = false; std::cout << "    unknown destination " << m.dest << std::endl; }
    }
    check (idsOk, String ((int) all.size()) + " presets reference valid parameters");

    for (int i = 0; i < (int) all.size(); ++i)
    {
        auto p = makeProc();
        p->loadFactoryPreset (i);
        auto out = render (*p, 2.5, { { 0.0, MidiMessage::noteOn (1, 36, 0.9f) }, { 0.4, MidiMessage::noteOn (1, 43, 0.6f) },
                                      { 0.6, MidiMessage::noteOff (1, 36) }, { 0.8, MidiMessage::noteOff (1, 43) },
                                      { 1.0, MidiMessage::noteOn (1, 48, 1.0f) }, { 1.6, MidiMessage::noteOff (1, 48) } });
        const double pk = peakDb (out);
        check (allFinite (out) && pk > -36.0 && pk < 3.0, String (all[(size_t) i].category) + " / " + all[(size_t) i].name + ": peak " + String (pk, 1) + " dBFS");
    }
}

static double cpuPercent (int os, bool both, double seconds = 8.0)
{
    auto p = makeProc (48000.0, 256);
    p->loadFactoryPreset (2);
    setParam (*p, "oversampling", (float) os);
    if (both) { setParam (*p, "sum_mute2", 0.0f); setParam (*p, "cho_mix", 40.0f); setParam (*p, "comp_on", 1.0f); }
    double best = 1e9;
    for (int run = 0; run < 3; ++run)
    {
        std::vector<Ev> ev;
        for (double t = 0.0; t < seconds; t += 0.25) { ev.push_back ({ t, MidiMessage::noteOn (1, 36 + ((int) (t * 4) % 12), 0.8f) }); ev.push_back ({ t + 0.2, MidiMessage::noteOff (1, 36 + ((int) (t * 4) % 12)) }); }
        const double t0 = Time::getMillisecondCounterHiRes();
        render (*p, seconds, ev, 256);
        best = std::min (best, (Time::getMillisecondCounterHiRes() - t0) / (seconds * 1000.0) * 100.0);
    }
    return best;
}

static void testCpu()
{
    std::cout << "CPU (48 kHz, block 256, best of 3)" << std::endl;
    const double one = cpuPercent (1, false), two = cpuPercent (1, true), four = cpuPercent (2, true);
    check (one < timing (3.0), "1 layer, 2x oversampling: " + String (one, 2) + " % of one core");
    check (two < timing (5.0), "2 layers + chorus + compressor, 2x: " + String (two, 2) + " % of one core");
    check (four < timing (9.0), "2 layers, 4x oversampling: " + String (four, 2) + " % of one core");

    auto p = makeProc (48000.0, 256);
    render (*p, 0.5, note (36, 0.0, 0.1));
    const double t0 = Time::getMillisecondCounterHiRes();
    render (*p, 10.0, {}, 256);
    const double idle = (Time::getMillisecondCounterHiRes() - t0) / 100.0;
    check (idle < timing (1.0), "idle after release (denormal check): " + String (idle, 3) + " % of one core");
}

//==============================================================================
static MouseEvent mouse (Component& c, Point<float> pos, Point<float> down, bool dragged, ModifierKeys mods = ModifierKeys (ModifierKeys::leftButtonModifier))
{
    return MouseEvent (Desktop::getInstance().getMainMouseSource(), pos, mods,
                       MouseInputSource::defaultPressure, MouseInputSource::defaultOrientation, MouseInputSource::defaultRotation,
                       MouseInputSource::defaultTiltX, MouseInputSource::defaultTiltY, &c, &c,
                       Time::getCurrentTime(), down, Time::getCurrentTime(), 1, dragged);
}

static void saveShot (Component& c, const String& name, float scale = 1.0f)
{
    if (snapshotDir == File()) return;
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, scale);
    PNGImageFormat png;
    auto f = snapshotDir.getChildFile (name + ".png");
    f.deleteFile();
    FileOutputStream os (f);
    png.writeImageToStream (img, os);
}

static void testEditor()
{
    std::cout << "Editor" << std::endl;
    auto p = makeProc();
    p->loadFactoryPreset (2);
    render (*p, 0.3, { { 0.0, MidiMessage::noteOn (1, 39, 0.9f) } });
    std::unique_ptr<AudioProcessorEditor> ed (p->createEditor());
    auto* editor = dynamic_cast<ResoOGEditor*> (ed.get());
    check (editor != nullptr && ed->getWidth() > 0, "editor created (" + String (ed->getWidth()) + " x " + String (ed->getHeight()) + ")");
    if (editor == nullptr) return;
    ed->setSize (ResoOGEditor::logicalWidth, ResoOGEditor::logicalHeight);

    const char* names[] = { "synth1", "cntrl1", "synth2-muted", "cntrl2", "output" };
    double worstMs = 0.0;
    for (int i = 0; i < 5; ++i)
    {
        editor->showPage (i);
        render (*p, 0.1, {});
        pumpMessages (60);
        const double t0 = Time::getMillisecondCounterHiRes();
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        worstMs = std::max (worstMs, Time::getMillisecondCounterHiRes() - t0);
        saveShot (*ed, names[i], 2.0f);
    }
    check (worstMs < timing (120.0), "full redraw at 2x scale: " + String (worstMs, 1) + " ms (worst page)");

    // knob drag changes the parameter
    editor->showPage (0);
    auto* page = editor->getPageComponent (0);
    auto* k = page->findKnob ("s1_lpf_cutoff");
    check (k != nullptr, "cutoff knob found");
    if (k != nullptr)
    {
        const float before = p->getParamReal ("s1_lpf_cutoff");
        const Point<float> c ((float) k->getWidth() * 0.5f, 45.0f);
        k->mouseDown (mouse (*k, c, c, false));
        k->mouseDrag (mouse (*k, c.translated (0.0f, -40.0f), c, true));
        k->mouseUp (mouse (*k, c.translated (0.0f, -40.0f), c, true));
        const float after = p->getParamReal ("s1_lpf_cutoff");
        check (after > before * 1.5f, "dragging the cutoff knob up raises it (" + String (before, 0) + " -> " + String (after, 0) + " Hz)");

        // dropping a modulation source onto a knob creates a routing
        DragAndDropTarget::SourceDetails sd (var ("src:" + String (Mod::layerSource (0, Mod::Lfo2))), nullptr, {});
        const int before2 = p->slotsForDestination ("s1_lpf_res").size();
        if (auto* kr = page->findKnob ("s1_lpf_res"))
        {
            check (kr->isInterestedInDragSource (sd), "resonance knob accepts a dragged modulation source");
            kr->itemDropped (sd);
        }
        check (p->slotsForDestination ("s1_lpf_res").size() == before2 + 1, "drop creates a modulation routing");

        // double-click and type a value
        k->mouseDoubleClick (mouse (*k, c, c, false));
        if (auto* te = dynamic_cast<TextEditor*> (k->getChildComponent (0)))
        {
            te->setText ("A3");
            te->onReturnKey();
            pumpMessages (30);
        }
        check (std::abs (p->getParamReal ("s1_lpf_cutoff") - 220.0f) < 0.5f, "typing \"A3\" sets the cutoff to 220 Hz");
    }

    editor->setModDrawer (true);
    pumpMessages (60);
    saveShot (*ed, "mod-drawer", 2.0f);
    check (editor->isModDrawerOpen(), "modulation panel opens");
    editor->setModDrawer (false);

    editor->setScalePercent (70);
    check (std::abs (ed->getWidth() - roundToInt (ResoOGEditor::logicalWidth * 0.7f)) <= 1, "window size 70 % (" + String (ed->getWidth()) + " px)");
    ed.reset();
}

static void testHostedPlugin (const String& path)
{
    std::cout << "Hosted VST3: " << path << std::endl;
    AudioPluginFormatManager fm;
    fm.addFormat (new VST3PluginFormat());
    OwnedArray<PluginDescription> types;
    KnownPluginList list;
    VST3PluginFormat vst3;
    list.scanAndAddFile (path, true, types, vst3);
    check (types.size() == 1 && types[0]->isInstrument, "VST3 scanned as an instrument");
    if (types.isEmpty()) return;
    String error;
    auto instance = fm.createPluginInstance (*types[0], 48000.0, 512, error);
    check (instance != nullptr, "VST3 instantiated " + error);
    if (instance == nullptr) return;
    instance->prepareToPlay (48000.0, 512);
    AudioBuffer<float> buf (2, 512);
    float pk = 0.0f;
    for (int b = 0; b < 40; ++b)
    {
        buf.clear();
        MidiBuffer midi;
        if (b == 0) midi.addEvent (MidiMessage::noteOn (1, 36, 0.9f), 0);
        instance->processBlock (buf, midi);
        pk = std::max (pk, buf.getMagnitude (0, 512));
    }
    check (pk > 0.01f, "VST3 plays a note (peak " + String (Decibels::gainToDecibels (pk), 1) + " dBFS)");
    check (instance->getParameters().size() >= 400, "VST3 exposes " + String (instance->getParameters().size()) + " parameters");
    instance->releaseResources();
}

//==============================================================================
// Screenshots for the manual: ResoOGTests --manual-shots docs/manual/img
static int findPreset (const String& name)
{
    for (int i = 0; i < (int) factoryPresets().size(); ++i) if (name == factoryPresets()[(size_t) i].name) return i;
    return 0;
}

static void manualShots (const File& dir)
{
    dir.createDirectory();
    auto write = [&dir] (const Image& img, const String& name)
    {
        JPEGImageFormat jpg;
        jpg.setQuality (0.88f);
        auto f = dir.getChildFile (name + ".jpg");
        f.deleteFile();
        FileOutputStream os (f);
        jpg.writeImageToStream (img, os);
        std::cout << "  " << f.getFileName() << "  " << img.getWidth() << " x " << img.getHeight() << std::endl;
    };
    struct Shot { String preset; int page; bool drawer; int note; };
    auto take = [&] (const Shot& s, std::function<void (ResoOGEditor&, const Image&)> crops)
    {
        auto p = makeProc();
        p->loadFactoryPreset (findPreset (s.preset));
        render (*p, 0.6, { { 0.0, MidiMessage::noteOn (1, s.note, 0.9f) } });
        std::unique_ptr<AudioProcessorEditor> ed (p->createEditor());
        auto* e = dynamic_cast<ResoOGEditor*> (ed.get());
        ed->setSize (ResoOGEditor::logicalWidth, ResoOGEditor::logicalHeight);
        e->showPage (s.page);
        if (s.drawer) { e->setModDrawer (true, 0); }
        for (int i = 0; i < 8; ++i) { render (*p, 0.05, {}); pumpMessages (35); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        crops (*e, img);
    };
    auto crop = [] (const Image& img, Rectangle<int> logical) { return img.getClippedImage (logical * 2); };

    take ({ "Seventies Fat Bass", 0, false, 36 }, [&] (ResoOGEditor& e, const Image& img)
    {
        write (img, "overview");
        write (crop (img, { 0, 0, 1200, 36 }), "topbar");
        write (crop (img, { 0, 798, 1200, 28 }), "bottombar");
        write (crop (img, { 0, 650, 1200, 148 }), "keyboard");
        write (crop (img, { 186, 48, 352, 548 }), "oscillators");
        write (crop (img, { 536, 48, 132, 548 }), "mixer");
        write (crop (img, { 666, 48, 508, 366 }), "filters");
        write (crop (img, { 26, 48, 162, 548 }), "noise-voicing");
        write (crop (img, { 666, 412, 508, 184 }), "subfilter");
        juce::ignoreUnused (e);
    });
    take ({ "Deep Blue S+H", 0, false, 39 }, [&] (ResoOGEditor& e, const Image& img)
    {
        if (auto* k = e.getPageComponent (0)->findKnob ("s1_lpf_cutoff"))
            write (crop (img, e.getLocalArea (k, k->getLocalBounds()).expanded (14, 6)), "knob-mod");
    });
    take ({ "Deep Blue S+H", 1, false, 39 }, [&] (ResoOGEditor&, const Image& img)
    {
        write (img, "cntrl");
        write (crop (img, { 26, 48, 384, 184 }), "lfo-panel");
    });
    take ({ "Stereo Matriarch Pad", 2, false, 43 }, [&] (ResoOGEditor&, const Image& img) { write (img, "synth2"); });
    take ({ "Init", 2, false, 43 }, [&] (ResoOGEditor&, const Image& img) { write (crop (img, { 26, 590, 1148, 60 }), "muted-badge"); });
    take ({ "Warehouse Reese", 4, false, 31 }, [&] (ResoOGEditor&, const Image& img)
    {
        write (img, "output");
        write (crop (img, { 316, 84, 568, 146 }), "meters");
    });
    take ({ "Deep Blue S+H", 0, true, 39 }, [&] (ResoOGEditor&, const Image& img)
    {
        write (img, "mod-panel");
    });
}

int main (int argc, char* argv[])
{
    ScopedJuceInitialiser_GUI gui;
    auto root = File::getSpecialLocation (File::currentExecutableFile);
    while (root.exists() && ! root.getChildFile ("CMakeLists.txt").existsAsFile()) root = root.getParentDirectory();
    if (root.exists())
    {
        snapshotDir = root.getChildFile ("snapshots");
        snapshotDir.createDirectory();
    }

    if (argc > 1 && String (argv[1]) == "--debugmod")
    {
        auto p = makeProc();
        p->editorOpen = true;
        setParam (*p, "s1_aenv_s", 100.0f);
        setParam (*p, "s1_lpf_eg", 0.0f);
        setParam (*p, "s1_lpf_cutoff", 300.0f);
        setParam (*p, "s1_lfo1_rate", 2.0f);
        p->addModulation (Mod::layerSource (0, Mod::Lfo1), "s1_lpf_cutoff", 0.4f);
        render (*p, 0.01, { { 0.0, MidiMessage::noteOn (1, 36, 0.8f) } });
        for (int i = 0; i < 12; ++i)
        {
            auto b = render (*p, 0.05, {});
            std::printf ("t %.2f lfo %.3f live %.3f rms %.1f\n", i * 0.05, p->liveSource[Mod::layerSource (0, Mod::Lfo1)].load(),
                         p->liveDest[Mod::destCode ("s1_lpf_cutoff")].load(), rmsDb (b, 0, b.getNumSamples()));
        }
        return 0;
    }

    if (argc > 2 && String (argv[1]) == "--manual-shots")
    {
        manualShots (File::getCurrentWorkingDirectory().getChildFile (String (CharPointer_UTF8 (argv[2]))));
        return 0;
    }

    if (argc > 1 && String (argv[1]) == "--audit")
    {
        // Changes every parameter on its own and reports the ones that leave the sound unchanged
        // parameters that only act when another switch is on
        auto enablers = [] (const String& id) -> std::vector<std::pair<String, float>>
        {
            const String L = id.substring (0, 3);
            if (id.endsWith ("xover")) return { { L + "xover_on", 1.0f } };
            if (id.contains ("lfo") && id.endsWith ("_div")) return { { id.replace ("_div", "_sync"), 1.0f } };
            if (id.contains ("rnd") && id.endsWith ("_div")) return { { id.replace ("_div", "_sync"), 1.0f } };
            if (id.contains ("lfo") && id.endsWith ("_phase")) return { { id.replace ("_phase", "_kbreset"), 1.0f } };
            if (id == "fx1_sat") return { { "fx1_sattype", 1.0f } };
            if (id == "fx2_sat") return { { "fx2_sattype", 1.0f } };
            if (id.startsWith ("dly_")) return { { "dly_mix", 40.0f }, { "dly_sync", id == "dly_time" ? 0.0f : 1.0f } };
            if (id.startsWith ("cho_")) return { { "cho_mix", 60.0f }, { "cho_sync", id == "cho_div" ? 1.0f : 0.0f } };
            if (id == "comp_on") return { { "comp_thresh", -25.0f } };
            if (id.startsWith ("comp_")) return { { "comp_on", 1.0f }, { "comp_thresh", -25.0f } };
            if (id == "pb_range") return { { "@bend", 1.0f } };
            if (id == "glide_legato") return { { "s1_osc_glide", 0.3f }, { "@staccato", 1.0f } };
            return {};
        };
        auto renderWith = [&enablers] (const String& id, int mode, const String& enablerFor)
        {
            auto p = makeProc();
            bool bend = false, staccato = false;
            for (auto& e : enablers (enablerFor))
            {
                if (e.first == "@bend") bend = true;
                else if (e.first == "@staccato") staccato = true;
                else if (e.first != id) setParam (*p, e.first, e.second);
            }
            setParam (*p, "sum_mute2", 0.0f);
            setParam (*p, "s1_mix_noise", 3.0f);
            setParam (*p, "s2_mix_noise", 3.0f);
            for (auto l : { "s1_", "s2_" })
            {
                setParam (*p, String (l) + "lfo1_rate", 3.0f);
                setParam (*p, String (l) + "menv_s", 50.0f);
            }
            for (int s = 0; s < 2; ++s)   // make the modulators audible
            {
                const String L = s == 0 ? "s1_" : "s2_";
                p->addModulation (Mod::layerSource (s, Mod::Lfo1), L + "osc_duty", 0.2f);
                p->addModulation (Mod::layerSource (s, Mod::Lfo2), L + "osc_freq", 0.05f);
                p->addModulation (Mod::layerSource (s, Mod::Lfo3), L + "spread", 0.3f);
                p->addModulation (Mod::layerSource (s, Mod::ModEnv), L + "osc_wave", 0.2f);
                p->addModulation (Mod::layerSource (s, Mod::Random1), L + "noise_color", 0.4f);
                p->addModulation (Mod::layerSource (s, Mod::Random2), L + "sub_phase", 0.3f);
            }
            if (id.isNotEmpty())
            {
                auto* rp = p->param (id);
                const float v = rp->getValue();
                float nv;
                if (auto* c = dynamic_cast<AudioParameterChoice*> (rp)) nv = rp->convertTo0to1 ((float) ((c->getIndex() + (mode + 1)) % c->choices.size()));
                else if (dynamic_cast<AudioParameterBool*> (rp)) nv = v > 0.5f ? 0.0f : 1.0f;
                else nv = mode == 0 ? (v < 0.5f ? 0.85f : 0.15f) : (v < 0.5f ? 0.6f : 0.35f);
                rp->setValueNotifyingHost (nv);
            }
            std::vector<Ev> ev { { 0.0, MidiMessage::noteOn (1, 36, 1.0f) }, { 0.5, MidiMessage::noteOn (1, 43, 0.6f) },
                                 { 0.9, MidiMessage::noteOff (1, 43) }, { 1.1, MidiMessage::noteOff (1, 36) } };
            if (staccato) ev = { { 0.0, MidiMessage::noteOn (1, 36, 1.0f) }, { 0.4, MidiMessage::noteOff (1, 36) },
                                 { 0.5, MidiMessage::noteOn (1, 43, 0.6f) }, { 1.1, MidiMessage::noteOff (1, 43) } };
            if (bend) ev.push_back ({ 0.2, MidiMessage::pitchWheel (1, 14000) });
            return render (*p, 1.6, ev);
        };
        auto diffDb = [&] (const AudioBuffer<float>& base, const AudioBuffer<float>& b)
        {
            const double ref = rmsDb (base, 0, base.getNumSamples());
            AudioBuffer<float> d (base);
            for (int c = 0; c < 2; ++c) d.addFrom (c, 0, b, c, 0, b.getNumSamples(), -1.0f);
            return rmsDb (d, 0, d.getNumSamples()) - ref;
        };
        auto ids = StringArray();
        for (int l = 0; l < 2; ++l) for (auto& d : Params::layerDefs()) ids.add (Params::layerID (l, d.id));
        for (auto& d : Params::globalDefs()) ids.add (d.id);
        int effective = 0;
        for (auto& id : ids)
        {
            const auto base = renderWith ({}, 0, id);
            const double d = std::max (diffDb (base, renderWith (id, 0, id)), diffDb (base, renderWith (id, 1, id)));
            if (d < -60.0) std::printf ("NO EFFECT  %-22s %.1f dB\n", id.toRawUTF8(), d);
            else ++effective;
        }
        std::printf ("%d of %d parameters change the sound\n", effective, ids.size());
        return 0;
    }

    if (argc > 1 && String (argv[1]) == "--bench")
    {
        for (int os = 0; os < 3; ++os)
            std::printf ("%dx: 1 layer %.2f %%, 2 layers + fx %.2f %%\n", 1 << os, cpuPercent (os, false), cpuPercent (os, true));
        return 0;
    }

    testParameters();
    testDefaultPatch();
    testAliasing();
    testMixerDrive();
    testLadderTuning();
    testVoices();
    testModulation();
    testRobustness();
    testState();
    testSilentChange();
    testPresets();
    testCpu();
    testEditor();
    if (argc > 1)
        testHostedPlugin (String (CharPointer_UTF8 (argv[1])));

    std::cout << std::endl << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " TEST(S) FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
