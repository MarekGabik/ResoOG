#include "FactoryPresets.h"
#include "Params/Parameters.h"

namespace
{
int src (int layer, Mod::LayerSource s) { return Mod::layerSource (layer, s); }
}

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        { "Init", "Basics", "Neutral starting point: one saw layer, open filter envelope. Start here for your own sounds.",
          {}, {} },

        { "Classic Ladder Bass", "Bass", "Round analog bass for techno and house. Tweak LPF CUTOFF for brightness and Filter DECAY for the pluck.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc1", 7.0f }, { "s1_mix_osc2", 6.5f }, { "s1_osc_detune", 0.06f },
            { "s1_lpf_cutoff", 420.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 5.0f },
            { "s1_fenv_d", 0.25f }, { "s1_fenv_s", 0.0f }, { "s1_aenv_d", 0.5f }, { "s1_aenv_s", 70.0f },
            { "s1_sub_wave", 2.0f }, { "s1_mix_sub", 4.0f }, { "fx1_sattype", 1.0f }, { "fx1_sat", 3.0f } }, {} },

        { "Deep Blue S+H", "Bass", "Sharktooth bass with a tempo-synced sample & hold on the cutoff and dub delay. Change Random 1 division for other rhythms.",
          { { "s1_osc_wave", 2.0f }, { "s1_osc_duty", 62.0f }, { "s1_mix_osc1", 8.6f }, { "s1_mix_osc2", 6.2f }, { "s1_osc_detune", 0.08f },
            { "s1_lpf_cutoff", 1200.0f }, { "s1_lpf_res", 6.2f }, { "s1_lpf_eg", 5.6f }, { "s1_hpf_cutoff", 42.0f },
            { "s1_fenv_d", 0.28f }, { "s1_fenv_s", 25.0f }, { "s1_aenv_d", 0.42f }, { "s1_aenv_s", 80.0f }, { "s1_aenv_r", 0.035f },
            { "s1_rnd1_sync", 1.0f }, { "s1_rnd1_div", 14.0f }, { "s1_rnd1_slew", 2.0f }, { "s1_osc_glide", 0.048f },
            { "s1_mix_sub", 7.0f }, { "s1_sub_wave", 2.0f }, { "s1_subf_cutoff", 180.0f }, { "s1_spread", 6.5f },
            { "dly_mix", 25.0f }, { "dly_div", 10.0f }, { "dly_fb", 4.5f }, { "dly_hpf", 420.0f },
            { "fx1_sattype", 1.0f }, { "fx1_sat", 4.5f } },
          { { src (0, Mod::Random1), "s1_lpf_cutoff", 0.2f }, { src (0, Mod::ModEnv), "s1_osc_duty", 0.24f } } },

        { "Sub Foundation", "Sub", "Clean sine sub with a hint of harmonics for small speakers. Raise OSC 1 for more growl.",
          { { "s1_osc_wave", 0.0f }, { "s1_mix_osc1", 3.0f }, { "s1_mix_osc2", 0.0f }, { "s1_mix_sub", 8.0f },
            { "s1_lpf_cutoff", 700.0f }, { "s1_lpf_eg", 1.5f }, { "s1_lpf_res", 0.0f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 0.08f },
            { "comp_on", 1.0f }, { "comp_thresh", -10.0f }, { "comp_ratio", 3.0f } }, {} },

        { "Acid Squelch", "Acid", "Resonant saw with accent and legato glide - play overlapping notes and high velocities. RESONANCE and EG AMOUNT do the talking.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc2", 0.0f }, { "s1_lpf_cutoff", 320.0f }, { "s1_lpf_res", 8.2f }, { "s1_lpf_eg", 7.0f },
            { "s1_fenv_d", 0.18f }, { "s1_fenv_s", 0.0f }, { "s1_aenv_s", 85.0f }, { "s1_accent", 1.0f }, { "s1_legato", 1.0f },
            { "s1_osc_glide", 0.07f }, { "glide_legato", 1.0f }, { "s1_mix_sub", 2.0f },
            { "fx1_sattype", 3.0f }, { "fx1_sat", 5.0f }, { "dly_mix", 18.0f }, { "dly_div", 10.0f } }, {} },

        { "Reese Wide", "Bass", "Two detuned saws spread wide, low end kept mono by the crossover. DETUNE sets the speed of the phasing.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc1", 7.5f }, { "s1_mix_osc2", 7.5f }, { "s1_osc_detune", 0.14f }, { "s1_spread", 8.0f },
            { "s1_xover_on", 1.0f }, { "s1_xover", 0.6f }, { "s1_sub_wave", 0.0f }, { "s1_mix_sub", 6.5f },
            { "s1_lpf_cutoff", 1600.0f }, { "s1_lpf_res", 2.0f }, { "s1_lpf_eg", 2.0f }, { "s1_aenv_s", 100.0f },
            { "s1_lfo1_rate", 0.3f }, { "fx1_sattype", 2.0f }, { "fx1_sat", 4.0f } },
          { { src (0, Mod::Lfo1), "s1_lpf_cutoff", 0.08f } } },

        { "Sync Growl", "Lead", "Hard-synced oscillator 2 swept by the mod envelope. MOD DECAY sets the sweep, DETUNE the base colour.",
          { { "s1_osc_sync", 1.0f }, { "s1_osc_detune", 5.0f }, { "s1_osc_wave", 3.0f }, { "s1_mix_osc1", 2.0f }, { "s1_mix_osc2", 8.0f },
            { "s1_lpf_cutoff", 2600.0f }, { "s1_lpf_res", 2.5f }, { "s1_lpf_eg", 3.0f }, { "s1_menv_a", 0.002f }, { "s1_menv_d", 0.6f },
            { "s1_mix_sub", 5.0f } },
          { { src (0, Mod::ModEnv), "s1_osc_detune", 0.35f } } },

        { "Pulse Pluck", "Pluck", "Narrow pulse with a slow duty-cycle LFO, short plucky envelope. Good for arpeggios.",
          { { "s1_osc_wave", 4.0f }, { "s1_osc_duty", 30.0f }, { "s1_mix_osc2", 0.0f }, { "s1_lpf_cutoff", 900.0f }, { "s1_lpf_eg", 6.0f },
            { "s1_lpf_res", 4.0f }, { "s1_fenv_d", 0.12f }, { "s1_fenv_s", 0.0f }, { "s1_aenv_d", 0.3f }, { "s1_aenv_s", 0.0f }, { "s1_aenv_r", 0.2f },
            { "s1_lfo2_rate", 0.4f }, { "dly_mix", 22.0f }, { "dly_div", 11.0f }, { "dly_fb", 4.0f } },
          { { src (0, Mod::Lfo2), "s1_osc_duty", 0.25f } } },

        { "Dub Techno Stab", "Dub", "Both layers: dark saw stab with chorus on synth 2 and long filtered delay. Turn DELAY FEEDBACK for longer echoes.",
          { { "s1_osc_wave", 3.0f }, { "s1_lpf_cutoff", 650.0f }, { "s1_lpf_res", 5.0f }, { "s1_lpf_eg", 3.0f },
            { "s1_fenv_d", 0.3f }, { "s1_fenv_s", 0.0f }, { "s1_aenv_d", 0.35f }, { "s1_aenv_s", 0.0f }, { "s1_aenv_r", 0.3f },
            { "s2_osc_wave", 4.0f }, { "s2_osc2_oct", 1.0f }, { "s2_lpf_cutoff", 1100.0f }, { "s2_lpf_eg", 2.0f }, { "s2_mix_sub", 0.0f },
            { "s2_aenv_d", 0.35f }, { "s2_aenv_s", 0.0f }, { "s2_aenv_r", 0.3f },
            { "sum_mute2", 0.0f }, { "sum_lvl2", -6.0f }, { "cho_mix", 45.0f }, { "cho_expand", 1.0f },
            { "dly_mix", 38.0f }, { "dly_div", 10.0f }, { "dly_fb", 6.5f }, { "dly_hpf", 450.0f } }, {} },

        { "Techno Rumble", "Techno", "Saturated sine sub with a dark noise layer - the rumbling kick companion. NOISE COLOR changes the texture.",
          { { "s1_osc_wave", 1.0f }, { "s1_mix_osc1", 8.0f }, { "s1_mix_osc2", 0.0f }, { "s1_mix_noise", 2.5f }, { "s1_noise_color", -0.8f },
            { "s1_mix_sub", 7.0f }, { "s1_lpf_cutoff", 240.0f }, { "s1_lpf_res", 4.0f }, { "s1_lpf_eg", 2.0f },
            { "s1_aenv_d", 0.9f }, { "s1_aenv_s", 30.0f }, { "s1_aenv_r", 0.4f },
            { "fx1_sattype", 1.0f }, { "fx1_sat", 7.0f }, { "comp_on", 1.0f }, { "comp_thresh", -14.0f } }, {} },

        { "Duo Split", "Duo", "Duophonic: hold two notes - synth 1 plays the lower as bass, synth 2 the higher as a lead with delay.",
          { { "voice_mode", 1.0f }, { "s1_osc_wave", 3.0f }, { "s1_lpf_cutoff", 500.0f }, { "s1_lpf_eg", 4.0f },
            { "s2_osc_wave", 4.0f }, { "s2_osc_duty", 40.0f }, { "s2_lpf_cutoff", 2400.0f }, { "s2_lpf_res", 3.0f }, { "s2_mix_sub", 0.0f },
            { "s2_osc_glide", 0.08f }, { "sum_mute2", 0.0f }, { "sum_lvl2", -5.0f }, { "sum_pan1", -0.15f }, { "sum_pan2", 0.25f },
            { "cho_mix", 30.0f }, { "dly_mix", 20.0f } }, {} },

        { "Ambient Drone Bass", "Ambient", "Slow, evolving triangle bass: Perlin drift on the detune, a slow LFO on the cutoff. Hold long notes.",
          { { "s1_osc_wave", 1.2f }, { "s1_lpf_cutoff", 500.0f }, { "s1_lpf_res", 3.5f }, { "s1_lpf_eg", 2.0f },
            { "s1_aenv_a", 1.2f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 3.0f }, { "s1_fenv_a", 1.5f }, { "s1_fenv_s", 60.0f }, { "s1_fenv_r", 3.0f },
            { "s1_lfo3_rate", 0.08f }, { "s1_rnd2_rate", 0.2f }, { "s1_rnd2_mode", 2.0f },
            { "dly_mix", 30.0f }, { "dly_fb", 6.0f }, { "dly_div", 7.0f }, { "s1_spread", 4.0f } },
          { { src (0, Mod::Lfo3), "s1_lpf_cutoff", 0.12f }, { src (0, Mod::Random2), "s1_osc_detune", 0.06f } } },

        { "Perlin Growl", "Bass", "Sharktooth growl with a smooth Perlin random moving the filter - never repeats exactly.",
          { { "s1_osc_wave", 2.0f }, { "s1_lpf_cutoff", 700.0f }, { "s1_lpf_res", 6.0f }, { "s1_lpf_eg", 3.0f },
            { "s1_rnd2_rate", 2.5f }, { "s1_rnd2_mode", 2.0f }, { "s1_aenv_s", 90.0f }, { "fx1_sattype", 3.0f }, { "fx1_sat", 4.0f } },
          { { src (0, Mod::Random2), "s1_lpf_cutoff", 0.18f } } },

        { "Bounce Wobble", "Bass", "Synced ramp LFO on the cutoff through the Bounce function: wobbles that fall and bounce. Change LFO 1 division.",
          { { "s1_osc_wave", 3.0f }, { "s1_lpf_cutoff", 300.0f }, { "s1_lpf_res", 5.5f }, { "s1_lpf_eg", 1.0f }, { "s1_aenv_s", 100.0f },
            { "s1_lfo1_sync", 1.0f }, { "s1_lfo1_div", 11.0f }, { "s1_lfo1_wave", 3.0f } },
          { { src (0, Mod::Lfo1), "s1_lpf_cutoff", 0.35f, Mod::Bounce, 40.0f, 0, false } } },
        // ---- Moog-inspired (0.2) ----
        { "Seventies Fat Bass", "Classic", "In the spirit of the Minimoog Model D: two saws pushed into the mixer, round ladder and a short filter bite. Lower CUTOFF for funk, raise EG AMOUNT for more snap.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc1", 8.0f }, { "s1_mix_osc2", 8.0f }, { "s1_osc_detune", 0.05f }, { "s1_sub_wave", 2.0f }, { "s1_mix_sub", 5.0f },
            { "s1_lpf_cutoff", 480.0f }, { "s1_lpf_res", 2.5f }, { "s1_lpf_eg", 5.5f }, { "s1_fenv_d", 0.3f }, { "s1_fenv_s", 35.0f },
            { "s1_aenv_d", 0.5f }, { "s1_aenv_s", 80.0f }, { "s1_aenv_r", 0.08f } }, {} },

        { "Pedal Thunder", "Classic", "Inspired by the Taurus bass pedals: deep saw over a sine two octaves down, slow filter decay and a little glide. Play single long notes.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc1", 7.0f }, { "s1_mix_osc2", 5.0f }, { "s1_osc_detune", -0.08f }, { "s1_sub_offset", 2.0f }, { "s1_mix_sub", 7.0f },
            { "s1_lpf_cutoff", 330.0f }, { "s1_lpf_res", 1.5f }, { "s1_lpf_eg", 4.0f }, { "s1_fenv_d", 0.6f }, { "s1_fenv_s", 20.0f },
            { "s1_aenv_s", 90.0f }, { "s1_aenv_r", 0.3f }, { "s1_osc_glide", 0.06f }, { "comp_on", 1.0f }, { "comp_thresh", -12.0f }, { "comp_ratio", 3.0f } }, {} },

        { "Multidrive Growl", "Classic", "Sub 37 style growl: sharktooth and square driven hard in the mixer plus transistor drive. SATURATION on OUTPUT sets the dirt.",
          { { "s1_osc_wave", 2.4f }, { "s1_mix_osc1", 9.0f }, { "s1_mix_osc2", 8.5f }, { "s1_osc_detune", 0.03f }, { "s1_mix_sub", 6.0f },
            { "s1_lpf_cutoff", 650.0f }, { "s1_lpf_res", 4.0f }, { "s1_lpf_eg", 4.5f }, { "s1_fenv_d", 0.35f }, { "s1_fenv_s", 30.0f },
            { "fx1_sattype", 3.0f }, { "fx1_sat", 5.0f } }, {} },

        { "Sync Sweep Bass", "Classic", "A nod to the Prodigy's hard sync: the mod envelope sweeps oscillator 2 for that tearing attack. MOD DECAY = sweep length.",
          { { "s1_osc_wave", 3.0f }, { "s1_osc_sync", 1.0f }, { "s1_osc_detune", 2.0f }, { "s1_mix_osc1", 3.0f }, { "s1_mix_osc2", 8.0f }, { "s1_mix_sub", 6.0f },
            { "s1_lpf_cutoff", 1500.0f }, { "s1_lpf_res", 2.0f }, { "s1_lpf_eg", 2.5f }, { "s1_menv_a", 0.001f }, { "s1_menv_d", 0.35f } },
          { { src (0, Mod::ModEnv), "s1_osc_detune", 0.3f } } },

        { "Wide Voyager Lead", "Lead", "Voyager-like lead: saw and square an octave apart, glide, vibrato on the mod wheel, stereo spread and delay.",
          { { "s1_osc_wave", 3.4f }, { "s1_osc2_oct", 1.0f }, { "s1_osc_detune", 0.04f }, { "s1_mix_osc1", 7.0f }, { "s1_mix_osc2", 5.0f }, { "s1_mix_sub", 2.0f },
            { "s1_osc_glide", 0.12f }, { "s1_lpf_cutoff", 2000.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 3.0f }, { "s1_spread", 4.0f },
            { "s1_lfo1_rate", 5.5f }, { "dly_mix", 22.0f }, { "dly_div", 10.0f }, { "dly_fb", 4.0f } },
          { { src (0, Mod::Lfo1), "s1_osc_freq", 0.03f, 0, 50.0f, Mod::ModWheel } } },

        { "Patchable Seq Bass", "Classic", "Mother-32 flavour: resonant square for sequences with accent and a stepped random on the cutoff. Play velocities above 96 for accents.",
          { { "s1_osc_wave", 4.0f }, { "s1_osc_duty", 45.0f }, { "s1_mix_osc2", 0.0f }, { "s1_mix_sub", 4.0f },
            { "s1_lpf_cutoff", 400.0f }, { "s1_lpf_res", 6.0f }, { "s1_lpf_eg", 6.0f }, { "s1_fenv_d", 0.15f }, { "s1_fenv_s", 0.0f },
            { "s1_aenv_d", 0.25f }, { "s1_aenv_s", 20.0f }, { "s1_aenv_r", 0.1f }, { "s1_accent", 1.0f },
            { "s1_rnd1_sync", 1.0f }, { "s1_rnd1_div", 14.0f } },
          { { src (0, Mod::Random1), "s1_lpf_cutoff", 0.12f } } },

        { "Subharmonic Drone", "Ambient", "After the Subharmonicon: a fifth on oscillator 2, sub two octaves down, slow filter LFO. Hold one note and let it breathe.",
          { { "s1_osc_wave", 2.0f }, { "s1_osc_detune", 7.0f }, { "s1_mix_osc1", 6.0f }, { "s1_mix_osc2", 5.0f }, { "s1_sub_offset", 2.0f }, { "s1_mix_sub", 6.0f },
            { "s1_lpf_cutoff", 600.0f }, { "s1_lpf_res", 4.5f }, { "s1_lpf_eg", 1.0f }, { "s1_aenv_a", 2.0f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 4.0f },
            { "s1_lfo2_rate", 0.07f }, { "dly_mix", 28.0f }, { "dly_fb", 6.0f }, { "dly_div", 4.0f } },
          { { src (0, Mod::Lfo2), "s1_lpf_cutoff", 0.15f } } },

        { "Stereo Matriarch Pad", "Ambient", "Both layers, wide and slow like a stereo Matriarch patch: chorus on synth 2, ping-pong delay on synth 1. Long attack and release.",
          { { "s1_osc_wave", 3.0f }, { "s1_osc_detune", 0.12f }, { "s1_spread", 7.0f }, { "s1_lpf_cutoff", 900.0f }, { "s1_lpf_res", 2.0f }, { "s1_lpf_eg", 2.0f },
            { "s1_aenv_a", 1.5f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 3.0f }, { "s1_fenv_a", 1.8f }, { "s1_fenv_s", 60.0f }, { "s1_mix_sub", 3.0f },
            { "s2_osc_wave", 1.0f }, { "s2_osc2_oct", 1.0f }, { "s2_lpf_cutoff", 1600.0f }, { "s2_aenv_a", 2.0f }, { "s2_aenv_s", 100.0f }, { "s2_aenv_r", 3.5f }, { "s2_mix_sub", 0.0f },
            { "sum_mute2", 0.0f }, { "sum_lvl2", -5.0f }, { "cho_mix", 55.0f }, { "cho_expand", 1.0f }, { "dly_mix", 35.0f }, { "dly_fb", 6.0f } }, {} },

        { "Phatty Pluck", "Pluck", "Little Phatty style pluck: bright saw closing fast, dotted eighth delay. Shorter FILTER DECAY = tighter.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc2", 5.0f }, { "s1_osc2_oct", 1.0f }, { "s1_lpf_cutoff", 300.0f }, { "s1_lpf_res", 5.0f }, { "s1_lpf_eg", 7.0f },
            { "s1_fenv_d", 0.12f }, { "s1_fenv_s", 0.0f }, { "s1_aenv_d", 0.35f }, { "s1_aenv_s", 0.0f }, { "s1_aenv_r", 0.25f },
            { "dly_mix", 28.0f }, { "dly_div", 10.0f }, { "dly_fb", 4.5f } }, {} },

        { "Pedal Sub Pressure", "Sub", "Taurus-like pure sub weight: sine through tube saturation so it reads on small speakers.",
          { { "s1_osc_wave", 0.0f }, { "s1_mix_osc1", 5.0f }, { "s1_mix_osc2", 0.0f }, { "s1_sub_wave", 0.0f }, { "s1_mix_sub", 8.0f },
            { "s1_lpf_cutoff", 400.0f }, { "s1_lpf_eg", 1.0f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 0.15f },
            { "fx1_sattype", 1.0f }, { "fx1_sat", 6.0f } }, {} },

        { "Rubber Funk", "Bass", "Seventies funk bass with a rubbery filter and legato slides - play overlapping notes.",
          { { "s1_osc_wave", 3.6f }, { "s1_osc_duty", 40.0f }, { "s1_mix_osc2", 5.0f }, { "s1_lpf_cutoff", 420.0f }, { "s1_lpf_res", 5.0f }, { "s1_lpf_eg", 6.0f },
            { "s1_fenv_d", 0.18f }, { "s1_fenv_s", 10.0f }, { "s1_osc_glide", 0.03f }, { "s1_legato", 1.0f }, { "glide_legato", 1.0f } }, {} },

        { "Octave Pop Bass", "Bass", "Eighties synth-pop: oscillator 2 an octave up for the bounce, tight plucky envelope.",
          { { "s1_osc_wave", 3.0f }, { "s1_osc2_oct", 1.0f }, { "s1_mix_osc2", 5.5f }, { "s1_lpf_cutoff", 700.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 4.0f },
            { "s1_fenv_d", 0.2f }, { "s1_fenv_s", 15.0f }, { "s1_aenv_d", 0.3f }, { "s1_aenv_s", 50.0f }, { "s1_aenv_r", 0.08f } }, {} },

        { "Berlin Sequence", "Techno", "Berlin school sequencer bass: resonant saw, very slow LFO opening the filter over bars, sixteenth delay.",
          { { "s1_osc_wave", 3.0f }, { "s1_lpf_cutoff", 650.0f }, { "s1_lpf_res", 6.5f }, { "s1_lpf_eg", 5.0f }, { "s1_fenv_d", 0.2f }, { "s1_fenv_s", 0.0f },
            { "s1_aenv_d", 0.3f }, { "s1_aenv_s", 30.0f }, { "s1_lfo3_sync", 1.0f }, { "s1_lfo3_div", 1.0f }, { "s1_lfo3_wave", 1.0f },
            { "dly_mix", 30.0f }, { "dly_div", 13.0f }, { "dly_fb", 5.0f } },
          { { src (0, Mod::Lfo3), "s1_lpf_cutoff", 0.15f } } },

        { "Detroit Stab", "Techno", "Short square stab with chorus on a second layer and dotted delay - classic Detroit chords played one note at a time.",
          { { "s1_osc_wave", 4.0f }, { "s1_osc_duty", 35.0f }, { "s1_lpf_cutoff", 1100.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 3.0f },
            { "s1_aenv_d", 0.25f }, { "s1_aenv_s", 0.0f }, { "s1_aenv_r", 0.2f }, { "s1_fenv_d", 0.2f }, { "s1_fenv_s", 0.0f },
            { "s2_osc_wave", 3.0f }, { "s2_osc_freq", 7.0f }, { "s2_lpf_cutoff", 1400.0f }, { "s2_aenv_d", 0.25f }, { "s2_aenv_s", 0.0f }, { "s2_aenv_r", 0.2f }, { "s2_mix_sub", 0.0f },
            { "sum_mute2", 0.0f }, { "sum_lvl2", -7.0f }, { "cho_mix", 50.0f }, { "dly_mix", 30.0f }, { "dly_div", 10.0f }, { "dly_fb", 5.5f } }, {} },

        { "Warehouse Reese", "Techno", "Dark detuned Reese for warehouse techno: wide saws, mono crossover, tape saturation and compression.",
          { { "s1_osc_wave", 3.0f }, { "s1_osc_detune", 0.25f }, { "s1_mix_osc1", 8.0f }, { "s1_mix_osc2", 8.0f }, { "s1_spread", 9.0f },
            { "s1_xover_on", 1.0f }, { "s1_xover", 0.6f }, { "s1_mix_sub", 7.0f }, { "s1_lpf_cutoff", 900.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 1.5f },
            { "s1_aenv_s", 100.0f }, { "fx1_sattype", 2.0f }, { "fx1_sat", 6.0f }, { "comp_on", 1.0f }, { "comp_thresh", -14.0f } }, {} },

        { "Acid Square", "Acid", "Squelchy square-wave acid line with accent and slides. Automate CUTOFF and RESONANCE.",
          { { "s1_osc_wave", 4.0f }, { "s1_mix_osc2", 0.0f }, { "s1_mix_sub", 2.0f }, { "s1_lpf_cutoff", 280.0f }, { "s1_lpf_res", 8.5f }, { "s1_lpf_eg", 6.5f },
            { "s1_fenv_d", 0.16f }, { "s1_fenv_s", 0.0f }, { "s1_aenv_s", 85.0f }, { "s1_accent", 1.0f }, { "s1_legato", 1.0f },
            { "s1_osc_glide", 0.06f }, { "glide_legato", 1.0f }, { "dly_mix", 15.0f } }, {} },

        { "Acid Distorted", "Acid", "Saw acid pushed through transistor drive, with an LFO adding movement to the resonance.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc2", 0.0f }, { "s1_lpf_cutoff", 350.0f }, { "s1_lpf_res", 7.5f }, { "s1_lpf_eg", 6.5f },
            { "s1_fenv_d", 0.2f }, { "s1_fenv_s", 5.0f }, { "s1_accent", 1.0f }, { "fx1_sattype", 3.0f }, { "fx1_sat", 7.0f },
            { "s1_lfo2_rate", 0.3f }, { "dly_mix", 20.0f }, { "dly_div", 10.0f } },
          { { src (0, Mod::Lfo2), "s1_lpf_res", 0.1f } } },

        { "Dub Sub Wobble", "Dub", "Soft triangle sub with a slow synced wobble and a long filtered dub delay.",
          { { "s1_osc_wave", 1.0f }, { "s1_mix_osc2", 0.0f }, { "s1_mix_sub", 6.0f }, { "s1_lpf_cutoff", 380.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 1.0f },
            { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 0.2f }, { "s1_lfo1_sync", 1.0f }, { "s1_lfo1_div", 8.0f },
            { "dly_mix", 40.0f }, { "dly_fb", 7.0f }, { "dly_hpf", 500.0f }, { "dly_div", 10.0f } },
          { { src (0, Mod::Lfo1), "s1_lpf_cutoff", 0.1f } } },

        { "Chord Echo Dub", "Dub", "Square pluck with a fifth on synth 2, chorus and long echoes - one finger dub chords.",
          { { "s1_osc_wave", 4.0f }, { "s1_osc_duty", 40.0f }, { "s1_lpf_cutoff", 800.0f }, { "s1_lpf_res", 4.0f }, { "s1_lpf_eg", 3.0f },
            { "s1_aenv_d", 0.3f }, { "s1_aenv_s", 0.0f }, { "s1_aenv_r", 0.3f }, { "s1_fenv_d", 0.25f }, { "s1_fenv_s", 0.0f },
            { "s2_osc_wave", 4.0f }, { "s2_osc_freq", 7.0f }, { "s2_lpf_cutoff", 900.0f }, { "s2_aenv_d", 0.3f }, { "s2_aenv_s", 0.0f }, { "s2_aenv_r", 0.3f }, { "s2_mix_sub", 0.0f },
            { "sum_mute2", 0.0f }, { "sum_lvl2", -6.0f }, { "cho_mix", 40.0f }, { "dly_mix", 42.0f }, { "dly_fb", 7.0f }, { "dly_div", 10.0f }, { "dly_hpf", 600.0f } }, {} },

        { "Breathing Noise Bass", "Ambient", "Triangle bass with pink noise on its own high pass (HPF Noise order) and Perlin drift on the cutoff.",
          { { "s1_osc_wave", 1.0f }, { "s1_mix_noise", 4.0f }, { "s1_noise_color", -0.5f }, { "s1_flt_order", 2.0f }, { "s1_hpf_cutoff", 1500.0f }, { "s1_hpf_res", 3.0f },
            { "s1_lpf_cutoff", 450.0f }, { "s1_lpf_res", 3.0f }, { "s1_lpf_eg", 1.5f }, { "s1_aenv_a", 0.8f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 2.0f },
            { "s1_rnd2_rate", 0.4f }, { "s1_rnd2_mode", 2.0f }, { "dly_mix", 25.0f } },
          { { src (0, Mod::Random2), "s1_lpf_cutoff", 0.12f }, { src (0, Mod::Random1), "s1_hpf_cutoff", 0.1f } } },

        { "Glass Sine Lead", "Lead", "Pure sine and triangle two octaves apart, slow glide, vibrato on the mod wheel - singing lead.",
          { { "s1_osc_wave", 0.5f }, { "s1_osc2_oct", 2.0f }, { "s1_mix_osc1", 7.0f }, { "s1_mix_osc2", 3.0f }, { "s1_mix_sub", 0.0f },
            { "s1_osc_glide", 0.15f }, { "s1_lpf_cutoff", 5000.0f }, { "s1_lpf_eg", 0.0f }, { "s1_aenv_a", 0.03f }, { "s1_aenv_s", 100.0f }, { "s1_aenv_r", 0.4f },
            { "s1_lfo1_rate", 5.0f }, { "dly_mix", 30.0f }, { "dly_div", 10.0f }, { "dly_fb", 5.0f } },
          { { src (0, Mod::Lfo1), "s1_osc_freq", 0.025f, 0, 50.0f, Mod::ModWheel } } },

        { "Velocity Bite", "Bass", "Play soft for a round bass, hard for a biting one: velocity opens the filter envelope.",
          { { "s1_osc_wave", 3.0f }, { "s1_lpf_cutoff", 350.0f }, { "s1_lpf_res", 3.5f }, { "s1_lpf_eg", 2.0f }, { "s1_fenv_d", 0.25f }, { "s1_fenv_s", 10.0f } },
          { { Mod::Velocity, "s1_lpf_eg", 0.35f } } },

        { "Pressure Swell", "Lead", "Aftertouch (or MPE pressure) opens the filter and adds vibrato - for expressive keyboards.",
          { { "s1_osc_wave", 3.0f }, { "s1_osc2_oct", 1.0f }, { "s1_mix_osc2", 5.0f }, { "s1_lpf_cutoff", 600.0f }, { "s1_lpf_res", 4.0f }, { "s1_lpf_eg", 2.0f },
            { "s1_aenv_s", 100.0f }, { "s1_lfo1_rate", 5.5f } },
          { { Mod::Pressure, "s1_lpf_cutoff", 0.3f }, { src (0, Mod::Lfo1), "s1_osc_freq", 0.02f, 0, 50.0f, Mod::Pressure } } },

        { "Duo Bass & Lead", "Duo", "Duophonic split: the lower held note is a fat bass, the higher a square lead with glide and delay.",
          { { "voice_mode", 1.0f }, { "s1_osc_wave", 3.0f }, { "s1_mix_osc2", 7.0f }, { "s1_lpf_cutoff", 450.0f }, { "s1_lpf_eg", 4.5f }, { "s1_fenv_d", 0.3f },
            { "s2_osc_wave", 4.0f }, { "s2_osc_duty", 30.0f }, { "s2_osc2_oct", 1.0f }, { "s2_mix_osc2", 4.0f }, { "s2_mix_sub", 0.0f }, { "s2_lpf_cutoff", 2200.0f },
            { "s2_osc_glide", 0.1f }, { "sum_mute2", 0.0f }, { "sum_lvl2", -6.0f }, { "sum_pan2", 0.2f }, { "dly_mix", 15.0f } }, {} },

        { "Triplet Wobble", "Bass", "Synced eighth-triplet sine wobble on the cutoff. Change LFO 1 DIVISION for other grooves.",
          { { "s1_osc_wave", 3.0f }, { "s1_mix_osc2", 7.0f }, { "s1_osc_detune", 0.1f }, { "s1_lpf_cutoff", 350.0f }, { "s1_lpf_res", 5.0f }, { "s1_lpf_eg", 0.0f },
            { "s1_aenv_s", 100.0f }, { "s1_lfo1_sync", 1.0f }, { "s1_lfo1_div", 12.0f }, { "s1_lfo1_kbreset", 1.0f }, { "s1_lfo1_phase", 270.0f } },
          { { src (0, Mod::Lfo1), "s1_lpf_cutoff", 0.3f } } },
    };
    return presets;
}
