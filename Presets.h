// Factory presets — Zenon / prog / psytech / dark / hi-tech.
#pragma once
#include <utility>
#include "SqueFMEngine.h"
#include <string>
#include <vector>
#include <initializer_list>

namespace sq
{
struct Preset { std::string name; Params p; };

namespace pr
{
    // step-sync indices
    enum { HOLD = 0, S64 = 1, S32T = 2, S32 = 3, S16T = 4, S16 = 5, S16D = 6, S8T = 7, S8 = 8, S8D = 9, S4T = 10, S4 = 11, S2 = 13, BAR = 14 };
    // lfo-sync indices
    enum { L16 = 4, L8T = 6, L8 = 7, L4 = 10, L2 = 12, LBAR = 13, L2BAR = 14, L4BAR = 15 };
    enum { SINE, TRI, UP, DOWN, SQUARE, SH, SMOOTH, SQUELCH, CHAOS, BLIP };

    inline StepParams S (float lo, float hi, int shape, int sync) { StepParams s; s.lo = lo; s.hi = hi; s.shape = shape; s.mode = 0; s.sync = sync; return s; }
    inline StepParams H (float lo, float hi, int shape, float hz)  { StepParams s; s.lo = lo; s.hi = hi; s.shape = shape; s.mode = 1; s.hz = hz; return s; }

    inline void steps (Params& p, std::initializer_list<StepParams> l)
    {
        int i = 0; for (auto& s : l) if (i < kMaxSteps) p.steps[(size_t) i++] = s;
        const int n = (int) l.size();
        for (; i < kMaxSteps && n > 0; ++i) p.steps[(size_t) i] = p.steps[(size_t) (i % n)];
    }
    inline void mod (Params& p, int slot, int src, int dst, float amt, int via = 0) { p.matrix[(size_t) slot] = { src, via, dst, amt }; }
    inline void lfo (Params& p, int i, int shape, int sync, bool retrig = false) { auto& l = p.lfo[(size_t) i]; l.mode = 0; l.shape = shape; l.sync = sync; l.retrig = retrig; }
    inline void lfoHz (Params& p, int i, int shape, float hz) { auto& l = p.lfo[(size_t) i]; l.mode = 1; l.shape = shape; l.hz = hz; }
}

// ---------------------------------------------------------------- FM families --
inline constexpr const char* kFmFamilyNames[] = { "Warm FM", "Metallic FM", "Glass FM", "Hi-Tech FM", "FM Growl",
                                                  "FM Creature", "FM Percussion", "FM Digital Noise", "Vocal / Formant FM", "Aggressive FM Lead" };
constexpr int kNumFmFamilies = 10;

// Loads a starting operator architecture for one FM sound family into p (voice level is kept if already set).
inline void applyFmFamily (Params& p, int fam)
{
    struct O { float r, lv, a, d, s, rl; };
    auto set = [&p] (int algo, float bright, float chr, float fb, float body, std::initializer_list<O> ops)
    {
        p.fvAlgo = algo; p.fvBright = bright; p.fvChar = chr; p.fvFb = fb; p.fvBody = body;
        int i = 0;
        for (const auto& o : ops) { auto& q = p.op[(size_t) i++]; q.ratio = o.r; q.fine = 0.f; q.level = o.lv; q.a = o.a; q.d = o.d; q.s = o.s; q.r = o.rl; }
        if (p.fvLevel < 0.01f) p.fvLevel = 0.7f;
    };
    switch (fam)
    {
        case 0: set (0, 0.8f, 0.f,  0.f,  0.3f, { { 1, 1.f, .002f, .8f, .8f, .3f }, { 1, .45f, .002f, .7f, .35f, .3f }, { 2, .2f, .002f, .4f, .2f, .3f }, { 1, 0, .002f, .3f, 0, .3f } }); break;
        case 1: set (1, 1.1f, 0.4f, .2f,  0.1f, { { 1, 1.f, .001f, .9f, .7f, .3f }, { 3.5f, .6f, .001f, .5f, .2f, .3f }, { 7.07f, .4f, .001f, .35f, .1f, .2f }, { 11.3f, .3f, .001f, .2f, 0, .2f } }); break;
        case 2: set (3, 0.7f, 0.f,  0.f,  0.f,  { { 1, .8f, .001f, 1.2f, .5f, .4f }, { 4, .3f, .001f, .3f, 0, .3f }, { 2, .6f, .001f, 1.5f, .3f, .4f }, { 14, .25f, .001f, .15f, 0, .2f } }); break;
        case 3: set (0, 1.6f, 0.25f, .35f, 0.f, { { 1, 1.f, .0005f, .25f, .3f, .08f }, { 2, .7f, .0005f, .08f, .1f, .05f }, { 5.5f, .6f, .0005f, .05f, 0, .04f }, { 13, .5f, .0005f, .03f, 0, .03f } }); break;
        case 4: set (2, 1.2f, 0.f,  .45f, 0.4f, { { .5f, 1.f, .003f, 1.f, .9f, .2f }, { 1, .7f, .003f, .8f, .6f, .2f }, { 1.5f, .5f, .003f, .6f, .5f, .2f }, { .5f, .45f, .003f, .7f, .6f, .2f } }); break;
        case 5: set (6, 1.f,  0.5f, .3f,  0.2f, { { 1, 1.f, .002f, 1.5f, .6f, .3f }, { 1.41f, .6f, .02f, 1.2f, .4f, .3f }, { .71f, .5f, .002f, .6f, .3f, .3f }, { 2.83f, .4f, .05f, .9f, .2f, .3f } }); break;
        case 6: set (4, 1.4f, 0.1f, .2f,  0.f,  { { 1, 1.f, .0005f, .18f, 0, .1f }, { 1.6f, .6f, .0005f, .12f, 0, .08f }, { 2.4f, .4f, .0005f, .09f, 0, .06f }, { 3.7f, .9f, .0005f, .05f, 0, .04f } }); break;
        case 7: set (0, 2.5f, 1.f,  1.f,  0.f,  { { 1, 1.f, .001f, .6f, .6f, .1f }, { 7.9f, .9f, .001f, .5f, .7f, .1f }, { 13.7f, .9f, .001f, .5f, .7f, .1f }, { 19.1f, 1.f, .001f, .5f, .8f, .1f } }); break;
        case 8: set (5, 0.9f, 0.f,  .1f,  0.2f, { { 1, .9f, .01f, 1.f, .8f, .3f }, { 3, .45f, .01f, .8f, .5f, .3f }, { 1, .3f, .01f, .8f, .5f, .3f }, { 6, .35f, .01f, .9f, .6f, .3f } }); break;
        case 9: set (1, 1.3f, 0.05f, .5f, 0.2f, { { 1, 1.f, .002f, 1.f, .9f, .2f }, { 1, .8f, .002f, .9f, .6f, .2f }, { 2, .5f, .002f, .4f, .3f, .2f }, { 3, .4f, .002f, .2f, .2f, .2f } }); break;
        default: break;
    }
}

inline std::vector<Preset> factoryPresets()
{
    using namespace pr;
    std::vector<Preset> v;

    { Params p; steps (p, { S (0.3f, 0.55f, SINE, S16) }); v.push_back ({ "Init", p }); }

    { // ZENON — rolling swamp: classic ladder squelch with every step its own LFO
        Params p;
        p.waveA = 1.f; p.waveB = 1.5f; p.levelB = 0.3f; p.fineB = 8.f; p.levelSub = 0.35f;
        p.cutoff = 300.f; p.resonance = 0.8f; p.drive = 0.45f; p.feedback = 0.12f; p.envAmount = 0.12f; p.keyTrack = 0.4f;
        p.fmIndex = 0.08f; p.fmRatio = 2.f; p.glide = 0.05f;
        p.numSteps = 16; p.stepDiv = 3; p.seqSmooth = 0.08f;
        steps (p, { S (.22f, .62f, SQUELCH, S16), S (.30f, .45f, SINE, S32), S (.40f, .40f, SINE, HOLD), S (.20f, .70f, SQUELCH, S32T),
                    S (.25f, .50f, TRI, S16), S (.18f, .66f, SQUELCH, S32), S (.35f, .35f, SINE, HOLD), S (.30f, .58f, SMOOTH, S16),
                    S (.20f, .72f, SQUELCH, S16), S (.28f, .48f, SH, S32), S (.32f, .55f, DOWN, S16), S (.22f, .22f, SINE, HOLD),
                    H (.25f, .60f, SINE, 11.f), S (.30f, .52f, SMOOTH, S32T), S (.38f, .58f, TRI, S16), S (.20f, .68f, SQUELCH, S64) });
        lfo (p, 0, SINE, LBAR); mod (p, 0, SrcLfo1, DstRes, 0.25f);
        p.spitRate = 1.5f; p.spitDecay = 0.03f; mod (p, 1, SrcSpit, DstCutoff, 0.35f);
        mod (p, 2, SrcMacro1, DstCutoff, 0.6f); mod (p, 3, SrcMacro2, DstFmIndex, 0.6f);
        mod (p, 4, SrcMacro3, DstCombMix, 0.7f); mod (p, 5, SrcMacro4, DstTalkMix, 0.8f);
        p.combSemi = 12.f; p.combFeedback = 0.6f;
        p.phaserMix = 0.2f; p.phaserRate = 0.06f; p.delayMix = 0.15f; p.delayDiv = 7; p.reverbMix = 0.1f;
        v.push_back ({ "Zenon Swamp Roller", p });
    }
    { // PSYTECH — metallic FM ripper: mod env + sequencer drive the FM index
        Params p;
        p.waveA = 1.f; p.fmRatio = 3.5f; p.fmIndex = 0.25f; p.fmFeedback = 0.35f; p.fmTarget = 0; p.levelSub = 0.3f;
        p.cutoff = 900.f; p.resonance = 0.55f; p.drive = 0.5f; p.slope = 2.f; p.envAmount = 0.2f;
        p.mD = 0.09f; mod (p, 0, SrcModEnv, DstFmIndex, 0.55f);
        mod (p, 1, SrcSeq, DstFmIndex, 0.35f);
        p.combMix = 0.3f; p.combSemi = 12.f; p.combFeedback = 0.75f; p.combDamp = 0.2f;
        lfo (p, 1, SH, L16); mod (p, 2, SrcLfo2, DstCombFreq, 0.15f);
        mod (p, 3, SrcMacro1, DstFmRatio, 0.25f); mod (p, 4, SrcMacro2, DstFmIndex, 0.5f);
        mod (p, 5, SrcVelocity, DstFmIndex, 0.2f);
        p.numSteps = 16; p.stepDiv = 3; p.seqSmooth = 0.03f;
        steps (p, { S (.35f, .75f, BLIP, S32), S (.40f, .60f, SH, S32T), S (.30f, .80f, SQUELCH, S16), H (.35f, .70f, SQUARE, 22.f),
                    S (.45f, .65f, UP, S16), S (.30f, .85f, BLIP, S64), S (.50f, .50f, SINE, HOLD), S (.30f, .75f, CHAOS, S32) });
        p.biteType = 1; p.biteAmount = 0.2f; p.delayMix = 0.12f; p.delayDiv = 5; p.reverbMix = 0.08f; p.outGainDb = -2.f;
        v.push_back ({ "Psytech Metal Ripper", p });
    }
    { // HI-TECH — laser zaps: pitch-envelope zap + FM blips + crush
        Params p;
        p.waveA = 0.f; p.waveB = 2.2f; p.levelB = 0.4f; p.octB = 1; p.levelSub = 0.f;
        p.fmMode = 0; p.fmRatio = 5.f; p.fmIndex = 0.35f; p.fmFeedback = 0.2f; p.fmTarget = 2;
        p.pitchEnvAmt = 36.f; p.pitchEnvDecay = 0.035f;
        p.cutoff = 2500.f; p.resonance = 0.7f; p.drive = 0.35f; p.slope = 2.f; p.filterMode = 1;
        p.aD = 0.09f; p.aS = 0.f; p.aR = 0.04f; p.legato = false; p.glide = 0.f;
        p.numSteps = 16; p.stepDiv = 3; p.seqSmooth = 0.02f;
        steps (p, { S (.45f, .90f, BLIP, S32), H (.40f, .85f, SH, 30.f), S (.50f, .80f, DOWN, S16), S (.40f, .95f, BLIP, S64) });
        p.mD = 0.05f; mod (p, 0, SrcModEnv, DstFmIndex, 0.6f);
        lfo (p, 0, SH, L16); mod (p, 1, SrcLfo1, DstFmRatio, 0.3f);
        mod (p, 2, SrcRandom, DstPan, 0.6f);
        mod (p, 3, SrcMacro1, DstPitch, 0.5f); mod (p, 4, SrcMacro2, DstFmIndex, 0.5f);
        p.crush = 0.2f; p.delayMix = 0.3f; p.delayDiv = 4; p.delayFb = 0.55f; p.reverbMix = 0.15f; p.outGainDb = 2.f;
        v.push_back ({ "Hitech Laser Zaps", p });
    }
    { // DARK — forest croak: sub-audio FM rip + chaotic formants
        Params p;
        p.waveA = 1.6f; p.levelSub = 0.25f;
        p.fmMode = 1; p.fmHz = 9.f; p.fmIndex = 0.45f; p.fmFeedback = 0.4f; p.fmTarget = 2;
        p.cutoff = 500.f; p.resonance = 0.75f; p.drive = 0.6f; p.feedback = 0.3f;
        p.talkMix = 0.55f; p.vowel = 0.3f;
        p.chaosRate = 7.f; p.chaosAmt = 0.95f; mod (p, 0, SrcChaos, DstVowel, 0.4f);
        mod (p, 1, SrcSeq, DstFmIndex, 0.3f);
        lfoHz (p, 1, SMOOTH, 0.7f); mod (p, 2, SrcLfo2, DstFmRatio, 0.35f);
        p.combMix = 0.25f; p.combFeedback = -0.7f; p.combSemi = 7.f;
        mod (p, 3, SrcMacro1, DstFmIndex, 0.5f); mod (p, 4, SrcMacro2, DstCombMix, 0.6f);
        p.numSteps = 16; p.stepDiv = 3; p.seqSmooth = 0.12f; p.wander = 0.2f;
        steps (p, { S (.25f, .60f, SMOOTH, S32), S (.20f, .55f, CHAOS, S16), H (.30f, .65f, SINE, 6.f), S (.22f, .50f, SQUELCH, S32T),
                    S (.28f, .62f, SH, S32), S (.35f, .35f, SINE, HOLD), H (.20f, .58f, CHAOS, 13.f), S (.25f, .55f, SQUELCH, S16) });
        p.biteType = 2; p.biteAmount = 0.3f; p.phaserMix = 0.35f; p.reverbMix = 0.2f; p.outGainDb = -4.f;
        v.push_back ({ "Dark Forest Croak", p });
    }
    { // PROG — rubber bass: tight, punchy, subtle per-step movement
        Params p;
        p.waveA = 1.2f; p.levelSub = 0.6f; p.cutoff = 200.f; p.resonance = 0.45f; p.drive = 0.5f;
        p.envAmount = 0.35f; p.fD = 0.08f; p.aD = 0.16f; p.aS = 0.f; p.aR = 0.05f; p.keyTrack = 0.6f;
        p.pitchEnvAmt = 12.f; p.pitchEnvDecay = 0.015f; p.legato = false; p.glide = 0.f;
        p.numSteps = 4; p.stepDiv = 3; p.seqDepth = 0.6f;
        steps (p, { S (.25f, .25f, SINE, HOLD), S (.30f, .38f, SINE, S16), S (.35f, .45f, SQUELCH, S16), S (.30f, .50f, SQUELCH, S32) });
        mod (p, 0, SrcMacro1, DstCutoff, 0.5f); mod (p, 1, SrcMacro2, DstRes, 0.5f); mod (p, 2, SrcMacro3, DstFmIndex, 0.4f);
        p.outGainDb = 3.f;
        v.push_back ({ "Prog Rubber Bass", p });
    }
    { // SPITS — random wet transients spit through the filter and FM
        Params p;
        p.waveA = 1.f; p.waveB = 0.f; p.levelB = 0.4f; p.octB = 1; p.levelSub = 0.2f;
        p.cutoff = 350.f; p.resonance = 0.9f; p.drive = 0.4f;
        p.fmRatio = 1.5f; p.fmIndex = 0.1f;
        p.spitRate = 9.f; p.spitDecay = 0.025f; p.spitRandom = 0.8f;
        mod (p, 0, SrcSpit, DstCutoff, 0.7f); mod (p, 1, SrcSpit, DstFmIndex, 0.6f); mod (p, 2, SrcSpit, DstVowel, 0.5f);
        p.talkMix = 0.35f;
        mod (p, 3, SrcMacro1, DstSpitRate, 0.6f); mod (p, 4, SrcMacro2, DstRes, 0.4f);
        p.numSteps = 8; p.stepDiv = 6; p.seqDepth = 0.7f;
        steps (p, { S (.20f, .45f, SMOOTH, S8), S (.25f, .40f, SINE, S4), S (.18f, .50f, TRI, S8T) });
        p.delayMix = 0.25f; p.delayDiv = 5; p.reverbMix = 0.2f;
        v.push_back ({ "Spit Machine", p });
    }
    { // SUB-AUDIO RIPPER — FM op at a few Hz rips the waveform
        Params p;
        p.waveA = 1.f; p.waveB = 2.f; p.levelB = 0.5f; p.semiB = 7.f;
        p.fmMode = 1; p.fmHz = 4.f; p.fmIndex = 0.7f; p.fmFeedback = 0.25f; p.fmTarget = 2;
        lfo (p, 0, TRI, LBAR); mod (p, 0, SrcLfo1, DstFmRatio, 0.5f);
        mod (p, 1, SrcSeq, DstFmIndex, 0.4f);
        p.cutoff = 700.f; p.resonance = 0.65f; p.drive = 0.5f;
        p.numSteps = 16; p.stepDiv = 3;
        steps (p, { H (.30f, .60f, SINE, 3.f), S (.25f, .55f, SMOOTH, S16), H (.35f, .65f, TRI, 7.5f), S (.30f, .30f, SINE, HOLD) });
        mod (p, 2, SrcMacro1, DstFmIndex, 0.5f); mod (p, 3, SrcMacro2, DstFmRatio, 0.5f);
        p.phaserMix = 0.3f; p.reverbMix = 0.12f; p.outGainDb = -3.f;
        v.push_back ({ "Sub-Audio Ripper", p });
    }
    { // COMB INSECTS — buzzing metallic comb swarm for dark/forest fx
        Params p;
        p.waveA = 2.6f; p.levelNoise = 0.35f; p.levelSub = 0.f;
        p.combMix = 0.85f; p.combFeedback = 0.92f; p.combDamp = 0.1f; p.combKey = false; p.combHz = 1800.f;
        lfo (p, 0, SH, L16); mod (p, 0, SrcLfo1, DstCombFreq, 0.35f);
        lfoHz (p, 1, SINE, 13.f); mod (p, 1, SrcLfo2, DstCombFb, 0.15f);
        p.cutoff = 3000.f; p.resonance = 0.5f; p.filterMode = 1;
        p.numSteps = 16; p.stepDiv = 2; p.seqDepth = 0.8f;
        steps (p, { H (.50f, .85f, SH, 25.f), S (.55f, .80f, BLIP, S64), H (.45f, .90f, CHAOS, 18.f) });
        mod (p, 2, SrcMacro1, DstCombFreq, 0.5f); mod (p, 3, SrcMacro2, DstCombFb, 0.3f);
        p.delayMix = 0.3f; p.delayDiv = 4; p.reverbMix = 0.3f; p.outGainDb = -6.f;
        v.push_back ({ "Comb Insects", p });
    }
    { // TALKING FROG — sequencer drives vowels as well as the filter
        Params p;
        p.waveA = 2.2f; p.waveB = 1.f; p.levelB = 0.5f; p.syncB = true; p.semiB = 7.f;
        p.cutoff = 900.f; p.resonance = 0.55f; p.drive = 0.35f; p.slope = 2.f;
        p.talkMix = 0.75f; p.vowel = 0.2f; mod (p, 0, SrcSeq, DstVowel, 0.7f);
        lfo (p, 0, SINE, L2BAR); mod (p, 1, SrcLfo1, DstPitchB, 0.3f);
        mod (p, 2, SrcMacro1, DstVowel, 0.6f); mod (p, 3, SrcMacro2, DstPitchB, 0.4f);
        p.numSteps = 8; p.stepDiv = 6; p.seqSmooth = 0.25f;
        steps (p, { S (.30f, .70f, SMOOTH, S4), S (.40f, .60f, SINE, S8), H (.25f, .75f, CHAOS, 5.f), S (.35f, .65f, TRI, S8T) });
        p.delayMix = 0.22f; p.delayDiv = 5; p.reverbMix = 0.18f; p.outGainDb = -5.f;
        p.outGainDb = -7.f;
        v.push_back ({ "Talking Frog", p });
    }
    { // DISCO VALLEY ACID — screaming 303-style squelch with every step a pluck
        Params p;
        p.waveA = 1.f; p.levelSub = 0.f; p.cutoff = 380.f; p.resonance = 0.95f; p.drive = 0.6f;
        p.envAmount = 0.5f; p.fD = 0.12f; p.resEnvAmount = 0.15f; p.velToFilter = 0.6f; p.seqDepth = 0.75f;
        p.numSteps = 32; p.stepDiv = 3; p.seqSmooth = 0.04f;
        steps (p, { S (.25f, .65f, SQUELCH, S16), S (.30f, .55f, SQUELCH, S16), S (.28f, .70f, SQUELCH, S32), S (.25f, .45f, SQUELCH, S16),
                    S (.30f, .80f, SQUELCH, S32T), S (.27f, .60f, SQUELCH, S16), S (.20f, .20f, SINE, HOLD), S (.25f, .75f, SQUELCH, S32) });
        p.biteType = 2; p.biteAmount = 0.35f;
        mod (p, 0, SrcMacro1, DstCutoff, 0.5f); mod (p, 1, SrcMacro2, DstRes, 0.3f); mod (p, 2, SrcModWheel, DstSeqDepth, 0.4f);
        p.delayMix = 0.2f; p.delayDiv = 7; p.delayFb = 0.5f; p.reverbMix = 0.1f;
        p.outGainDb = -3.f;
        v.push_back ({ "Disco Valley Acid", p });
    }
    { // NIGHT FULL-ON — triplet gallop movement
        Params p;
        p.waveA = 1.f; p.waveB = 1.f; p.levelB = 0.35f; p.fineB = -10.f; p.levelSub = 0.4f;
        p.cutoff = 260.f; p.resonance = 0.7f; p.drive = 0.55f; p.envAmount = 0.2f;
        p.fmRatio = 1.f; p.fmIndex = 0.12f;
        p.numSteps = 12; p.stepDiv = 2; p.swing = 0.1f;
        steps (p, { S (.22f, .60f, SQUELCH, S16T), S (.30f, .50f, SINE, S32T), S (.26f, .66f, BLIP, S16T) });
        lfo (p, 0, SINE, L2BAR); mod (p, 0, SrcLfo1, DstCutoff, 0.15f);
        mod (p, 1, SrcSeq, DstFmIndex, 0.25f); mod (p, 2, SrcMacro1, DstCutoff, 0.5f);
        p.phaserMix = 0.25f; p.delayMix = 0.18f; p.delayDiv = 5;
        v.push_back ({ "Night Full-On Gallop", p });
    }
    { // HI-TECH BASS — fast, clean, mono FM-flavoured 1/16 bass for 180 BPM
        Params p;
        p.waveA = 0.6f; p.levelSub = 0.55f; p.fmRatio = 1.f; p.fmIndex = 0.2f; p.mD = 0.05f;
        mod (p, 0, SrcModEnv, DstFmIndex, 0.4f);
        p.cutoff = 240.f; p.resonance = 0.35f; p.drive = 0.4f; p.envAmount = 0.4f; p.fD = 0.05f;
        p.aD = 0.11f; p.aS = 0.f; p.aR = 0.03f; p.legato = false; p.glide = 0.f;
        p.pitchEnvAmt = 7.f; p.pitchEnvDecay = 0.012f;
        p.seqOn = false;
        mod (p, 1, SrcMacro1, DstCutoff, 0.5f); mod (p, 2, SrcMacro2, DstFmIndex, 0.5f);
        p.outGainDb = 3.f;
        v.push_back ({ "Hitech Clean Bass", p });
    }
    // loudness match (~-14 dB RMS on a 16th roll), capped so peaks stay below the soft clipper
    static const std::pair<const char*, float> kTrim[] = {
        { "Init", 0.3f }, { "Zenon Swamp Roller", 0.1f }, { "Psytech Metal Ripper", -1.3f }, { "Hitech Laser Zaps", -1.0f },
        { "Dark Forest Croak", 1.7f }, { "Prog Rubber Bass", 4.8f }, { "Spit Machine", -1.0f }, { "Sub-Audio Ripper", 3.7f },
        { "Comb Insects", 7.4f }, { "Talking Frog", 0.5f }, { "Disco Valley Acid", -2.0f }, { "Night Full-On Gallop", 1.4f },
        { "Hitech Clean Bass", 4.6f } };
    for (auto& pr : v) for (const auto& t : kTrim) if (pr.name == t.first) pr.p.outGainDb += t.second;

    // ================= v4 reference patches =================
    { // A - FAT WOW: saw stack + clean sine sub, resonant 24 dB ladder swept by the dedicated filter LFO
        Params p;
        p.waveA = 1.f; p.waveB = 1.f; p.levelA = 0.4f; p.levelB = 0.28f; p.fineB = 7.f; p.octB = 0;
        p.levelSub = 0.42f; p.subShape = 1; p.subPost = true; p.subOct = 0;
        p.cutoff = 150.f; p.resonance = 0.9f; p.slope = 3.f; p.drive = 0.f; p.filterBass = 0.35f; p.keyTrack = 0.5f;
        p.envAmount = 0.35f; p.fA = 0.004f; p.fD = 0.35f; p.fS = 0.f; p.fR = 0.2f; p.velToFilter = 0.3f;
        p.aA = 0.004f; p.aD = 0.3f; p.aS = 0.9f; p.aR = 0.15f;
        p.flOn = true; p.flMode = 0; p.flSync = L4; p.flShape = SINE; p.flAmt = 3.9f; p.flBipolar = false; p.flSmooth = 0.15f; p.flRetrig = true;
        p.satDrive = 0.35f; p.satMix = 0.45f; p.satAsym = 0.25f;
        p.seqOn = false; p.glide = 0.03f; p.reverbMix = 0.06f; p.outGainDb = -6.f;
        mod (p, 0, SrcMacro1, DstCutoff, 0.5f); mod (p, 1, SrcMacro2, DstRes, 0.4f); mod (p, 2, SrcMacro3, DstDrive, 0.6f);
        v.push_back ({ "A Fat WOW", p });
    }
    { // B - AGGRESSIVE SWAMP: growl FM into the ladder, stepped organic filter motion, asymmetric growl
        Params p;
        p.waveA = 1.4f; p.levelA = 0.7f; p.levelB = 0.25f; p.waveB = 1.f; p.octB = -1; p.fineB = -6.f;
        p.levelSub = 0.5f; p.subShape = 1; p.subPost = true;
        applyFmFamily (p, 4); p.fvLevel = 0.45f; p.fvRoute = 0; p.fvBright = 1.1f;
        p.cutoff = 260.f; p.resonance = 0.66f; p.slope = 3.f; p.drive = 0.4f; p.filterBass = 0.6f; p.keyTrack = 0.4f; p.envAmount = 0.2f; p.fD = 0.2f;
        p.flOn = true; p.flSync = L16; p.flShape = 6; p.flAmt = 2.2f; p.flBipolar = true; p.flSmooth = 0.3f; p.flRetrig = false; p.flFm = 0.35f;
        p.seqOn = true; p.seqDepth = 0.45f; p.numSteps = 16; p.stepDiv = 3; p.seqSmooth = 0.12f;
        steps (p, { S (.25f, .55f, SQUELCH, S16), S (.30f, .45f, SMOOTH, S32), S (.35f, .35f, SINE, HOLD), S (.22f, .62f, SQUELCH, S32T),
                    S (.28f, .5f, TRI, S16), S (.2f, .6f, CHAOS, S32), S (.33f, .33f, SINE, HOLD), S (.26f, .56f, SMOOTH, S16) });
        p.satDrive = 0.45f; p.satMix = 0.6f; p.satAsym = 0.45f; p.satFold = 0.15f;
        p.aA = 0.003f; p.aD = 0.25f; p.aS = 0.85f; p.aR = 0.12f;
        p.phaserMix = 0.18f; p.phaserRate = 0.07f; p.reverbMix = 0.08f; p.outGainDb = -2.f;
        mod (p, 0, SrcMacro1, DstCutoff, 0.5f); mod (p, 1, SrcMacro2, DstFmIndex, 0.6f); mod (p, 2, SrcMacro3, DstDrive, 0.6f);
        v.push_back ({ "B Aggressive Swamp", p });
    }
    { // C - HI-TECH FM CREATURE: cross-ring operators, clean parallel route, index shaped by envelopes + filter LFO
        Params p;
        p.levelA = 0.f; p.levelB = 0.f; p.levelSub = 0.f;
        applyFmFamily (p, 5); p.fvLevel = 0.9f; p.fvRoute = 1; p.fvBright = 0.75f; p.fvChar = 0.18f; p.fvFb = 0.15f; p.fvWidth = 0.6f; p.fvDecay = 0.6f; p.fvVel = 0.5f; p.fvLowCut = 0.8f;
        p.op[1].d = 0.35f; p.op[1].s = 0.1f; p.op[3].a = 0.0005f; p.op[3].d = 0.12f; p.op[3].s = 0.05f;
        p.flOn = true; p.flSync = L16; p.flShape = SH; p.flAmt = 0.f; p.flFm = 0.4f; p.flRetrig = true;
        p.pitchEnvAmt = 7.f; p.pitchEnvDecay = 0.025f;
        p.aA = 0.0008f; p.aD = 0.4f; p.aS = 0.f; p.aR = 0.12f;
        p.seqOn = false; p.delayMix = 0.18f; p.delayDiv = 4; p.delayFb = 0.35f; p.pingPong = true; p.reverbMix = 0.1f; p.outGainDb = 3.f;
        mod (p, 0, SrcMacro1, DstFmIndex, 0.6f); mod (p, 1, SrcMacro2, DstFmRatio, 0.25f);
        v.push_back ({ "C HiTech FM Creature", p });
    }
    { // D - PSYCHEDELIC FM LEAD: aggressive lead family, sequenced FM timbre + character lanes, slow filter LFO
        Params p;
        p.levelA = 0.25f; p.waveA = 1.f; p.levelB = 0.f; p.levelSub = 0.15f; p.subShape = 1; p.subPost = true;
        applyFmFamily (p, 9); p.fvLevel = 0.75f; p.fvRoute = 0; p.fvWidth = 0.7f; p.fvBright = 0.85f; p.fvFb = 0.3f; p.fvLowCut = 0.6f;
        p.cutoff = 1800.f; p.resonance = 0.35f; p.slope = 2.f; p.drive = 0.25f; p.filterBass = 0.4f; p.keyTrack = 0.6f; p.envAmount = 0.25f; p.fD = 0.5f;
        p.flOn = true; p.flSync = LBAR; p.flShape = TRI; p.flAmt = 1.6f; p.flBipolar = true; p.flRetrig = false;
        p.seqOn = true; p.seqDepth = 0.f; p.numSteps = 16; p.stepDiv = 3; p.seqSmooth = 0.15f;
        p.lnFm = 0.6f; p.lnChar = 0.35f; p.accAmt = 0.6f;
        const float fmLane[16] = { .2f, .5f, .8f, .4f, .9f, .3f, .6f, 1.f, .2f, .7f, .4f, .9f, .5f, .3f, .8f, .6f };
        const float chLane[16] = { 0, 0, .2f, 0, 0, .6f, 0, 0, .1f, 0, .8f, 0, 0, 0, .4f, 1.f };
        for (int i = 0; i < kMaxSteps; ++i) { auto& st = p.steps[(size_t) i]; st.fmv = fmLane[i % 16]; st.chr = 0.5f + chLane[i % 16] * 0.5f; st.acc = (i % 4 == 2) ? 1.f : 0.f; }
        p.glide = 0.06f; p.aA = 0.004f; p.aD = 0.4f; p.aS = 0.8f; p.aR = 0.25f;
        p.delayMix = 0.28f; p.delayDiv = 7; p.delayFb = 0.45f; p.pingPong = true; p.reverbMix = 0.15f; p.reverbSize = 0.75f; p.outGainDb = -1.f;
        mod (p, 0, SrcMacro1, DstFmIndex, 0.6f); mod (p, 1, SrcMacro2, DstCutoff, 0.5f);
        v.push_back ({ "D Psy FM Lead", p });
    }
    return v;
}
} // namespace sq
