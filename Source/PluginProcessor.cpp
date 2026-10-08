#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/FactoryPresets.h"

using namespace juce;

ResoOGProcessor::ResoOGProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", Params::createLayout())
{
    for (int l = 0; l < 2; ++l)
        for (int i = 0; i < LP::count; ++i)
        {
            const auto id = Params::layerID (l, Params::layerDefs()[(size_t) i].id);
            layerRaw[l][i] = apvts.getRawParameterValue (id);
            layerParam[l][i] = apvts.getParameter (id);
            jassert (layerRaw[l][i] != nullptr);
        }
    for (int i = 0; i < GP::count; ++i)
    {
        globalRaw[i] = apvts.getRawParameterValue (Params::globalDefs()[(size_t) i].id);
        globalParam[i] = apvts.getParameter (Params::globalDefs()[(size_t) i].id);
        jassert (globalRaw[i] != nullptr);
    }
    for (int s = 0; s < Params::numModSlots; ++s)
        for (int i = 0; i < MP::count; ++i)
            modRaw[s][i] = apvts.getRawParameterValue (Params::modID (s, Params::modSlotDefs()[(size_t) i].id));

    for (auto& v : liveDest) v.store (-1.0f);
    for (auto& v : liveSource) v.store (0.0f);
    for (int i = 0; i < 2; ++i) { liveDrive[i].store (0.0f); liveNote[i].store (-1.0f); meterIn[i].store (-100.0f); meterOut[i].store (-100.0f); }

    Random r;
    const char* chars = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    for (int i = 0; i < 4; ++i) instanceID << String::charToString ((juce_wchar) chars[r.nextInt (57)]);

    uiState.setProperty ("presetName", "Init", nullptr);
    history = std::make_unique<StateHistory> (*this);
}

ResoOGProcessor::~ResoOGProcessor()
{
    cancelPendingUpdate();
}

bool ResoOGProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void ResoOGProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    zeros.setSize (2, chunkSize);
    zeros.clear();
    for (int l = 0; l < 2; ++l)
    {
        layerBuf[l].setSize (2, std::max (samplesPerBlock, chunkSize) + chunkSize);
        for (int f = 0; f < 2; ++f)
        {
            oversamplers[l][f] = std::make_unique<juce::dsp::Oversampling<float>> (2, f + 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
            oversamplers[l][f]->initProcessing ((size_t) chunkSize);
        }
        dc[l][0].setup (sr); dc[l][1].setup (sr);
        dc[l][0].reset(); dc[l][1].reset();
        saturators[l].prepare (sr);
    }

    osIndex = jlimit (0, 2, (int) globalRaw[GP::oversampling]->load());
    const double osRate = sr * (1 << osIndex);
    layers[0].prepare (osRate, 1);
    layers[1].prepare (osRate, 2);
    delay.prepare (sr, 2.5);
    chorus.prepare (sr);
    compressor.prepare (sr);
    inMeter.prepare (sr);
    outMeter.prepare (sr);
    matrix.reset();
    keyboardState.reset();
    numHeld = 0;
    assigned[0] = assigned[1] = -1;
    pendingLatency = osIndex == 0 ? 0 : (int) std::lround (oversamplers[0][osIndex - 1]->getLatencyInSamples());
    setLatencySamples (pendingLatency);
}

void ResoOGProcessor::setOversampling (int index)
{
    osIndex = index;
    const double osRate = sr * (1 << osIndex);
    for (int l = 0; l < 2; ++l)
    {
        layers[l].setSampleRate (osRate);
        for (auto& o : oversamplers[l]) if (o) o->reset();
    }
    pendingLatency = osIndex == 0 ? 0 : (int) std::lround (oversamplers[0][osIndex - 1]->getLatencyInSamples());
    triggerAsyncUpdate();
}

void ResoOGProcessor::handleAsyncUpdate()
{
    setLatencySamples (pendingLatency.load());
}

//==============================================================================
void ResoOGProcessor::noteOn (int note, float vel)
{
    for (int i = 0; i < numHeld; ++i)
        if (held[i] == note)
        {
            for (int j = i; j < numHeld - 1; ++j) { held[j] = held[j + 1]; heldVel[j] = heldVel[j + 1]; }
            --numHeld;
            break;
        }
    if (numHeld >= 32)
    {
        for (int j = 0; j < numHeld - 1; ++j) { held[j] = held[j + 1]; heldVel[j] = heldVel[j + 1]; }
        --numHeld;
    }
    held[numHeld] = note;
    heldVel[numHeld] = vel;
    ++numHeld;
    lastVelocity = vel;
    assignNotes (true);
}

void ResoOGProcessor::noteOff (int note, float relVel)
{
    releaseVel = relVel;
    if (sustain || lastUiHold)
        return;
    for (int i = 0; i < numHeld; ++i)
        if (held[i] == note)
        {
            for (int j = i; j < numHeld - 1; ++j) { held[j] = held[j + 1]; heldVel[j] = heldVel[j + 1]; }
            --numHeld;
            assignNotes (false);
            return;
        }
}

// Last-note priority (Layered) or lowest / highest split (Duophonic)
void ResoOGProcessor::assignNotes (bool newNote)
{
    const bool glideLegato = globalRaw[GP::glide_legato]->load() > 0.5f;
    if (numHeld == 0)
    {
        for (int l = 0; l < 2; ++l)
            if (assigned[l] >= 0) { layers[l].noteOff(); assigned[l] = -1; }
        return;
    }

    int want[2];
    float vel[2];
    const bool duo = globalRaw[GP::voice_mode]->load() > 0.5f;
    if (duo && numHeld > 1)
    {
        int lo = 0, hi = 0;
        for (int i = 1; i < numHeld; ++i)
        {
            if (held[i] < held[lo]) lo = i;
            if (held[i] > held[hi]) hi = i;
        }
        want[0] = held[lo]; vel[0] = heldVel[lo];
        want[1] = held[hi]; vel[1] = heldVel[hi];
    }
    else
    {
        want[0] = want[1] = held[numHeld - 1];
        vel[0] = vel[1] = heldVel[numHeld - 1];
    }

    for (int l = 0; l < 2; ++l)
    {
        if (want[l] == assigned[l] && ! (newNote && want[l] == held[numHeld - 1] && ! duo))
            continue;
        float p[LP::count];
        for (int i = 0; i < LP::count; ++i) p[i] = layerRaw[l][i]->load();
        const bool legato = assigned[l] >= 0;
        layers[l].noteOn (want[l], vel[l], legato, p, glideLegato);
        assigned[l] = want[l];
    }
}

void ResoOGProcessor::handleMidi (const MidiMessage& m)
{
    if (m.isNoteOn())
    {
        if (m.getNoteNumber() < 128) physicallyDown[m.getNoteNumber()] = true;
        noteOn (m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isNoteOff())
    {
        if (m.getNoteNumber() < 128) physicallyDown[m.getNoteNumber()] = false;
        noteOff (m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isPitchWheel())
        pitchBend = jlimit (-1.0f, 1.0f, (float) (m.getPitchWheelValue() - 8192) / 8191.0f);
    else if (m.isChannelPressure())
        pressure = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())
        pressure = (float) m.getAfterTouchValue() / 127.0f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float v = (float) m.getControllerValue() / 127.0f;
        if (cc == 1) modWheel = v;
        else if (cc == 74) timbre = v;
        else if (cc == 64)
        {
            const bool was = sustain;
            sustain = v >= 0.5f;
            if (was && ! sustain && ! lastUiHold)
            {
                // release the notes that are no longer held down
                int w = 0;
                for (int i = 0; i < numHeld; ++i)
                    if (physicallyDown[held[i]]) { held[w] = held[i]; heldVel[w] = heldVel[i]; ++w; }
                numHeld = w;
                assignNotes (false);
            }
        }
        else if (cc == 120 || cc == 123)
        {
            numHeld = 0;
            for (auto& d : physicallyDown) d = false;
            assignNotes (false);
            if (cc == 120) { layers[0].allOff(); layers[1].allOff(); }
        }
    }
}

//==============================================================================
void ResoOGProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const double t0 = Time::getMillisecondCounterHiRes();
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n == 0 || buffer.getNumChannels() < 2)
        return;

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    // on-screen wheels and hold
    const float ub = uiPitchBend.load(), uw = uiModWheel.load();
    if (std::abs (ub - lastUiBend) > 1.0e-6f) { pitchBend = ub; lastUiBend = ub; }
    if (std::abs (uw - lastUiWheel) > 1.0e-6f) { modWheel = uw; lastUiWheel = uw; }
    const bool hold = uiHold.load();
    if (lastUiHold && ! hold)
    {
        lastUiHold = false;
        int w = 0;
        for (int i = 0; i < numHeld; ++i)
            if (physicallyDown[held[i]]) { held[w] = held[i]; heldVel[w] = heldVel[i]; ++w; }
        numHeld = w;
        assignNotes (false);
    }
    lastUiHold = hold;

    const int wantOs = jlimit (0, 2, (int) globalRaw[GP::oversampling]->load());
    if (wantOs != osIndex)
        setOversampling (wantOs);

    Transport t;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            t.playing = pos->getIsPlaying();
            if (auto bpm = pos->getBpm()) t.bpm = jlimit (20.0, 999.0, *bpm);
            if (auto ppq = pos->getPpqPosition()) t.ppq = *ppq;
        }

    auto it = midi.cbegin();
    int pos = 0;
    while (pos < n)
    {
        while (it != midi.cend() && (*it).samplePosition <= pos)
        {
            handleMidi ((*it).getMessage());
            ++it;
        }
        int next = std::min (n, pos + chunkSize);
        if (it != midi.cend())
            next = std::min (next, std::max (pos + 1, (*it).samplePosition));

        Transport ct = t;
        ct.ppq = t.ppq + (double) pos / sr * t.bpm / 60.0;
        processChunk (buffer, pos, next - pos, ct);
        pos = next;
    }
    while (it != midi.cend()) { handleMidi ((*it).getMessage()); ++it; }

    // meters for the UI
    meterIn[0] = inMeter.rmsDb (0); meterIn[1] = inMeter.rmsDb (1);
    meterOut[0] = outMeter.rmsDb (0); meterOut[1] = outMeter.rmsDb (1);
    meterCorr = outMeter.correlation();
    meterPeak = std::max (meterPeak.load() - 0.5f, outMeter.peakDb());
    outMeter.decayPeak();
    for (int l = 0; l < 2; ++l)
    {
        liveDrive[l] = layers[l].getDrive();
        liveNote[l] = layers[l].isSounding() ? layers[l].getPitch() : -1.0f;
    }
    for (int i = 0; i < Mod::numSources; ++i) liveSource[i] = sources[i];

    const double elapsed = Time::getMillisecondCounterHiRes() - t0;
    const float load = (float) (elapsed / (1000.0 * n / sr) * 100.0);
    cpuLoad = cpuLoad.load() * 0.9f + load * 0.1f;
}

void ResoOGProcessor::processChunk (AudioBuffer<float>& out, int start, int len, const Transport& t)
{
    const double seconds = len / sr;

    // 1. sources
    const float noteKey = assigned[0] >= 0 ? (float) assigned[0] : (assigned[1] >= 0 ? (float) assigned[1] : 60.0f);
    sources[Mod::None] = 0.0f;
    sources[Mod::Velocity] = lastVelocity;
    sources[Mod::Keyboard] = (noteKey - 60.0f) / 60.0f;
    sources[Mod::ModWheel] = modWheel;
    sources[Mod::Pressure] = pressure;
    sources[Mod::PitchBend] = pitchBend;
    sources[Mod::ReleaseVelocity] = releaseVel;
    sources[Mod::Timbre] = timbre;
    sources[Mod::Constant] = 1.0f;
    for (int l = 0; l < 2; ++l)
    {
        if (cur[l][LP::lfo1] <= 0.0f)   // first chunk: modulated values not computed yet
            for (int i = 0; i < LP::count; ++i) cur[l][i] = layerRaw[l][i]->load();
        layers[l].updateModulators (cur[l], seconds, t, sources + Mod::FirstLayerSource + l * Mod::perLayerSources);
    }

    // 2. matrix
    for (int s = 0; s < Params::numModSlots; ++s)
    {
        auto& sl = slots[s];
        sl.on = modRaw[s][MP::on]->load() > 0.5f;
        sl.src = jlimit (0, Mod::numSources - 1, (int) modRaw[s][MP::src]->load());
        sl.dst = jlimit (0, Mod::maxDestCode, (int) modRaw[s][MP::dst]->load());
        sl.amt = modRaw[s][MP::amt]->load();
        sl.bipolar = modRaw[s][MP::bi]->load() > 0.5f;
        sl.ctl = jlimit (0, Mod::numSources - 1, (int) modRaw[s][MP::ctl]->load());
        sl.ctlAmt = modRaw[s][MP::ctlamt]->load();
        sl.fn = (int) modRaw[s][MP::fn]->load();
        sl.fnAmt = modRaw[s][MP::fnamt]->load() / 100.0f;
    }
    matrix.process (slots, sources, seconds, modOut);

    // 3. modulated parameter values
    for (int l = 0; l < 2; ++l)
        for (int i = 0; i < LP::count; ++i) cur[l][i] = layerRaw[l][i]->load();
    for (int i = 0; i < GP::count; ++i) curG[i] = globalRaw[i]->load();

    for (int s = 0; s < Params::numModSlots; ++s)
    {
        if (! slots[s].active()) continue;
        const int d = slots[s].dst;
        if (! modTouched[d]) { modTouched[d] = true; modSum[d] = 0.0f; }
        modSum[d] += modOut[s];
    }
    for (int s = 0; s < Params::numModSlots; ++s)
    {
        if (! slots[s].active()) continue;
        const int d = slots[s].dst;
        if (! modTouched[d]) continue;
        modTouched[d] = false;
        const int block = d / 100, idx = d % 100;
        RangedAudioParameter* prm = nullptr;
        float* target = nullptr;
        if ((block == 1 || block == 2) && idx < LP::count) { prm = layerParam[block - 1][idx]; target = &cur[block - 1][idx]; }
        else if (block == 3 && idx < GP::count) { prm = globalParam[idx]; target = &curG[idx]; }
        if (prm == nullptr) continue;
        const float norm = jlimit (0.0f, 1.0f, prm->convertTo0to1 (*target) + modSum[d]);
        *target = prm->convertFrom0to1 (norm);
        if (editorOpen.load (std::memory_order_relaxed)) liveDest[d].store (norm, std::memory_order_relaxed);
    }

    // 4. layers
    const float bendSemis = pitchBend * curG[GP::pb_range];
    float* outL = out.getWritePointer (0) + start;
    float* outR = out.getWritePointer (1) + start;
    const bool muted[2] = { curG[GP::sum_mute1] > 0.5f, curG[GP::sum_mute2] > 0.5f };

    for (int l = 0; l < 2; ++l)
    {
        float* bl = layerBuf[l].getWritePointer (0);
        float* br = layerBuf[l].getWritePointer (1);
        if (muted[l])
        {
            std::fill (bl, bl + len, 0.0f);
            std::fill (br, br + len, 0.0f);
            continue;
        }
        if (osIndex == 0)
            layers[l].render (cur[l], bl, br, len, bendSemis);
        else
        {
            auto& os = *oversamplers[l][osIndex - 1];
            juce::dsp::AudioBlock<const float> zin (zeros.getArrayOfReadPointers(), 2, (size_t) len);
            auto up = os.processSamplesUp (zin);
            layers[l].render (cur[l], up.getChannelPointer (0), up.getChannelPointer (1), (int) up.getNumSamples(), bendSemis);
            float* chans[2] = { bl, br };
            juce::dsp::AudioBlock<float> down (chans, 2, (size_t) len);
            os.processSamplesDown (down);
        }
        for (int i = 0; i < len; ++i)
        {
            bl[i] = dc[l][0].process (bl[i]);
            br[i] = dc[l][1].process (br[i]);
        }
    }

    // 5. per-layer effects
    {
        float* l1 = layerBuf[0].getWritePointer (0); float* r1 = layerBuf[0].getWritePointer (1);
        float* l2 = layerBuf[1].getWritePointer (0); float* r2 = layerBuf[1].getWritePointer (1);
        saturators[0].process (l1, r1, len, (int) curG[GP::fx1_sattype], curG[GP::fx1_sat]);
        const double dTime = curG[GP::dly_sync] > 0.5f ? Params::divisionInBeats ((int) curG[GP::dly_div]) * 60.0 / t.bpm : (double) curG[GP::dly_time];
        delay.process (l1, r1, len, (float) std::min (2.4, dTime), curG[GP::dly_fb], curG[GP::dly_hpf], curG[GP::dly_mix] / 100.0f, curG[GP::dly_stereo] > 0.5f);

        saturators[1].process (l2, r2, len, (int) curG[GP::fx2_sattype], curG[GP::fx2_sat]);
        const bool choSync = curG[GP::cho_sync] > 0.5f;
        const double beats = Params::divisionInBeats ((int) curG[GP::cho_div]);
        const double choHz = choSync ? t.bpm / 60.0 / beats : (double) curG[GP::cho_rate];
        double hostPhase = t.ppq / beats;
        hostPhase -= std::floor (hostPhase);
        chorus.process (l2, r2, len, (float) choHz, curG[GP::cho_depth], curG[GP::cho_hpf], curG[GP::cho_mix] / 100.0f,
                        curG[GP::cho_expand] > 0.5f, choSync && t.playing, hostPhase);

        // 6. summing
        auto gainOf = [] (float db) { return db <= -59.9f ? 0.0f : Decibels::decibelsToGain (db); };
        const float g1 = gainOf (curG[GP::sum_lvl1]), g2 = gainOf (curG[GP::sum_lvl2]);
        const float p1 = curG[GP::sum_pan1], p2 = curG[GP::sum_pan2];
        const float g1l = g1 * std::min (1.0f, 1.0f - p1), g1r = g1 * std::min (1.0f, 1.0f + p1);
        const float g2l = g2 * std::min (1.0f, 1.0f - p2), g2r = g2 * std::min (1.0f, 1.0f + p2);
        for (int i = 0; i < len; ++i)
        {
            outL[i] = l1[i] * g1l + l2[i] * g2l;
            outR[i] = r1[i] * g1r + r2[i] * g2r;
        }
    }

    inMeter.process (outL, outR, len);
    if (curG[GP::comp_on] > 0.5f)
        compressor.process (outL, outR, len, curG[GP::comp_attack], curG[GP::comp_ratio], curG[GP::comp_thresh],
                            curG[GP::comp_mix] / 100.0f, curG[GP::comp_fet] > 0.5f);
    else
        compressor.reset();
    meterGr = compressor.getGainReductionDb();

    const float master = curG[GP::master_vol] <= -59.9f ? 0.0f : Decibels::decibelsToGain (curG[GP::master_vol]);
    for (int i = 0; i < len; ++i)
    {
        outL[i] = jlimit (-4.0f, 4.0f, outL[i] * master);
        outR[i] = jlimit (-4.0f, 4.0f, outR[i] * master);
    }
    outMeter.process (outL, outR, len);
}

//==============================================================================
void ResoOGProcessor::setParamReal (const String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
    else
        jassertfalse;
}

float ResoOGProcessor::getParamReal (const String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

int ResoOGProcessor::findFreeSlot() const
{
    for (int s = 0; s < Params::numModSlots; ++s)
        if ((int) getParamReal (Params::modID (s, "src")) == 0 || (int) getParamReal (Params::modID (s, "dst")) == 0)
            return s;
    return -1;
}

int ResoOGProcessor::addModulation (int source, const String& destID, float amount)
{
    const int code = Mod::destCode (destID);
    if (code == 0 || source <= 0) return -1;
    for (int s = 0; s < Params::numModSlots; ++s)   // same routing already there: just enable it
        if ((int) getParamReal (Params::modID (s, "src")) == source && (int) getParamReal (Params::modID (s, "dst")) == code)
        {
            setParamReal (Params::modID (s, "on"), 1.0f);
            return s;
        }
    const int s = findFreeSlot();
    if (s < 0) return -1;
    for (auto& d : Params::modSlotDefs())
        if (auto* p = param (Params::modID (s, d.id))) p->setValueNotifyingHost (p->getDefaultValue());
    setParamReal (Params::modID (s, "src"), (float) source);
    setParamReal (Params::modID (s, "dst"), (float) code);
    setParamReal (Params::modID (s, "amt"), amount);
    setParamReal (Params::modID (s, "on"), 1.0f);
    return s;
}

void ResoOGProcessor::removeModulation (int s)
{
    for (auto& d : Params::modSlotDefs())
        if (auto* p = param (Params::modID (s, d.id)))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getDefaultValue());
            p->endChangeGesture();
        }
}

Array<int> ResoOGProcessor::slotsForDestination (const String& destID) const
{
    Array<int> a;
    const int code = Mod::destCode (destID);
    if (code == 0) return a;
    for (int s = 0; s < Params::numModSlots; ++s)
        if ((int) getParamReal (Params::modID (s, "dst")) == code && (int) getParamReal (Params::modID (s, "src")) > 0)
            a.add (s);
    return a;
}

//==============================================================================
void ResoOGProcessor::resetToDefaults()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

int ResoOGProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

void ResoOGProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < getNumPrograms())
        loadFactoryPreset (index);
}

const String ResoOGProcessor::getProgramName (int index)
{
    if (index >= 0 && index < getNumPrograms()) return factoryPresets()[(size_t) index].name;
    return {};
}

void ResoOGProcessor::loadFactoryPreset (int index)
{
    const auto& all = factoryPresets();
    if (index < 0 || index >= (int) all.size()) return;
    const auto& pr = all[(size_t) index];
    resetToDefaults();
    for (auto& v : pr.values)
        if (auto* p = param (v.first))
            p->setValueNotifyingHost (p->convertTo0to1 (v.second));
    int slot = 0;
    for (auto& m : pr.mods)
    {
        if (slot >= Params::numModSlots) break;
        auto set = [&] (const char* id, float v) { if (auto* p = param (Params::modID (slot, id))) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        set ("on", 1.0f);
        set ("src", (float) m.source);
        set ("dst", (float) Mod::destCode (m.dest));
        set ("amt", m.amount);
        set ("bi", m.bipolar ? 1.0f : 0.0f);
        set ("fn", (float) m.function);
        set ("fnamt", m.functionAmount);
        set ("ctl", (float) m.controller);
        ++slot;
    }
    currentPreset = index;
    uiState.setProperty ("presetName", String (pr.name), nullptr);
    uiState.setProperty ("presetIndex", index, nullptr);
    if (history) history->resync();
}

void ResoOGProcessor::getStateInformation (MemoryBlock& dest)
{
    ValueTree root ("ResoOG");
    root.setProperty ("version", JucePlugin_VersionString, nullptr);
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (uiState.createCopy(), nullptr);
    if (history) root.appendChild (history->toValueTree(), nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, dest);
}

void ResoOGProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return;
    auto root = ValueTree::fromXml (*xml);
    if (! root.hasType ("ResoOG")) return;
    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    auto ui = root.getChildWithName ("UI");
    if (ui.isValid()) uiState.copyPropertiesAndChildrenFrom (ui, nullptr);
    currentPreset = (int) uiState.getProperty ("presetIndex", 0);
    if (history)
    {
        history->fromValueTree (root.getChildWithName ("AB"));
        history->resync();
    }
}

AudioProcessorEditor* ResoOGProcessor::createEditor() { return new ResoOGEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ResoOGProcessor(); }
