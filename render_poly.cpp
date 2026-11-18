// Renders the poly-sequencer demos through the real SqueFM engine (one held MIDI note, 136 BPM).
// g++ -O2 -std=c++17 render_poly.cpp -o render_poly && ./render_poly outdir
#include "SqueFMEngine.h"
#include "Presets.h"
#include <cstdio>
#include <string>
#include <vector>
#include <chrono>
static void writeWav24 (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb"); if (! f) return;
    const uint32_t n = (uint32_t) L.size(), bytes = n * 6;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f); w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 6); w16 (6); w16 (24);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (uint32_t i = 0; i < n; ++i) for (float x : { L[i], R[i] }) { const int32_t v = (int32_t) std::lround (sq::clampf (x, -1.f, 1.f) * 8388607.f); const uint8_t b[3] = { (uint8_t) v, (uint8_t) (v >> 8), (uint8_t) (v >> 16) }; std::fwrite (b, 1, 3, f); }
    std::fclose (f);
}
int main (int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    const int sr = 48000; const double bpm = 136, bars = 8;
    struct Job { const char* preset; const char* file; int note; };
    const Job jobs[] = { { "Poly 5v7 Hypnotic", "Poly1_5v7_Hypnotic", 40 }, { "Poly 5-7-3 Evolving", "Poly2_5-7-3_Evolving", 52 }, { "Poly F Hungarian 136", "Poly3_F_HungarianMinor_136", 53 } };
    auto all = sq::factoryPresets();
    for (auto& j : jobs)
    {
        sq::Params p; for (auto& x : all) if (x.name == j.preset) p = x.p;
        sq::Engine e; e.prepare ((double) sr); e.setParams (p);
        const int hold = (int) (bars * 4 * 60.0 / bpm * sr), total = hold + sr * 2;
        std::vector<float> L ((size_t) total), R ((size_t) total);
        double ppq = 0; bool on = false;
        const auto t0 = std::chrono::steady_clock::now();
        for (int pos = 0; pos < total; pos += 256)
        {
            const int n = std::min (256, total - pos);
            e.setTransport (bpm, ppq, true);
            if (! on && pos == 0) { e.noteOn (j.note, 0.9f); on = true; }
            if (on && pos + n > hold) { const int a = hold - pos; e.render (&L[(size_t) pos], &R[(size_t) pos], a); e.noteOff (j.note); on = false; e.render (&L[(size_t) (pos + a)], &R[(size_t) (pos + a)], n - a); }
            else e.render (&L[(size_t) pos], &R[(size_t) pos], n);
            ppq += n * bpm / 60.0 / sr;
        }
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        double pk = 0, ss = 0; for (float x : L) { pk = std::max (pk, (double) std::fabs (x)); ss += x * x; }
        std::printf ("%-28s events %4lld  peak %.3f  rms %.1f dB  realtime x%.0f\n", j.preset, e.psEvents, pk, 10 * std::log10 (ss / L.size()), total / (double) sr / sec);
        writeWav24 (out + "/" + j.file + ".wav", L, R, sr);
    }
}
