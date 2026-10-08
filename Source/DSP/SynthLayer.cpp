#include "SynthLayer.h"

using namespace dsp;

namespace
{
// Mixer bus: clean up to about 0.6, then progressive soft saturation (CP-3 style)
inline float mixerSat (float x)
{
    const float x2 = x * x;
    return x / std::sqrt (std::sqrt (1.0f + x2 * x2));
}

inline float log2Hz (float hz) { return std::log2 (std::max (1.0f, hz)); }
}

void SynthLayer::prepare (double rate, uint32_t seed)
{
    noise.seed (seed * 7919u + 13u);
    rnd[0].seed (seed * 104729u + 1u);
    rnd[1].seed (seed * 1299709u + 7u);
    setSampleRate (rate);
    reset();
}

void SynthLayer::setSampleRate (double rate)
{
    fs = rate;
    fenv.setSampleRate (fs);
    amp.setSampleRate (fs);
    menv.setSampleRate (fs);
}

void SynthLayer::reset()
{
    osc1.reset (0.0); osc2.reset (0.0); sub.reset (0.0);
    noise.reset();
    lpfL.reset(); lpfR.reset(); hpfL.reset(); hpfR.reset(); subFilter.reset(); sideHp.reset();
    xoverSub.reset(); xoverL.reset(); xoverR.reset();
    fenv.hardReset(); amp.hardReset(); menv.hardReset();
    rampsPrimed = false;
    currentNote = -1;
    driveMeter = drivePeak = 0.0f;
}

void SynthLayer::noteOn (int note, float vel, bool legato, const float* p, bool glideOnLegatoOnly)
{
    velocity = vel;
    accented = p[LP::accent] > 0.5f && vel > 96.0f / 127.0f;
    const int mode = (int) p[LP::legato];   // 0 Re-Trig, 1 Legato, 2 Add

    const bool glide = p[LP::osc_glide] > 0.0005f && currentNote >= 0 && (! glideOnLegatoOnly || legato);
    targetPitch = note;
    if (! glide) pitch = note;
    currentNote = note;

    const bool retrigger = ! (legato && mode == 1) || ! amp.isActive();
    if (retrigger)
    {
        const bool fromZero = mode == 0;
        fenv.noteOn (fromZero);
        amp.noteOn (fromZero);
        menv.noteOn (fromZero);
        if (p[LP::osc_keyreset] > 0.5f)
            pendingPhaseReset = true;
        for (int i = 0; i < 3; ++i)
            if (p[LP::lfo (i, LP::kbreset)] > 0.5f && p[LP::lfo (i, LP::sync)] < 0.5f)
                lfo[i].reset (p[LP::lfo (i, LP::phase)] / 360.0);
    }
}

void SynthLayer::noteOff()
{
    fenv.noteOff();
    amp.noteOff();
    menv.noteOff();
}

void SynthLayer::allOff()
{
    fenv.hardReset();
    amp.hardReset();
    menv.hardReset();
    currentNote = -1;
}

void SynthLayer::updateModulators (const float* p, double seconds, const Transport& t, float* out)
{
    menv.set (p[LP::menv_delay], p[LP::menv_a], p[LP::menv_hold], p[LP::menv_d], p[LP::menv_s] / 100.0f, p[LP::menv_r]);
    menv.advance ((int) std::lround (seconds * fs));

    for (int i = 0; i < 3; ++i)
    {
        const bool synced = p[LP::lfo (i, LP::sync)] > 0.5f;
        const double offset = p[LP::lfo (i, LP::phase)] / 360.0;
        if (synced)
        {
            const double beats = Params::divisionInBeats ((int) p[LP::lfo (i, LP::div)]);
            if (t.playing) lfo[i].setPhase (t.ppq / beats + offset);
            else           lfo[i].advance (t.bpm / 60.0 / beats, seconds);
        }
        else
            lfo[i].advance (p[LP::lfo (i, LP::rate)], seconds);
        lastSources[Mod::Lfo1 + i] = Lfo::shape ((int) p[LP::lfo (i, LP::wave)], lfo[i].getPhase());
    }

    for (int i = 0; i < 2; ++i)
    {
        const bool synced = p[LP::rnd (i, LP::rsync)] > 0.5f;
        const double beats = Params::divisionInBeats ((int) p[LP::rnd (i, LP::rdiv)]);
        const double hz = synced ? t.bpm / 60.0 / beats : (double) p[LP::rnd (i, LP::rrate)];
        double hostPhase = t.ppq / beats;
        hostPhase -= std::floor (hostPhase);
        lastSources[Mod::Random1 + i] = rnd[i].process ((int) p[LP::rnd (i, LP::rmode)], hz, p[LP::rnd (i, LP::rslew)], seconds,
                                                         synced && t.playing, hostPhase);
    }

    lastSources[Mod::FilterEnv] = fenv.getValue();
    lastSources[Mod::AmpEnv] = amp.getValue();
    lastSources[Mod::ModEnv] = menv.getValue();
    lastSources[Mod::Accent] = accented ? 1.0f : 0.0f;
    for (int i = 0; i < numSources; ++i)
        out[i] = lastSources[i];
}

void SynthLayer::render (const float* p, float* L, float* R, int n, float bend)
{
    fenv.set (0.0f, p[LP::fenv_a], 0.0f, p[LP::fenv_d], p[LP::fenv_s] / 100.0f, p[LP::fenv_r]);
    amp.set (0.0f, p[LP::aenv_a], 0.0f, p[LP::aenv_d], p[LP::aenv_s] / 100.0f, p[LP::aenv_r]);

    // targets for the per-sample ramps
    float tg[numRamps];
    tg[rFreq] = p[LP::osc_freq];
    tg[rDetune] = p[LP::osc_detune];
    tg[rWave] = p[LP::osc_wave];
    tg[rDuty] = p[LP::osc_duty] / 100.0f;
    tg[rColour] = p[LP::noise_color];
    tg[rLvl1] = p[LP::mix_osc1] * 0.055f;
    tg[rLvl2] = p[LP::mix_osc2] * 0.055f;
    tg[rLvlN] = p[LP::mix_noise] * 0.07f;
    tg[rLvlS] = p[LP::mix_sub] * 0.07f;
    tg[rLpf] = log2Hz (p[LP::lpf_cutoff]);
    tg[rLpfK] = 4.25f * std::pow (p[LP::lpf_res] / 10.0f, 1.25f);
    tg[rLpfEg] = p[LP::lpf_eg] / 10.0f * 7.0f;
    tg[rHpf] = log2Hz (p[LP::hpf_cutoff]);
    tg[rHpfR] = 1.0f / (2.0f * 0.5f * std::pow (24.0f, p[LP::hpf_res] / 10.0f));
    tg[rHpfEg] = p[LP::hpf_eg] / 10.0f * 7.0f;
    tg[rSub] = log2Hz (p[LP::subf_cutoff]);
    tg[rSubR] = 1.0f / (2.0f * 0.5f * std::pow (24.0f, p[LP::subf_res] / 10.0f));
    tg[rSubEg] = p[LP::subf_eg] / 10.0f * 7.0f;
    tg[rSpread] = p[LP::spread] / 10.0f;
    tg[rXover] = p[LP::xover];

    // CP-3 style drive: channels above 7 push the summing stage into saturation (ramped, no zipper)
    float overdrive = 0.0f;
    for (int ch : { LP::mix_osc1, LP::mix_osc2, LP::mix_noise })
        overdrive += std::max (0.0f, p[ch] - 7.0f) / 3.0f;
    tg[rPre] = 1.0f + 1.5f * overdrive;
    tg[rPost] = 1.0f / std::pow (tg[rPre], 0.6f);
    tg[rSubPre] = 1.0f + 1.5f * std::max (0.0f, p[LP::mix_sub] - 7.0f) / 3.0f;
    tg[rSubPost] = 1.0f / std::pow (tg[rSubPre], 0.6f);

    if (! rampsPrimed)
    {
        for (int i = 0; i < numRamps; ++i) ramps[i].reset (tg[i]);
        rampsPrimed = true;
    }
    else
        for (int i = 0; i < numRamps; ++i) ramps[i].set (tg[i], n);

    if (! amp.isActive() && ! pendingPhaseReset)
    {
        std::fill (L, L + n, 0.0f);
        std::fill (R, R + n, 0.0f);
        for (int i = 0; i < numRamps; ++i) ramps[i].reset (tg[i]);
        driveMeter *= 0.9f;
        return;
    }

    const int oct = (int) p[LP::osc2_oct];
    const bool sync = p[LP::osc_sync] > 0.5f;
    const int order = (int) p[LP::flt_order];
    const bool xoverOn = p[LP::xover_on] > 0.5f;
    const int subWave = (int) p[LP::sub_wave];
    const int subOffset = (int) p[LP::sub_offset];
    const int subMode = (int) p[LP::subf_mode];
    const bool accentOn = p[LP::accent] > 0.5f;
    const float accentEg = accented ? 1.5f : 1.0f;
    const float accentAmp = accentOn ? (accented ? 1.0f : 0.8f) : 1.0f;
    sub.setShape (subWave == 0 ? 0.0f : (subWave == 1 ? 3.0f : 4.0f), 0.5f);

    const float osc2Phase = p[LP::osc2_phase] / 360.0f;
    const float subPhase = p[LP::sub_phase] / 360.0f;
    if (std::abs (osc2Phase - appliedOsc2Phase) > 1.0e-4f)
    {
        if (! sync) osc2.shift (osc2Phase - appliedOsc2Phase);
        appliedOsc2Phase = osc2Phase;
    }
    if (std::abs (subPhase - appliedSubPhase) > 1.0e-4f)
    {
        sub.shift (subPhase - appliedSubPhase);
        appliedSubPhase = subPhase;
    }

    const double glideT = p[LP::osc_glide];
    const double glideCoef = glideT < 0.0005 ? 1.0 : 1.0 - std::exp (-1.0 / (glideT / 3.0 * fs));
    const float nyq = (float) std::min (24000.0, 0.45 * fs);
    const float piOverFs = (float) (pi / fs);
    const float sideG = std::tan (piOverFs * 120.0f);
    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        if (pendingPhaseReset && amp.getStage() != Envelope::Kill)
        {
            osc1.reset (0.0);
            osc2.reset (osc2Phase);
            sub.reset (subPhase);
            pendingPhaseReset = false;
        }

        pitch += (targetPitch - pitch) * glideCoef;
        const float freq = ramps[rFreq].next(), det = ramps[rDetune].next();
        const float wave = ramps[rWave].next(), duty = ramps[rDuty].next();
        const double m1 = pitch + bend + freq;
        const double m2 = m1 + 12.0 * oct + det;
        const double ms = (subOffset == 1 ? m1 : pitch + bend) - (subOffset == 2 ? 24.0 : 12.0);

        osc1.setShape (wave, duty);
        osc2.setShape (wave, duty);
        bool wrapped = false; double wrapX = 0.0;
        const float o1 = osc1.tick (mtof (m1) / fs, wrapped, wrapX);
        float o2;
        if (sync && wrapped) o2 = osc2.tickSynced (mtof (m2) / fs, wrapX, osc2Phase);
        else { bool w; double x; o2 = osc2.tick (mtof (m2) / fs, w, x); }
        bool sw; double sx;
        const float so = sub.tick (mtof (ms) / fs, sw, sx);
        const float nz = noise.process (ramps[rColour].next());

        const float g1 = ramps[rLvl1].next(), g2 = ramps[rLvl2].next(), gn = ramps[rLvlN].next(), gs = ramps[rLvlS].next();
        const float spread = ramps[rSpread].next();
        const float a1L = o1 * g1, a1R = a1L * (1.0f - spread);
        const float a2R = o2 * g2, a2L = a2R * (1.0f - spread);
        const float noiseSig = nz * gn;

        const float fe = fenv.next() * accentEg;
        const float ae = amp.next();

        float busL, busR, noiseBus = 0.0f;
        if (order == 2)
        {
            busL = a1L + a2L;
            busR = a1R + a2R;
            noiseBus = mixerSat (noiseSig);
        }
        else
        {
            busL = a1L + a2L + noiseSig;
            busR = a1R + a2R + noiseSig;
        }
        const float preGain = ramps[rPre].next(), postGain = ramps[rPost].next();
        busL *= preGain;
        busR *= preGain;
        peak = std::max (peak, std::max (std::abs (busL), std::abs (busR)));
        busL = mixerSat (busL) * postGain;
        busR = mixerSat (busR) * postGain;

        // main filters
        const float lpfHz = clampf (std::exp2 (ramps[rLpf].next() + ramps[rLpfEg].next() * fe), 8.0f, nyq);
        const float k = ramps[rLpfK].next();
        const float gl = std::tan (piOverFs * lpfHz);
        const float hpfHz = clampf (std::exp2 (ramps[rHpf].next() + ramps[rHpfEg].next() * fe), 8.0f, nyq);
        const float hr = ramps[rHpfR].next();
        const float gh = std::tan (piOverFs * hpfHz);

        float vL, vR;
        if (order == 0)
        {
            vL = hpfL.process (lpfL.process (busL, gl, k), gh, hr).hp;
            vR = hpfR.process (lpfR.process (busR, gl, k), gh, hr).hp;
        }
        else if (order == 1)
        {
            vL = 0.7f * (lpfL.process (busL, gl, k) + hpfL.process (busL, gh, hr).hp);
            vR = 0.7f * (lpfR.process (busR, gl, k) + hpfR.process (busR, gh, hr).hp);
        }
        else
        {
            const float nh = hpfL.process (noiseBus, gh, hr).hp;
            vL = lpfL.process (busL, gl, k) + nh;
            vR = lpfR.process (busR, gl, k) + nh;
        }

        // sub path
        const float subHz = clampf (std::exp2 (ramps[rSub].next() + ramps[rSubEg].next() * fe), 8.0f, nyq);
        const float sr = ramps[rSubR].next();
        const auto sf = subFilter.process (mixerSat (so * gs * ramps[rSubPre].next()) * ramps[rSubPost].next(), std::tan (piOverFs * subHz), sr);
        float subOut = subMode == 2 ? sf.lp : (subMode == 1 ? sf.bp * 2.0f * sr : sf.hp);

        // crossover: low pass on the sub (left) or high pass on the oscillators (right); lows stay mono
        const float xv = ramps[rXover].next();
        if (xoverOn)
        {
            if (xv < -0.01f)
                subOut = xoverSub.lowpass (subOut, std::tan (piOverFs * std::min (nyq, Params::crossoverSubLP (xv))));
            else if (xv > 0.01f)
            {
                const float g = std::tan (piOverFs * Params::crossoverVcoHP (xv));
                vL = xoverL.highpass (vL, g);
                vR = xoverR.highpass (vR, g);
            }
            const float mid = 0.5f * (vL + vR);
            const float side = sideHp.process (0.5f * (vL - vR), sideG, 0.70710678f).hp;
            vL = mid + side;
            vR = mid - side;
        }

        const float gain = ae * accentAmp * 0.5f;
        L[i] = (vL + subOut) * gain;
        R[i] = (vR + subOut) * gain;
    }

    drivePeak = peak;
    const float drive = clampf ((peak - 0.6f) / 0.8f, 0.0f, 1.0f);
    driveMeter = std::max (drive, driveMeter * 0.95f);
}
