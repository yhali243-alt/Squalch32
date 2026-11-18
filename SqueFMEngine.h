// =============================================================================
//  DISCO VALLEY  ·  SqueFM
//  Psychedelic FM / ladder / comb synth with a 32-step LFO sequencer.
//  Built for Zenon, psytech, prog, dark and hi-tech.
//
//  Pure C++17 DSP engine (no framework dependency).
//
//  VOICE (mono/legato, 2x oversampled):
//    OSC A (morph) ─┐   ◄── FM OP (sine, ratio or fixed-Hz, self-feedback) phase-modulates A / B / both
//    OSC B (morph) ─┤       and can also be heard directly
//    FM OP ─────────┼─ MIXER ─ DRIVE ─ BITE(pre) ─ LADDER 6-24dB LP/BP/HP ─ BITE(post) ─ VCA
//    SUB · NOISE ───┤                     ▲                                   │
//    FEEDBACK ──────┘                     └──────────── feedback ─────────────┘
//  → COMB (keytracked, ± feedback) → TALK (formants) → CRUSH → PAN
//  → PHASER → PING-PONG DELAY → REVERB → OUT
//
//  MODULATION:
//    · 32-step SEQUENCER: every step = its own LFO with LOW/HIGH cutoff range,
//      own shape and own speed (BPM-synced division or free Hz).
//    · 3 LFOs (BPM or Hz), Mod Env, Filter Env, Amp Env, Pitch Env,
//      SPIT generator (random wet transients), CHAOS (logistic map),
//      per-note random, velocity, mod wheel, key, 4 MACROS.
//    · 8-slot MOD MATRIX with "via" scaling (like Serum's aux source).
// =============================================================================
#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

namespace sq
{
constexpr int   kMaxSteps = 32;
constexpr int   kMatrixSlots = 8;
constexpr int   kNumLfos = 3;
constexpr int   kNumMacros = 4;
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float fastTanh (float x)
{
    x = clampf (x, -4.5f, 4.5f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ---------------------------------------------------------------- tables -----
// note divisions (quarter-note beats) used by LFO rates and step speeds
inline constexpr float kRateBeats[] = { 1.f/16.f, 1.f/12.f, 1.f/8.f, 1.f/6.f, 1.f/4.f, 3.f/8.f, 1.f/3.f, 1.f/2.f,
                                        3.f/4.f, 2.f/3.f, 1.f, 3.f/2.f, 2.f, 4.f, 8.f, 16.f };
inline constexpr const char* kRateNames[] = { "1/64", "1/32T", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8",
                                              "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1 Bar", "2 Bar", "4 Bar" };
constexpr int kNumRates = 16;

// step-speed choices = HOLD + the rate list
inline constexpr const char* kStepSyncNames[] = { "HOLD", "1/64", "1/32T", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8",
                                                  "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1 Bar", "2 Bar", "4 Bar" };
constexpr int kNumStepSync = 17;

// step lengths
inline constexpr float kDivBeats[]       = { 1.f/16.f, 1.f/8.f, 1.f/6.f, 1.f/4.f, 3.f/8.f, 1.f/3.f, 1.f/2.f, 3.f/4.f, 2.f/3.f, 1.f, 2.f, 4.f };
inline constexpr const char* kDivNames[] = { "1/64", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/2", "1 Bar" };
constexpr int kNumDivs = 12;

inline constexpr const char* kStepShapeNames[] = { "Sine", "Tri", "Ramp Up", "Ramp Down", "Square", "S&H", "Smooth Rnd", "Squelch", "Chaos", "Blip" };
constexpr int kNumStepShapes = 10;

inline constexpr const char* kLfoShapeNames[] = { "Sine", "Tri", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Rnd", "Chaos" };
constexpr int kNumLfoShapes = 8;

inline constexpr const char* kBiteNames[] = { "Soft", "Fold", "Asym", "Rectify", "Crush" };
constexpr int kNumBites = 5;

inline constexpr const char* kFmTargetNames[] = { "Osc A", "Osc B", "A + B" };
inline constexpr const char* kSyncHzNames[]   = { "BPM", "Hz" };

// ---- modulation sources ----
enum Src { SrcNone, SrcSeq, SrcLfo1, SrcLfo2, SrcLfo3, SrcModEnv, SrcFiltEnv, SrcAmpEnv, SrcPitchEnv,
           SrcVelocity, SrcModWheel, SrcKey, SrcRandom, SrcSpit, SrcChaos,
           SrcMacro1, SrcMacro2, SrcMacro3, SrcMacro4, NumSrc };
inline constexpr const char* kSrcNames[] = { "—", "Sequencer", "LFO 1", "LFO 2", "LFO 3", "Mod Env", "Filter Env", "Amp Env", "Pitch Env",
                                             "Velocity", "Mod Wheel", "Key", "Random", "Spit", "Chaos",
                                             "Macro 1", "Macro 2", "Macro 3", "Macro 4" };

// ---- modulation destinations ----
enum Dst { DstNone, DstCutoff, DstRes, DstDrive, DstFmIndex, DstFmRatio, DstFmFeedback, DstWaveA, DstWaveB,
           DstPitchB, DstPitch, DstCombFreq, DstCombFb, DstCombMix, DstVowel, DstTalkMix, DstBite, DstSeqDepth,
           DstLfo1Rate, DstLfo2Rate, DstLfo3Rate, DstPan, DstVolume, DstNoise, DstSub, DstDelayMix, DstSpitRate, NumDst };
inline constexpr const char* kDstNames[] = { "—", "Cutoff", "Resonance", "Drive", "FM Index", "FM Ratio", "FM Feedback", "Osc A Wave", "Osc B Wave",
                                             "Osc B Pitch", "Pitch", "Comb Freq", "Comb Fdbk", "Comb Mix", "Vowel", "Talk Mix", "Bite", "Seq Depth",
                                             "LFO 1 Rate", "LFO 2 Rate", "LFO 3 Rate", "Pan", "Volume", "Noise", "Sub", "Delay Mix", "Spit Rate" };

// ---------------------------------------------------------------- params -----
struct StepParams
{
    float lo = 0.25f, hi = 0.6f;   // cutoff range 0..1 (20 Hz .. 18 kHz, log)
    int   shape = 0;               // kStepShapeNames
    int   mode  = 0;               // 0 = BPM sync, 1 = Hz
    int   sync  = 5;               // kStepSyncNames index (0 = HOLD)
    float hz    = 4.f;             // used when mode == Hz
};

struct LfoParams
{
    int mode = 0;          // 0 BPM, 1 Hz
    int sync = 7;          // kRateNames
    float hz = 2.f;
    int shape = 0;
    bool retrig = false;
};

struct MatrixSlot { int src = 0, via = 0, dst = 0; float amount = 0.f; };

struct Params
{
    // OSC A / B
    float waveA = 1.f, waveB = 1.f;
    int   octA = 0, octB = 0;          // -2..2
    float semiA = 0.f, semiB = 0.f, fineA = 0.f, fineB = 0.f;
    bool  syncB = false, oscReset = true;
    // FM operator
    int   fmMode = 0;                  // 0 ratio, 1 fixed Hz (sub-audio ripping)
    float fmRatio = 2.f, fmFine = 0.f, fmHz = 8.f, fmIndex = 0.f, fmFeedback = 0.f;
    int   fmTarget = 0;
    // mixer
    float levelA = 0.8f, levelB = 0.f, levelFm = 0.f, levelSub = 0.3f, levelNoise = 0.f, feedback = 0.f;
    int   subOct = 0;
    // ladder
    float cutoff = 600.f, resonance = 0.6f, drive = 0.25f, slope = 3.f;
    int   filterMode = 0;
    float keyTrack = 0.3f, envAmount = 0.35f, resEnvAmount = 0.f;
    // comb
    float combMix = 0.f, combFeedback = 0.6f, combDamp = 0.3f, combSemi = 12.f, combHz = 440.f;
    bool  combKey = true;
    // envelopes
    float fA = 0.002f, fD = 0.25f, fS = 0.f, fR = 0.2f; bool fLoop = false;
    float aA = 0.002f, aD = 0.3f,  aS = 0.85f, aR = 0.12f;
    float mA = 0.002f, mD = 0.3f,  mS = 0.f,  mR = 0.2f; bool mLoop = false;
    float pitchEnvAmt = 0.f, pitchEnvDecay = 0.05f;
    float velToFilter = 0.3f, velToAmp = 0.3f;
    // performance
    float glide = 0.04f; bool legato = true; float bendRange = 2.f;
    // sequencer
    bool  seqOn = true, seqRetrig = true, seqPhaseReset = true;
    int   numSteps = 16, stepDiv = 3;
    float seqDepth = 1.f, seqSmooth = 0.1f, speedScale = 1.f, wander = 0.f, swing = 0.f, chaos = 0.6f;
    std::array<StepParams, kMaxSteps> steps {};
    // LFOs, spit, chaos source
    std::array<LfoParams, kNumLfos> lfo {};
    float spitRate = 3.f, spitDecay = 0.04f, spitRandom = 0.7f;
    float chaosRate = 6.f, chaosAmt = 0.8f;
    // matrix + macros
    std::array<MatrixSlot, kMatrixSlots> matrix {};
    std::array<float, kNumMacros> macro {};
    // talk / bite / crush
    float talkMix = 0.f, vowel = 0.f;
    int   biteType = 0; float biteAmount = 0.f, biteMix = 1.f; bool bitePost = true;
    float crush = 0.f;
    // fx
    float phaserMix = 0.f, phaserRate = 0.15f, phaserDepth = 0.7f, phaserFb = 0.5f;
    float delayMix = 0.f, delayFb = 0.45f, delayTone = 0.5f, delayMod = 0.2f; int delayDiv = 7; bool pingPong = true;
    float reverbMix = 0.f, reverbSize = 0.7f, reverbDamp = 0.5f;
    float outGainDb = 0.f;
    int   quality = 1;          // 0 = 2x oversampling, 1 = 4x (below 60 kHz host rate)
};

// ------------------------------------------------------------------ util -----
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) / 16777215.0f; }
    float bi()   { return next() * 2.f - 1.f; }
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)       { t /= dt;            return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

inline float wrap01 (float x) { return x - std::floor (x); }

// morphing oscillator with phase-modulation input
struct MorphOsc
{
    float phase = 0.f;
    bool  wrapped = false;

    float tick (float inc, float morph, float pm)
    {
        inc = clampf (inc, -0.49f, 0.49f);
        const float dt = std::fabs (inc) + 1e-9f;
        const float t  = wrap01 (phase + pm);
        float out;
        if (morph < 1.f)
        {
            const float tri = t < 0.5f ? 4.f * t - 1.f : 3.f - 4.f * t;
            const float saw = (2.f * t - 1.f) - polyBlep (t, dt);
            out = tri + (saw - tri) * morph;
        }
        else if (morph < 2.f)
        {
            const float saw = (2.f * t - 1.f) - polyBlep (t, dt);
            out = saw + (pulse (t, dt, 0.5f) - saw) * (morph - 1.f);
        }
        else out = pulse (t, dt, 0.5f - (morph - 2.f) * 0.44f);

        phase += inc;
        wrapped = false;
        if (phase >= 1.f) { phase -= std::floor (phase); wrapped = true; }
        else if (phase < 0.f) phase -= std::floor (phase);
        return out;
    }
    static float pulse (float t, float dt, float pw)
    {
        float v = t < pw ? 1.f : -1.f;
        v += polyBlep (t, dt);
        float t2 = t - pw; if (t2 < 0.f) t2 += 1.f;
        v -= polyBlep (t2, dt);
        return v;
    }
};

struct Ladder
{
    float s[4] {}, y[5] {};
    void reset() { std::memset (s, 0, sizeof (s)); std::memset (y, 0, sizeof (y)); }
    inline void process (float x, float G, float k)
    {
        const float oneMinusG = 1.f - G;
        const float b1 = s[0] * oneMinusG, b2 = s[1] * oneMinusG, b3 = s[2] * oneMinusG, b4 = s[3] * oneMinusG;
        const float G2 = G * G, G3 = G2 * G, G4 = G3 * G;
        const float S  = G3 * b1 + G2 * b2 + G * b3 + b4;
        const float y4est = (G4 * x + S) / (1.f + k * G4);
        const float u = fastTanh (x - k * y4est);
        y[0] = u;
        float in = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v = (in - s[i]) * G;
            const float out = v + s[i];
            s[i] = out + v;
            in = out;
            y[i + 1] = out;
        }
        s[0] = fastTanh (s[0] * 0.8f) * 1.25f;
    }
    inline float output (float slope, int mode) const
    {
        if (mode == 0)
        {
            const float sl = clampf (slope, 0.f, 3.f);
            const int i = std::min (2, (int) sl);
            const float f = sl - (float) i;
            return y[i + 1] + (y[i + 2] - y[i + 1]) * f;
        }
        const float f = clampf (slope / 3.f, 0.f, 1.f);
        if (mode == 1)
        {
            const float bp2 = 2.f * (y[1] - y[2]);
            const float bp4 = 4.f * (y[2] - 2.f * y[3] + y[4]);
            return bp2 + (bp4 - bp2) * f;
        }
        const float hp2 = y[0] - 2.f * y[1] + y[2];
        const float hp4 = y[0] - 4.f * y[1] + 6.f * y[2] - 4.f * y[3] + y[4];
        return hp2 + (hp4 - hp2) * f;
    }
};

struct Svf
{
    float ic1 = 0.f, ic2 = 0.f, a1 = 0.f, a2 = 0.f, a3 = 0.f;
    void set (float freq, float q, float sr)
    {
        const float g = std::tan (kPi * clampf (freq, 20.f, sr * 0.45f) / sr);
        const float k = 1.f / q;
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    float bp (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        return v1;
    }
    void reset() { ic1 = ic2 = 0.f; }
};

struct Env
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage st = Idle; float v = 0.f, aC = 1.f, dC = 1.f, rC = 1.f, sus = 0.f; bool loop = false;
    static float coef (float t, float sr) { return 1.f - std::exp (-4.6f / (std::max (t, 0.0005f) * sr)); }
    void set (float a, float d, float s, float r, bool lp, float sr)
    { aC = std::min (1.f, coef (a, sr) * 1.6f); dC = coef (d, sr); rC = coef (r, sr); sus = s; loop = lp; }
    void gateOn()  { st = Attack; }
    void gateOff() { if (st != Idle) st = Release; }
    float tick()
    {
        switch (st)
        {
            case Attack:  v += (1.25f - v) * aC; if (v >= 1.f) { v = 1.f; st = Decay; } break;
            case Decay:   v += (sus - v) * dC; if (std::fabs (v - sus) < 0.003f) st = loop ? Attack : Sustain; break;
            case Sustain: v = sus; break;
            case Release: v += (0.f - v) * rC; if (v < 1e-5f) { v = 0.f; st = Idle; } break;
            case Idle: default: v = 0.f; break;
        }
        return v;
    }
};

struct Decimator2x
{
    static constexpr int N = 63;
    float h[N] {}, buf[N * 2] {}; int pos = 0;
    Decimator2x()
    {
        const float fc = 0.22f; float sum = 0.f;
        for (int i = 0; i < N; ++i)
        {
            const float m = (float) i - (N - 1) * 0.5f;
            const float sinc = m == 0.f ? 2.f * fc : std::sin (kTwoPi * fc * m) / (kPi * m);
            const float w = 0.42f - 0.5f * std::cos (kTwoPi * i / (N - 1)) + 0.08f * std::cos (2.f * kTwoPi * i / (N - 1));
            h[i] = sinc * w; sum += h[i];
        }
        for (auto& c : h) c /= sum;
    }
    void reset() { std::memset (buf, 0, sizeof (buf)); pos = 0; }
    void push (float x) { buf[pos] = x; buf[pos + N] = x; pos = (pos + 1) % N; }
    float out() const { float acc = 0.f; const float* b = buf + pos; for (int i = 0; i < N; ++i) acc += h[i] * b[i]; return acc; }
};

struct Fdn
{
    static constexpr int L = 8;
    std::vector<float> line[L]; int len[L] {}, idx[L] {}; float lp[L] {};
    std::vector<float> ap[2]; int apLen[2] {}, apIdx[2] {};
    void prepare (float sr)
    {
        static const int base[L] = { 1031, 1327, 1523, 1871, 2053, 2311, 2539, 2803 };
        for (int i = 0; i < L; ++i) { line[i].assign ((size_t) (base[i] * 1.8f * sr / 48000.f) + 8, 0.f); idx[i] = 0; lp[i] = 0.f; len[i] = (int) (base[i] * sr / 48000.f); }
        const int apb[2] = { 241, 557 };
        for (int i = 0; i < 2; ++i) { apLen[i] = (int) (apb[i] * sr / 48000.f); ap[i].assign ((size_t) apLen[i] + 1, 0.f); apIdx[i] = 0; }
    }
    void reset() { for (auto& l : line) std::fill (l.begin(), l.end(), 0.f); for (auto& a : ap) std::fill (a.begin(), a.end(), 0.f); std::memset (lp, 0, sizeof (lp)); }
};

// ================================================================ ENGINE =====
class Engine
{
public:
    // ---- GUI feedback ----
    std::atomic<int>   uiStep { 0 };
    std::atomic<float> uiStepFrac { 0.f }, uiSeqValue { 0.f }, uiCutoffHz { 600.f }, uiEnv { 0.f };
    std::array<std::atomic<float>, NumSrc> uiSrc {};
    std::array<std::atomic<float>, kMatrixSlots> uiSlot {};
    static constexpr int kScopeSize = 256;
    float scope[kScopeSize] {};
    std::atomic<int> scopeWrite { 0 };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate; osr = sr * 2.f;
        dcR = std::exp (-kTwoPi * 6.f / sr);
        smCoef = 1.f - std::exp (-1.f / (0.004f * sr));
        delayBufLen = (int) (sr * 4.f) + 4;
        delayL.assign ((size_t) delayBufLen, 0.f); delayR.assign ((size_t) delayBufLen, 0.f);
        combLen = (int) (sr / 15.f) + 8;
        combBuf.assign ((size_t) combLen, 0.f);
        fdn.prepare (sr);
        reset();
    }

    void reset()
    {
        ladder.reset(); dec.reset(); decA.reset(); fdn.reset(); dcX = dcY = 0.f; smCut = -1.f;
        std::fill (delayL.begin(), delayL.end(), 0.f); std::fill (delayR.begin(), delayR.end(), 0.f);
        std::fill (combBuf.begin(), combBuf.end(), 0.f);
        for (auto& f : formant) f.reset();
        std::memset (phZ, 0, sizeof (phZ)); phFb[0] = phFb[1] = 0.f;
        oscA = {}; oscB = {}; oscSub = {}; fEnv = {}; aEnv = {}; mEnv = {};
        heldCount = 0; lastFilterOut = 0.f; seqY = 0.f; internalPos = 0.0; internalSec = 0.0;
        for (auto& l : lfoState) l = {};
        std::fill (std::begin (modDst), std::end (modDst), 0.f);
    }

    void setParams (const Params& p) { P = p; }
    void setTransport (double bpmIn, double ppqIn, bool playingIn) { bpm = bpmIn > 1.0 ? bpmIn : 120.0; ppq = ppqIn; playing = playingIn; }
    void pitchBend (float v) { bend = clampf (v, -1.f, 1.f); }
    void modWheel  (float v) { modWheelVal = clampf (v, 0.f, 1.f); }

    void noteOn (int note, float vel)
    {
        removeHeld (note);
        if (heldCount < 128) held[heldCount++] = note;
        const bool wasHeld = heldCount > 1;
        targetNote = (float) note;
        velocity = vel;
        if (! wasHeld || ! P.legato)
        {
            fEnv.gateOn(); aEnv.gateOn(); mEnv.gateOn();
            pitchEnvV = 1.f;
            noteRandom = rng.bi();
            if (P.seqRetrig && ! playing) { internalPos = 0.0; internalSec = 0.0; }
            for (int i = 0; i < kNumLfos; ++i) if (P.lfo[(size_t) i].retrig) lfoState[(size_t) i].phase = 0.0;
            if (P.oscReset && aEnv.v < 0.05f) { oscA.phase = 0.f; oscB.phase = 0.f; oscSub.phase = 0.f; fmPhase = 0.f; }
            if (! wasHeld && aEnv.v < 0.0001f) currentNote = targetNote;
        }
    }

    void noteOff (int note)
    {
        removeHeld (note);
        if (heldCount > 0) { targetNote = (float) held[heldCount - 1]; if (! P.legato) { fEnv.gateOn(); aEnv.gateOn(); mEnv.gateOn(); } }
        else { fEnv.gateOff(); aEnv.gateOff(); mEnv.gateOff(); }
    }
    void allNotesOff() { heldCount = 0; fEnv.gateOff(); aEnv.gateOff(); mEnv.gateOff(); }

    void render (float* outL, float* outR, int n)
    {
        prepareBlock();
        const double dppq = beatsPerSec() / (double) sr;
        for (int i = 0; i < n; ++i) { float l, r; renderSample (l, r); outL[i] = l; outR[i] = r; if (playing) ppq += dppq; }
    }

    // value of a step's LFO at phase p (unipolar 0..1) — shared with the GUI preview
    static float stepShape (int shape, float p, float rCur, float rPrev)
    {
        switch (shape)
        {
            case 0: return 0.5f - 0.5f * std::cos (kTwoPi * p);
            case 1: return p < 0.5f ? 2.f * p : 2.f - 2.f * p;
            case 2: return p;
            case 3: return 1.f - p;
            case 4: return p < 0.5f ? 1.f : 0.f;
            case 5: return 0.5f + 0.5f * rCur;
            case 6: { const float a = 0.5f + 0.5f * rPrev, b = 0.5f + 0.5f * rCur; return a + (b - a) * (0.5f - 0.5f * std::cos (kPi * p)); }
            case 7: return std::exp (-p * 6.f);
            case 8: { const float a = 0.5f + 0.5f * rPrev, b = 0.5f + 0.5f * rCur; return a + (b - a) * (p * p * (3.f - 2.f * p)); }
            case 9: return p < 0.12f ? 1.f - p / 0.12f : 0.f;
            default: return 0.f;
        }
    }

    static float normToHz (float n) { return 20.f * std::exp2 (clampf (n, 0.f, 1.f) * 9.8136f); } // 20 Hz .. 18 kHz
    static float hzToNorm (float hz) { return clampf (std::log2 (std::max (hz, 20.f) / 20.f) / 9.8136f, 0.f, 1.f); }

private:
    Params P;
    float sr = 48000.f, osr = 96000.f;
    double bpm = 120.0, ppq = 0.0, internalPos = 0.0, internalSec = 0.0;
    bool playing = false;

    int held[128] {}; int heldCount = 0;
    float targetNote = 60.f, currentNote = 60.f, velocity = 1.f, bend = 0.f, modWheelVal = 0.f, noteRandom = 0.f;

    MorphOsc oscA, oscB, oscSub;
    float fmPhase = 0.f, fmPrev1 = 0.f, fmPrev2 = 0.f;
    Ladder ladder; Decimator2x dec, decA;
    int osFactor = 2; float dcX = 0.f, dcY = 0.f, dcR = 0.9995f;
    float smCut = -1.f, smRes = 0.f, smDrive = 0.f, smFm = 0.f, smGain = 1.f, smMacro[kNumMacros] {}, smCoef = 1.f; Env fEnv, aEnv, mEnv; Rng rng;
    float lastFilterOut = 0.f, pitchEnvV = 0.f, pitchEnvCoef = 0.f;

    // sequencer
    int lastStep = -1, lastCycle = -1;
    float randCur = 0.f, randPrev = 0.f, chaosX = 0.37f, chaosCur = 0.f, chaosPrev = 0.f, wanderJit = 0.f, seqY = 0.f, seqCoef = 1.f;
    float seqUni = 0.f;

    // lfos
    struct LfoState { double phase = 0.0; float rnd = 0.f, rndPrev = 0.f, chaos = 0.4f; int lastCycle = -1; };
    std::array<LfoState, kNumLfos> lfoState {};
    float lfoVal[kNumLfos] {};

    // spit + chaos sources
    float spitV = 0.f, spitCoef = 0.f;
    double chaosPhase = 0.0; float chaosSrcX = 0.41f, chaosSrc = 0.f, chaosSrcPrev = 0.f, chaosSrcOut = 0.f;

    float modDst[NumDst] {};
    float glideCoef = 1.f, outGain = 1.f;
    int   ctrlCounter = 0;

    // fx state
    std::vector<float> combBuf; int combLen = 0, combW = 0; float combLp = 0.f;
    Svf formant[3]; float crushHold = 0.f; int crushCount = 0; float biteHold = 0.f; int biteCount = 0;
    float phZ[2][6] {}, phFb[2] {}, phA[2] {}; double phPhase = 0.0;
    std::vector<float> delayL, delayR; int delayBufLen = 0, delayW = 0; double delayLfo = 0.0;
    float dlTone[2] {}, dlHp[2] {}, delaySmoothed = 0.f;
    Fdn fdn;

    void removeHeld (int note)
    {
        int w = 0;
        for (int i = 0; i < heldCount; ++i) if (held[i] != note) held[w++] = held[i];
        heldCount = w;
    }

    void prepareBlock()
    {
        fEnv.set (P.fA, P.fD, P.fS, P.fR, P.fLoop, sr);
        aEnv.set (P.aA, P.aD, P.aS, P.aR, false, sr);
        mEnv.set (P.mA, P.mD, P.mS, P.mR, P.mLoop, sr);
        glideCoef = P.glide > 0.0005f ? 1.f - std::exp (-1.f / (P.glide * 0.35f * sr)) : 1.f;
        const float tau = 0.0005f + P.seqSmooth * P.seqSmooth * 0.2f;
        seqCoef = 1.f - std::exp (-1.f / (tau * sr));
        pitchEnvCoef = std::exp (-1.f / (std::max (0.001f, P.pitchEnvDecay) * sr));
        spitCoef = std::exp (-1.f / (std::max (0.002f, P.spitDecay) * sr));
        outGain = std::pow (10.f, P.outGainDb / 20.f);
        const int wantOs = (P.quality >= 1 && sr < 60000.f) ? 4 : 2;
        if (wantOs != osFactor) { osFactor = wantOs; osr = sr * (float) osFactor; dec.reset(); decA.reset(); }
        if (smCut < 0.f) { smCut = P.cutoff; smRes = P.resonance; smDrive = P.drive; smFm = P.fmIndex; smGain = outGain; for (int m = 0; m < kNumMacros; ++m) smMacro[m] = P.macro[(size_t) m]; }
    }

    double beatsPerSec() const { return bpm / 60.0; }

    // ------------------------------------------------------- sequencer --------
    float tickSequencer()
    {
        const int nSteps = std::max (1, std::min (kMaxSteps, P.numSteps));
        const double stepBeats = kDivBeats[std::max (0, std::min (kNumDivs - 1, P.stepDiv))];
        double pos;
        if (playing) pos = ppq / stepBeats;
        else { pos = internalPos; internalPos += beatsPerSec() / stepBeats / (double) sr; }
        internalSec += 1.0 / sr;

        if (P.swing > 0.001f)
        {
            const double pair = std::floor (pos * 0.5) * 2.0;
            const double t = (pos - pair) * 0.5;
            const double sp = 0.5 + 0.25 * (double) P.swing;
            pos = pair + (t < sp ? t / sp : 1.0 + (t - sp) / (1.0 - sp));
        }
        const double fl = std::floor (pos);
        int step = (int) (((long long) fl) % nSteps); if (step < 0) step += nSteps;
        const float frac = (float) (pos - fl);
        const StepParams& st = P.steps[(size_t) step];

        // how far through this step's own LFO are we?
        double cycles = 0.0; bool hold = false;
        const double stepSec = stepBeats / beatsPerSec();
        if (st.mode == 0)
        {
            const int idx = std::max (0, std::min (kNumStepSync - 1, st.sync));
            if (idx == 0) hold = true;
            else
            {
                const double period = kRateBeats[idx - 1];
                cycles = P.seqPhaseReset ? (double) frac * stepBeats / period : pos * stepBeats / period;
            }
        }
        else
        {
            const double hz = (double) std::max (0.01f, st.hz);
            cycles = P.seqPhaseReset ? (double) frac * stepSec * hz : internalSec * hz;
        }
        cycles *= (double) P.speedScale;
        const int cyc = (int) std::floor (cycles);
        const float ph = (float) (cycles - (double) cyc);

        if (step != lastStep) { lastStep = step; wanderJit = P.wander * rng.bi() * 0.25f; lastCycle = -1; }
        if (cyc != lastCycle)
        {
            lastCycle = cyc;
            randPrev = randCur; randCur = rng.bi();
            const float r = 3.5f + P.chaos * 0.5f;
            chaosX = clampf (r * chaosX * (1.f - chaosX), 0.0001f, 0.9999f);
            chaosPrev = chaosCur; chaosCur = chaosX * 2.f - 1.f;
        }
        float u = 1.f;
        if (! hold) u = st.shape == 8 ? stepShape (8, ph, chaosCur, chaosPrev) : stepShape (st.shape, ph, randCur, randPrev);
        seqUni = u;
        const float target = clampf (st.lo + (st.hi - st.lo) * u + wanderJit, 0.f, 1.f);
        seqY += (target - seqY) * seqCoef;

        uiStep.store (step, std::memory_order_relaxed);
        uiStepFrac.store (frac, std::memory_order_relaxed);
        return seqY;
    }

    // ------------------------------------------------------------ LFOs --------
    float tickLfo (int i, float rateMod)
    {
        const auto& lp = P.lfo[(size_t) i];
        auto& s = lfoState[(size_t) i];
        double cycles;
        if (lp.mode == 0)
        {
            const double period = kRateBeats[std::max (0, std::min (kNumRates - 1, lp.sync))] / (double) rateMod;
            if (playing && ! lp.retrig) cycles = ppq / period;
            else { s.phase += beatsPerSec() / period / sr; cycles = s.phase; }
        }
        else
        {
            s.phase += (double) (lp.hz * rateMod) / sr;
            cycles = s.phase;
        }
        const int cyc = (int) std::floor (cycles);
        const float p = (float) (cycles - (double) cyc);
        if (cyc != s.lastCycle)
        {
            s.lastCycle = cyc;
            s.rndPrev = s.rnd; s.rnd = rng.bi();
            s.chaos = clampf (3.9f * s.chaos * (1.f - s.chaos), 0.0001f, 0.9999f);
        }
        switch (lp.shape)
        {
            case 0: return std::sin (kTwoPi * p);
            case 1: return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
            case 2: return 2.f * p - 1.f;
            case 3: return 1.f - 2.f * p;
            case 4: return p < 0.5f ? 1.f : -1.f;
            case 5: return s.rnd;
            case 6: return s.rndPrev + (s.rnd - s.rndPrev) * (0.5f - 0.5f * std::cos (kPi * p));
            case 7: return s.chaos * 2.f - 1.f;
            default: return 0.f;
        }
    }

    // ------------------------------------------------------------ render -----
    void renderSample (float& outL, float& outR)
    {
        // ---------- smoothed continuous controls (no zipper on automation) ----------
        smCut += (P.cutoff - smCut) * smCoef; smRes += (P.resonance - smRes) * smCoef;
        smDrive += (P.drive - smDrive) * smCoef; smFm += (P.fmIndex - smFm) * smCoef; smGain += (outGain - smGain) * smCoef;
        for (int m = 0; m < kNumMacros; ++m) smMacro[m] += (P.macro[(size_t) m] - smMacro[m]) * smCoef;

        // ---------- mod sources ----------
        const float seqN = tickSequencer();
        for (int i = 0; i < kNumLfos; ++i)
            lfoVal[i] = tickLfo (i, std::exp2 (clampf (modDst[DstLfo1Rate + i], -1.f, 1.f) * 4.f));
        const float fe = fEnv.tick(), ae = aEnv.tick(), me = mEnv.tick();
        pitchEnvV *= pitchEnvCoef;

        // spit: sparse random wet transients
        const float spitHz = P.spitRate * std::exp2 (clampf (modDst[DstSpitRate], -1.f, 1.f) * 4.f);
        if (rng.next() < spitHz / sr) spitV = std::max (spitV, 1.f - P.spitRandom * rng.next());
        spitV *= spitCoef;

        // chaos source (smoothed logistic map)
        chaosPhase += P.chaosRate / sr;
        if (chaosPhase >= 1.0)
        {
            chaosPhase -= 1.0;
            chaosSrcX = clampf ((3.5f + P.chaosAmt * 0.5f) * chaosSrcX * (1.f - chaosSrcX), 0.0001f, 0.9999f);
            chaosSrcPrev = chaosSrc; chaosSrc = chaosSrcX * 2.f - 1.f;
        }
        const float cp = (float) chaosPhase;
        chaosSrcOut = chaosSrcPrev + (chaosSrc - chaosSrcPrev) * (cp * cp * (3.f - 2.f * cp));

        float src[NumSrc];
        src[SrcNone] = 0.f; src[SrcSeq] = seqUni; src[SrcLfo1] = lfoVal[0]; src[SrcLfo2] = lfoVal[1]; src[SrcLfo3] = lfoVal[2];
        src[SrcModEnv] = me; src[SrcFiltEnv] = fe; src[SrcAmpEnv] = ae; src[SrcPitchEnv] = pitchEnvV;
        src[SrcVelocity] = velocity; src[SrcModWheel] = modWheelVal; src[SrcKey] = clampf ((currentNote - 60.f) / 48.f, -1.f, 1.f);
        src[SrcRandom] = noteRandom; src[SrcSpit] = spitV; src[SrcChaos] = chaosSrcOut;
        for (int m = 0; m < kNumMacros; ++m) src[SrcMacro1 + m] = smMacro[m];

        // ---------- matrix ----------
        std::fill (std::begin (modDst), std::end (modDst), 0.f);
        for (int s = 0; s < kMatrixSlots; ++s)
        {
            const auto& sl = P.matrix[(size_t) s];
            if (sl.src <= 0 || sl.dst <= 0 || sl.src >= NumSrc || sl.dst >= NumDst) continue;
            float v = src[sl.src] * sl.amount;
            if (sl.via > 0 && sl.via < NumSrc) v *= src[sl.via];
            modDst[sl.dst] += v;
            if ((ctrlCounter & 63) == 0) uiSlot[(size_t) s].store (v, std::memory_order_relaxed);
        }
        const float* md = modDst;

        // ---------- pitch ----------
        currentNote += (targetNote - currentNote) * glideCoef;
        const float pitchSemis = currentNote - 69.f + bend * P.bendRange + P.pitchEnvAmt * pitchEnvV + md[DstPitch] * 24.f;
        const float fA = 440.f * std::exp2 ((pitchSemis + P.octA * 12.f + P.semiA + P.fineA * 0.01f) / 12.f);
        const float fB = 440.f * std::exp2 ((pitchSemis + P.octB * 12.f + P.semiB + P.fineB * 0.01f + md[DstPitchB] * 24.f) / 12.f);
        const float incA = fA / osr, incB = fB / osr;
        const float subInc = incA * (P.subOct == 0 ? 0.5f : 0.25f);

        // FM operator
        float fmFreq;
        if (P.fmMode == 0) fmFreq = fA * P.fmRatio * std::exp2 ((P.fmFine * 0.01f + md[DstFmRatio] * 24.f) / 12.f);
        else               fmFreq = P.fmHz * std::exp2 (md[DstFmRatio] * 4.f);
        const float fmInc = clampf (fmFreq / osr, 0.f, 0.45f);
        const float fmIdx = clampf (smFm + md[DstFmIndex], 0.f, 1.5f);
        const float pmDepth = fmIdx * fmIdx * 2.5f;               // in cycles
        const float fmFb = clampf (P.fmFeedback + md[DstFmFeedback], 0.f, 1.f) * 0.35f;

        // ---------- filter ----------
        const float velF = 1.f - P.velToFilter + P.velToFilter * velocity;
        const float baseOct = std::log2 (std::max (20.f, smCut));
        const float seqOct  = std::log2 (normToHz (seqN));
        const float depth   = P.seqOn ? clampf (P.seqDepth + md[DstSeqDepth], 0.f, 1.f) : 0.f;
        float cutOct = baseOct + (seqOct - baseOct) * depth
                     + P.envAmount * fe * 6.f * velF
                     + P.keyTrack * (currentNote - 60.f) / 12.f
                     + md[DstCutoff] * 6.f;
        const float fc = clampf (std::exp2 (cutOct), 16.f, std::min (20000.f, osr * 0.42f));
        const float g = std::tan (kPi * fc / osr), G = g / (1.f + g);
        const float res = clampf (smRes + P.resEnvAmount * fe + md[DstRes] * 0.6f, 0.f, 1.15f);
        const float k = res * 4.1f;
        const float drive = clampf (smDrive + md[DstDrive] * 0.6f, 0.f, 1.f);
        const float dGain = 1.f + drive * drive * 22.f, dBias = 0.15f * drive, dComp = 1.f / (1.f + drive * 2.2f);
        const float wA = clampf (P.waveA + md[DstWaveA] * 3.f, 0.f, 3.f);
        const float wB = clampf (P.waveB + md[DstWaveB] * 3.f, 0.f, 3.f);
        const float lvNoise = clampf (P.levelNoise + md[DstNoise], 0.f, 1.f);
        const float lvSub = clampf (P.levelSub + md[DstSub], 0.f, 1.f);
        const float biteAmt = clampf (P.biteAmount + md[DstBite], 0.f, 1.f);

        // ---------- voice @ 2x ----------
        for (int os = 0; os < osFactor; ++os)
        {
            const float op = std::sin (kTwoPi * (fmPhase + fmFb * (fmPrev1 + fmPrev2)));
            fmPrev2 = fmPrev1; fmPrev1 = op;
            fmPhase += fmInc; if (fmPhase >= 1.f) fmPhase -= 1.f;
            const float pm = op * pmDepth;
            const float pmA = (P.fmTarget == 0 || P.fmTarget == 2) ? pm : 0.f;
            const float pmB = (P.fmTarget == 1 || P.fmTarget == 2) ? pm : 0.f;

            const float a = oscA.tick (incA, wA, pmA);
            if (P.syncB && oscA.wrapped) oscB.phase = oscA.phase * (incB / std::max (incA, 1e-9f));
            const float b = oscB.tick (incB, wB, pmB);
            const float sub = oscSub.tick (subInc, 2.f, 0.f);

            float mix = (a * P.levelA + b * P.levelB + op * P.levelFm + sub * lvSub + rng.bi() * lvNoise) * 0.5f;
            mix += lastFilterOut * P.feedback * 1.8f;
            float x = fastTanh (mix * dGain + dBias) - fastTanh (dBias);
            x *= (1.f + drive * 0.6f);
            if (! P.bitePost) x = bite (x, biteAmt);
            ladder.process (x, G, k);
            float y = ladder.output (P.slope, P.filterMode) * (1.f + k * 0.28f);
            if (P.bitePost) y = bite (y, biteAmt);
            lastFilterOut = y;
            const float yo = y * dComp * 1.6f;
            if (osFactor == 4) { decA.push (yo); if (os & 1) dec.push (decA.out()); }
            else dec.push (yo);
        }
        float v = dec.out();

        // ---------- comb ----------
        const float combMix = clampf (P.combMix + md[DstCombMix], 0.f, 1.f);
        if (combMix > 0.001f)
        {
            float cf = P.combKey ? (440.f * std::exp2 ((currentNote - 69.f + P.combSemi) / 12.f)) : P.combHz;
            cf *= std::exp2 (md[DstCombFreq] * 4.f);
            cf = clampf (cf, 16.f, sr * 0.45f);
            const float dS = clampf (sr / cf, 2.f, (float) combLen - 3.f);
            float rp = (float) combW - dS; while (rp < 0.f) rp += (float) combLen;
            const int i0 = (int) rp; const float t = rp - (float) i0;
            const float d = combBuf[(size_t) i0] + (combBuf[(size_t) ((i0 + 1) % combLen)] - combBuf[(size_t) i0]) * t;
            combLp += (d - combLp) * (1.f - P.combDamp * 0.9f);
            const float fb = clampf (P.combFeedback + md[DstCombFb], -0.98f, 0.98f);
            const float w = fastTanh (v + combLp * fb);
            combBuf[(size_t) combW] = w; combW = (combW + 1) % combLen;
            v = v + (w * 0.8f - v) * combMix;
        }

        const float amp = ae * (1.f - P.velToAmp + P.velToAmp * velocity) * clampf (1.f + md[DstVolume], 0.f, 2.f);
        v *= amp;

        // ---------- talk ----------
        const float talkMix = clampf (P.talkMix + md[DstTalkMix], 0.f, 1.f);
        if (talkMix > 0.001f)
        {
            if ((ctrlCounter & 7) == 0)
            {
                const float pos = clampf ((P.vowel + md[DstVowel]) * 4.f, 0.f, 4.f);
                static const float F[5][3] = { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 280, 2250, 2900 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
                const int i0 = std::min (3, (int) pos); const float t = pos - (float) i0;
                for (int b = 0; b < 3; ++b) formant[b].set (F[i0][b] + (F[i0 + 1][b] - F[i0][b]) * t, 9.f, sr);
            }
            const float f = formant[0].bp (v) + formant[1].bp (v) * 0.7f + formant[2].bp (v) * 0.35f;
            v = v + (f * 1.4f - v) * talkMix;
        }

        // ---------- crush ----------
        if (P.crush > 0.001f)
        {
            const int hold = 1 + (int) (P.crush * P.crush * 24.f);
            if (++crushCount >= hold) { crushCount = 0; crushHold = v; }
            const float q = std::exp2 (15.f - P.crush * 12.f);
            v = v + (std::round (crushHold * q) / q - v) * std::min (1.f, P.crush * 2.f);
        }

        // ---------- DC blocker (asymmetric drive/bite leave offset) ----------
        { const float y = v - dcX + dcR * dcY; dcX = v; dcY = y; v = y; }

        // ---------- pan ----------
        const float pan = clampf (md[DstPan], -1.f, 1.f);
        float L = v * std::sqrt (0.5f * (1.f - pan)) * 1.4142f;
        float R = v * std::sqrt (0.5f * (1.f + pan)) * 1.4142f;

        // ---------- phaser ----------
        if (P.phaserMix > 0.001f)
        {
            if ((ctrlCounter & 15) == 0)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float l = 0.5f + 0.5f * std::sin ((float) (kTwoPi * phPhase) + (ch ? kPi * 0.5f : 0.f));
                    const float t = std::tan (kPi * clampf (180.f * std::exp2 (l * P.phaserDepth * 6.f), 20.f, sr * 0.45f) / sr);
                    phA[ch] = (t - 1.f) / (t + 1.f);
                }
            phPhase += P.phaserRate / sr; if (phPhase >= 1.0) phPhase -= 1.0;
            float* io[2] = { &L, &R };
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = *io[ch] + phFb[ch] * P.phaserFb * 0.9f;
                for (int s = 0; s < 6; ++s) { const float y = phA[ch] * x + phZ[ch][s]; phZ[ch][s] = x - phA[ch] * y; x = y; }
                phFb[ch] = x;
                *io[ch] = *io[ch] * (1.f - P.phaserMix * 0.5f) + x * P.phaserMix * 0.5f;
            }
        }

        // ---------- delay ----------
        const float dMix = clampf (P.delayMix + md[DstDelayMix], 0.f, 1.f);
        if (dMix > 0.001f || delaySmoothed > 0.0001f)
        {
            const float target = (float) (kDivBeats[std::max (0, std::min (kNumDivs - 1, P.delayDiv))] * 60.0 / bpm) * sr;
            delaySmoothed += (target - delaySmoothed) * 0.0005f;
            delayLfo += 0.35 / sr; if (delayLfo >= 1.0) delayLfo -= 1.0;
            const float dS = clampf (delaySmoothed * (1.f + P.delayMod * 0.006f * std::sin ((float) (kTwoPi * delayLfo))), 4.f, (float) delayBufLen - 4.f);
            const float rl = readDelay (delayL, dS), rr = readDelay (delayR, dS * (P.pingPong ? 1.f : 1.013f));
            const float tc = 0.05f + P.delayTone * 0.9f;
            dlTone[0] += (rl - dlTone[0]) * tc; dlTone[1] += (rr - dlTone[1]) * tc;
            dlHp[0] += (dlTone[0] - dlHp[0]) * 0.01f; dlHp[1] += (dlTone[1] - dlHp[1]) * 0.01f;
            const float fbL = (dlTone[0] - dlHp[0]) * P.delayFb, fbR = (dlTone[1] - dlHp[1]) * P.delayFb;
            if (P.pingPong) { delayL[(size_t) delayW] = fastTanh ((L + R) * 0.5f + fbR); delayR[(size_t) delayW] = fastTanh (fbL); }
            else            { delayL[(size_t) delayW] = fastTanh (L + fbL); delayR[(size_t) delayW] = fastTanh (R + fbR); }
            delayW = (delayW + 1) % delayBufLen;
            L += rl * dMix; R += rr * dMix;
            if (dMix <= 0.001f) delaySmoothed *= 0.9999f;
        }

        // ---------- reverb ----------
        if (P.reverbMix > 0.001f)
        {
            float x = (L + R) * 0.5f;
            for (int a = 0; a < 2; ++a)
            {
                auto& b = fdn.ap[a]; int& ix = fdn.apIdx[a];
                const float d = b[(size_t) ix]; const float y = -0.6f * x + d;
                b[(size_t) ix] = x + 0.6f * y; ix = (ix + 1) % fdn.apLen[a]; x = y;
            }
            const float size = 0.45f + P.reverbSize * 1.3f, fbG = 0.72f + P.reverbSize * 0.255f, damp = 0.1f + (1.f - P.reverbDamp) * 0.85f;
            float o[Fdn::L]; float sum = 0.f;
            for (int i = 0; i < Fdn::L; ++i)
            {
                const int len = std::min ((int) fdn.line[i].size() - 2, (int) (fdn.len[i] * size));
                int rd = fdn.idx[i] - len; if (rd < 0) rd += (int) fdn.line[i].size();
                fdn.lp[i] += (fdn.line[i][(size_t) rd] - fdn.lp[i]) * damp;
                o[i] = fdn.lp[i]; sum += o[i];
            }
            sum *= 2.f / Fdn::L;
            float wl = 0.f, wr = 0.f;
            for (int i = 0; i < Fdn::L; ++i)
            {
                fdn.line[i][(size_t) fdn.idx[i]] = (o[i] - sum) * fbG + x * 0.35f;
                fdn.idx[i] = (fdn.idx[i] + 1) % (int) fdn.line[i].size();
                if (i & 1) wr += o[i]; else wl += o[i];
            }
            L += wl * 0.35f * P.reverbMix; R += wr * 0.35f * P.reverbMix;
        }

        outL = softClip (L * smGain); outR = softClip (R * smGain);

        if ((ctrlCounter & 63) == 0)
        {
            uiSeqValue.store (seqN, std::memory_order_relaxed);
            uiCutoffHz.store (fc, std::memory_order_relaxed);
            uiEnv.store (ae, std::memory_order_relaxed);
            for (int s = 0; s < NumSrc; ++s) uiSrc[(size_t) s].store (src[s], std::memory_order_relaxed);
            const int w = scopeWrite.load (std::memory_order_relaxed);
            scope[w] = seqN;
            scopeWrite.store ((w + 1) % kScopeSize, std::memory_order_relaxed);
        }
        ++ctrlCounter;
    }

    float bite (float x, float a)
    {
        if (a < 0.001f) return x;
        float y;
        switch (P.biteType)
        {
            case 0: y = fastTanh (x * (1.f + a * 12.f)) * (1.f - a * 0.3f); break;
            case 1: y = std::sin (x * (1.f + a * 7.f) * 1.5707963f) * 0.8f; break;
            case 2: { const float g = x * (1.f + a * 10.f); y = (g > 0.f ? fastTanh (g) : fastTanh (g * 0.35f) * 1.6f) * 0.8f; } break;
            case 3: { const float g = fastTanh (x * (1.f + a * 6.f)); y = g + (std::fabs (g) * 1.6f - 0.5f - g) * a; } break;
            default: { const int hold = 1 + (int) (a * a * 40.f);
                       if (++biteCount >= hold) { biteCount = 0; biteHold = x; }
                       const float q = std::exp2 (14.f - a * 11.f); y = std::round (biteHold * q) / q; } break;
        }
        return x + (y - x) * P.biteMix;
    }

    float readDelay (const std::vector<float>& b, float d) const
    {
        float rp = (float) delayW - d; while (rp < 0.f) rp += (float) delayBufLen;
        const int i0 = (int) rp; const float t = rp - (float) i0; const int i1 = (i0 + 1) % delayBufLen;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * t;
    }
    static float softClip (float x)
    {
        const float a = std::fabs (x);
        if (a < 0.85f) return x;
        return (x < 0.f ? -1.f : 1.f) * (0.85f + 0.15f * fastTanh ((a - 0.85f) / 0.15f));
    }
};
} // namespace sq
