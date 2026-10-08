#pragma once

#include "Widgets.h"

class ResoOGProcessor;

// One of the five panel pages. Owns its controls; refreshLive() is called by the editor timer.
class Page : public juce::Component
{
public:
    explicit Page (ResoOGProcessor& p) : proc (p) {}
    virtual void refreshLive();
    void setModEditCallback (std::function<void (int)> cb);
    Knob* findKnob (const juce::String& paramID) const;

    static constexpr int width = 1136, height = 536;

protected:
    Knob* knob (const juce::String& id, const juce::String& label, juce::Colour, Knob::Style = {});
    LedButton* button (const juce::String& id, const juce::String& label);
    LedRadio* radio (const juce::String& id, const juce::String& label, juce::StringArray names = {});
    ModulePanel* panel (const juce::String& title, juce::Colour, juce::Rectangle<int> bounds, int source = -1, const juce::String& right = {});
    Stack* stack (std::vector<juce::Component*> kids, int gap = 10, bool left = false);

    // Two knobs on one spot: free rate / synced division, switched by a sync parameter
    juce::Component* rateKnob (const juce::String& rateID, const juce::String& divID, const juce::String& syncID,
                               const juce::String& label, juce::Colour, juce::String lo = ".05", juce::String hi = "50");

    ResoOGProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    juce::Array<Knob*> knobs;
    struct LivePanel { ModulePanel* panel; int source; };
    juce::Array<LivePanel> livePanels;
    struct RatePair { Knob* rate; Knob* div; juce::String syncID; };
    juce::Array<RatePair> ratePairs;
};

class SynthPage : public Page
{
public:
    SynthPage (ResoOGProcessor&, int layer);
    void refreshLive() override;

private:
    int layer;
    DriveMeter* drive = nullptr;
    ModulePanel* filters = nullptr;
};

class CntrlPage : public Page
{
public:
    CntrlPage (ResoOGProcessor&, int layer);

private:
    int layer;
};

class MeterDisplay : public juce::Component
{
public:
    explicit MeterDisplay (ResoOGProcessor& p) : proc (p) {}
    void refresh();
    void paint (juce::Graphics&) override;

private:
    ResoOGProcessor& proc;
    float in[2] { -100, -100 }, out[2] { -100, -100 }, corr = 1.0f, gr = 0.0f;
};

class VolumeFader : public juce::Component, public juce::SettableTooltipClient
{
public:
    VolumeFader (ResoOGProcessor&, const juce::String& paramID);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> track() const;
    juce::RangedAudioParameter* param;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    float value = 0.0f, dragStart = 0.0f, dragY = 0.0f;
};

class OutputPage : public Page
{
public:
    explicit OutputPage (ResoOGProcessor&);
    void refreshLive() override;

private:
    MeterDisplay* meters = nullptr;
};
