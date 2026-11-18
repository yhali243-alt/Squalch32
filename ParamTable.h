// Central description of every global parameter: id, display name, range, and
// which field of sq::Params it drives. Used by the plugin (APVTS layout and
// per-block reading) and by presets, so nothing is defined twice.
#pragma once
#include "SquelchEngine.h"
#include <string>

namespace sq
{
inline constexpr const char* kOctNames[]     = { "-2", "-1", "0", "+1", "+2" };
inline constexpr const char* kSubOctNames[]  = { "-1", "-2" };
inline constexpr const char* kModeNames[]    = { "LP", "BP", "HP" };

struct Desc
{
    enum Kind { Float, Choice, Bool, Int };
    const char* id;
    const char* name;
    Kind  kind;
    float min, max, centre;              // centre: value at knob middle (skew), <= min means linear
    const char* const* choices; int numChoices;
    float Params::* f;
    int   Params::* i;
    bool  Params::* b;
    const char* unit;
    bool bipolar;
};

#define SQF(id, name, mn, mx, ctr, field, unit, bip) Desc { id, name, Desc::Float, mn, mx, ctr, nullptr, 0, &Params::field, nullptr, nullptr, unit, bip }
#define SQC(id, name, arr, n, field)                 Desc { id, name, Desc::Choice, 0, (float) (n - 1), -1, arr, n, nullptr, &Params::field, nullptr, "", false }
#define SQI(id, name, mn, mx, field)                 Desc { id, name, Desc::Int, mn, mx, -1, nullptr, 0, nullptr, &Params::field, nullptr, "", false }
#define SQB(id, name, field)                         Desc { id, name, Desc::Bool, 0, 1, -1, nullptr, 0, nullptr, nullptr, &Params::field, "", false }

inline const std::vector<Desc>& paramTable()
{
    static const std::vector<Desc> t = {
        // oscillators
        SQF ("o1wave", "Osc1 Wave", 0, 3, -1, osc1Wave, "", false),
        SQC ("o1oct",  "Osc1 Oct",  kOctNames, 5, osc1Oct),
        SQF ("o1semi", "Osc1 Semi", -12, 12, -1, osc1Semi, "st", true),
        SQF ("o1fine", "Osc1 Fine", -100, 100, -1, osc1Fine, "ct", true),
        SQF ("o2wave", "Osc2 Wave", 0, 3, -1, osc2Wave, "", false),
        SQC ("o2oct",  "Osc2 Oct",  kOctNames, 5, osc2Oct),
        SQF ("o2semi", "Osc2 Semi", -12, 12, -1, osc2Semi, "st", true),
        SQF ("o2fine", "Osc2 Fine", -100, 100, -1, osc2Fine, "ct", true),
        SQB ("o2sync", "Osc2 Sync", osc2Sync),
        SQF ("fm",     "FM 1>2", 0, 1, 0.25f, fmAmount, "", false),
        // mixer
        SQF ("lv1",    "Osc1 Level", 0, 1, -1, osc1Level, "", false),
        SQF ("lv2",    "Osc2 Level", 0, 1, -1, osc2Level, "", false),
        SQF ("lvsub",  "Sub Level",  0, 1, -1, subLevel, "", false),
        SQC ("suboct", "Sub Oct", kSubOctNames, 2, subOct),
        SQF ("lvnoise","Noise", 0, 1, 0.25f, noiseLevel, "", false),
        SQF ("fdbk",   "Feedback", 0, 1, -1, feedback, "", false),
        // filter
        SQF ("cutoff", "Cutoff", 20, 20000, 800, cutoff, "Hz", false),
        SQF ("res",    "Resonance", 0, 1.1f, -1, resonance, "", false),
        SQF ("drive",  "Multidrive", 0, 1, -1, drive, "", false),
        SQF ("slope",  "Slope", 0, 3, -1, slope, "", false),
        SQC ("fmode",  "Filter Mode", kModeNames, 3, filterMode),
        SQF ("ktrk",   "Key Track", 0, 1, -1, keyTrack, "", false),
        SQF ("envamt", "Env Amount", -1, 1, -1, envAmount, "", true),
        SQF ("resenv", "Res Env", -1, 1, -1, resEnvAmount, "", true),
        // envelopes
        SQF ("fa", "Flt Attack",  0.0005f, 5, 0.3f, fA, "s", false),
        SQF ("fd", "Flt Decay",   0.005f, 5, 0.4f, fD, "s", false),
        SQF ("fs", "Flt Sustain", 0, 1, -1, fS, "", false),
        SQF ("fr", "Flt Release", 0.005f, 5, 0.4f, fR, "s", false),
        SQB ("floop", "Flt Loop", fLoop),
        SQF ("aa", "Amp Attack",  0.0005f, 5, 0.3f, aA, "s", false),
        SQF ("ad", "Amp Decay",   0.005f, 5, 0.4f, aD, "s", false),
        SQF ("as", "Amp Sustain", 0, 1, -1, aS, "", false),
        SQF ("ar", "Amp Release", 0.005f, 5, 0.4f, aR, "s", false),
        SQF ("velf", "Vel > Filter", 0, 1, -1, velToFilter, "", false),
        SQF ("vela", "Vel > Amp", 0, 1, -1, velToAmp, "", false),
        // performance
        SQF ("glide", "Glide", 0, 2, 0.15f, glide, "s", false),
        SQB ("legato", "Legato", legato),
        SQF ("bend", "Bend Range", 0, 24, -1, bendRange, "st", false),
        // sequencer
        SQB ("seqon", "Seq On", seqOn),
        SQB ("seqretrig", "Seq Retrig", seqRetrig),
        SQI ("steps", "Steps", 1, 32, numSteps),
        SQC ("stepdiv", "Step Length", kDivNames, kNumDivs, stepDiv),
        SQF ("seqamt", "Seq > Cutoff", 0, 6, -1, seqAmount, "oct", false),
        SQF ("seqsmooth", "Smooth", 0, 1, -1, seqSmooth, "", false),
        SQF ("speed", "LFO Speed", 0.25f, 4, 1, speedScale, "x", false),
        SQF ("wander", "Wander", 0, 1, -1, wander, "", false),
        SQF ("swing", "Swing", 0, 0.75f, -1, swing, "", false),
        SQF ("chaos", "Chaos", 0, 1, -1, chaos, "", false),
        SQF ("seqres", "Seq > Res", -1, 1, -1, seqToRes, "", true),
        SQF ("seqpitch", "Seq > Pitch", -1, 1, -1, seqToPitch, "", true),
        SQF ("seqdrive", "Seq > Drive", -1, 1, -1, seqToDrive, "", true),
        SQF ("seqtalk", "Seq > Talk", -1, 1, -1, seqToTalk, "", true),
        // lfo2
        SQF ("l2rate", "LFO2 Rate", 0.01f, 30, 2, lfo2Rate, "Hz", false),
        SQC ("l2shape", "LFO2 Shape", kLfo2ShapeNames, kNumLfo2Shapes, lfo2Shape),
        SQF ("l2pitch", "LFO2 > Pitch", -1, 1, -1, lfo2ToPitch, "", true),
        SQF ("l2cut", "LFO2 > Cutoff", -1, 1, -1, lfo2ToCutoff, "", true),
        SQF ("l2wave", "LFO2 > Wave", -1, 1, -1, lfo2ToWave, "", true),
        SQF ("l2res", "LFO2 > Res", -1, 1, -1, lfo2ToRes, "", true),
        // talk / bite / crush
        SQF ("talkmix", "Talk Mix", 0, 1, -1, talkMix, "", false),
        SQF ("vowel", "Vowel", 0, 1, -1, talkVowel, "", false),
        SQC ("bitetype", "Bite Type", kBiteNames, kNumBites, biteType),
        SQF ("bite", "Bite", 0, 1, -1, biteAmount, "", false),
        SQF ("bitemix", "Bite Mix", 0, 1, -1, biteMix, "", false),
        SQB ("bitepost", "Bite Post", bitePost),
        SQF ("crush", "Crush", 0, 1, -1, crush, "", false),
        // phaser
        SQF ("phmix", "Phaser Mix", 0, 1, -1, phaserMix, "", false),
        SQF ("phrate", "Phaser Rate", 0.01f, 8, 0.5f, phaserRate, "Hz", false),
        SQF ("phdepth", "Phaser Depth", 0, 1, -1, phaserDepth, "", false),
        SQF ("phfb", "Phaser Fdbk", 0, 0.95f, -1, phaserFb, "", false),
        // delay
        SQF ("dlmix", "Delay Mix", 0, 1, -1, delayMix, "", false),
        SQC ("dldiv", "Delay Time", kDivNames, kNumDivs, delayDiv),
        SQF ("dlfb", "Delay Fdbk", 0, 0.95f, -1, delayFb, "", false),
        SQF ("dltone", "Delay Tone", 0, 1, -1, delayTone, "", false),
        SQF ("dlmod", "Delay Wobble", 0, 1, -1, delayMod, "", false),
        SQB ("dlpp", "Ping Pong", pingPong),
        // reverb
        SQF ("rvmix", "Reverb Mix", 0, 1, -1, reverbMix, "", false),
        SQF ("rvsize", "Reverb Size", 0, 1, -1, reverbSize, "", false),
        SQF ("rvdamp", "Reverb Damp", 0, 1, -1, reverbDamp, "", false),
        // master
        SQF ("out", "Output", -24, 12, -1, outGainDb, "dB", true),
    };
    return t;
}

#undef SQF
#undef SQC
#undef SQI
#undef SQB

// per-step parameter IDs:  s<idx>rate, s<idx>shape, s<idx>depth, s<idx>off   (idx = 1..32)
inline std::string stepId (int step, const char* what) { return "s" + std::to_string (step + 1) + what; }

// value of a descriptor's field in a Params instance (as float)
inline float getField (const Params& p, const Desc& d)
{
    if (d.f) return p.*(d.f);
    if (d.i) return (float) (p.*(d.i)) - (d.kind == Desc::Choice && d.choices == kOctNames ? -2.f : 0.f);
    return (p.*(d.b)) ? 1.f : 0.f;
}
inline void setField (Params& p, const Desc& d, float v)
{
    if (d.f) p.*(d.f) = v;
    else if (d.i) p.*(d.i) = (int) std::lround (v) + (d.kind == Desc::Choice && d.choices == kOctNames ? -2 : 0);
    else p.*(d.b) = v > 0.5f;
}
} // namespace sq
