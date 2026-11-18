---
name: squefm-sound-design
description: Sound-design rules and DSP/dev standards for Disco Valley SqueFM (JUCE VST3 psytrance synth). Use when changing the engine, presets, FM voice, filter, sequencer or GUI of this repo, or when validating audio.
---

# SqueFM sound design + development

One engine: `SqueFMEngine.h` (pure C++17). The plugin (`PluginProcessor.*`), the demo
renderers (`render_demo.cpp`, `render_reference.cpp`) and the analysis harness
(`analyze.cpp`) all include it. Never fork a second engine.

## Signal path (know it before you touch it)
```
OSC A / OSC B (polyblep morph) + legacy FM op + noise + SUB (if not "clean")
+ 4-OP FM VOICE (route "Into Filter")
  -> pre-drive (tanh) -> Bite(pre) -> ZDF ladder 6..24 dB LP/BP/HP (bass-keep comp) -> Bite(post)
  -> GROWL post-saturation (parallel, asym, fold)            [all at 2x/4x oversampling]
  -> decimate | parallel path: FM voice "Parallel Clean" + SUB clean (decimated separately)
  -> comb -> + parallel path -> VCA -> talk -> crush -> DC block -> pan (+ FM width side)
  -> phaser -> delay -> reverb -> soft clip
```
Cutoff in octaves: `base + seq*depth + env + key + matrix + FILTER LFO*amount + offset + accent`.

## Sound-design rules
- **Fat = stable fundamental + controlled mids.** Put the sub on a sine, route it
  "Sub Clean (post filter)" so filter sweeps and resonance never thin the low end. Keep the sub mono
  (it bypasses pan/width). Width lives only in the FM side signal and the effects.
- **WOW needs audible resonance.** Hot input kills ladder resonance (the input tanh compresses the
  feedback). For WOW: osc levels ~0.3-0.45, pre-drive ~0, resonance 0.85-0.92, 24 dB slope,
  Bass Keep 0.3-0.5, filter LFO unipolar 3-4 octaves from a low base (120-200 Hz), smooth 0.1-0.2.
  Add growl *after* the filter (Growl + Parallel) instead of pre-drive.
- **Envelope and LFO are separate jobs.** Filter env = the "B" of BWOW (attack contour), filter LFO
  = the "OW" movement. Never fake one with the other.
- **FM must have a pitch.** Integer carrier/modulator ratios keep a fundamental; inharmonic ratios
  (Character knob, non-integer ratios) give metal/bells. Raise index with *envelopes on the modulators*
  (short modulator decay = bright attack, darker sustain) — a static index sounds dead.
- **FM mud:** non-integer ratios create sidebands below the fundamental. Use FM Low Cut 0.5-0.8 on
  hi-tech/lead patches.
- **Hi-tech hits:** Parallel Clean route, modulator decays 30-150 ms, pitch zap 5-12 st over 20-40 ms,
  index jumped per 16th (Filter LFO > FM with S&H, or the FM TIMBRE lane). Avoid char>0.5 + feedback>0.4
  together unless you want noise (Digital Noise family).
- **Leads:** Y or stack algorithm, OP4 feedback 0.3-0.5, brightness ~0.8-1, FM TIMBRE lane depth 0.5-0.8,
  CHARACTER lane for occasional metallic steps, accents every 4th 16th, ping-pong 1/8D.
- **Gain staging:** reference patches sit around -12..-16 dB RMS (dry) with peaks < 0.9.
  Use the preset `outGainDb`, never more saturation, to match levels.

## FM voice reference
8 algorithms in `Engine::kAlgoMods/kAlgoCarrier` (OP1 always a carrier, OP4 has self feedback).
High level controls: Brightness (global index), Character (pushes modulators off integer ratios),
Body (pure sine at the fundamental), Decay (scales all op decays), Feedback, Width (detuned OP1
copy -> side), Vel > Timbre, Key Scale, Low Cut. Families in `Presets.h::applyFmFamily`.

## Sequencer
32 steps; every step: cutoff OPEN/CLOSED range, own LFO shape + BPM/Hz rate, plus lanes
FM timbre / character / resonance / drive (0.5 = neutral, lane depth scales), probability, accent.
Probability only consumes RNG when < 1 (keeps old projects bit-identical).

## Poly sequencer (v5, `PolySeq.h` + engine `tickPolySeq`)
- Off by default (`psOn`). When on, MIDI note-on = start + root reference (if `psFollow`), last note-off = stop.
  The voice is played by `triggerVoice()`; the MIDI held-note list is never touched by the sequencer.
- Three lanes (trigger / pitch / mod), each: steps 1-32, rate (`kRateBeats`, incl. dotted + triplets),
  direction (fwd / rev / ping-pong), phase in steps (may be negative). Position is computed from beats since
  start (`laneCounter`), never accumulated per lane -> no drift; host PPQ drives it while the transport runs,
  a free beat clock when stopped; transport start re-aligns.
- A trigger step uses the pitch lane's CURRENT degree. Scales: `kScales` (order permanent), `degreeToMidi`
  handles negative degrees / octave wrap. Own RNG (`psRng`, reseeded at start) for probability.
- Mod lane -> one destination (FM bright/char/fb, res, cutoff, drive), smoothed, via `psMod()`.
- Tests: `poly_test.cpp` (scales x12 roots, 5v7 exact sample timing, block sizes, 10-min drift, triplet/dotted,
  reverse/ping-pong, negative phase, tempo change, transport, note-off release, probability determinism).
  Demos: `render_poly.cpp`.

## Compatibility rules (hard)
- Parameter IDs, ranges, choice lists and order are permanent. Append new params at the end of
  `paramTable()`; new choices only at the end of a list (prefer new params over changing lists).
- New features must default to "off": legacy presets must render **bit-identically**
  (check with an old/new raw dump diff, see below).
- `setStateInformation` fills params missing from old projects with defaults.
- Plugin codes `Dval`/`Sqfm` and PRODUCT_NAME never change.

## Validation (do all before claiming anything)
```
g++ -O2 -std=c++17 analyze.cpp -o analyze && ./analyze          # peak/RMS/DC/click/block-size/determinism
g++ -O2 -std=c++17 render_reference.cpp -o rr && ./rr out        # A-D: note + sequence, dry + fx, RT factor
python3 spectro.py out/A_FatWOW_note_dry.wav a.png 3             # look at it: sub stable? resonant ridge moving?
```
- Block-size independence must be 0.0 (render 32 vs 128 samples).
- Fuzz all params at extremes (44.1/96 kHz, BPM 100/175): no NaN, peak <= soft-clip ceiling.
- Look at spectrograms (log frequency): WOW = arching resonant ridge over a flat sub; FM = sidebands
  that change during the note; no broadband rumble under 60 Hz on FM patches.
- Numbers never prove it sounds good: always render WAVs for the human to judge.

## Build
- Linux check: `CXXFLAGS="-DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0" cmake -B b -DJUCE_PATH=<juce> -DSQ_BUILD_TESTS=ON && cmake --build b --target SqueFM_VST3 SqueTest`
  then `xvfb-run -a b/SqueTest_artefacts/Release/SqueTest guidir` (presets finite, state roundtrip,
  old-state default fill, GUI PNGs of all tabs). Also compile with clang `-Wall -Wextra`.
- Mac: `build_mac.sh` (one-line curl install) builds target `SqueFM_VST3`, keeps the previous
  `SqueFM.vst3` as `~/Squalch32-build/SqueFM.previous.vst3`, ad-hoc codesigns. CI: `.github/workflows/build.yml`.
