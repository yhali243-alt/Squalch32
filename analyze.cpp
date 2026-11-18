// Offline audio-quality harness for SqueFM presets.
// g++ -O2 -std=c++17 analyze.cpp -o analyze && ./analyze
#include "SqueFMEngine.h"
#include "Presets.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <complex>
#include <algorithm>

struct Buf { std::vector<float> L, R; int sr; };

static Buf renderPreset (const sq::Preset& pr, int sr, int block, double bpm = 140.0, int bars = 4)
{
    sq::Engine e; e.prepare ((double) sr); e.setParams (pr.p);
    const double spb = sr * 60.0 / bpm;
    const int total = (int) (spb * 4 * bars) + sr;
    Buf b; b.sr = sr; b.L.assign ((size_t) total, 0.f); b.R.assign ((size_t) total, 0.f);
    int cur = -1;
    const int nSix = bars * 16;
    int pos = 0;
    for (int six = 0; six <= nSix; ++six)
    {
        const int evt = six < nSix ? (int) std::llround (six * spb / 4.0) : total;
        while (pos < evt)
        {
            const int n = std::min (block - (pos % block), evt - pos);
            e.setTransport (bpm, pos / spb, six <= nSix && pos / spb < bars * 4);
            e.render (&b.L[(size_t) pos], &b.R[(size_t) pos], n);
            pos += n;
        }
        if (cur >= 0) { e.noteOff (cur); cur = -1; }
        if (six < nSix && six % 4 != 0) { cur = 40 + ((six % 32) == 31 ? 3 : 0); e.noteOn (cur, six % 4 == 2 ? 1.f : 0.75f); }
    }
    return b;
}

static void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[i], a[j]); }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2 * M_PI / (double) len; std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len) { std::complex<double> w (1); for (size_t k = 0; k < len / 2; ++k) { auto u = a[i + k], v = a[i + k + len / 2] * w; a[i + k] = u + v; a[i + k + len / 2] = u - v; w *= wl; } }
    }
}

int main()
{
    const auto presets = sq::factoryPresets();
    std::printf ("%-22s %6s %6s %7s %7s %8s %7s %7s %6s %5s\n", "preset", "peak", "rmsdB", "dc", "click", "blkdiff", "centSD", "hf16k%", "det", "nan");
    for (const auto& pr : presets)
    {
        auto a = renderPreset (pr, 48000, 128);
        auto b = renderPreset (pr, 48000, 32);
        auto c = renderPreset (pr, 48000, 128);
        double peak = 0, ss = 0, dc = 0, click = 0, bd = 0; bool nan = false;
        const size_t N = a.L.size();
        for (size_t i = 0; i < N; ++i)
        {
            const float x = a.L[i];
            if (! std::isfinite (x)) nan = true;
            peak = std::max (peak, (double) std::fabs (x)); ss += x * x; dc += x;
            if (i >= 2) click = std::max (click, (double) std::fabs (x - 2 * a.L[i - 1] + a.L[i - 2]));
            if (i < (size_t)(48000*60.0/140.0*16)) bd = std::max (bd, (double) std::fabs (x - b.L[i]));
        }
        const double rms = std::sqrt (ss / N); dc /= N;
        bool det = a.L == c.L;
        // spectral centroid movement + HF energy
        const size_t W = 4096; std::vector<double> cents; double hf = 0, tot = 0;
        for (size_t s = 0; s + W < N; s += W)
        {
            std::vector<std::complex<double>> f (W);
            for (size_t k = 0; k < W; ++k) f[k] = a.L[s + k] * (0.5 - 0.5 * std::cos (2 * M_PI * k / W));
            fft (f); double num = 0, den = 0;
            for (size_t k = 1; k < W / 2; ++k) { const double m = std::norm (f[k]), hz = k * 48000.0 / W; num += hz * m; den += m; tot += m; if (hz > 16000) hf += m; }
            if (den > 1e-9) cents.push_back (num / den);
        }
        double mean = 0, sd = 0; for (double v : cents) mean += v; mean /= std::max<size_t> (1, cents.size());
        for (double v : cents) sd += (v - mean) * (v - mean); sd = std::sqrt (sd / std::max<size_t> (1, cents.size()));
        std::printf ("%-22s %6.3f %6.1f %7.4f %7.3f %8.5f %7.0f %7.3f %6s %5s\n", pr.name.c_str(), peak, 20 * std::log10 (rms + 1e-12), dc,
                     click / (rms + 1e-9), bd, sd, 100 * hf / (tot + 1e-12), det ? "yes" : "NO", nan ? "NaN" : "-");
    }
}
