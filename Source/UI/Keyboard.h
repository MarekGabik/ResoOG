#pragma once

#include "Theme.h"

class ResoOGProcessor;

// Bottom performance strip: pitch bend and mod wheel strips, an 18-key keyboard (velocity from where you
// strike: lower = louder, glissando by dragging), octave shift and Hold.
class KeyboardPanel : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit KeyboardPanel (ResoOGProcessor&);
    void refresh();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    static constexpr int numKeys = 18;
    static juce::String noteLabel (int midi);   // Ableton convention: 36 = C1

private:
    enum class Part { None, Bend, Wheel, Keys, Range, Hold, OctDown, OctUp };
    Part partAt (juce::Point<float>) const;
    juce::Rectangle<float> bendArea() const, wheelArea() const, keysArea() const, rangeArea() const;
    juce::Rectangle<float> holdArea() const, octDownArea() const, octUpArea() const;
    juce::Rectangle<float> keyRect (int i) const;
    int keyAt (juce::Point<float>) const;
    void play (int key, juce::Point<float>);
    void stopNote();

    ResoOGProcessor& proc;
    int startNote = 36;
    int playing = -1;
    Part dragPart = Part::None;
    float bend = 0.0f, wheel = 0.0f, rangeDragStart = 0.0f;
    int rangeStartNote = 36;
    bool lit[numKeys] {};
};
