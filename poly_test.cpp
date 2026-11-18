// Stage-1 tests for the polyrhythmic sequencer, run on the real SqueFM engine.
// g++ -O2 -std=c++17 poly_test.cpp -o poly_test && ./poly_test
#include "SqueFMEngine.h"
#include "Presets.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <string>
using namespace sq;
static int fails = 0;
#define CHECK(c, ...) do { if (! (c)) { ++fails; std::printf ("FAIL: " __VA_ARGS__); std::printf ("\n"); } } while (0)

struct Ev { long long sample; int note; };
static Params base()
{
    auto ps = factoryPresets(); Params p; for (auto& x : ps) if (x.name == "C HiTech FM Creature") p = x.p;
    p.psOn = true; p.delayMix = 0; p.reverbMix = 0; return p;
}
// renders with a held MIDI note; block = size of render calls; returns internal note events
static std::vector<Ev> run (Params p, double bpm, double seconds, int block, int sr = 48000, bool playing = false,
                            double bpm2 = 0, double changeAt = -1, std::vector<float>* audio = nullptr, int heldNote = 53)
{
    Engine e; e.prepare ((double) sr); e.setParams (p);
    const long long total = (long long) (seconds * sr);
    std::vector<float> L ((size_t) block), R ((size_t) block);
    std::vector<Ev> ev; long long last = 0; double ppq = 0;
    e.setTransport (bpm, 0, playing); e.noteOn (heldNote, 0.9f);
    for (long long pos = 0; pos < total; pos += block)
    {
        const int n = (int) std::min<long long> (block, total - pos);
        double b = (changeAt >= 0 && pos >= changeAt * sr) ? bpm2 : bpm;
        e.setTransport (b, ppq, playing);
        for (int i = 0; i < n; ++i)
        {
            e.render (&L[(size_t) i], &R[(size_t) i], 1);
            if (e.psEvents != last) { last = e.psEvents; ev.push_back ({ pos + i, e.psCurrentNote() }); }
            if (audio) audio->push_back (L[(size_t) i]);
        }
        ppq += n * b / 60.0 / sr;
    }
    return ev;
}

int main()
{
    // ---- 1. scale engine: all 12 roots x 10 scales, negative degrees, octave wrap ----
    {
        const int fHung[] = { 65, 67, 68, 71, 72, 73, 76, 77 };
        for (int d = 0; d < 8; ++d) CHECK (degreeToMidi (65, 0, d) == fHung[d], "F Hungarian degree %d -> %d", d, degreeToMidi (65, 0, d));
        CHECK (degreeToMidi (65, 0, -1) == 64, "F Hungarian degree -1 -> %d (want E 64)", degreeToMidi (65, 0, -1));
        CHECK (degreeToMidi (65, 0, -7) == 53, "degree -7 -> %d (want 53)", degreeToMidi (65, 0, -7));
        int checks = 0;
        for (int sc = 0; sc < kNumScales; ++sc)
            for (int root = 48; root < 60; ++root)
                for (int d = -20; d <= 20; ++d)
                {
                    const int n = kScales[sc].count, m = degreeToMidi (root, sc, d);
                    const int oct = (int) std::floor ((double) d / n), idx = ((d % n) + n) % n;
                    CHECK (m == root + 12 * oct + kScales[sc].iv[idx], "scale %d root %d deg %d", sc, root, d);
                    CHECK (((m - root) % 12 + 12) % 12 == kScales[sc].iv[idx], "pitch class");
                    if (d > -20) CHECK (m > degreeToMidi (root, sc, d - 1), "monotonic scale %d deg %d", sc, d);
                    ++checks;
                }
        std::printf ("scales: %d degree mappings checked (12 roots x 10 scales x 41 degrees)\n", checks);
    }
    // ---- 2. 5 vs 7 timing: trigger 5 steps 1/16, pitch 7 steps 1/8, all exact sample positions ----
    {
        Params p = base();
        p.psLane[0] = { 5, 4, 0, 0.f }; p.psLane[1] = { 7, 7, 0, 0.f }; p.psLane[2] = { 3, 6, 0, 0.f };
        p.psScale = 0; p.psFollow = true;
        for (int i = 0; i < kMaxSteps; ++i) p.psDeg[(size_t) i] = i % 7;
        const double bpm = 136, sr = 48000, spb = sr * 60 / bpm;
        auto ev = run (p, bpm, 20.0, 128);
        int bad = 0;
        for (size_t j = 0; j < ev.size(); ++j)
        {
            const long long want = (long long) std::ceil (j * 0.25 * spb - 1e-6);
            if (std::llabs (ev[j].sample - want) > 1) ++bad;
            const int pitchStep = (int) (((long long) std::floor (j * 0.25 / 0.5 + 1e-9)) % 7);
            if (ev[j].note != degreeToMidi (53, 0, pitchStep)) ++bad;
        }
        CHECK (bad == 0 && ev.size() > 100, "5v7 timing/pitch errors %d of %zu", bad, ev.size());
        std::printf ("5v7: %zu events, max timing error <= 1 sample, pitch follows 7-step lane\n", ev.size());
        // block-size independence
        auto ev2 = run (p, bpm, 20.0, 1), ev3 = run (p, bpm, 20.0, 1024);
        bool same = ev.size() == ev2.size() && ev.size() == ev3.size();
        for (size_t j = 0; same && j < ev.size(); ++j) same = ev[j].sample == ev2[j].sample && ev[j].sample == ev3[j].sample && ev[j].note == ev3[j].note;
        CHECK (same, "block-size dependence");
        std::printf ("block sizes 1 / 128 / 1024: identical events: %s\n", same ? "yes" : "NO");
        // drift: 10 minutes, last event still exact
        auto evl = run (p, bpm, 600.0, 512);
        const long long want = (long long) std::ceil ((evl.size() - 1) * 0.25 * spb - 1e-6);
        CHECK (std::llabs (evl.back().sample - want) <= 1, "drift after 10 min: %lld samples", evl.back().sample - want);
        std::printf ("10 minute run: %zu events, last event error %lld samples (no drift)\n", evl.size(), evl.back().sample - want);
    }
    // ---- 3. triplet / dotted / reverse / ping-pong / negative phase ----
    {
        Params p = base();
        for (int i = 0; i < kMaxSteps; ++i) p.psDeg[(size_t) i] = i;
        const double bpm = 120, spb = 48000 * 60 / bpm;
        p.psLane[0] = { 4, 6, 0, 0.f }; p.psLane[1] = { 4, 6, 1, 0.f };   // 1/8T trigger, reverse pitch at same rate
        auto ev = run (p, bpm, 4, 64);
        bool ok = ev.size() >= 10;
        for (size_t j = 0; ok && j < 10; ++j) ok = std::llabs (ev[j].sample - (long long) std::ceil (j * (1.0 / 3.0) * spb - 1e-6)) <= 1 && ev[j].note == degreeToMidi (53, 0, 3 - (int) (j % 4));
        CHECK (ok, "triplet + reverse");
        p.psLane[0] = { 4, 5, 0, 0.f }; p.psLane[1] = { 5, 5, 2, 0.f };   // dotted 1/16, ping-pong pitch 0 1 2 3 4 3 2 1 0 ...
        ev = run (p, bpm, 4, 64);
        const int pp[] = { 0, 1, 2, 3, 4, 3, 2, 1, 0, 1 };
        ok = ev.size() >= 10;
        for (size_t j = 0; ok && j < 10; ++j) ok = std::llabs (ev[j].sample - (long long) std::ceil (j * 0.375 * spb - 1e-6)) <= 1 && ev[j].note == degreeToMidi (53, 0, pp[j]);
        CHECK (ok, "dotted + ping-pong");
        p.psLane[0] = { 4, 4, 0, 0.f }; p.psLane[1] = { 7, 4, 0, -2.f };  // negative phase: pitch lane starts two steps "before" -> step 5
        ev = run (p, bpm, 2, 64);
        CHECK (! ev.empty() && ev[0].note == degreeToMidi (53, 0, 5) && ev[1].note == degreeToMidi (53, 0, 6) && ev[2].note == degreeToMidi (53, 0, 0), "negative phase");
        std::printf ("triplet, dotted, reverse, ping-pong, negative phase: checked\n");
    }
    // ---- 4. tempo change, transport, note off, determinism, probability ----
    {
        Params p = base();
        p.psLane[0] = { 3, 4, 0, 0.f };
        auto ev = run (p, 120, 4, 128, 48000, false, 150, 2.0);
        // after the change at 2 s, step spacing must be 1/16 at 150 bpm = 4800 samples
        bool ok = true; for (size_t j = 1; j < ev.size(); ++j) if (ev[j - 1].sample >= 2 * 48000 + 6000) ok = ok && std::llabs ((ev[j].sample - ev[j - 1].sample) - 4800) <= 1;
        CHECK (ok, "tempo change spacing");
        auto evp = run (p, 140, 4, 256, 48000, true);
        CHECK (evp.size() >= 37 && evp[0].sample == 0, "host-playing run size %zu first %lld", evp.size(), evp.empty() ? -1LL : evp[0].sample);
        // stuck notes: note off -> amp envelope must die
        Engine e; e.prepare (48000.0); e.setParams (p); e.setTransport (140, 0, false); e.noteOn (50, 1.f);
        std::vector<float> L (48000), R (48000); e.render (L.data(), R.data(), 48000); e.noteOff (50); e.render (L.data(), R.data(), 48000);
        double tail = 0; for (int i = 47000; i < 48000; ++i) tail = std::max (tail, (double) std::fabs (L[(size_t) i]));
        CHECK (! e.psRunning() && tail < 1e-4, "stuck note after note off: tail %.6f", tail);
        // probability: deterministic between runs, some steps skipped
        for (int i = 0; i < kMaxSteps; ++i) p.psProb[(size_t) i] = 0.5f;
        auto a = run (p, 140, 6, 128), b = run (p, 140, 6, 512);
        bool det = a.size() == b.size(); for (size_t j = 0; det && j < a.size(); ++j) det = a[j].sample == b[j].sample;
        const double expectEv = 6.0 * 140 / 60 * 4;
        CHECK (det && a.size() > expectEv * 0.3 && a.size() < expectEv * 0.7, "probability det %d count %zu", det, a.size());
        std::printf ("tempo change, host transport, note-off release, probability determinism: checked (%zu of ~%.0f steps fired at p=0.5)\n", a.size(), expectEv);
        // poly off: plain MIDI behaviour unchanged (no internal events)
        p.psOn = false; auto off = run (p, 140, 2, 128);
        CHECK (off.empty(), "poly off must not generate events");
    }
    std::printf (fails ? "\n%d FAILURES\n" : "\nALL POLY TESTS PASSED\n", fails);
    return fails ? 1 : 0;
}
