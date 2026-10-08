#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params/Parameters.h"
#include "DSP/SynthLayer.h"
#include "DSP/Effects.h"
#include "DSP/ModMatrix.h"
#include "State/StateHistory.h"

class ResoOGProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    ResoOGProcessor();
    ~ResoOGProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ResoOG"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Presets
    void loadFactoryPreset (int index);
    void resetToDefaults();
    juce::String getPresetName() const { return uiState.getProperty ("presetName", "Init").toString(); }
    void setPresetName (const juce::String& n) { uiState.setProperty ("presetName", n, nullptr); }

    // Parameter helpers (message thread)
    juce::RangedAudioParameter* param (const juce::String& id) const { return apvts.getParameter (id); }
    void setParamReal (const juce::String& id, float value);
    float getParamReal (const juce::String& id) const;

    // Modulation helpers (message thread)
    int addModulation (int source, const juce::String& destParamID, float amount);   // returns slot or -1
    void removeModulation (int slot);
    juce::Array<int> slotsForDestination (const juce::String& destParamID) const;
    int findFreeSlot() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree uiState { "UI" };
    std::unique_ptr<StateHistory> history;
    juce::MidiKeyboardState keyboardState;
    juce::String instanceID;

    // UI -> audio
    std::atomic<float> uiPitchBend { 0.0f }, uiModWheel { 0.0f };
    std::atomic<bool> uiHold { false };
    std::atomic<bool> editorOpen { false };

    // audio -> UI (live values)
    std::atomic<float> liveDest[Mod::maxDestCode + 1];   // modulated normalised value per destination code (-1 = not modulated)
    std::atomic<float> liveSource[Mod::numSources];
    std::atomic<float> liveDrive[2], liveNote[2];
    std::atomic<float> meterIn[2], meterOut[2], meterCorr { 1.0f }, meterGr { 0.0f }, meterPeak { -100.0f };
    std::atomic<float> cpuLoad { 0.0f };

    static constexpr int chunkSize = 32;

private:
    void handleAsyncUpdate() override;
    void handleMidi (const juce::MidiMessage&);
    void noteOn (int note, float velocity);
    void noteOff (int note, float releaseVelocity);
    void assignNotes (bool legatoChange);
    void processChunk (juce::AudioBuffer<float>& out, int start, int len, const Transport& t);
    void setOversampling (int index);

    std::atomic<float>* layerRaw[2][LP::count] {};
    std::atomic<float>* globalRaw[GP::count] {};
    std::atomic<float>* modRaw[Params::numModSlots][MP::count] {};
    juce::RangedAudioParameter* layerParam[2][LP::count] {};
    juce::RangedAudioParameter* globalParam[GP::count] {};

    SynthLayer layers[2];
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[2][2];   // [layer][2x, 4x]
    int osIndex = 1;
    juce::AudioBuffer<float> zeros, layerBuf[2];
    dsp::DcBlocker dc[2][2];

    fx::Saturator saturators[2];
    fx::StereoDelay delay;
    fx::Chorus chorus;
    fx::Compressor compressor;
    fx::Meter inMeter, outMeter;

    ModMatrix matrix;
    ModMatrix::Slot slots[Params::numModSlots];
    float sources[Mod::numSources] {};
    float modOut[Params::numModSlots] {};
    float modSum[Mod::maxDestCode + 1] {};
    bool modTouched[Mod::maxDestCode + 1] {};
    float cur[2][LP::count] {}, curG[GP::count] {};

    // notes
    int held[32] {}; float heldVel[32] {};
    bool physicallyDown[128] {};
    int numHeld = 0;
    int assigned[2] = { -1, -1 };
    bool sustain = false;
    float modWheel = 0.0f, pressure = 0.0f, pitchBend = 0.0f, releaseVel = 0.0f, timbre = 0.0f, lastVelocity = 0.0f;
    float lastUiBend = 0.0f, lastUiWheel = 0.0f;
    bool lastUiHold = false;

    double sr = 48000.0;
    int currentPreset = 0;
    std::atomic<int> pendingLatency { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResoOGProcessor)
};
