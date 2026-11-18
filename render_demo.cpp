// Offline renderer: plays every factory preset through a psy groove and writes WAVs.
// Build: g++ -O2 -std=c++17 -I../Source/dsp render_demo.cpp -o render_demo
#include "SqueFMEngine.h"
#include "Presets.h"
#include <cstdio>
#include <string>
#include <vector>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) (sq::clampf (L[i], -1.f, 1.f) * 32767.f), b = (int16_t) (sq::clampf (R[i], -1.f, 1.f) * 32767.f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : ".";
    const int sr = 48000; const double bpm = 132.0; const int bars = 8;
    const int block = 128;
    const double samplesPerBeat = sr * 60.0 / bpm;
    const int total = (int) (samplesPerBeat * 4 * bars) + sr * 2;

    // psy "rolling" bass pattern: 16ths, root E1 (40), skipping the kick on each beat
    // pattern per beat: [rest, note, note, note]; lead-type presets get a held melodic line
    const auto presets = sq::factoryPresets();
    for (const auto& pr : presets)
    {
        sq::Engine e; e.prepare (sr); e.setParams (pr.p);
        std::vector<float> L ((size_t) total), R ((size_t) total);
        const bool held = pr.name.find ("Swamp") != std::string::npos || pr.name.find ("Ripper") != std::string::npos
                       || pr.name.find ("Croak") != std::string::npos || pr.name.find ("Spit") != std::string::npos
                       || pr.name.find ("Insects") != std::string::npos || pr.name.find ("Frog") != std::string::npos;
        const int notesHeld[] = { 40, 40, 43, 38 };
        int curNote = -1; long long lastSixteenth = -1;
        double peak = 0, rms = 0; bool bad = false;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            const double ppq = pos / samplesPerBeat;
            const bool playing = ppq < bars * 4;
            e.setTransport (bpm, ppq, playing);
            const long long six = (long long) (ppq * 4.0);
            if (playing && six != lastSixteenth)
            {
                lastSixteenth = six;
                if (held)
                {
                    if (six % 16 == 0)
                    {
                        const int nn = notesHeld[(six / 16) % 4] + (pr.name.find ("Insects") != std::string::npos ? 24 : 12);
                        if (curNote >= 0) e.noteOff (curNote);
                        e.noteOn (nn, 0.9f); curNote = nn;
                    }
                }
                else
                {
                    if (curNote >= 0) { e.noteOff (curNote); curNote = -1; }
                    const int sub = (int) (six % 4);
                    if (sub != 0)
                    {
                        const int base = pr.name.find ("Zaps") != std::string::npos ? 64 : (pr.name.find ("Acid") != std::string::npos ? 52 : 40);
                        const int nn = base + ((six % 32) == 31 ? 3 : 0);
                        e.noteOn (nn, sub == 2 ? 1.f : 0.75f); curNote = nn;
                    }
                }
            }
            if (! playing && curNote >= 0) { e.noteOff (curNote); curNote = -1; }
            e.render (&L[(size_t) pos], &R[(size_t) pos], n);
        }
        for (int i = 0; i < total; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) bad = true;
            peak = std::max (peak, (double) std::fabs (L[(size_t) i]));
            rms += L[(size_t) i] * L[(size_t) i];
        }
        rms = std::sqrt (rms / total);
        std::string fn = pr.name; for (auto& c : fn) if (c == ' ') c = '_';
        writeWav (outDir + "/" + fn + ".wav", L, R, sr);
        std::printf ("%-20s peak %.3f  rms %.3f  %s\n", pr.name.c_str(), peak, rms, bad ? "NaN!!" : "ok");
    }
    return 0;
}
