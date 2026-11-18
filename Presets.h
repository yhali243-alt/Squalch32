// Factory presets — tuned for 125-138 BPM Zenon-style progressive / dark psy.
#pragma once
#include "SquelchEngine.h"
#include <string>
#include <vector>

namespace sq
{
struct Preset { std::string name; Params p; };

namespace detail
{
    inline void steps (Params& p, std::initializer_list<StepParams> list)
    {
        int i = 0;
        for (auto& s : list) if (i < kMaxSteps) p.steps[(size_t) i++] = s;
        // repeat the pattern to fill all 32 slots so longer step counts stay musical
        const int n = (int) list.size();
        for (; i < kMaxSteps && n > 0; ++i) p.steps[(size_t) i] = p.steps[(size_t) (i % n)];
    }
    // rate index helpers  (0 OFF,1 1/8,2 1/4,3 1/2,4 1,5 1.5,6 2,7 3,8 4,9 6,10 8,11 12,12 16)
    // shape: 0 sine 1 tri 2 rampUp 3 rampDn 4 square 5 S&H 6 smoothRnd 7 squelch 8 chaos
}

inline std::vector<Preset> factoryPresets()
{
    using namespace detail;
    std::vector<Preset> v;

    { Params p; v.push_back ({ "Init", p }); }

    { // 1. Bog Walker — the core swampy rolling squelch
        Params p;
        p.osc1Wave = 1.f; p.osc2Wave = 1.6f; p.osc2Level = 0.35f; p.osc2Fine = 9.f; p.subLevel = 0.35f;
        p.cutoff = 260.f; p.resonance = 0.78f; p.drive = 0.45f; p.slope = 3.f; p.keyTrack = 0.4f;
        p.envAmount = 0.25f; p.fD = 0.18f; p.aS = 0.9f; p.glide = 0.06f; p.feedback = 0.15f;
        p.numSteps = 16; p.stepDiv = 3; p.seqAmount = 3.2f; p.seqSmooth = 0.12f; p.seqToRes = 0.25f;
        steps (p, { {7,7,0.9f,-0.2f},{4,0,0.4f,0.1f},{0,0,0,0.5f},{6,7,0.8f,-0.1f},
                    {3,1,0.6f,0.0f},{8,7,0.7f,-0.3f},{0,0,0,0.2f},{2,0,0.7f,0.3f},
                    {7,7,1.0f,-0.25f},{6,5,0.5f,0.0f},{4,3,0.6f,0.2f},{0,0,0,-0.4f},
                    {9,7,0.8f,-0.1f},{3,6,0.9f,0.1f},{5,1,0.5f,0.4f},{12,7,0.6f,-0.2f} });
        p.phaserMix = 0.25f; p.phaserRate = 0.07f; p.delayMix = 0.18f; p.delayDiv = 7; p.reverbMix = 0.12f;
        v.push_back ({ "Bog Walker", p });
    }
    { // 2. Swamp Talk — formant vowels following the sequence
        Params p;
        p.osc1Wave = 2.2f; p.osc2Wave = 1.f; p.osc2Level = 0.5f; p.osc2Sync = true; p.osc2Semi = 7.f;
        p.cutoff = 900.f; p.resonance = 0.55f; p.drive = 0.35f; p.filterMode = 0; p.slope = 2.f;
        p.envAmount = 0.15f; p.talkMix = 0.75f; p.talkVowel = 0.3f; p.seqToTalk = 0.9f;
        p.numSteps = 8; p.stepDiv = 6; p.seqAmount = 1.6f; p.seqSmooth = 0.3f; p.seqToPitch = 0.0f;
        p.lfo2Rate = 0.11f; p.lfo2ToWave = 0.3f;
        steps (p, { {4,6,0.9f,0.f},{2,0,0.8f,0.3f},{6,8,0.7f,-0.2f},{3,1,0.9f,0.1f},
                    {8,6,0.6f,-0.3f},{1,0,1.0f,0.f},{4,8,0.8f,0.2f},{5,5,0.5f,-0.1f} });
        p.delayMix = 0.22f; p.delayDiv = 5; p.reverbMix = 0.18f; p.reverbSize = 0.8f;
        p.outGainDb = -5.f;
        v.push_back ({ "Swamp Talk", p });
    }
    { // 3. Squalch Rider — screaming acid pluck pattern
        Params p;
        p.osc1Wave = 1.f; p.subLevel = 0.f; p.cutoff = 380.f; p.resonance = 0.95f; p.drive = 0.6f;
        p.envAmount = 0.55f; p.fD = 0.12f; p.resEnvAmount = 0.15f; p.velToFilter = 0.6f;
        p.numSteps = 32; p.stepDiv = 3; p.seqAmount = 2.6f; p.seqSmooth = 0.05f; p.seqToRes = 0.2f;
        p.biteType = 2; p.biteAmount = 0.35f; p.bitePost = true;
        steps (p, { {4,7,0.9f,0.f},{4,7,0.6f,-0.2f},{6,7,0.8f,0.1f},{4,7,0.5f,-0.3f},
                    {8,7,0.9f,0.2f},{4,7,0.7f,-0.1f},{0,0,0,-0.4f},{6,7,1.0f,0.f},
                    {4,7,0.8f,0.3f},{7,7,0.6f,-0.2f},{4,7,0.9f,0.f},{12,7,0.4f,-0.3f},
                    {4,7,0.7f,0.1f},{6,7,0.9f,-0.1f},{2,2,0.8f,0.f},{8,7,1.0f,0.2f} });
        p.delayMix = 0.2f; p.delayDiv = 7; p.delayFb = 0.5f; p.reverbMix = 0.1f;
        v.push_back ({ "Squalch Rider", p });
    }
    { // 4. Mycelium Drift — slow evolving organic pad-lead
        Params p;
        p.osc1Wave = 0.4f; p.osc2Wave = 1.2f; p.osc2Level = 0.6f; p.osc2Fine = -14.f; p.osc2Oct = 1; p.subLevel = 0.2f;
        p.cutoff = 700.f; p.resonance = 0.65f; p.drive = 0.2f; p.slope = 2.4f; p.envAmount = 0.1f;
        p.aA = 0.4f; p.aR = 1.2f; p.aS = 1.f;
        p.numSteps = 32; p.stepDiv = 9; p.seqAmount = 2.2f; p.seqSmooth = 0.55f; p.wander = 0.25f; p.speedScale = 0.75f;
        p.seqToTalk = 0.4f; p.talkMix = 0.3f;
        steps (p, { {2,6,0.8f,0.f},{1,0,0.6f,0.2f},{3,6,0.7f,-0.2f},{1,1,0.9f,0.1f},
                    {2,8,0.8f,-0.1f},{3,0,0.5f,0.3f},{1,6,0.9f,0.f},{2,1,0.6f,-0.3f} });
        p.phaserMix = 0.45f; p.phaserRate = 0.05f; p.delayMix = 0.3f; p.delayDiv = 9; p.reverbMix = 0.35f; p.reverbSize = 0.9f;
        v.push_back ({ "Mycelium Drift", p });
    }
    { // 5. Zenon Groove Bass — tight, sub-heavy rolling bass with subtle movement
        Params p;
        p.osc1Wave = 1.3f; p.subLevel = 0.6f; p.cutoff = 180.f; p.resonance = 0.45f; p.drive = 0.5f;
        p.envAmount = 0.35f; p.fD = 0.09f; p.aD = 0.18f; p.aS = 0.f; p.aR = 0.06f; p.keyTrack = 0.6f;
        p.numSteps = 16; p.stepDiv = 3; p.seqAmount = 1.1f; p.seqSmooth = 0.1f; p.legato = false;
        steps (p, { {0,0,0,0.f},{0,0,0,0.3f},{0,0,0,0.6f},{4,7,0.5f,0.1f} });
        p.outGainDb = 4.f;
        v.push_back ({ "Zenon Groove Bass", p });
    }
    { // 6. Spore Rain — FM blips with S&H and ping-pong
        Params p;
        p.osc1Wave = 0.f; p.osc2Wave = 0.f; p.osc2Level = 0.6f; p.osc2Oct = 1; p.osc2Semi = 7.f; p.fmAmount = 0.45f;
        p.cutoff = 1800.f; p.resonance = 0.7f; p.drive = 0.3f; p.slope = 2.f; p.envAmount = 0.3f; p.fD = 0.08f;
        p.aD = 0.12f; p.aS = 0.2f; p.subLevel = 0.f;
        p.numSteps = 12; p.stepDiv = 1; p.seqAmount = 2.5f; p.seqSmooth = 0.02f; p.seqToPitch = 0.08f;
        steps (p, { {6,5,1.f,0.f},{4,5,0.9f,0.2f},{8,5,0.8f,-0.2f},{3,8,1.f,0.f},{12,5,0.6f,0.3f},{4,7,1.f,-0.1f} });
        p.delayMix = 0.35f; p.delayDiv = 5; p.delayFb = 0.6f; p.reverbMix = 0.25f;
        p.outGainDb = 3.f;
        v.push_back ({ "Spore Rain", p });
    }
    { // 7. Deep Mud — dirty feedback + 18 dB slope, slow triangle swells
        Params p;
        p.osc1Wave = 1.8f; p.osc2Wave = 1.f; p.osc2Level = 0.4f; p.osc2Oct = 1; p.osc2Fine = 5.f; p.subLevel = 0.4f;
        p.cutoff = 220.f; p.resonance = 0.7f; p.drive = 0.8f; p.feedback = 0.5f; p.slope = 2.f; p.envAmount = 0.2f;
        p.numSteps = 8; p.stepDiv = 7; p.seqAmount = 2.8f; p.seqSmooth = 0.35f;
        steps (p, { {3,1,0.8f,0.f},{4,1,0.6f,0.1f},{6,0,0.5f,-0.2f},{2,1,0.9f,0.f} });
        p.biteType = 1; p.biteAmount = 0.25f; p.phaserMix = 0.5f; p.phaserRate = 0.12f; p.phaserFb = 0.7f;
        p.outGainDb = -5.f;
        v.push_back ({ "Deep Mud", p });
    }
    { // 8. Alien Frog — sync + FM + talk, chaotic croaks
        Params p;
        p.osc1Wave = 1.f; p.osc2Wave = 2.f; p.osc2Level = 0.7f; p.osc2Sync = true; p.osc2Semi = 12.f; p.fmAmount = 0.2f;
        p.cutoff = 500.f; p.resonance = 0.85f; p.drive = 0.5f; p.envAmount = 0.3f; p.fD = 0.1f;
        p.talkMix = 0.5f; p.seqToTalk = 0.7f; p.chaos = 0.9f;
        p.lfo2Rate = 0.3f; p.lfo2Shape = 5; p.lfo2ToWave = 0.4f;
        p.numSteps = 16; p.stepDiv = 3; p.seqAmount = 2.4f; p.seqSmooth = 0.06f; p.seqToRes = 0.3f; p.swing = 0.2f;
        steps (p, { {6,8,0.9f,0.f},{4,7,0.8f,0.f},{0,0,0,-0.5f},{8,8,0.7f,0.2f},{3,6,0.9f,0.f},{12,7,0.5f,-0.2f},{4,8,1.f,0.1f},{0,0,0,0.4f} });
        p.biteType = 3; p.biteAmount = 0.3f; p.delayMix = 0.2f; p.delayDiv = 4; p.reverbMix = 0.15f;
        p.outGainDb = -3.f;
        v.push_back ({ "Alien Frog", p });
    }
    { // 9. Sludge Tentacle — slow 1/8T sweeps, wavefolded bite, swing
        Params p;
        p.osc1Wave = 0.8f; p.osc2Wave = 1.f; p.osc2Level = 0.5f; p.osc2Fine = 12.f; p.subLevel = 0.3f;
        p.cutoff = 320.f; p.resonance = 0.82f; p.drive = 0.35f; p.slope = 3.f;
        p.numSteps = 24; p.stepDiv = 5; p.seqAmount = 3.5f; p.seqSmooth = 0.2f; p.swing = 0.3f; p.wander = 0.1f;
        steps (p, { {5,0,0.9f,-0.2f},{3,3,0.8f,0.2f},{8,7,0.7f,0.f},{1,1,1.f,0.f},{6,2,0.6f,-0.1f},{4,8,0.9f,0.1f} });
        p.biteType = 1; p.biteAmount = 0.4f; p.biteMix = 0.6f; p.phaserMix = 0.3f; p.delayMix = 0.25f; p.delayDiv = 8; p.reverbMix = 0.2f;
        v.push_back ({ "Sludge Tentacle", p });
    }
    { // 10. Fractal Bubbles — fast high-rate step LFOs = bubbling water texture
        Params p;
        p.osc1Wave = 2.6f; p.osc2Wave = 0.f; p.osc2Level = 0.3f; p.osc2Oct = 2; p.subLevel = 0.f;
        p.cutoff = 1200.f; p.resonance = 0.9f; p.drive = 0.25f; p.filterMode = 1; p.slope = 1.f;
        p.numSteps = 16; p.stepDiv = 3; p.seqAmount = 2.f; p.seqSmooth = 0.03f; p.speedScale = 1.5f;
        steps (p, { {12,0,0.6f,0.f},{10,5,0.5f,0.2f},{11,1,0.7f,-0.1f},{12,8,0.6f,0.1f} });
        p.delayMix = 0.3f; p.delayDiv = 2; p.delayFb = 0.55f; p.reverbMix = 0.3f; p.crush = 0.15f;
        p.outGainDb = -4.f;
        v.push_back ({ "Fractal Bubbles", p });
    }
    return v;
}
} // namespace sq
