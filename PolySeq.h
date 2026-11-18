// SqueFM V5 - polyrhythmic sequencer core: scale engine + independent lane clock math.
// Pure C++17, no allocation, safe to call per sample.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sq
{
// ----------------------------------------------------------------- scales ----
// Semitone intervals from the root. These are dark / exotic scale choices for psychedelic
// production (not "official" genre scales). Order is permanent (parameter choice list).
struct ScaleDef { const char* name; int count; int iv[12]; };
inline constexpr ScaleDef kScales[] = {
    { "Hungarian Minor",       7, { 0, 2, 3, 6, 7, 8, 11 } },
    { "Double Harmonic Major", 7, { 0, 1, 4, 5, 7, 8, 11 } },
    { "Harmonic Minor",        7, { 0, 2, 3, 5, 7, 8, 11 } },
    { "Phrygian Dominant",     7, { 0, 1, 4, 5, 7, 8, 10 } },
    { "Phrygian",              7, { 0, 1, 3, 5, 7, 8, 10 } },
    { "Locrian Natural 2",     7, { 0, 2, 3, 5, 6, 8, 10 } },
    { "Dorian b2",             7, { 0, 1, 3, 5, 7, 9, 10 } },
    { "Whole Tone",            6, { 0, 2, 4, 6, 8, 10 } },
    { "Diminished Half-Whole", 8, { 0, 1, 3, 4, 6, 7, 9, 10 } },
    { "Chromatic",            12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
};
constexpr int kNumScales = 10;
inline constexpr const char* kScaleNames[] = { "Hungarian Minor", "Double Harmonic Major", "Harmonic Minor", "Phrygian Dominant", "Phrygian",
                                               "Locrian Natural 2", "Dorian b2", "Whole Tone", "Diminished Half-Whole", "Chromatic" };
inline constexpr const char* kNoteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
inline constexpr const char* kLaneDirNames[] = { "Forward", "Reverse", "Ping-Pong" };
inline constexpr const char* kPsModDestNames[] = { "FM Brightness", "FM Character", "FM Feedback", "Resonance", "Cutoff", "Drive" };
constexpr int kNumPsModDests = 6;

inline int floorDiv (long long a, long long b) { long long q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return (int) q; }
inline int posMod (long long a, int n) { long long m = a % n; return (int) (m < 0 ? m + n : m); }

// Scale degree (0 = root, may be negative or > count) -> MIDI note, exact for all 12 roots.
inline int degreeToMidi (int rootMidi, int scale, int degree)
{
    const ScaleDef& s = kScales[std::max (0, std::min (kNumScales - 1, scale))];
    const int oct = floorDiv (degree, s.count);
    const int idx = posMod (degree, s.count);
    return std::max (0, std::min (127, rootMidi + 12 * oct + s.iv[idx]));
}

// Step index for lane step counter k (k = number of whole steps elapsed, may be negative).
// dir: 0 forward, 1 reverse, 2 ping-pong (ends not repeated).
inline int laneStepIndex (long long k, int nSteps, int dir)
{
    const int n = std::max (1, std::min (32, nSteps));
    if (n == 1) return 0;
    if (dir == 1) return n - 1 - posMod (k, n);
    if (dir == 2) { const int period = 2 * n - 2; const int m = posMod (k, period); return m < n ? m : period - m; }
    return posMod (k, n);
}

// Lane position: elapsed beats since sequence start -> whole-step counter (phase offset in steps).
inline long long laneCounter (double beats, double stepBeats, double phaseSteps)
{
    return (long long) std::floor (beats / stepBeats + phaseSteps + 1e-9);
}
} // namespace sq
