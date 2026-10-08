#include "Pages.h"
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
Knob* Page::knob (const String& id, const String& label, Colour c, Knob::Style s)
{
    auto* k = new Knob (proc, id, label, c, s);
    owned.add (k);
    knobs.add (k);
    return k;
}

LedButton* Page::button (const String& id, const String& label) { return static_cast<LedButton*> (owned.add (new LedButton (proc, id, label))); }
LedRadio* Page::radio (const String& id, const String& label, StringArray names) { return static_cast<LedRadio*> (owned.add (new LedRadio (proc, id, label, names))); }
Stack* Page::stack (std::vector<Component*> kids, int gap, bool left) { return static_cast<Stack*> (owned.add (new Stack (kids, gap, left))); }

ModulePanel* Page::panel (const String& title, Colour c, Rectangle<int> b, int source, const String& right)
{
    auto* p = static_cast<ModulePanel*> (owned.add (new ModulePanel (title, c, source, right)));
    p->setBounds (b);
    addAndMakeVisible (p);
    if (source > 0)
    {
        const int s = (source - Mod::FirstLayerSource) % Mod::perLayerSources;
        if (s >= Mod::Lfo1 && s <= Mod::Random2) livePanels.add ({ p, source });
    }
    return p;
}

Component* Page::rateKnob (const String& rateID, const String& divID, const String& syncID, const String& label, Colour c, String lo, String hi)
{
    Knob::Style s;
    s.lo = lo; s.hi = hi;
    auto* rate = knob (rateID, label, c, s);
    Knob::Style ds;
    ds.lo = "8 bar"; ds.hi = "1/64";
    auto* div = knob (divID, label, c, ds);
    auto* holder = owned.add (new Component());
    holder->setSize (rate->getWidth(), rate->getHeight());
    holder->addAndMakeVisible (rate);
    holder->addChildComponent (div);
    rate->setTopLeftPosition (0, 0);
    div->setTopLeftPosition (0, 0);
    ratePairs.add ({ rate, div, syncID });
    return holder;
}

void Page::refreshLive()
{
    for (auto* k : knobs)
        if (k->isVisible()) k->refreshLive();
    for (auto& lp : livePanels)
    {
        float v = proc.liveSource[lp.source].load();
        lp.panel->setLiveValue (0.5f + 0.5f * v);
    }
    for (auto& rp : ratePairs)
    {
        const bool synced = proc.getParamReal (rp.syncID) > 0.5f;
        if (rp.div->isVisible() != synced)
        {
            rp.div->setVisible (synced);
            rp.rate->setVisible (! synced);
        }
    }
}

void Page::setModEditCallback (std::function<void (int)> cb)
{
    for (auto* k : knobs) k->onEditModulation = cb;
}

Knob* Page::findKnob (const String& id) const
{
    for (auto* k : knobs) if (k->getParamID() == id) return k;
    return nullptr;
}

//==============================================================================
namespace
{
Knob::Style range (String lo, String hi, bool bipolar = false)
{
    Knob::Style s;
    s.lo = lo; s.hi = hi; s.bipolar = bipolar;
    return s;
}
Knob::Style marks (StringArray m, bool bipolar = false, int width = 0)
{
    Knob::Style s;
    s.marks = m; s.bipolar = bipolar; s.width = width;
    return s;
}
}

SynthPage::SynthPage (ResoOGProcessor& p, int l) : Page (p), layer (l)
{
    auto L = [this] (const char* s) { return Params::layerID (layer, s); };
    const int c0 = 0, c1 = 160, c2 = 510, c3 = 640, c4 = 976;
    const int r0 = 0, r1 = 182, r2 = 364, rowH = 172;
    const int w3 = c4 - c3 - 10;

    auto* noise = panel ("NOISE", Palette::noise, { c0, r0, 150, rowH });
    noise->setGrid ({ 1.0f }, { knob (L ("noise_color"), "NOISE COLOR", Palette::noise, marks ({ "RED", "PINK", "WHITE", "BLUE", "VIOLET" }, true, 136)) });

    auto* osc = panel ("DUAL OSCILLATOR", Palette::osc, { c1, r0, 340, rowH * 2 + 10 });
    osc->setGrid ({ 1, 1, 1 }, {
        knob (L ("osc_freq"), "FREQUENCY", Palette::osc, range ("-7", "+7", true)),
        knob (L ("osc_detune"), "OSC 2 DETUNE", Palette::osc, range ("-7", "+7", true)),
        knob (L ("osc_glide"), "GLIDE", Palette::osc, range ("0", "10s")),
        radio (L ("osc2_oct"), "OSC 2 OCTAVE", { "+0", "+1", "+2" }),
        button (L ("osc_sync"), "HARD SYNC"),
        button (L ("osc_keyreset"), "KEY RESET"),
        knob (L ("osc2_phase"), "OSC 2 PHASE", Palette::osc, range (String (CharPointer_UTF8 ("0\xc2\xb0")), String (CharPointer_UTF8 ("360\xc2\xb0")))),
        knob (L ("osc_wave"), "WAVESHAPE", Palette::osc, marks ({ "~sine", "~tri", "~shark", "~saw", "~square" })),
        knob (L ("osc_duty"), "DUTY CYCLE", Palette::osc, range ("0", "100")) });

    auto* mixer = panel ("MIXER", Palette::mixer, { c2, r0, 120, Page::height });
    Knob::Style ms = range ("0", "10");
    ms.small = true;
    ms.drive = true;
    drive = static_cast<DriveMeter*> (owned.add (new DriveMeter()));
    drive->setSize (96, 24);
    drive->setTooltip ("How hard the mixer saturates. Levels above 7 drive the CP-3 style mixer.");
    mixer->setGrid ({ 1.0f }, {
        knob (L ("mix_osc1"), "OSC 1", Palette::osc, ms),
        knob (L ("mix_osc2"), "OSC 2", Palette::osc, ms),
        knob (L ("mix_noise"), "NOISE", Palette::noise, ms),
        knob (L ("mix_sub"), "SUB", Palette::sub, ms),
        drive });

    auto* lpf = panel ("LOW PASS FILTER", Palette::filter, { c3, r0, w3, rowH }, -1, "24 dB");
    lpf->setGrid ({ 1, 1, 1 }, {
        knob (L ("lpf_cutoff"), "CUTOFF", Palette::filter, range ("16", "20k")),
        knob (L ("lpf_res"), "RESONANCE", Palette::filter, range ("0", "10")),
        knob (L ("lpf_eg"), "EG AMOUNT", Palette::filter, range ("-10", "+10", true)) });

    auto* hpf = panel ("HIGH PASS FILTER", Palette::filter, { c3, r1, w3, rowH }, -1, "12 dB");
    hpf->setGrid ({ 1, 1, 1 }, {
        knob (L ("hpf_cutoff"), "CUTOFF", Palette::filter, range ("16", "20k")),
        knob (L ("hpf_res"), "RESONANCE", Palette::filter, range ("0", "10")),
        knob (L ("hpf_eg"), "EG AMOUNT", Palette::filter, range ("-10", "+10", true)) });

    filters = panel ("FILTERS", Palette::filter, { c4, r0, Page::width - c4, rowH * 2 + 10 });
    filters->setGrid ({ 1.0f }, {
        radio (L ("flt_order"), "ORDER", { "SERIAL", "PARALLEL", "HPF NOISE" }),
        knob (L ("xover"), "OSC CROSSOVER", Palette::filter, range ("SUB", "VCOS", true)),
        button (L ("xover_on"), "ON / OFF") });

    auto* voicing = panel ("VOICING", Palette::voicing, { c0, r1, 150, rowH * 2 + 10 });
    voicing->setGrid ({ 1.0f }, {
        radio (L ("legato"), "LEGATO MODE", { "RE-TRIG", "LEGATO", "ADD" }),
        knob (L ("spread"), "DUAL OSC SPREAD", Palette::voicing, range ("0", "10")),
        button (L ("accent"), "ACCENT") });

    auto* sub = panel ("SUB OSC", Palette::sub, { c1, r2, 340, rowH });
    sub->setGrid ({ 1, 1, 1.15f }, {
        knob (L ("sub_phase"), "PHASE", Palette::sub, range (String (CharPointer_UTF8 ("0\xc2\xb0")), String (CharPointer_UTF8 ("360\xc2\xb0")))),
        knob (L ("sub_wave"), "WAVESHAPE", Palette::sub, marks ({ "~sine", "~saw", "~square" })),
        radio (L ("sub_offset"), "FREQ OFFSET", { "-1 OCTAVE", "-1 LINKED", "-2 OCTAVE" }) });

    auto* subf = panel ("SUB FILTER", Palette::sub, { c3, r2, Page::width - c3, rowH }, -1, "mono");
    subf->setGrid ({ 1, 1, 1, 1.2f }, {
        knob (L ("subf_cutoff"), "CUTOFF", Palette::sub, range ("16", "8k")),
        knob (L ("subf_res"), "RESONANCE", Palette::sub, range ("0", "10")),
        knob (L ("subf_eg"), "EG AMOUNT", Palette::sub, range ("-10", "+10", true)),
        radio (L ("subf_mode"), "FILTER MODE", { "HIGH", "BAND", "LOW" }) });
}

void SynthPage::refreshLive()
{
    Page::refreshLive();
    if (drive) drive->setLevel (proc.liveDrive[layer].load());
}

//==============================================================================
CntrlPage::CntrlPage (ResoOGProcessor& p, int l) : Page (p), layer (l)
{
    auto L = [this] (const String& s) { return Params::layerID (layer, s); };
    const int c0 = 0, c1 = 382, c2 = 836, rowH = 172;
    const int w1 = c2 - c1 - 10;

    for (int i = 0; i < 3; ++i)
    {
        const String n (i + 1), pre = "lfo" + n + "_";
        const int src = Mod::layerSource (layer, (Mod::LayerSource) (Mod::Lfo1 + i));
        const auto col = Mod::sourceColour (src);
        auto* pn = panel ("LFO " + n, col, { c0, i * (rowH + 10), 372, rowH }, src);
        pn->setGrid ({ 1.0f, 0.75f, 1.0f, 1.0f }, {
            rateKnob (L (pre + "rate"), L (pre + "div"), L (pre + "sync"), "RATE", col),
            stack ({ button (L (pre + "sync"), "SYNC"), button (L (pre + "kbreset"), "KB RESET") }, 6),
            knob (L (pre + "wave"), "WAVESHAPE", col, marks ({ "~sine", "~tri", "~rampup", "~rampdown", "~square" })),
            knob (L (pre + "phase"), "PHASE", col, range (String (CharPointer_UTF8 ("0\xc2\xb0")), String (CharPointer_UTF8 ("360\xc2\xb0")))) });
    }

    auto env = [&] (const String& title, const String& pre, Mod::LayerSource s, int row)
    {
        const int src = Mod::layerSource (layer, s);
        const auto col = Mod::sourceColour (src);
        auto* pn = panel (title, col, { c1, row * (rowH + 10), w1, rowH }, src);
        pn->setEnvelopeIcon (4);
        pn->setGrid ({ 1, 1, 1, 1 }, {
            knob (L (pre + "a"), "ATTACK", col, range ("1ms", "10s")),
            knob (L (pre + "d"), "DECAY", col, range ("1ms", "30s")),
            knob (L (pre + "s"), "SUSTAIN", col, range ("0", "100")),
            knob (L (pre + "r"), "RELEASE", col, range ("1ms", "30s")) });
    };
    env ("FILTER ENVELOPE", "fenv_", Mod::FilterEnv, 0);
    env ("AMP ENVELOPE", "aenv_", Mod::AmpEnv, 1);

    {
        const int src = Mod::layerSource (layer, Mod::ModEnv);
        const auto col = Mod::sourceColour (src);
        auto* pn = panel ("MOD ENVELOPE", col, { c1, 2 * (rowH + 10), Page::width - c1, rowH }, src);
        pn->setEnvelopeIcon (6);
        pn->setGrid ({ 1, 1, 1, 1, 1, 1 }, {
            knob (L ("menv_delay"), "DELAY", col, range ("0", "2s")),
            knob (L ("menv_a"), "ATTACK", col, range ("1ms", "10s")),
            knob (L ("menv_hold"), "HOLD", col, range ("0", "2s")),
            knob (L ("menv_d"), "DECAY", col, range ("1ms", "30s")),
            knob (L ("menv_s"), "SUSTAIN", col, range ("0", "100")),
            knob (L ("menv_r"), "RELEASE", col, range ("1ms", "30s")) });
    }

    for (int i = 0; i < 2; ++i)
    {
        const String n (i + 1), pre = "rnd" + n + "_";
        const int src = Mod::layerSource (layer, (Mod::LayerSource) (Mod::Random1 + i));
        const auto col = Mod::sourceColour (src);
        auto* pn = panel ("RANDOM " + n, col, { c2, i * (rowH + 10), Page::width - c2, rowH }, src);
        pn->setGrid ({ 1.0f, 1.0f, 1.1f }, {
            rateKnob (L (pre + "rate"), L (pre + "div"), L (pre + "sync"), "RATE", col),
            knob (L (pre + "slew"), "SLEW", col, range ("0", "10")),
            stack ({ button (L (pre + "sync"), "SYNC"), radio (L (pre + "mode"), {}, { "S+H", "NOISE", "PERLIN" }) }, 8) });
    }
}

//==============================================================================
void MeterDisplay::refresh()
{
    const float ni[2] = { proc.meterIn[0].load(), proc.meterIn[1].load() };
    const float no[2] = { proc.meterOut[0].load(), proc.meterOut[1].load() };
    const float nc = proc.meterCorr.load(), ng = proc.meterGr.load();
    bool changed = std::abs (nc - corr) > 0.01f || std::abs (ng - gr) > 0.05f;
    for (int c = 0; c < 2; ++c)
        changed = changed || std::abs (ni[c] - in[c]) > 0.2f || std::abs (no[c] - out[c]) > 0.2f;
    if (! changed) return;
    for (int c = 0; c < 2; ++c) { in[c] = ni[c]; out[c] = no[c]; }
    corr = nc;
    gr = ng;
    repaint();
}

void MeterDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0c0e));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (Colour (0xff26272d));
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
    const float H = r.getHeight();

    // K-14 bars: 14 segments of 2 dB from -24 to +4 (0 = -14 dBFS)
    auto bar = [&] (float x, float db)
    {
        const float k = db + 14.0f;
        for (int i = 0; i < 14; ++i)
        {
            const float segDb = -24.0f + 2.0f * (float) i;
            const float y = H - 22.0f - (float) i * 7.5f;
            const auto c = i >= 12 ? Palette::red : (i >= 9 ? Palette::curve : Palette::green);
            g.setColour (c.withAlpha (k > segDb ? 1.0f : 0.12f));
            g.fillRoundedRectangle (x, y, 8.0f, 5.5f, 1.5f);
        }
    };
    bar (18.0f, in[0]); bar (30.0f, in[1]);
    bar (r.getWidth() - 38.0f, out[0]); bar (r.getWidth() - 26.0f, out[1]);
    g.setFont (Fonts::bold (9.5f));
    g.setColour (Palette::label);
    g.drawText ("IN", 14, (int) H - 15, 28, 12, Justification::centred, false);
    g.drawText ("OUT", (int) r.getWidth() - 44, (int) H - 15, 30, 12, Justification::centred, false);

    auto gauge = [&] (float cx, const String& title, StringArray ticks, float val01, float from01, Colour col, const String& read)
    {
        const float cy = 96.0f, rad = 62.0f;
        const float a0 = -MathConstants<float>::pi / 3.0f, span = MathConstants<float>::twoPi / 3.0f;
        Path track;
        track.addCentredArc (cx, cy, rad, rad, 0.0f, a0, a0 + span, true);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.strokePath (track, PathStrokeType (3.0f));
        g.setFont (Fonts::mono (9.5f));
        for (int i = 0; i < ticks.size(); ++i)
        {
            const float a = a0 + span * (float) i / (float) (ticks.size() - 1);
            const Point<float> d (std::sin (a), -std::cos (a));
            g.setColour (Colours::white.withAlpha (0.45f));
            g.drawLine (cx + d.x * 58.0f, cy + d.y * 58.0f, cx + d.x * 66.0f, cy + d.y * 66.0f, 1.0f);
            g.setColour (Palette::textDim);
            g.drawText (ticks[i], Rectangle<float> (30.0f, 12.0f).withCentre ({ cx + d.x * 77.0f, cy + d.y * 77.0f }), Justification::centred, false);
        }
        const float a = a0 + span * jlimit (0.0f, 1.0f, val01), af = a0 + span * from01;
        Path v;
        v.addCentredArc (cx, cy, rad, rad, 0.0f, std::min (a, af), std::max (a, af), true);
        g.setColour (col);
        g.strokePath (v, PathStrokeType (3.0f));
        g.drawLine (cx, cy, cx + std::sin (a) * 64.0f, cy - std::cos (a) * 64.0f, 1.6f);
        g.setColour (Colour (0xff2b2c33));
        g.fillEllipse (cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
        g.setColour (col);
        g.drawEllipse (cx - 4.0f, cy - 4.0f, 8.0f, 8.0f, 1.0f);
        g.setFont (Fonts::mono (12.0f));
        g.drawText (read, Rectangle<float> (120.0f, 14.0f).withCentre ({ cx, 112.0f }), Justification::centred, false);
        g.setFont (Fonts::bold (10.0f).withExtraKerningFactor (0.08f));
        g.setColour (Palette::label);
        g.drawText (title, Rectangle<float> (160.0f, 12.0f).withCentre ({ cx, H - 9.0f }), Justification::centred, false);
    };
    const float w = r.getWidth();
    gauge (w * 0.33f, CharPointer_UTF8 ("CORRELATION \xc3\x98"), { "-1", "-.5", "0", "+.5", "+1" }, 0.5f + 0.5f * corr, 0.5f, Palette::green,
           (corr >= 0 ? "+" : "") + String (corr, 2));
    gauge (w * 0.67f, "GAIN REDUCTION", { "-20", "-12", "-8", "-4", "0" }, 1.0f - jlimit (0.0f, 20.0f, gr) / 20.0f * 1.0f, 1.0f, Palette::curve,
           "-" + String (gr, 1) + " dB");
}

//==============================================================================
VolumeFader::VolumeFader (ResoOGProcessor& p, const String& id)
{
    param = p.param (id);
    attachment = std::make_unique<ParameterAttachment> (*param, [this] (float v) { value = param->convertTo0to1 (v); repaint(); });
    attachment->sendInitialUpdate();
    setTooltip ("Output volume after the compressor. Double-click = 0 dB");
    setSize (70, 300);
}

Rectangle<float> VolumeFader::track() const
{
    return { 22.0f, 24.0f, 8.0f, (float) getHeight() - 48.0f };
}

void VolumeFader::paint (Graphics& g)
{
    g.setColour (Palette::label);
    g.setFont (Fonts::bold (10.5f));
    g.drawText ("VOLUME", 0, 0, getWidth(), 13, Justification::centred, false);

    const auto t = track();
    g.setColour (Colour (0xff0b0c0e));
    g.fillRoundedRectangle (t, 4.0f);
    g.setColour (Colour (0xff26272d));
    g.drawRoundedRectangle (t, 4.0f, 1.0f);
    const float y = t.getBottom() - value * t.getHeight();
    g.setColour (Palette::curve.withAlpha (0.85f));
    g.fillRoundedRectangle (t.withTop (y), 4.0f);

    g.setFont (Fonts::mono (9.5f));
    g.setColour (Palette::scale);
    for (float db : { 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -60.0f })
    {
        const float yy = t.getBottom() - param->convertTo0to1 (db) * t.getHeight();
        g.drawText (db <= -60.0f ? String (CharPointer_UTF8 ("-\xe2\x88\x9e")) : (db > 0 ? "+" : "") + String ((int) db),
                    (int) t.getRight() + 6, (int) yy - 6, 30, 12, Justification::centredLeft, false);
    }

    auto cap = Rectangle<float> (30.0f, 16.0f).withCentre ({ t.getCentreX(), y });
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.0f), 4.0f);
    g.setGradientFill (ColourGradient (Colour (0xffd8d9dd), 0.0f, cap.getY(), Colour (0xff7d7f86), 0.0f, cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (cap, 4.0f, 1.0f);
    g.drawLine (cap.getX() + 4.0f, cap.getCentreY(), cap.getRight() - 4.0f, cap.getCentreY(), 1.0f);

    g.setColour (Palette::textDim);
    g.setFont (Fonts::mono (10.5f));
    g.drawText (param->getCurrentValueAsText(), 0, getHeight() - 14, getWidth(), 13, Justification::centred, false);
}

void VolumeFader::mouseDown (const MouseEvent& e)
{
    dragStart = value;
    dragY = e.position.y;
    attachment->beginGesture();
}

void VolumeFader::mouseDrag (const MouseEvent& e)
{
    const float speed = e.mods.isShiftDown() ? 0.2f : 1.0f;
    const float v = jlimit (0.0f, 1.0f, dragStart + (dragY - e.position.y) / track().getHeight() * speed);
    attachment->setValueAsPartOfGesture (param->convertFrom0to1 (v));
}

void VolumeFader::mouseUp (const MouseEvent&) { attachment->endGesture(); }
void VolumeFader::mouseDoubleClick (const MouseEvent&) { attachment->setValueAsCompleteGesture (0.0f); }

//==============================================================================
OutputPage::OutputPage (ResoOGProcessor& p) : Page (p)
{
    const int sideW = 262;
    auto fx = [&] (int n, Colour col, int x)
    {
        const String s (n);
        auto* pn = panel ("SYNTH " + s + " EFFECTS", col, { x, 0, sideW, Page::height });
        std::vector<Component*> items {
            knob ("fx" + s + "_sat", "SATURATION", col, range ("0", "10")),
            knob ("fx" + s + "_sattype", "SAT TYPE", col, marks ({ "OFF", "TUBE", "TAPE", "DRIVE" })) };
        if (n == 1)
        {
            items.push_back (rateKnob ("dly_time", "dly_div", "dly_sync", "DELAY TIME", col, "0", "1.4s"));
            items.push_back (knob ("dly_fb", "DELAY FEEDBACK", col, range ("0", "10")));
            items.push_back (knob ("dly_hpf", "DELAY HPF", col, range ("40", "2k")));
            items.push_back (knob ("dly_mix", "DELAY MIX", col, range ("DRY", "WET")));
            items.push_back (button ("dly_stereo", "DELAY STEREO"));
            items.push_back (button ("dly_sync", "SYNC"));
        }
        else
        {
            items.push_back (rateKnob ("cho_rate", "cho_div", "cho_sync", "CHORUS RATE", col, ".05", "20"));
            items.push_back (knob ("cho_depth", "CHORUS DEPTH", col, range ("0", "10")));
            items.push_back (knob ("cho_hpf", "CHORUS HPF", col, range ("40", "2k")));
            items.push_back (knob ("cho_mix", "CHORUS MIX", col, range ("DRY", "WET")));
            items.push_back (button ("cho_expand", "CHORUS EXPAND"));
            items.push_back (button ("cho_sync", "SYNC"));
        }
        pn->setGrid ({ 1, 1 }, items);
    };
    fx (1, Palette::fx1, 0);
    fx (2, Palette::fx2, Page::width - sideW);

    auto* sum = panel ("SUMMING", Palette::curve, { sideW + 10, 0, Page::width - 2 * sideW - 20, Page::height });
    meters = static_cast<MeterDisplay*> (owned.add (new MeterDisplay (proc)));
    meters->setSize (560, 136);
    auto* fader = static_cast<VolumeFader*> (owned.add (new VolumeFader (proc, "master_vol")));
    fader->setSize (70, 330);
    std::vector<Component*> items {
        knob ("sum_lvl1", "SYNTH 1 LEVEL", Palette::fx1, range ("-inf", "+6")),
        knob ("sum_lvl2", "SYNTH 2 LEVEL", Palette::fx2, range ("-inf", "+6")),
        nullptr,
        knob ("comp_attack", "ATTACK", Palette::curve, range (".01ms", "1s")),
        knob ("comp_ratio", "RATIO", Palette::curve, range ("1:1", "20:1")),
        knob ("sum_pan1", "PAN", Palette::fx1, range ("L", "R", true)),
        knob ("sum_pan2", "PAN", Palette::fx2, range ("L", "R", true)),
        nullptr,
        knob ("comp_thresh", "THRESHOLD", Palette::curve, range ("-30", "0")),
        knob ("comp_mix", "MIX", Palette::curve, range ("DRY", "WET")),
        button ("sum_mute1", "MUTE 1"),
        button ("sum_mute2", "MUTE 2"),
        nullptr,
        button ("comp_fet", "FET"),
        button ("comp_on", "COMPRESSOR") };
    sum->setGrid ({ 1, 1, 0.8f, 1, 1 }, items);
    sum->addAndMakeVisible (meters);
    sum->addAndMakeVisible (fader);

    // grid below the meters: re-place everything after the generic layout
    auto relayout = [sum, m = meters, fader, items]
    {
        auto body = sum->getLocalBounds().withTrimmedTop (ModulePanel::headerHeight + 2).reduced (8, 8);
        m->setTopLeftPosition (body.getCentreX() - m->getWidth() / 2, body.getY());
        body.removeFromTop (m->getHeight() + 8);
        const float cw = body.getWidth() / 4.8f;
        const float xs[5] = { 0.5f, 1.5f, 2.4f, 3.3f, 4.3f };
        const float rowY[3] = { 0.2f, 0.56f, 0.88f };
        for (size_t i = 0; i < items.size(); ++i)
            if (items[i] != nullptr)
                items[i]->setCentrePosition (body.getX() + roundToInt (cw * xs[i % 5]), body.getY() + roundToInt (body.getHeight() * rowY[i / 5]));
        fader->setBounds (body.getX() + roundToInt (cw * xs[2]) - 35, body.getY() - 2, 70, body.getHeight() + 4);
    };
    relayout();
}

void OutputPage::refreshLive()
{
    Page::refreshLive();
    if (meters) meters->refresh();
}
