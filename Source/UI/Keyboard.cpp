#include "Keyboard.h"
#include "PluginProcessor.h"

using namespace juce;

namespace
{
bool isBlack (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
}

KeyboardPanel::KeyboardPanel (ResoOGProcessor& p) : proc (p)
{
    startNote = (int) proc.uiState.getProperty ("kbStart", 36);
}

String KeyboardPanel::noteLabel (int n)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return String (names[n % 12]) + String (n / 12 - 2);
}

Rectangle<float> KeyboardPanel::bendArea() const  { return { 18.0f, 30.0f, 18.0f, (float) getHeight() - 42.0f }; }
Rectangle<float> KeyboardPanel::wheelArea() const { return { 58.0f, 30.0f, 18.0f, (float) getHeight() - 42.0f }; }
Rectangle<float> KeyboardPanel::rangeArea() const { return { 108.0f, 12.0f, (float) getWidth() - 108.0f - 136.0f, 6.0f }; }
Rectangle<float> KeyboardPanel::keysArea() const  { return { 108.0f, 26.0f, (float) getWidth() - 108.0f - 136.0f, (float) getHeight() - 38.0f }; }
Rectangle<float> KeyboardPanel::holdArea() const  { return { (float) getWidth() - 116.0f, 34.0f, 96.0f, 26.0f }; }
Rectangle<float> KeyboardPanel::octDownArea() const { return { (float) getWidth() - 116.0f, 96.0f, 30.0f, 26.0f }; }
Rectangle<float> KeyboardPanel::octUpArea() const   { return { (float) getWidth() - 50.0f, 96.0f, 30.0f, 26.0f }; }

Rectangle<float> KeyboardPanel::keyRect (int i) const
{
    const auto a = keysArea();
    const float w = (a.getWidth() - 4.0f * (numKeys - 1)) / numKeys;
    return { a.getX() + (float) i * (w + 4.0f), a.getY(), w, a.getHeight() };
}

int KeyboardPanel::keyAt (Point<float> p) const
{
    for (int i = 0; i < numKeys; ++i)
        if (keyRect (i).expanded (2.0f, 0.0f).contains (p)) return i;
    return -1;
}

KeyboardPanel::Part KeyboardPanel::partAt (Point<float> p) const
{
    if (bendArea().expanded (8.0f, 4.0f).contains (p)) return Part::Bend;
    if (wheelArea().expanded (8.0f, 4.0f).contains (p)) return Part::Wheel;
    if (rangeArea().expanded (0.0f, 6.0f).contains (p)) return Part::Range;
    if (keysArea().contains (p)) return Part::Keys;
    if (holdArea().contains (p)) return Part::Hold;
    if (octDownArea().contains (p)) return Part::OctDown;
    if (octUpArea().contains (p)) return Part::OctUp;
    return Part::None;
}

void KeyboardPanel::refresh()
{
    bool changed = false;
    const float n0 = proc.liveNote[0].load(), n1 = proc.liveNote[1].load();
    for (int i = 0; i < numKeys; ++i)
    {
        const int n = startNote + i;
        const bool on = proc.keyboardState.isNoteOnForChannels (0xffff, n)
                        || (n0 >= 0.0f && std::abs (n0 - (float) n) < 0.5f) || (n1 >= 0.0f && std::abs (n1 - (float) n) < 0.5f);
        if (on != lit[i]) { lit[i] = on; changed = true; }
    }
    if (changed) repaint();
}

void KeyboardPanel::paint (Graphics& g)
{
    g.fillAll (Palette::bar);
    g.setColour (Colour (0xff17181c));
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());

    auto strip = [&] (Rectangle<float> r, const String& name, float pos01)
    {
        g.setColour (Palette::textDim);
        g.setFont (Fonts::bold (10.5f));
        g.drawText (name, r.withY (10.0f).withHeight (13.0f).expanded (10.0f, 0.0f), Justification::centred, false);
        g.setColour (Colour (0xff16171b));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (Colour (0xff26272d));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
        const float y = r.getBottom() - 8.0f - pos01 * (r.getHeight() - 16.0f);
        g.setColour (Colour (0xff4a4b53));
        g.fillRoundedRectangle (r.getX() + 2.0f, y - 5.0f, r.getWidth() - 4.0f, 10.0f, 3.0f);
    };
    strip (bendArea(), "PB", 0.5f + 0.5f * bend);
    strip (wheelArea(), "MW", wheel);

    // range bar
    const auto ra = rangeArea();
    g.setColour (Colour (0xff16171b));
    g.fillRoundedRectangle (ra, 3.0f);
    const float tx = ra.getX() + ra.getWidth() * (float) startNote / 128.0f;
    g.setColour (Colour (0xff3a3b42));
    g.fillRoundedRectangle (tx, ra.getY(), ra.getWidth() * (float) numKeys / 128.0f, ra.getHeight(), 3.0f);

    for (int i = 0; i < numKeys; ++i)
    {
        const int n = startNote + i;
        auto r = keyRect (i);
        const bool black = isBlack (n);
        if (black) g.setGradientFill (ColourGradient (Colour (0xff24252a), 0.0f, r.getY(), Colour (0xff16171b), 0.0f, r.getBottom(), false));
        else       g.setGradientFill (ColourGradient (Colour (0xffe9e8e4), 0.0f, r.getY(), Colour (0xffc9c7c1), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (Colours::black);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        if (lit[i])
        {
            g.setColour (Palette::curve.withAlpha (0.25f));
            g.drawRoundedRectangle (r.expanded (2.0f), 7.0f, 4.0f);
            g.setColour (Palette::curve);
            g.drawRoundedRectangle (r, 6.0f, 2.0f);
        }
        g.setColour (lit[i] ? (black ? Palette::curve : Colour (0xff7a5a10)) : (black ? Colour (0xff7a7b84) : Colour (0xff55565e)));
        g.setFont (Fonts::mono (10.5f).boldened());
        g.drawText (noteLabel (n), r.withHeight (18.0f).translated (0.0f, 4.0f), Justification::centred, false);
    }

    // side: hold + octave
    auto pill = [&] (Rectangle<float> r, const String& t, bool on)
    {
        g.setColour (on ? Colour (0xff2a2415) : Palette::button);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (on ? Palette::curve : Colour (0xff2a2b31));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
        g.setColour (on ? Palette::curve : Palette::label);
        g.setFont (Fonts::bold (11.0f));
        g.drawText (t, r, Justification::centred, false);
    };
    pill (holdArea(), "Hold", proc.uiHold.load());
    g.setColour (Palette::textDim);
    g.setFont (Fonts::bold (10.5f));
    g.drawText ("OCTAVE", (int) holdArea().getX(), 76, 96, 13, Justification::centred, false);
    pill (octDownArea(), "-", false);
    pill (octUpArea(), "+", false);
    g.setColour (Palette::text);
    g.setFont (Fonts::mono (10.5f));
    g.drawText (noteLabel (startNote), (int) octDownArea().getRight(), 96, (int) (octUpArea().getX() - octDownArea().getRight()), 26, Justification::centred, false);
}

void KeyboardPanel::play (int key, Point<float> p)
{
    if (key < 0) return;
    const int n = startNote + key;
    if (n == playing) return;
    stopNote();
    const auto r = keyRect (key);
    const float vel = jlimit (0.2f, 1.0f, 0.3f + 0.7f * (p.y - r.getY()) / r.getHeight());
    proc.keyboardState.noteOn (1, n, vel);
    playing = n;
}

void KeyboardPanel::stopNote()
{
    if (playing >= 0) proc.keyboardState.noteOff (1, playing, 0.5f);
    playing = -1;
}

void KeyboardPanel::mouseDown (const MouseEvent& e)
{
    dragPart = partAt (e.position);
    switch (dragPart)
    {
        case Part::Keys: play (keyAt (e.position), e.position); break;
        case Part::Bend:
        case Part::Wheel: mouseDrag (e); break;
        case Part::Range: rangeDragStart = e.position.x; rangeStartNote = startNote; break;
        case Part::Hold: proc.uiHold = ! proc.uiHold.load(); repaint(); break;
        case Part::OctDown:
        case Part::OctUp:
            startNote = jlimit (0, 128 - numKeys, startNote + (dragPart == Part::OctUp ? 12 : -12));
            proc.uiState.setProperty ("kbStart", startNote, nullptr);
            repaint();
            break;
        case Part::None: break;
    }
}

void KeyboardPanel::mouseDrag (const MouseEvent& e)
{
    if (dragPart == Part::Keys)
        play (keyAt (e.position), e.position);
    else if (dragPart == Part::Bend)
    {
        const auto r = bendArea();
        bend = jlimit (-1.0f, 1.0f, ((r.getCentreY() - e.position.y) / (r.getHeight() * 0.5f - 8.0f)));
        proc.uiPitchBend = bend;
        repaint();
    }
    else if (dragPart == Part::Wheel)
    {
        const auto r = wheelArea();
        wheel = jlimit (0.0f, 1.0f, (r.getBottom() - 8.0f - e.position.y) / (r.getHeight() - 16.0f));
        proc.uiModWheel = wheel;
        repaint();
    }
    else if (dragPart == Part::Range)
    {
        const int delta = roundToInt ((e.position.x - rangeDragStart) / rangeArea().getWidth() * 128.0f);
        int s = jlimit (0, 128 - numKeys, rangeStartNote + delta);
        while (s > 0 && isBlack (s)) --s;
        if (s != startNote) { startNote = s; proc.uiState.setProperty ("kbStart", s, nullptr); repaint(); }
    }
}

void KeyboardPanel::mouseUp (const MouseEvent&)
{
    if (dragPart == Part::Keys) stopNote();
    if (dragPart == Part::Bend)
    {
        bend = 0.0f;   // springs back
        proc.uiPitchBend = 0.0f;
        repaint();
    }
    dragPart = Part::None;
}
