// =============================================================================
//  SQUALCH-32  —  psychedelic step-LFO ladder synth
//  Pure C++17 DSP engine (no framework dependency) so it can be tested offline.
//
//  Signal flow (voice runs at 2x oversampling):
//    OSC1 (morph tri>saw>square>pulse) ─┐
//    OSC2 (morph, hard-sync, FM)  ──────┤
//    SUB  (-1/-2 oct square)      ──────┼─ MIXER ─ MULTIDRIVE ─ LADDER (6/12/18/24, LP/BP/HP) ─ VCA
//    NOISE                        ──────┤                ▲                  │
//    FEEDBACK (filter out → mixer) ─────┘                └──── feedback ────┘
//    → decimate → TALK (formant vowels) → CRUSH → PHASER → PING-PONG DELAY → FDN REVERB → OUT
//
//  Modulation: 32-step LFO sequencer — every step is its own LFO
//  (own rate in cycles-per-step, own shape, own depth and own offset) driving
//  the ladder cutoff in octaves, plus routable amounts to resonance / pitch /
//  drive / talk-vowel.  Free LFO2, looping filter envelope, amp envelope.
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
constexpr float kPi       = 3.14159265358979f;
constexpr float kTwoPi    = 6.28318530717959f;

inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float fastTanh (float x)
{
    // Pade approximant, accurate enough for saturation, clamped for stability
    x = clampf (x, -4.5f, 4.5f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ---------------------------------------------------------------- tables -----
// Cycles of the step-LFO per step (0 = static step, like a classic Moog step seq)
inline constexpr float kStepRates[]     = { 0.f, 0.125f, 0.25f, 0.5f, 1.f, 1.5f, 2.f, 3.f, 4.f, 6.f, 8.f, 12.f, 16.f };
inline constexpr const char* kStepRateNames[] = { "OFF", "1/8", "1/4", "1/2", "1", "1.5", "2", "3", "4", "6", "8", "12", "16" };
constexpr int kNumStepRates = 13;

inline constexpr const char* kShapeNames[] = { "Sine", "Tri", "Ramp Up", "Ramp Down", "Square", "S&H", "Smooth Rnd", "Squelch", "Chaos" };
constexpr int kNumShapes = 9;

inline constexpr const char* kBiteNames[] = { "Soft", "Fold", "Asym", "Rectify", "Crush" };
constexpr int kNumBites = 5;

// Note divisions in quarter-note beats
inline constexpr float kDivBeats[]          = { 1.f/16.f, 1.f/8.f, 1.f/6.f, 1.f/4.f, 3.f/8.f, 1.f/3.f, 1.f/2.f, 3.f/4.f, 2.f/3.f, 1.f, 2.f, 4.f };
inline constexpr const char* kDivNames[]    = { "1/64", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/2", "1 Bar" };
constexpr int kNumDivs = 12;

inline constexpr const char* kLfo2ShapeNames[] = { "Sine", "Tri", "Saw", "Square", "S&H", "Smooth Rnd" };
constexpr int kNumLfo2Shapes = 6;

// ---------------------------------------------------------------- params -----
struct StepParams
{
    int   rate   = 4;     // index into kStepRates
    int   shape  = 0;     // index into kShapeNames
    float depth  = 0.f;   // -1..1
    float offset = 0.f;   // -1..1
};

struct Params
{
    // oscillators
    float osc1Wave = 1.f, osc2Wave = 1.f;       // 0..3  tri > saw > square > narrow pulse
    int   osc1Oct  = 0,   osc2Oct  = 0;         // -2..2
    float osc1Semi = 0.f, osc2Semi = 0.f;       // -12..12
    float osc1Fine = 0.f, osc2Fine = 0.f;       // cents -100..100
    bool  osc2Sync = false;
    float fmAmount = 0.f;                       // 0..1 osc1 -> osc2 FM
    // mixer
    float osc1Level = 0.8f, osc2Level = 0.f, subLevel = 0.3f, noiseLevel = 0.f, feedback = 0.f;
    int   subOct = 0;                           // 0 = -1 oct, 1 = -2 oct
    // ladder filter
    float cutoff = 600.f;                       // Hz
    float resonance = 0.6f;                     // 0..1.1 (self-osc around 1)
    float drive = 0.25f;                        // 0..1 multidrive
    float slope = 3.f;                          // 0..3 continuous (6/12/18/24 dB)
    int   filterMode = 0;                       // 0 LP, 1 BP, 2 HP
    float keyTrack = 0.3f;                      // 0..1
    float envAmount = 0.4f;                     // -1..1  (x 6 octaves)
    float resEnvAmount = 0.f;                   // -1..1  squelch: env -> resonance
    // envelopes (seconds)
    float fA = 0.002f, fD = 0.25f, fS = 0.f, fR = 0.2f;   bool fLoop = false;
    float aA = 0.002f, aD = 0.3f,  aS = 0.85f, aR = 0.15f;
    float velToFilter = 0.3f, velToAmp = 0.3f;
    // performance
    float glide = 0.04f; bool legato = true; float bendRange = 2.f;
    // step-LFO sequencer
    bool  seqOn = true, seqRetrig = true;
    int   numSteps = 16, stepDiv = 3;
    float seqAmount = 2.5f;     // octaves of cutoff modulation 0..6
    float seqSmooth = 0.15f;    // 0..1
    float speedScale = 1.f;     // 0.25..4 global multiplier of every step-LFO
    float wander = 0.f;         // 0..1 random drift per step
    float swing = 0.f;          // 0..0.75 delays every 2nd step
    float chaos = 0.6f;         // 0..1 logistic-map chaos character (shape "Chaos")
    float seqToRes = 0.f, seqToPitch = 0.f, seqToDrive = 0.f, seqToTalk = 0.f; // -1..1
    std::array<StepParams, kMaxSteps> steps {};
    // LFO 2
    float lfo2Rate = 0.25f; int lfo2Shape = 0;
    float lfo2ToPitch = 0.f, lfo2ToCutoff = 0.f, lfo2ToWave = 0.f, lfo2ToRes = 0.f;   // -1..1
    // talk (formant) + crush
    float talkMix = 0.f, talkVowel = 0.f, crush = 0.f;
    // BITE! creative distortion
    int biteType = 0; float biteAmount = 0.f; bool bitePost = true; float biteMix = 1.f;
    // phaser
    float phaserMix = 0.f, phaserRate = 0.15f, phaserDepth = 0.7f, phaserFb = 0.5f;
    // delay
    float delayMix = 0.f, delayFb = 0.45f, delayTone = 0.5f, delayMod = 0.2f; int delayDiv = 7; bool pingPong = true;
    // reverb
    float reverbMix = 0.f, reverbSize = 0.7f, reverbDamp = 0.5f;
    // master
    float outGainDb = 0.f;
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
    if (t < dt)        { t /= dt;             return t + t - t * t - 1.f; }
    if (t > 1.f - dt)  { t = (t - 1.f) / dt;  return t * t + t + t + 1.f; }
    return 0.f;
}

// Morphing oscillator: 0 tri, 1 saw, 2 square, 3 narrow pulse (continuous)
struct MorphOsc
{
    float phase = 0.f;
    bool  wrapped = false;

    float tick (float inc, float morph)
    {
        inc = clampf (inc, -0.49f, 0.49f);
        const float dt = std::fabs (inc) + 1e-9f;
        const float t  = phase;
        float out;
        if (morph < 1.f)
        {
            const float tri = t < 0.5f ? 4.f * t - 1.f : 3.f - 4.f * t;
            const float saw = (2.f * t - 1.f) - polyBlep (t, dt);
            out = tri + (saw - tri) * morph;
        }
        else if (morph < 2.f)
        {
            const float m   = morph - 1.f;
            const float saw = (2.f * t - 1.f) - polyBlep (t, dt);
            out = saw + (pulse (t, dt, 0.5f) - saw) * m;
        }
        else
        {
            const float pw = 0.5f - (morph - 2.f) * 0.44f;
            out = pulse (t, dt, pw);
        }
        phase += inc;
        wrapped = false;
        if (phase >= 1.f) { phase -= std::floor (phase); wrapped = true; }
        else if (phase < 0.f) { phase -= std::floor (phase); }
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

// ---------------------------------------------------------------- ladder -----
// Zero-delay-feedback 4-pole ladder with saturating feedback path.
struct Ladder
{
    float s[4] {};
    float y[5] {};

    void reset() { std::memset (s, 0, sizeof (s)); std::memset (y, 0, sizeof (y)); }

    // G = g/(1+g), k = 0..~4.4
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
            const float v   = (in - s[i]) * G;
            const float out = v + s[i];
            s[i] = out + v;
            in = out;
            y[i + 1] = out;
        }
        // gentle per-stage saturation of state = analog-ish compression
        s[0] = fastTanh (s[0] * 0.8f) * 1.25f;
    }

    inline float output (float slope, int mode) const
    {
        if (mode == 0)
        {
            const float sl = clampf (slope, 0.f, 3.f);
            const int   i  = std::min (2, (int) sl);
            const float f  = sl - (float) i;
            return y[i + 1] + (y[i + 2] - y[i + 1]) * f;
        }
        if (mode == 1)
        {
            const float bp2 = 2.f * (y[1] - y[2]);
            const float bp4 = 4.f * (y[2] - 2.f * y[3] + y[4]);
            const float f = clampf (slope / 3.f, 0.f, 1.f);
            return bp2 + (bp4 - bp2) * f;
        }
        const float hp2 = y[0] - 2.f * y[1] + y[2];
        const float hp4 = y[0] - 4.f * y[1] + 6.f * y[2] - 4.f * y[3] + y[4];
        const float f = clampf (slope / 3.f, 0.f, 1.f);
        return hp2 + (hp4 - hp2) * f;
    }
};

// ------------------------------------------------------------------- SVF -----
struct Svf
{
    float ic1 = 0.f, ic2 = 0.f, a1 = 0.f, a2 = 0.f, a3 = 0.f, k = 1.f;
    void set (float freq, float q, float sr)
    {
        const float g = std::tan (kPi * clampf (freq, 20.f, sr * 0.45f) / sr);
        k  = 1.f / q;
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    float bp (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1;
        ic2 = 2.f * v2 - ic2;
        return v1;
    }
    void reset() { ic1 = ic2 = 0.f; }
};

// -------------------------------------------------------------- envelope -----
struct Env
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage st = Idle;
    float v = 0.f;
    float aC = 1.f, dC = 1.f, rC = 1.f, sus = 0.f;
    bool  loop = false;

    static float coef (float t, float sr) { return 1.f - std::exp (-4.6f / (std::max (t, 0.0005f) * sr)); }
    void set (float a, float d, float s, float r, bool lp, float sr)
    {
        aC = coef (a, sr) * 1.6f; dC = coef (d, sr); rC = coef (r, sr); sus = s; loop = lp;
        aC = std::min (aC, 1.f);
    }
    void gateOn()  { st = Attack; }
    void gateOff() { if (st != Idle) st = Release; }
    float tick()
    {
        switch (st)
        {
            case Attack:  v += (1.25f - v) * aC; if (v >= 1.f) { v = 1.f; st = Decay; } break;
            case Decay:   v += (sus - v) * dC;
                          if (std::fabs (v - sus) < 0.003f) { if (loop) st = Attack; else st = Sustain; }
                          break;
            case Sustain: v = sus; break;
            case Release: v += (0.f - v) * rC; if (v < 1e-5f) { v = 0.f; st = Idle; } break;
            case Idle:    default: v = 0.f; break;
        }
        return v;
    }
};

// ---------------------------------------------------- 2x FIR decimator -------
struct Decimator2x
{
    static constexpr int N = 63;
    float h[N] {};
    float buf[N * 2] {};
    int   pos = 0;

    Decimator2x()
    {
        const float fc = 0.22f; // of oversampled rate
        float sum = 0.f;
        for (int i = 0; i < N; ++i)
        {
            const float m = (float) i - (N - 1) * 0.5f;
            const float sinc = m == 0.f ? 2.f * fc : std::sin (kTwoPi * fc * m) / (kPi * m);
            const float w = 0.42f - 0.5f * std::cos (kTwoPi * i / (N - 1)) + 0.08f * std::cos (2.f * kTwoPi * i / (N - 1));
            h[i] = sinc * w;
            sum += h[i];
        }
        for (auto& c : h) c /= sum;
    }
    void reset() { std::memset (buf, 0, sizeof (buf)); pos = 0; }
    void push (float x) { buf[pos] = x; buf[pos + N] = x; pos = (pos + 1) % N; }
    float out() const
    {
        float acc = 0.f;
        const float* b = buf + pos; // oldest first
        for (int i = 0; i < N; ++i) acc += h[i] * b[i];
        return acc;
    }
};

// ------------------------------------------------------------- phaser --------
struct Phaser
{
    float z[2][6] {};
    float fbS[2] {};
    float a[2] = { 0.f, 0.f };
    double ph = 0.0;
    void reset() { std::memset (z, 0, sizeof (z)); fbS[0] = fbS[1] = 0.f; }
};

// ------------------------------------------------------------- reverb --------
struct Fdn
{
    static constexpr int L = 8;
    std::vector<float> line[L];
    int len[L] {}, idx[L] {};
    float lp[L] {};
    std::vector<float> ap[2]; int apLen[2] {}, apIdx[2] {};
    void prepare (float sr)
    {
        static const int base[L] = { 1031, 1327, 1523, 1871, 2053, 2311, 2539, 2803 };
        for (int i = 0; i < L; ++i) { line[i].assign ((size_t) (base[i] * 1.8f * sr / 48000.f) + 8, 0.f); idx[i] = 0; lp[i] = 0.f; }
        const int apb[2] = { 241, 557 };
        for (int i = 0; i < 2; ++i) { apLen[i] = (int) (apb[i] * sr / 48000.f); ap[i].assign ((size_t) apLen[i] + 1, 0.f); apIdx[i] = 0; }
        for (int i = 0; i < L; ++i) len[i] = (int) (base[i] * sr / 48000.f);
    }
    void reset() { for (auto& l : line) std::fill (l.begin(), l.end(), 0.f); for (auto& a : ap) std::fill (a.begin(), a.end(), 0.f); std::memset (lp, 0, sizeof (lp)); }
};

// ================================================================ ENGINE =====
class Engine
{
public:
    // ---- GUI feedback (lock-free, written by audio thread) ----
    std::atomic<int>   uiStep { 0 };
    std::atomic<float> uiStepFrac { 0.f };
    std::atomic<float> uiSeqValue { 0.f };
    std::atomic<float> uiCutoffHz { 600.f };
    std::atomic<float> uiEnv { 0.f };
    static constexpr int kScopeSize = 256;
    float scope[kScopeSize] {};
    std::atomic<int> scopeWrite { 0 };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        osr = sr * 2.f;
        delayBufLen = (int) (sr * 4.0f) + 4;
        delayL.assign ((size_t) delayBufLen, 0.f);
        delayR.assign ((size_t) delayBufLen, 0.f);
        fdn.prepare (sr);
        reset();
    }

    void reset()
    {
        ladder.reset(); dec.reset(); phaser.reset(); fdn.reset();
        std::fill (delayL.begin(), delayL.end(), 0.f);
        std::fill (delayR.begin(), delayR.end(), 0.f);
        for (auto& f : formant) f.reset();
        osc1 = {}; osc2 = {}; subOsc = {};
        fEnv = {}; aEnv = {};
        heldCount = 0; lastFilterOut = 0.f; seqY = 0.f; internalPos = 0.0;
        dlTone[0] = dlTone[1] = dlHp[0] = dlHp[1] = 0.f;
        crushHold = 0.f; crushCount = 0;
    }

    void setParams (const Params& p) { P = p; }

    void setTransport (double bpmIn, double ppqIn, bool playingIn)
    {
        bpm = bpmIn > 1.0 ? bpmIn : 120.0;
        ppq = ppqIn;
        playing = playingIn;
    }

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
            fEnv.gateOn(); aEnv.gateOn();
            if (P.seqRetrig && ! playing) internalPos = 0.0;
            if (! wasHeld && aEnv.v < 0.0001f) currentNote = targetNote; // no glide from silence
        }
    }

    void noteOff (int note)
    {
        removeHeld (note);
        if (heldCount > 0)
        {
            targetNote = (float) held[heldCount - 1];
            if (! P.legato) { fEnv.gateOn(); aEnv.gateOn(); }
        }
        else
        {
            fEnv.gateOff(); aEnv.gateOff();
        }
    }

    void allNotesOff() { heldCount = 0; fEnv.gateOff(); aEnv.gateOff(); }

    // ------------------------------------------------------------ render -----
    void render (float* outL, float* outR, int n)
    {
        prepareBlock();
        for (int i = 0; i < n; ++i)
        {
            float l, r;
            renderSample (l, r);
            outL[i] = l; outR[i] = r;
        }
    }

private:
    Params P;
    float sr = 48000.f, osr = 96000.f;
    double bpm = 120.0, ppq = 0.0, internalPos = 0.0;
    bool playing = false;

    int held[128] {}; int heldCount = 0;
    float targetNote = 60.f, currentNote = 60.f, velocity = 1.f, bend = 0.f, modWheelVal = 0.f;

    MorphOsc osc1, osc2, subOsc;
    Ladder ladder; Decimator2x dec; Env fEnv, aEnv; Rng rng;
    float lastFilterOut = 0.f;

    // sequencer state
    int   lastStep = -1, lastCycle = -1;
    float randCur = 0.f, randPrev = 0.f, wanderJit = 0.f, seqY = 0.f, seqCoef = 1.f;
    float chaosX = 0.37f, chaosCur = 0.f, chaosPrev = 0.f;
    // lfo2
    double lfo2Phase = 0.0; float lfo2Rand = 0.f, lfo2RandPrev = 0.f;
    // block constants
    float glideCoef = 1.f, outGain = 1.f;
    int   ctrlCounter = 0;

    // fx
    Svf formant[3]; float talkPos = 0.f;
    float crushHold = 0.f; int crushCount = 0;
    Phaser phaser;
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
        glideCoef = P.glide > 0.0005f ? 1.f - std::exp (-1.f / (P.glide * 0.35f * sr)) : 1.f;
        const float tau = 0.0006f + P.seqSmooth * P.seqSmooth * 0.25f;
        seqCoef = 1.f - std::exp (-1.f / (tau * sr));
        outGain = std::pow (10.f, P.outGainDb / 20.f);
    }

    static float shapeValue (int shape, float p, float rCur, float rPrev)
    {
        switch (shape)
        {
            case 0: return std::sin (kTwoPi * p);
            case 1: return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
            case 2: return 2.f * p - 1.f;
            case 3: return 1.f - 2.f * p;
            case 4: return p < 0.5f ? 1.f : -1.f;
            case 5: return rCur;
            case 6: return rPrev + (rCur - rPrev) * (0.5f - 0.5f * std::cos (kPi * p));
            case 7: return std::exp (-p * 7.f) * 1.6f - 0.35f; // squelch pluck (mostly positive)
            case 8: return rPrev + (rCur - rPrev) * (p * p * (3.f - 2.f * p)); // chaos (fed with logistic map)
            default: return 0.f;
        }
    }

    float tickSequencer()
    {
        const int   nSteps    = std::max (1, std::min (kMaxSteps, P.numSteps));
        const double stepBeats = kDivBeats[std::max (0, std::min (kNumDivs - 1, P.stepDiv))];
        double pos;
        if (playing)
        {
            pos = ppq / stepBeats;
        }
        else
        {
            pos = internalPos;
            internalPos += (bpm / 60.0) / stepBeats / (double) sr;
        }
        // swing: warp time inside each pair of steps
        if (P.swing > 0.001f)
        {
            const double pair = std::floor (pos * 0.5) * 2.0;
            const double t = (pos - pair) * 0.5;               // 0..1 over two steps
            const double sp = 0.5 + 0.5 * (double) P.swing * 0.5; // split point
            pos = pair + (t < sp ? t / sp : 1.0 + (t - sp) / (1.0 - sp));
        }
        const double fl = std::floor (pos);
        int step = (int) (((long long) fl) % nSteps);
        if (step < 0) step += nSteps;
        const float frac = (float) (pos - fl);

        const StepParams& sp = P.steps[(size_t) step];
        const float cycles = kStepRates[std::max (0, std::min (kNumStepRates - 1, sp.rate))] * P.speedScale;
        const float lp = frac * cycles;
        const int   cycle = (int) std::floor (lp);
        const float ph = lp - (float) cycle;

        if (step != lastStep)
        {
            lastStep = step;
            wanderJit = P.wander * rng.bi() * 0.6f;
            lastCycle = -1;
        }
        if (cycle != lastCycle)
        {
            lastCycle = cycle;
            randPrev = randCur;
            randCur = rng.bi();
            const float r = 3.5f + P.chaos * 0.5f;          // logistic map, 3.5 = periodic, 4 = full chaos
            chaosX = clampf (r * chaosX * (1.f - chaosX), 0.0001f, 0.9999f);
            chaosPrev = chaosCur; chaosCur = chaosX * 2.f - 1.f;
        }

        float lfo = 0.f;
        if (cycles > 0.f) lfo = sp.shape == 8 ? shapeValue (8, ph, chaosCur, chaosPrev)
                                               : shapeValue (sp.shape, ph, randCur, randPrev);
        const float target = sp.offset + sp.depth * lfo + wanderJit;
        seqY += (target - seqY) * seqCoef;

        uiStep.store (step, std::memory_order_relaxed);
        uiStepFrac.store (frac, std::memory_order_relaxed);
        return P.seqOn ? seqY : 0.f;
    }

    float tickLfo2()
    {
        const double inc = (double) P.lfo2Rate / sr;
        lfo2Phase += inc;
        if (lfo2Phase >= 1.0) { lfo2Phase -= 1.0; lfo2RandPrev = lfo2Rand; lfo2Rand = rng.bi(); }
        const float p = (float) lfo2Phase;
        switch (P.lfo2Shape)
        {
            case 0: return std::sin (kTwoPi * p);
            case 1: return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
            case 2: return 2.f * p - 1.f;
            case 3: return p < 0.5f ? 1.f : -1.f;
            case 4: return lfo2Rand;
            case 5: return lfo2RandPrev + (lfo2Rand - lfo2RandPrev) * (0.5f - 0.5f * std::cos (kPi * p));
            default: return 0.f;
        }
    }

    void renderSample (float& outL, float& outR)
    {
        // ---------------- control ----------------
        const float seq  = tickSequencer();
        const float lfo2 = tickLfo2();
        const float fe   = fEnv.tick();
        const float ae   = aEnv.tick();

        currentNote += (targetNote - currentNote) * glideCoef;

        const float seqAmt = P.seqAmount * (1.f + modWheelVal * 1.5f);
        float pitchSemis = currentNote - 69.f + bend * P.bendRange
                         + P.lfo2ToPitch * lfo2 * 12.f + P.seqToPitch * seq * 12.f;

        const float f1 = 440.f * std::exp2 ((pitchSemis + P.osc1Oct * 12.f + P.osc1Semi + P.osc1Fine * 0.01f) / 12.f);
        const float f2 = 440.f * std::exp2 ((pitchSemis + P.osc2Oct * 12.f + P.osc2Semi + P.osc2Fine * 0.01f) / 12.f);
        const float inc1 = f1 / osr, inc2 = f2 / osr;
        const float subInc = inc1 * (P.subOct == 0 ? 0.5f : 0.25f);

        const float velF = 1.f - P.velToFilter + P.velToFilter * velocity;
        float cutOct = std::log2 (std::max (20.f, P.cutoff))
                     + P.envAmount * fe * 6.f * velF
                     + P.keyTrack * (currentNote - 60.f) / 12.f
                     + seqAmt * seq
                     + P.lfo2ToCutoff * lfo2 * 3.f;
        const float fc = clampf (std::exp2 (cutOct), 16.f, std::min (20000.f, osr * 0.42f));
        const float g  = std::tan (kPi * fc / osr);
        const float G  = g / (1.f + g);

        float res = P.resonance + P.resEnvAmount * fe + P.seqToRes * seq * 0.6f + P.lfo2ToRes * lfo2 * 0.4f;
        res = clampf (res, 0.f, 1.15f);
        const float k = res * 4.1f;

        float drive = clampf (P.drive + P.seqToDrive * seq * 0.5f, 0.f, 1.f);
        const float dGain = 1.f + drive * drive * 22.f;
        const float dBias = 0.15f * drive;
        const float dComp = 1.f / (1.f + drive * 2.2f);

        const float w1 = clampf (P.osc1Wave + P.lfo2ToWave * lfo2 * 1.5f, 0.f, 3.f);
        const float w2 = clampf (P.osc2Wave + P.lfo2ToWave * lfo2 * 1.5f, 0.f, 3.f);

        // ---------------- voice @ 2x ----------------
        for (int os = 0; os < 2; ++os)
        {
            const float o1 = osc1.tick (inc1, w1);
            if (P.osc2Sync && osc1.wrapped) osc2.phase = osc1.phase * (inc2 / std::max (inc1, 1e-9f));
            const float o2 = osc2.tick (inc2 * (1.f + P.fmAmount * o1 * 3.f), w2);
            const float so = subOsc.tick (subInc, 2.f);
            const float nz = rng.bi();

            float mix = (o1 * P.osc1Level + o2 * P.osc2Level + so * P.subLevel + nz * P.noiseLevel) * 0.5f;
            mix += lastFilterOut * P.feedback * 1.8f;

            float x = fastTanh (mix * dGain + dBias) - fastTanh (dBias);
            x *= (1.f + drive * 0.6f);

            if (! P.bitePost) x = bite (x);
            ladder.process (x, G, k);
            float y = ladder.output (P.slope, P.filterMode) * (1.f + k * 0.28f);
            if (P.bitePost) y = bite (y);
            lastFilterOut = y;
            dec.push (y * dComp * 1.6f);
        }
        float v = dec.out();
        const float velA = 1.f - P.velToAmp + P.velToAmp * velocity;
        v *= ae * velA;

        // ---------------- TALK (formant) ----------------
        if (P.talkMix > 0.001f)
        {
            if ((ctrlCounter & 7) == 0)
            {
                talkPos = clampf (P.talkVowel * 4.f + P.seqToTalk * seq * 2.5f, 0.f, 4.f);
                static const float F[5][3] = { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 280, 2250, 2900 },
                                               { 450, 800, 2830 },  { 325, 700, 2530 } };
                const int i0 = std::min (3, (int) talkPos);
                const float t = talkPos - (float) i0;
                for (int b = 0; b < 3; ++b)
                    formant[b].set (F[i0][b] + (F[i0 + 1][b] - F[i0][b]) * t, 9.f, sr);
            }
            const float f = formant[0].bp (v) * 1.0f + formant[1].bp (v) * 0.7f + formant[2].bp (v) * 0.35f;
            v = v + (f * 1.4f - v) * P.talkMix;
        }

        // ---------------- CRUSH ----------------
        if (P.crush > 0.001f)
        {
            const int hold = 1 + (int) (P.crush * P.crush * 24.f);
            if (++crushCount >= hold) { crushCount = 0; crushHold = v; }
            const float bits = 16.f - P.crush * 12.f;
            const float q = std::exp2 (bits - 1.f);
            const float c = std::round (crushHold * q) / q;
            v = v + (c - v) * std::min (1.f, P.crush * 2.f);
        }

        float L = v, R = v;

        // ---------------- PHASER ----------------
        if (P.phaserMix > 0.001f)
        {
            if ((ctrlCounter & 15) == 0)
            {
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float l = 0.5f + 0.5f * std::sin ((float) (kTwoPi * phaser.ph) + (ch ? kPi * 0.5f : 0.f));
                    const float f = 180.f * std::exp2 (l * P.phaserDepth * 6.f);
                    const float t = std::tan (kPi * clampf (f, 20.f, sr * 0.45f) / sr);
                    phaser.a[ch] = (t - 1.f) / (t + 1.f);
                }
            }
            phaser.ph += P.phaserRate / sr; if (phaser.ph >= 1.0) phaser.ph -= 1.0;
            float* io[2] = { &L, &R };
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = *io[ch] + phaser.fbS[ch] * P.phaserFb * 0.9f;
                const float a = phaser.a[ch];
                for (int s = 0; s < 6; ++s)
                {
                    const float y = a * x + phaser.z[ch][s];
                    phaser.z[ch][s] = x - a * y;
                    x = y;
                }
                phaser.fbS[ch] = x;
                *io[ch] = *io[ch] + (x - *io[ch]) * P.phaserMix * 0.5f + x * P.phaserMix * 0.5f;
            }
        }

        // ---------------- DELAY ----------------
        if (P.delayMix > 0.001f || delaySmoothed > 0.0001f)
        {
            const float target = (float) (kDivBeats[std::max (0, std::min (kNumDivs - 1, P.delayDiv))] * 60.0 / bpm) * sr;
            delaySmoothed += (target - delaySmoothed) * 0.0005f;
            delayLfo += 0.35 / sr; if (delayLfo >= 1.0) delayLfo -= 1.0;
            const float mod = 1.f + P.delayMod * 0.006f * std::sin ((float) (kTwoPi * delayLfo));
            const float dS  = clampf (delaySmoothed * mod, 4.f, (float) delayBufLen - 4.f);
            const float rl  = readDelay (delayL, dS), rr = readDelay (delayR, dS * (P.pingPong ? 1.f : 1.013f));
            const float toneC = 0.05f + P.delayTone * 0.9f;
            dlTone[0] += (rl - dlTone[0]) * toneC; dlTone[1] += (rr - dlTone[1]) * toneC;
            dlHp[0] += (dlTone[0] - dlHp[0]) * 0.01f; dlHp[1] += (dlTone[1] - dlHp[1]) * 0.01f;
            const float fbL = (dlTone[0] - dlHp[0]) * P.delayFb, fbR = (dlTone[1] - dlHp[1]) * P.delayFb;
            const float in = (L + R) * 0.5f;
            if (P.pingPong) { delayL[(size_t) delayW] = fastTanh (in + fbR); delayR[(size_t) delayW] = fastTanh (fbL); }
            else            { delayL[(size_t) delayW] = fastTanh (L + fbL);  delayR[(size_t) delayW] = fastTanh (R + fbR); }
            delayW = (delayW + 1) % delayBufLen;
            L += rl * P.delayMix; R += rr * P.delayMix;
            if (P.delayMix <= 0.001f) delaySmoothed *= 0.9999f;
        }

        // ---------------- REVERB ----------------
        if (P.reverbMix > 0.001f)
        {
            float x = (L + R) * 0.5f;
            for (int a = 0; a < 2; ++a)
            {
                auto& b = fdn.ap[a]; int& ix = fdn.apIdx[a];
                const float d = b[(size_t) ix];
                const float y = -0.6f * x + d;
                b[(size_t) ix] = x + 0.6f * y;
                ix = (ix + 1) % fdn.apLen[a];
                x = y;
            }
            const float size = 0.45f + P.reverbSize * 1.3f;
            const float fbG  = 0.72f + P.reverbSize * 0.255f;
            const float damp = 0.1f + (1.f - P.reverbDamp) * 0.85f;
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
                const float fb = (o[i] - sum) * fbG + x * 0.35f;
                fdn.line[i][(size_t) fdn.idx[i]] = fb;
                fdn.idx[i] = (fdn.idx[i] + 1) % (int) fdn.line[i].size();
                if (i & 1) wr += o[i]; else wl += o[i];
            }
            L += wl * 0.35f * P.reverbMix; R += wr * 0.35f * P.reverbMix;
        }

        // ---------------- OUT ----------------
        L *= outGain; R *= outGain;
        outL = softClip (L); outR = softClip (R);

        // GUI feeds
        if ((ctrlCounter & 63) == 0)
        {
            uiSeqValue.store (seq, std::memory_order_relaxed);
            uiCutoffHz.store (fc, std::memory_order_relaxed);
            uiEnv.store (ae, std::memory_order_relaxed);
            const int w = scopeWrite.load (std::memory_order_relaxed);
            scope[w] = seq;
            scopeWrite.store ((w + 1) % kScopeSize, std::memory_order_relaxed);
        }
        ++ctrlCounter;
    }

    float biteHold = 0.f; int biteCount = 0;
    float bite (float x)
    {
        if (P.biteAmount < 0.001f) return x;
        const float a = P.biteAmount;
        float y;
        switch (P.biteType)
        {
            case 0: y = fastTanh (x * (1.f + a * 12.f)) * (1.f - a * 0.3f); break;                       // soft
            case 1: { const float g = x * (1.f + a * 7.f);                                                 // wavefold
                      y = std::sin (g * 1.5707963f) * 0.8f; } break;
            case 2: { const float g = x * (1.f + a * 10.f);                                                // asymmetric tube-ish
                      y = g > 0.f ? fastTanh (g) : fastTanh (g * 0.35f) * 1.6f; y *= 0.8f; } break;
            case 3: { const float g = fastTanh (x * (1.f + a * 6.f));                                       // half/full rectify blend = octave-up grit
                      y = g + (std::fabs (g) * 1.6f - 0.5f - g) * a; } break;
            default: { const int hold = 1 + (int) (a * a * 40.f);                                           // sample-rate crush
                       if (++biteCount >= hold) { biteCount = 0; biteHold = x; }
                       const float q = std::exp2 (14.f - a * 11.f);
                       y = std::round (biteHold * q) / q; } break;
        }
        return x + (y - x) * P.biteMix;
    }

    float readDelay (const std::vector<float>& b, float d) const
    {
        float rp = (float) delayW - d;
        while (rp < 0.f) rp += (float) delayBufLen;
        const int i0 = (int) rp; const float t = rp - (float) i0;
        const int i1 = (i0 + 1) % delayBufLen;
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * t;
    }

    static float softClip (float x)
    {
        const float a = std::fabs (x);
        if (a < 0.85f) return x;
        const float s = x < 0.f ? -1.f : 1.f;
        return s * (0.85f + 0.15f * fastTanh ((a - 0.85f) / 0.15f));
    }
};

} // namespace sq
