#include "Effects.h"

using namespace dsp;

namespace fx
{
//==============================================================================
void Saturator::process (float* L, float* R, int n, int type, float amount)
{
    if (type == Off || amount <= 0.0f)
        return;

    const float g = std::pow (10.0f, amount * 1.8f / 20.0f);      // up to +18 dB drive
    const float comp = 1.0f / std::pow (g, 0.6f);
    const float tapeCoef = OnePole::coefFor (16000.0 - amount * 900.0, sr);

    for (int i = 0; i < n; ++i)
    {
        float* ch[2] = { L + i, R + i };
        for (int c = 0; c < 2; ++c)
        {
            float x = *ch[c] * g;
            float y;
            if (type == Tube)
                y = x >= 0.0f ? std::tanh (x) : std::tanh (1.35f * x) / 1.35f;   // asymmetric: even harmonics
            else if (type == Tape)
            {
                y = 0.6366198f * std::atan (1.5707963f * x);
                y = (c == 0 ? tapeL : tapeR).lp (y, tapeCoef);
            }
            else
            {
                const float h = clampf (x, -1.5f, 1.5f);
                y = h - h * h * h / 6.75f;
            }
            y *= comp;
            *ch[c] = c == 0 ? dcL.process (y) : dcR.process (y);
        }
    }
}

//==============================================================================
void StereoDelay::prepare (double fs, double maxSeconds)
{
    sr = fs;
    size = (int) (fs * maxSeconds) + 8;
    bufL.assign ((size_t) size, 0.0f);
    bufR.assign ((size_t) size, 0.0f);
    reset();
}

void StereoDelay::reset()
{
    std::fill (bufL.begin(), bufL.end(), 0.0f);
    std::fill (bufR.begin(), bufR.end(), 0.0f);
    pos = 0;
    hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
}

float StereoDelay::read (const std::vector<float>& buf, double d) const
{
    double rp = pos - d;
    while (rp < 0.0) rp += size;
    const int i0 = (int) rp;
    const int i1 = (i0 + 1) % size;
    const float f = (float) (rp - i0);
    return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
}

void StereoDelay::process (float* L, float* R, int n, float timeS, float feedback, float hpfHz, float mix, bool pingPong)
{
    if (size == 0)
        return;
    const float fb = clampf (feedback / 10.0f, 0.0f, 1.0f) * 0.97f;
    const float g = std::tan ((float) (pi * std::min ((double) hpfHz, 0.45 * sr) / sr));
    const float lpc = OnePole::coefFor (9000.0, sr);
    const float dry = std::min (1.0f, 2.0f * (1.0f - mix));
    const float wet = std::min (1.0f, 2.0f * mix);
    const double target = clampf (timeS, 0.001f, (float) ((size - 4) / sr)) * sr;
    const double smooth = 1.0 - std::exp (-1.0 / (0.06 * sr));   // tape-like glide on time changes

    for (int i = 0; i < n; ++i)
    {
        smoothTime += (target - smoothTime) * smooth;
        const float dl = read (bufL, smoothTime);
        const float dr = read (bufR, smoothTime);

        float fl = lpL.lp (hpL.process (dl, g, 0.70710678f).hp, lpc);
        float fr = lpR.lp (hpR.process (dr, g, 0.70710678f).hp, lpc);

        float inL, inR;
        if (pingPong)
        {
            inL = 0.5f * (L[i] + R[i]) + fr * fb;
            inR = fl * fb;
        }
        else
        {
            inL = L[i] + fl * fb;
            inR = R[i] + fr * fb;
        }
        bufL[(size_t) pos] = std::tanh (inL);
        bufR[(size_t) pos] = std::tanh (inR);
        pos = (pos + 1) % size;

        // the wet signal is taken after the HPF: repeats never muddy the sub
        L[i] = L[i] * dry + fl * wet;
        R[i] = R[i] * dry + fr * wet;
    }
}

//==============================================================================
void Chorus::prepare (double fs)
{
    sr = fs;
    size = (int) (fs * 0.05) + 4;
    bufL.assign ((size_t) size, 0.0f);
    bufR.assign ((size_t) size, 0.0f);
    reset();
}

void Chorus::reset()
{
    std::fill (bufL.begin(), bufL.end(), 0.0f);
    std::fill (bufR.begin(), bufR.end(), 0.0f);
    pos = 0;
    splitL.reset(); splitR.reset();
}

void Chorus::process (float* L, float* R, int n, float rateHz, float depth, float hpfHz, float mix, bool expand,
                      bool hostSync, double hostPhase)
{
    if (mix <= 0.0f || size == 0)
        return;
    const float sc = OnePole::coefFor (hpfHz, sr);
    const double base = 0.008 * sr, dep = 0.006 * sr * depth / 10.0;
    const double inc = rateHz / sr;
    if (hostSync) phase = hostPhase;

    auto rd = [this] (const std::vector<float>& b, double d)
    {
        double rp = pos - d;
        while (rp < 0.0) rp += size;
        const int i0 = (int) rp;
        const float f = (float) (rp - i0);
        return b[(size_t) i0] + f * (b[(size_t) ((i0 + 1) % size)] - b[(size_t) i0]);
    };

    for (int i = 0; i < n; ++i)
    {
        const float lowL = splitL.lp (L[i], sc), lowR = splitR.lp (R[i], sc);
        const float hiL = L[i] - lowL, hiR = R[i] - lowR;
        bufL[(size_t) pos] = hiL;
        bufR[(size_t) pos] = hiR;

        const double mL = base + dep * (0.5 + 0.5 * std::sin (twoPi * phase));
        const double mR = base + dep * (0.5 + 0.5 * std::sin (twoPi * (phase + 0.25)));
        float wl = rd (bufL, mL), wr = rd (bufR, mR);
        pos = (pos + 1) % size;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;

        if (expand)
        {
            const float m = 0.5f * (wl + wr), s = 0.5f * (wl - wr) * 2.2f;
            wl = m + s;
            wr = m - s;
        }
        L[i] = lowL + hiL * (1.0f - 0.5f * mix) + wl * 0.75f * mix;
        R[i] = lowR + hiR * (1.0f - 0.5f * mix) + wr * 0.75f * mix;
    }
}

//==============================================================================
void Compressor::process (float* L, float* R, int n, float attackMs, float ratio, float thresholdDb, float mix, bool fet)
{
    const float att = (float) (1.0 - std::exp (-1.0 / (std::max (0.01f, attackMs) * 0.001 * sr)));
    const float slope = 1.0f - 1.0f / std::max (1.0f, ratio);
    const float makeup = std::pow (10.0f, -thresholdDb * slope * 0.5f / 20.0f);
    const float knee = 6.0f;
    float maxGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float dl = L[i], dr = R[i];
        // feedback topology (FET): detect the previous output; otherwise feed-forward
        const float det = fet ? std::max (std::abs (lastOutL), std::abs (lastOutR)) : std::max (std::abs (dl), std::abs (dr));
        const float levelDb = 20.0f * std::log10 (det + 1.0e-9f);

        const float over = levelDb - thresholdDb;
        float target = 0.0f;
        if (over > knee * 0.5f) target = over * slope;
        else if (over > -knee * 0.5f) target = slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
        if (fet) target *= 1.6f;   // feedback detection sees reduced levels: compensate the effective ratio

        // program dependent release: longer when compressing hard
        const float rel = (float) (1.0 - std::exp (-1.0 / ((releaseMs + env * 25.0f) * 0.001 * sr)));
        env += (target > env ? att : rel) * (target - env);

        float g = std::pow (10.0f, -env / 20.0f) * makeup;
        float yl = dl * g, yr = dr * g;
        if (fet)
        {
            yl = std::tanh (yl * 1.1f) / 1.1f;
            yr = std::tanh (yr * 1.1f) / 1.1f;
        }
        lastOutL = yl / makeup;
        lastOutR = yr / makeup;
        L[i] = dl + mix * (yl - dl);
        R[i] = dr + mix * (yr - dr);
        maxGr = std::max (maxGr, env);
    }
    gr = maxGr;
}

//==============================================================================
void Meter::process (const float* L, const float* R, int n)
{
    for (int i = 0; i < n; ++i)
    {
        msL += coef * (L[i] * L[i] - msL);
        msR += coef * (R[i] * R[i] - msR);
        lr += coef * (L[i] * R[i] - lr);
        peak = std::max (peak, std::max (std::abs (L[i]), std::abs (R[i])));
    }
    const float den = std::sqrt (msL * msR);
    corr = den > 1.0e-9f ? clampf (lr / den, -1.0f, 1.0f) : 1.0f;
    if (! (msL < 1.0e6f)) msL = 0.0f;
    if (! (msR < 1.0e6f)) msR = 0.0f;
}

float Meter::rmsDb (int ch) const
{
    return 10.0f * std::log10 ((ch == 0 ? msL : msR) + 1.0e-12f);
}

float Meter::peakDb() const
{
    return 20.0f * std::log10 (peak + 1.0e-9f);
}
}
