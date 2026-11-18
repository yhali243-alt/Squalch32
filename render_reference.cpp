// Renders the four reference patches through the real SqueFM engine:
// isolated note + musical sequence, dry (FX off) and processed, 48 kHz / 24-bit WAV.
// g++ -O2 -std=c++17 render_reference.cpp -o render_reference && ./render_reference outdir
#include "SqueFMEngine.h"
#include "Presets.h"
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <cmath>

struct Ev { double beat; int note; float vel; double len; };   // beats

static void writeWav24 (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 6;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 6); w16 (6); w16 (24);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
        for (float x : { L[i], R[i] })
        {
            const int32_t v = (int32_t) std::lround (sq::clampf (x, -1.f, 1.f) * 8388607.f);
            const uint8_t b[3] = { (uint8_t) (v & 0xff), (uint8_t) ((v >> 8) & 0xff), (uint8_t) ((v >> 16) & 0xff) };
            std::fwrite (b, 1, 3, f);
        }
    std::fclose (f);
}

struct Result { std::vector<float> L, R; double renderSec = 0; };

static Result render (sq::Params p, const std::vector<Ev>& evs, double bpm, double beats, int sr, bool dry)
{
    if (dry) { p.delayMix = 0; p.reverbMix = 0; p.phaserMix = 0; }
    sq::Engine e; e.prepare ((double) sr); e.setParams (p);
    const double spb = sr * 60.0 / bpm;
    const int total = (int) (spb * beats) + sr;
    Result r; r.L.assign ((size_t) total, 0.f); r.R.assign ((size_t) total, 0.f);
    struct T { int s; bool on; int note; float vel; };
    std::vector<T> ts;
    for (auto& ev : evs) { ts.push_back ({ (int) std::llround (ev.beat * spb), true, ev.note, ev.vel }); ts.push_back ({ (int) std::llround ((ev.beat + ev.len) * spb), false, ev.note, 0.f }); }
    std::sort (ts.begin(), ts.end(), [] (const T& a, const T& b) { return a.s != b.s ? a.s < b.s : (! a.on && b.on); });
    const int block = 128; size_t ti = 0; int pos = 0;
    const auto t0 = std::chrono::steady_clock::now();
    while (pos < total)
    {
        int n = std::min (block, total - pos);
        if (ti < ts.size() && ts[ti].s < pos + n) n = std::max (0, ts[ti].s - pos);
        if (n > 0)
        {
            e.setTransport (bpm, pos / spb, pos / spb < beats);
            e.render (&r.L[(size_t) pos], &r.R[(size_t) pos], n); pos += n;
        }
        while (ti < ts.size() && ts[ti].s <= pos) { if (ts[ti].on) e.noteOn (ts[ti].note, ts[ti].vel); else e.noteOff (ts[ti].note); ++ti; }
    }
    r.renderSec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    return r;
}

int main (int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    const int sr = 48000; const double bpm = 140.0;
    auto all = sq::factoryPresets();
    auto find = [&] (const std::string& n) { for (auto& p : all) if (p.name == n) return p.p; return sq::Params{}; };

    // psy bass roll: off-beat 16ths (skipping the kick), E1
    std::vector<Ev> roll; for (int s = 0; s < 64; ++s) if (s % 4) roll.push_back ({ s * 0.25, 40 + ((s % 32) == 31 ? 3 : 0), s % 4 == 2 ? 1.f : 0.8f, 0.2 });
    // creature hits: syncopated 16ths with gaps, different pitches
    std::vector<Ev> hits; { const int pat[16] = { 1,0,0,1, 0,0,1,0, 1,0,1,0, 0,1,0,0 }; const int nn[16] = { 52,0,0,59, 0,0,55,0, 64,0,52,0, 0,62,0,0 };
        for (int b = 0; b < 4; ++b) for (int s = 0; s < 16; ++s) if (pat[s]) hits.push_back ({ b * 4 + s * 0.25, nn[s], 0.9f, 0.18 }); }
    // lead line: 8ths with ties, phrygian on E
    std::vector<Ev> lead; { const int nn[16] = { 64,65,64,67, 64,70,69,67, 64,65,64,72, 71,69,67,65 };
        for (int b = 0; b < 2; ++b) for (int i = 0; i < 16; ++i) lead.push_back ({ b * 8 + i * 0.5, nn[i], 0.85f, 0.48 }); }

    struct Job { const char* preset; const char* file; std::vector<Ev> seq; int isoNote; };
    std::vector<Job> jobs = {
        { "A Fat WOW", "A_FatWOW", roll, 40 },
        { "B Aggressive Swamp", "B_AggressiveSwamp", roll, 40 },
        { "C HiTech FM Creature", "C_HiTechFMCreature", hits, 52 },
        { "D Psy FM Lead", "D_PsyFMLead", lead, 64 },
        { "Zenon Swamp Roller", "baseline_v3_ZenonSwampRoller", roll, 40 },   // old patch, same MIDI, for A/B
    };
    std::printf ("%-24s %-10s %7s %7s %8s %8s %6s\n", "patch", "take", "peak", "rmsdB", "dc", "clip%", "RTx");
    for (auto& j : jobs)
    {
        const auto p = find (j.preset);
        for (int dry = 1; dry >= 0; --dry)
        {
            struct Take { const char* tag; std::vector<Ev> ev; double beats; };
            const Take takes[2] = { { "note", { { 0.0, j.isoNote, 0.9f, 6.0 } }, 8.0 }, { "seq", j.seq, 16.0 } };
            for (auto& t : takes)
            {
                auto r = render (p, t.ev, bpm, t.beats, sr, dry != 0);
                double pk = 0, ss = 0, dc = 0; size_t clip = 0;
                for (size_t i = 0; i < r.L.size(); ++i) { const double x = r.L[i]; pk = std::max (pk, std::fabs (x)); ss += x * x; dc += x; if (std::fabs (x) > 0.99) ++clip; }
                const double len = (double) r.L.size() / sr;
                std::printf ("%-24s %-4s %-5s %7.3f %7.1f %8.5f %8.4f %6.0f\n", j.preset, t.tag, dry ? "dry" : "fx", pk, 10 * std::log10 (ss / r.L.size() + 1e-20),
                             dc / r.L.size(), 100.0 * clip / r.L.size(), len / r.renderSec);
                writeWav24 (out + "/" + j.file + "_" + t.tag + (dry ? "_dry" : "") + ".wav", r.L, r.R, sr);
            }
        }
    }
    return 0;
}
