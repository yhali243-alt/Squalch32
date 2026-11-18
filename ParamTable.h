// Every parameter of SqueFM in one place: ids, ranges, and the field it drives.
#pragma once
#include "SqueFMEngine.h"
#include <string>
#include <vector>
#include <functional>

namespace sq
{
inline constexpr const char* kOctNames[]    = { "-2", "-1", "0", "+1", "+2" };
inline constexpr const char* kSubOctNames[] = { "-1", "-2" };
inline constexpr const char* kModeNames[]   = { "LP", "BP", "HP" };
inline constexpr const char* kFmModeNames[] = { "Ratio", "Hz" };

struct Desc
{
    enum Kind { Float, Choice, Bool, Int };
    std::string id, name;
    Kind kind;
    float min = 0, max = 1, centre = -1, def = 0;
    std::vector<std::string> choices;
    std::string unit;
    bool bipolar = false;
    std::function<float (const Params&)> get;
    std::function<void (Params&, float)> set;
};

namespace detail
{
    inline std::vector<std::string> names (const char* const* a, int n) { return std::vector<std::string> (a, a + n); }

    inline Desc F (std::string id, std::string name, float mn, float mx, float ctr, float Params::* f, std::string unit = "", bool bip = false)
    {
        Desc d; d.id = id; d.name = name; d.kind = Desc::Float; d.min = mn; d.max = mx; d.centre = ctr; d.unit = unit; d.bipolar = bip;
        d.get = [f] (const Params& p) { return p.*f; }; d.set = [f] (Params& p, float v) { p.*f = v; }; return d;
    }
    inline Desc C (std::string id, std::string name, std::vector<std::string> ch, int Params::* f, int offset = 0)
    {
        Desc d; d.id = id; d.name = name; d.kind = Desc::Choice; d.choices = ch; d.max = (float) ch.size() - 1;
        d.get = [f, offset] (const Params& p) { return (float) (p.*f - offset); };
        d.set = [f, offset] (Params& p, float v) { p.*f = (int) std::lround (v) + offset; }; return d;
    }
    inline Desc B (std::string id, std::string name, bool Params::* f)
    {
        Desc d; d.id = id; d.name = name; d.kind = Desc::Bool;
        d.get = [f] (const Params& p) { return p.*f ? 1.f : 0.f; }; d.set = [f] (Params& p, float v) { p.*f = v > 0.5f; }; return d;
    }
    inline Desc I (std::string id, std::string name, int mn, int mx, int Params::* f)
    {
        Desc d; d.id = id; d.name = name; d.kind = Desc::Int; d.min = (float) mn; d.max = (float) mx;
        d.get = [f] (const Params& p) { return (float) (p.*f); }; d.set = [f] (Params& p, float v) { p.*f = (int) std::lround (v); }; return d;
    }
}

inline std::string stepId (int i, const char* w) { return "s" + std::to_string (i + 1) + w; }
inline std::string lfoId  (int i, const char* w) { return "l" + std::to_string (i + 1) + w; }
inline std::string slotId (int i, const char* w) { return "x" + std::to_string (i + 1) + w; }
inline std::string macroId (int i) { return "m" + std::to_string (i + 1); }

inline const std::vector<Desc>& paramTable()
{
    static const std::vector<Desc> t = []
    {
        using namespace detail;
        std::vector<Desc> v = {
            // OSC A/B
            F ("waveA", "Osc A Wave", 0, 3, -1, &Params::waveA),
            C ("octA", "Osc A Oct", names (kOctNames, 5), &Params::octA, -2),
            F ("semiA", "Osc A Semi", -12, 12, -1, &Params::semiA, "st", true),
            F ("fineA", "Osc A Fine", -100, 100, -1, &Params::fineA, "ct", true),
            F ("waveB", "Osc B Wave", 0, 3, -1, &Params::waveB),
            C ("octB", "Osc B Oct", names (kOctNames, 5), &Params::octB, -2),
            F ("semiB", "Osc B Semi", -12, 12, -1, &Params::semiB, "st", true),
            F ("fineB", "Osc B Fine", -100, 100, -1, &Params::fineB, "ct", true),
            B ("syncB", "Osc B Sync", &Params::syncB),
            B ("oscReset", "Osc Reset", &Params::oscReset),
            // FM
            C ("fmMode", "FM Mode", names (kFmModeNames, 2), &Params::fmMode),
            F ("fmRatio", "FM Ratio", 0.25f, 16, 2, &Params::fmRatio, "x"),
            F ("fmFine", "FM Fine", -100, 100, -1, &Params::fmFine, "ct", true),
            F ("fmHz", "FM Hz", 0.1f, 2000, 30, &Params::fmHz, "Hz"),
            F ("fmIndex", "FM Index", 0, 1, -1, &Params::fmIndex),
            F ("fmFb", "FM Feedback", 0, 1, -1, &Params::fmFeedback),
            C ("fmTarget", "FM Target", names (kFmTargetNames, 3), &Params::fmTarget),
            // mixer
            F ("lvA", "Osc A Level", 0, 1, -1, &Params::levelA),
            F ("lvB", "Osc B Level", 0, 1, -1, &Params::levelB),
            F ("lvFm", "FM Op Level", 0, 1, -1, &Params::levelFm),
            F ("lvSub", "Sub Level", 0, 1, -1, &Params::levelSub),
            C ("subOct", "Sub Oct", names (kSubOctNames, 2), &Params::subOct),
            F ("lvNoise", "Noise", 0, 1, 0.25f, &Params::levelNoise),
            F ("fdbk", "Feedback", 0, 1, -1, &Params::feedback),
            // ladder
            F ("cutoff", "Cutoff", 20, 20000, 800, &Params::cutoff, "Hz"),
            F ("res", "Resonance", 0, 1.1f, -1, &Params::resonance),
            F ("drive", "Drive", 0, 1, -1, &Params::drive),
            F ("slope", "Slope", 0, 3, -1, &Params::slope),
            C ("fmode", "Filter Mode", names (kModeNames, 3), &Params::filterMode),
            F ("ktrk", "Key Track", 0, 1, -1, &Params::keyTrack),
            F ("envamt", "Env Amount", -1, 1, -1, &Params::envAmount, "", true),
            F ("resenv", "Res Env", -1, 1, -1, &Params::resEnvAmount, "", true),
            // comb
            F ("combMix", "Comb Mix", 0, 1, -1, &Params::combMix),
            F ("combFb", "Comb Feedback", -0.98f, 0.98f, -1, &Params::combFeedback, "", true),
            F ("combDamp", "Comb Damp", 0, 1, -1, &Params::combDamp),
            F ("combSemi", "Comb Tune", -24, 24, -1, &Params::combSemi, "st", true),
            F ("combHz", "Comb Hz", 20, 5000, 400, &Params::combHz, "Hz"),
            B ("combKey", "Comb Key", &Params::combKey),
            // envelopes
            F ("fa", "Flt Attack", 0.0005f, 5, 0.3f, &Params::fA, "s"),
            F ("fd", "Flt Decay", 0.005f, 5, 0.4f, &Params::fD, "s"),
            F ("fs", "Flt Sustain", 0, 1, -1, &Params::fS),
            F ("fr", "Flt Release", 0.005f, 5, 0.4f, &Params::fR, "s"),
            B ("floop", "Flt Loop", &Params::fLoop),
            F ("aa", "Amp Attack", 0.0005f, 5, 0.3f, &Params::aA, "s"),
            F ("ad", "Amp Decay", 0.005f, 5, 0.4f, &Params::aD, "s"),
            F ("as", "Amp Sustain", 0, 1, -1, &Params::aS),
            F ("ar", "Amp Release", 0.005f, 5, 0.4f, &Params::aR, "s"),
            F ("ma", "Mod Attack", 0.0005f, 5, 0.3f, &Params::mA, "s"),
            F ("md", "Mod Decay", 0.005f, 5, 0.4f, &Params::mD, "s"),
            F ("ms", "Mod Sustain", 0, 1, -1, &Params::mS),
            F ("mr", "Mod Release", 0.005f, 5, 0.4f, &Params::mR, "s"),
            B ("mloop", "Mod Loop", &Params::mLoop),
            F ("penv", "Pitch Env", -48, 48, -1, &Params::pitchEnvAmt, "st", true),
            F ("pdec", "Pitch Decay", 0.002f, 1, 0.06f, &Params::pitchEnvDecay, "s"),
            F ("velf", "Vel > Filter", 0, 1, -1, &Params::velToFilter),
            F ("vela", "Vel > Amp", 0, 1, -1, &Params::velToAmp),
            F ("glide", "Glide", 0, 2, 0.15f, &Params::glide, "s"),
            B ("legato", "Legato", &Params::legato),
            F ("bend", "Bend Range", 0, 24, -1, &Params::bendRange, "st"),
            // sequencer globals
            B ("seqOn", "Seq On", &Params::seqOn),
            B ("seqRetrig", "Seq Retrig", &Params::seqRetrig),
            B ("seqReset", "Step Phase Reset", &Params::seqPhaseReset),
            I ("steps", "Steps", 1, 32, &Params::numSteps),
            C ("stepDiv", "Step Length", names (kDivNames, kNumDivs), &Params::stepDiv),
            F ("seqDepth", "Seq Depth", 0, 1, -1, &Params::seqDepth),
            F ("seqSmooth", "Seq Smooth", 0, 1, -1, &Params::seqSmooth),
            F ("speed", "Seq Speed", 0.25f, 4, 1, &Params::speedScale, "x"),
            F ("wander", "Wander", 0, 1, -1, &Params::wander),
            F ("swing", "Swing", 0, 0.75f, -1, &Params::swing),
            F ("chaos", "Seq Chaos", 0, 1, -1, &Params::chaos),
            // spit / chaos sources
            F ("spitRate", "Spit Rate", 0.1f, 40, 4, &Params::spitRate, "Hz"),
            F ("spitDecay", "Spit Decay", 0.002f, 0.4f, 0.05f, &Params::spitDecay, "s"),
            F ("spitRnd", "Spit Random", 0, 1, -1, &Params::spitRandom),
            F ("chaosRate", "Chaos Rate", 0.1f, 40, 4, &Params::chaosRate, "Hz"),
            F ("chaosAmt", "Chaos Amount", 0, 1, -1, &Params::chaosAmt),
            // talk / bite / crush
            F ("talkMix", "Talk Mix", 0, 1, -1, &Params::talkMix),
            F ("vowel", "Vowel", 0, 1, -1, &Params::vowel),
            C ("biteType", "Bite Type", names (kBiteNames, kNumBites), &Params::biteType),
            F ("bite", "Bite", 0, 1, -1, &Params::biteAmount),
            F ("biteMix", "Bite Mix", 0, 1, -1, &Params::biteMix),
            B ("bitePost", "Bite Post", &Params::bitePost),
            F ("crush", "Crush", 0, 1, -1, &Params::crush),
            // fx
            F ("phMix", "Phaser Mix", 0, 1, -1, &Params::phaserMix),
            F ("phRate", "Phaser Rate", 0.01f, 8, 0.5f, &Params::phaserRate, "Hz"),
            F ("phDepth", "Phaser Depth", 0, 1, -1, &Params::phaserDepth),
            F ("phFb", "Phaser Fdbk", 0, 0.95f, -1, &Params::phaserFb),
            F ("dlMix", "Delay Mix", 0, 1, -1, &Params::delayMix),
            C ("dlDiv", "Delay Time", names (kDivNames, kNumDivs), &Params::delayDiv),
            F ("dlFb", "Delay Fdbk", 0, 0.95f, -1, &Params::delayFb),
            F ("dlTone", "Delay Tone", 0, 1, -1, &Params::delayTone),
            F ("dlMod", "Delay Wobble", 0, 1, -1, &Params::delayMod),
            B ("dlPP", "Ping Pong", &Params::pingPong),
            F ("rvMix", "Reverb Mix", 0, 1, -1, &Params::reverbMix),
            F ("rvSize", "Reverb Size", 0, 1, -1, &Params::reverbSize),
            F ("rvDamp", "Reverb Damp", 0, 1, -1, &Params::reverbDamp),
            F ("out", "Output", -24, 12, -1, &Params::outGainDb, "dB", true),
            C ("quality", "Quality", { "Normal 2x", "High 4x" }, &Params::quality),
        };

        // LFOs
        for (int i = 0; i < kNumLfos; ++i)
        {
            const std::string n = "LFO " + std::to_string (i + 1) + " ";
            Desc d;
            d = {}; d.id = lfoId (i, "mode"); d.name = n + "Mode"; d.kind = Desc::Choice; d.choices = names (kSyncHzNames, 2); d.max = 1;
            d.get = [i] (const Params& p) { return (float) p.lfo[(size_t) i].mode; }; d.set = [i] (Params& p, float x) { p.lfo[(size_t) i].mode = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = lfoId (i, "sync"); d.name = n + "Sync Rate"; d.kind = Desc::Choice; d.choices = names (kRateNames, kNumRates); d.max = kNumRates - 1;
            d.get = [i] (const Params& p) { return (float) p.lfo[(size_t) i].sync; }; d.set = [i] (Params& p, float x) { p.lfo[(size_t) i].sync = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = lfoId (i, "hz"); d.name = n + "Hz"; d.kind = Desc::Float; d.min = 0.01f; d.max = 50; d.centre = 2; d.unit = "Hz";
            d.get = [i] (const Params& p) { return p.lfo[(size_t) i].hz; }; d.set = [i] (Params& p, float x) { p.lfo[(size_t) i].hz = x; }; v.push_back (d);
            d = {}; d.id = lfoId (i, "shape"); d.name = n + "Shape"; d.kind = Desc::Choice; d.choices = names (kLfoShapeNames, kNumLfoShapes); d.max = kNumLfoShapes - 1;
            d.get = [i] (const Params& p) { return (float) p.lfo[(size_t) i].shape; }; d.set = [i] (Params& p, float x) { p.lfo[(size_t) i].shape = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = lfoId (i, "retrig"); d.name = n + "Retrig"; d.kind = Desc::Bool;
            d.get = [i] (const Params& p) { return p.lfo[(size_t) i].retrig ? 1.f : 0.f; }; d.set = [i] (Params& p, float x) { p.lfo[(size_t) i].retrig = x > 0.5f; }; v.push_back (d);
        }
        // macros
        for (int i = 0; i < kNumMacros; ++i)
        {
            Desc d; d.id = macroId (i); d.name = "Macro " + std::to_string (i + 1); d.kind = Desc::Float; d.min = 0; d.max = 1;
            d.get = [i] (const Params& p) { return p.macro[(size_t) i]; }; d.set = [i] (Params& p, float x) { p.macro[(size_t) i] = x; }; v.push_back (d);
        }
        // matrix
        for (int i = 0; i < kMatrixSlots; ++i)
        {
            const std::string n = "Mod " + std::to_string (i + 1) + " ";
            Desc d;
            d = {}; d.id = slotId (i, "src"); d.name = n + "Source"; d.kind = Desc::Choice; d.choices = names (kSrcNames, NumSrc); d.max = NumSrc - 1;
            d.get = [i] (const Params& p) { return (float) p.matrix[(size_t) i].src; }; d.set = [i] (Params& p, float x) { p.matrix[(size_t) i].src = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = slotId (i, "via"); d.name = n + "Via"; d.kind = Desc::Choice; d.choices = names (kSrcNames, NumSrc); d.max = NumSrc - 1;
            d.get = [i] (const Params& p) { return (float) p.matrix[(size_t) i].via; }; d.set = [i] (Params& p, float x) { p.matrix[(size_t) i].via = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = slotId (i, "dst"); d.name = n + "Destination"; d.kind = Desc::Choice; d.choices = names (kDstNames, NumDst); d.max = NumDst - 1;
            d.get = [i] (const Params& p) { return (float) p.matrix[(size_t) i].dst; }; d.set = [i] (Params& p, float x) { p.matrix[(size_t) i].dst = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = slotId (i, "amt"); d.name = n + "Amount"; d.kind = Desc::Float; d.min = -1; d.max = 1; d.bipolar = true;
            d.get = [i] (const Params& p) { return p.matrix[(size_t) i].amount; }; d.set = [i] (Params& p, float x) { p.matrix[(size_t) i].amount = x; }; v.push_back (d);
        }
        // steps
        for (int i = 0; i < kMaxSteps; ++i)
        {
            const std::string n = "Step " + std::to_string (i + 1) + " ";
            Desc d;
            d = {}; d.id = stepId (i, "lo"); d.name = n + "Low"; d.kind = Desc::Float; d.min = 0; d.max = 1;
            d.get = [i] (const Params& p) { return p.steps[(size_t) i].lo; }; d.set = [i] (Params& p, float x) { p.steps[(size_t) i].lo = x; }; v.push_back (d);
            d = {}; d.id = stepId (i, "hi"); d.name = n + "High"; d.kind = Desc::Float; d.min = 0; d.max = 1;
            d.get = [i] (const Params& p) { return p.steps[(size_t) i].hi; }; d.set = [i] (Params& p, float x) { p.steps[(size_t) i].hi = x; }; v.push_back (d);
            d = {}; d.id = stepId (i, "shape"); d.name = n + "Shape"; d.kind = Desc::Choice; d.choices = names (kStepShapeNames, kNumStepShapes); d.max = kNumStepShapes - 1;
            d.get = [i] (const Params& p) { return (float) p.steps[(size_t) i].shape; }; d.set = [i] (Params& p, float x) { p.steps[(size_t) i].shape = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = stepId (i, "mode"); d.name = n + "Rate Mode"; d.kind = Desc::Choice; d.choices = names (kSyncHzNames, 2); d.max = 1;
            d.get = [i] (const Params& p) { return (float) p.steps[(size_t) i].mode; }; d.set = [i] (Params& p, float x) { p.steps[(size_t) i].mode = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = stepId (i, "sync"); d.name = n + "Sync Rate"; d.kind = Desc::Choice; d.choices = names (kStepSyncNames, kNumStepSync); d.max = kNumStepSync - 1;
            d.get = [i] (const Params& p) { return (float) p.steps[(size_t) i].sync; }; d.set = [i] (Params& p, float x) { p.steps[(size_t) i].sync = (int) std::lround (x); }; v.push_back (d);
            d = {}; d.id = stepId (i, "hz"); d.name = n + "Hz"; d.kind = Desc::Float; d.min = 0.05f; d.max = 60; d.centre = 4; d.unit = "Hz";
            d.get = [i] (const Params& p) { return p.steps[(size_t) i].hz; }; d.set = [i] (Params& p, float x) { p.steps[(size_t) i].hz = x; }; v.push_back (d);
        }
        // ===================== v4 parameters (appended; never reorder) =====================
        {
            std::vector<Desc> w = {
                B ("flOn", "Filter LFO On", &Params::flOn),
                C ("flMode", "Filter LFO Mode", names (kSyncHzNames, 2), &Params::flMode),
                C ("flSync", "Filter LFO Sync", names (kRateNames, kNumRates), &Params::flSync),
                F ("flHz", "Filter LFO Hz", 0.02f, 40, 2, &Params::flHz, "Hz"),
                C ("flShape", "Filter LFO Shape", names (kLfoShapeNames, kNumLfoShapes), &Params::flShape),
                F ("flAmt", "Filter LFO Amount", 0, 6, -1, &Params::flAmt, "oct"),
                F ("flPhase", "Filter LFO Phase", 0, 1, -1, &Params::flPhase),
                F ("flOffset", "Filter LFO Offset", -4, 4, -1, &Params::flOffset, "oct", true),
                F ("flSmooth", "Filter LFO Smooth", 0, 1, -1, &Params::flSmooth),
                F ("flRise", "Filter LFO Rise", 0, 4, 0.5f, &Params::flRise, "s"),
                B ("flRetrig", "Filter LFO Retrig", &Params::flRetrig),
                B ("flBip", "Filter LFO Bipolar", &Params::flBipolar),
                F ("flFm", "Filter LFO > FM", -1, 1, -1, &Params::flFm, "", true),
                F ("fBass", "Filter Bass Keep", 0, 1, -1, &Params::filterBass),
                C ("subShape", "Sub Shape", names (kSubShapeNames, 3), &Params::subShape),
                B ("subPost", "Sub Clean (post filter)", &Params::subPost),
                F ("subPhase", "Sub Phase", 0, 1, -1, &Params::subPhase),
                F ("subDecay", "Sub Decay", 0, 4, 0.4f, &Params::subDecay, "s"),
                F ("subSus", "Sub Sustain", 0, 1, -1, &Params::subSus),
                F ("satDrive", "Growl Drive", 0, 1, -1, &Params::satDrive),
                F ("satMix", "Growl Mix", 0, 1, -1, &Params::satMix),
                F ("satAsym", "Growl Asym", 0, 1, -1, &Params::satAsym),
                F ("satFold", "Growl Fold", 0, 1, -1, &Params::satFold),
                F ("fvLevel", "FM Voice Level", 0, 1, -1, &Params::fvLevel),
                C ("fvAlgo", "FM Algorithm", names (kFmAlgoNames, kNumFmAlgos), &Params::fvAlgo),
                C ("fvRoute", "FM Route", names (kFmRouteNames, 2), &Params::fvRoute),
                F ("fvBright", "FM Brightness", 0, 3, 1, &Params::fvBright),
                F ("fvBody", "FM Body", 0, 1, -1, &Params::fvBody),
                F ("fvChar", "FM Character", 0, 1, -1, &Params::fvChar),
                F ("fvDecay", "FM Decay", 0.1f, 6, 1, &Params::fvDecay, "x"),
                F ("fvFb", "FM Feedback", 0, 1, -1, &Params::fvFb),
                F ("fvWidth", "FM Width", 0, 1, -1, &Params::fvWidth),
                F ("fvVel", "FM Velocity", 0, 1, -1, &Params::fvVel),
                F ("fvKey", "FM Key Scale", -1, 1, -1, &Params::fvKey, "", true),
                F ("fvLowCut", "FM Low Cut", 0, 1, -1, &Params::fvLowCut),
                F ("lnFm", "Lane FM Depth", -1, 1, -1, &Params::lnFm, "", true),
                F ("lnChar", "Lane Character Depth", -1, 1, -1, &Params::lnChar, "", true),
                F ("lnRes", "Lane Resonance Depth", -1, 1, -1, &Params::lnRes, "", true),
                F ("lnDrive", "Lane Drive Depth", -1, 1, -1, &Params::lnDrive, "", true),
                F ("accAmt", "Accent Amount", 0, 1, -1, &Params::accAmt),
            };
            for (auto& d : w) v.push_back (d);
            for (int i = 0; i < 4; ++i)
            {
                const std::string n = "FM Op " + std::to_string (i + 1) + " ";
                const std::string pre = "op" + std::to_string (i + 1);
                auto opF = [&] (const char* w2, const char* nm, float mn, float mx, float ctr, float FmOpParams::* f, const char* unit)
                {
                    Desc d; d.id = pre + w2; d.name = n + nm; d.kind = Desc::Float; d.min = mn; d.max = mx; d.centre = ctr; d.unit = unit;
                    d.get = [i, f] (const Params& p) { return p.op[(size_t) i].*f; };
                    d.set = [i, f] (Params& p, float x) { p.op[(size_t) i].*f = x; };
                    v.push_back (d);
                };
                opF ("ratio", "Ratio", 0.125f, 24, 2, &FmOpParams::ratio, "x");
                opF ("fine", "Fine", -100, 100, -1, &FmOpParams::fine, "ct");
                opF ("level", "Level", 0, 1, -1, &FmOpParams::level, "");
                opF ("a", "Attack", 0.0005f, 4, 0.05f, &FmOpParams::a, "s");
                opF ("d", "Decay", 0.002f, 8, 0.4f, &FmOpParams::d, "s");
                opF ("s", "Sustain", 0, 1, -1, &FmOpParams::s, "");
                opF ("r", "Release", 0.002f, 8, 0.3f, &FmOpParams::r, "s");
            }
            for (int i = 0; i < kMaxSteps; ++i)
            {
                const std::string n = "Step " + std::to_string (i + 1) + " ";
                auto stF = [&] (const char* w2, const char* nm, float StepParams::* f)
                {
                    Desc d; d.id = stepId (i, w2); d.name = n + nm; d.kind = Desc::Float; d.min = 0; d.max = 1;
                    d.get = [i, f] (const Params& p) { return p.steps[(size_t) i].*f; };
                    d.set = [i, f] (Params& p, float x) { p.steps[(size_t) i].*f = x; };
                    v.push_back (d);
                };
                stF ("fm", "FM", &StepParams::fmv);
                stF ("chr", "Character", &StepParams::chr);
                stF ("rs", "Resonance", &StepParams::res);
                stF ("dv", "Drive", &StepParams::drv);
                stF ("pb", "Probability", &StepParams::prob);
                stF ("ac", "Accent", &StepParams::acc);
            }
        }
        // ===================== v5 POLY SEQUENCER parameters (appended; never reorder) =====================
        {
            v.push_back (B ("psOn", "Poly Seq On", &Params::psOn));
            v.push_back (B ("psFollow", "Poly Root From MIDI", &Params::psFollow));
            v.push_back (C ("psRoot", "Poly Root", names (kNoteNames, 12), &Params::psRoot));
            v.push_back (C ("psScale", "Poly Scale", names (kScaleNames, kNumScales), &Params::psScale));
            v.push_back (I ("psOct", "Poly Octave", -3, 3, &Params::psOct));
            v.push_back (F ("psGate", "Poly Gate", 0.02f, 1, -1, &Params::psGate));
            v.push_back (C ("psModDest", "Poly Mod Destination", names (kPsModDestNames, kNumPsModDests), &Params::psModDest));
            v.push_back (F ("psModDepth", "Poly Mod Depth", -1, 1, -1, &Params::psModDepth, "", true));
            v.push_back (F ("psModSmooth", "Poly Mod Smooth", 0, 1, -1, &Params::psModSmooth));
            static const char* laneTag[3] = { "T", "P", "M" };
            static const char* laneName[3] = { "Trigger", "Pitch", "Mod" };
            for (int l = 0; l < 3; ++l)
            {
                const std::string pre = std::string ("ps") + laneTag[l], nm = std::string ("Poly ") + laneName[l] + " ";
                Desc d;
                d = {}; d.id = pre + "Steps"; d.name = nm + "Steps"; d.kind = Desc::Int; d.min = 1; d.max = 32;
                d.get = [l] (const Params& p) { return (float) p.psLane[(size_t) l].steps; }; d.set = [l] (Params& p, float x) { p.psLane[(size_t) l].steps = (int) std::lround (x); }; v.push_back (d);
                d = {}; d.id = pre + "Div"; d.name = nm + "Rate"; d.kind = Desc::Choice; d.choices = names (kRateNames, kNumRates); d.max = kNumRates - 1;
                d.get = [l] (const Params& p) { return (float) p.psLane[(size_t) l].div; }; d.set = [l] (Params& p, float x) { p.psLane[(size_t) l].div = (int) std::lround (x); }; v.push_back (d);
                d = {}; d.id = pre + "Dir"; d.name = nm + "Direction"; d.kind = Desc::Choice; d.choices = names (kLaneDirNames, 3); d.max = 2;
                d.get = [l] (const Params& p) { return (float) p.psLane[(size_t) l].dir; }; d.set = [l] (Params& p, float x) { p.psLane[(size_t) l].dir = (int) std::lround (x); }; v.push_back (d);
                d = {}; d.id = pre + "Phase"; d.name = nm + "Phase"; d.kind = Desc::Float; d.min = -16; d.max = 16; d.unit = "st"; d.bipolar = true;
                d.get = [l] (const Params& p) { return p.psLane[(size_t) l].phase; }; d.set = [l] (Params& p, float x) { p.psLane[(size_t) l].phase = x; }; v.push_back (d);
            }
            for (int i = 0; i < kMaxSteps; ++i)
            {
                const std::string n = std::to_string (i + 1);
                Desc d;
                d = {}; d.id = "pt" + n + "on"; d.name = "Poly Trig " + n + " On"; d.kind = Desc::Bool;
                d.get = [i] (const Params& p) { return p.psOnStep[(size_t) i] ? 1.f : 0.f; }; d.set = [i] (Params& p, float x) { p.psOnStep[(size_t) i] = x > 0.5f; }; v.push_back (d);
                d = {}; d.id = "pt" + n + "pb"; d.name = "Poly Trig " + n + " Probability"; d.kind = Desc::Float; d.min = 0; d.max = 1;
                d.get = [i] (const Params& p) { return p.psProb[(size_t) i]; }; d.set = [i] (Params& p, float x) { p.psProb[(size_t) i] = x; }; v.push_back (d);
                d = {}; d.id = "pt" + n + "ac"; d.name = "Poly Trig " + n + " Accent"; d.kind = Desc::Float; d.min = 0; d.max = 1;
                d.get = [i] (const Params& p) { return p.psAcc[(size_t) i]; }; d.set = [i] (Params& p, float x) { p.psAcc[(size_t) i] = x; }; v.push_back (d);
                d = {}; d.id = "pp" + n + "deg"; d.name = "Poly Pitch " + n + " Degree"; d.kind = Desc::Int; d.min = -14; d.max = 14;
                d.get = [i] (const Params& p) { return (float) p.psDeg[(size_t) i]; }; d.set = [i] (Params& p, float x) { p.psDeg[(size_t) i] = (int) std::lround (x); }; v.push_back (d);
                d = {}; d.id = "pm" + n + "v"; d.name = "Poly Mod " + n + " Value"; d.kind = Desc::Float; d.min = 0; d.max = 1;
                d.get = [i] (const Params& p) { return p.psModV[(size_t) i]; }; d.set = [i] (Params& p, float x) { p.psModV[(size_t) i] = x; }; v.push_back (d);
            }
        }
        const Params defaults;
        for (auto& d : v) d.def = d.get (defaults);
        return v;
    }();
    return t;
}
} // namespace sq
