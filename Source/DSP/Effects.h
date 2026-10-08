#pragma once

#include "Primitives.h"
#include <vector>
#include <atomic>

// Output stage effects. Buffers are allocated in prepare() only.
namespace fx
{
// Tube / Tape / Drive saturation with loudness compensation
class Saturator
{
public:
    enum Type { Off = 0, Tube, Tape, Drive };
    void prepare (double fs) { sr = fs; reset(); dcL.setup (fs); dcR.setup (fs); }
    void reset() { tapeL.reset(); tapeR.reset(); dcL.reset(); dcR.reset(); }
    void process (float* L, float* R, int n, int type, float amount);   // amount 0..10

private:
    double sr = 48000.0;
    dsp::OnePole tapeL, tapeR;
    dsp::DcBlocker dcL, dcR;
};

// Bass-friendly stereo delay: high pass inside the feedback loop, optional ping-pong
class StereoDelay
{
public:
    void prepare (double fs, double maxSeconds);
    void reset();
    // time in seconds, feedback 0..10, hpf in Hz, mix 0..1
    void process (float* L, float* R, int n, float timeS, float feedback, float hpfHz, float mix, bool pingPong);

private:
    float read (const std::vector<float>& buf, double delaySamples) const;
    double sr = 48000.0;
    std::vector<float> bufL, bufR;
    int size = 0, pos = 0;
    double smoothTime = 0.3;
    dsp::Svf hpL, hpR;
    dsp::OnePole lpL, lpR;
};

// Stereo chorus on the high band only, with stereo expand
class Chorus
{
public:
    void prepare (double fs);
    void reset();
    void process (float* L, float* R, int n, float rateHz, float depth, float hpfHz, float mix, bool expand,
                  bool hostSync, double hostPhase);

private:
    double sr = 48000.0;
    std::vector<float> bufL, bufR;
    int size = 0, pos = 0;
    double phase = 0.0;
    dsp::OnePole splitL, splitR;
};

// Feedback FET-style compressor with automatic make-up gain and parallel mix
class Compressor
{
public:
    void prepare (double fs) { sr = fs; reset(); }
    void reset() { env = 0.0f; lastOutL = lastOutR = 0.0f; gr = 0.0f; }
    void process (float* L, float* R, int n, float attackMs, float ratio, float thresholdDb, float mix, bool fet);
    float getGainReductionDb() const { return gr; }

private:
    double sr = 48000.0;
    float env = 0.0f, lastOutL = 0.0f, lastOutR = 0.0f, gr = 0.0f, releaseMs = 120.0f;
};

// K-14 RMS meters and phase correlation (300 ms integration)
class Meter
{
public:
    void prepare (double fs) { coef = (float) (1.0 - std::exp (-1.0 / (0.3 * fs))); msL = msR = 0.0f; corr = 0.0f; }
    void process (const float* L, const float* R, int n);
    float rmsDb (int ch) const;
    float correlation() const { return corr; }
    float peakDb() const;
    void decayPeak() { peak *= 0.9f; }

private:
    float coef = 0.001f, msL = 0.0f, msR = 0.0f, lr = 0.0f, corr = 0.0f, peak = 0.0f;
};
}
