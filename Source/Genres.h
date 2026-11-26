#pragma once
#include "KickEngine.h"
#include <random>
#include <array>
#include <string>

struct Range { float lo, hi; };

struct GenreProfile
{
    const char* name;
    uint32_t colour;           // accent colour ARGB
    Range pitchStart, pitchEnd, pitchDecay, pitchCurve, tailBend;
    Range length, hold, shape, wave;
    Range clickLevel, clickDecay, clickTone;
    Range drive, distMix, tone;
    std::array<float, 4> distWeights; // Soft, Hard, Fold, Crush
    float rumbleChance; Range rumbleAmt, rumbleSize, rumbleDrive, rumbleTone, rumbleDuck;
    float bassChance;   Range bassLevel, bassDecay, bassCutoff;
    std::array<float, 4> bassWeights; // Rolling, Offbeat, Gallop, Reverse
    std::array<float, 2> octaveWeights; // bass at 0 or +12 semitones
};

// Ranges tuned by ear for each style.
inline const std::array<GenreProfile, 6>& genreProfiles()
{
    static const std::array<GenreProfile, 6> g {{
        { "SCHRANZ", 0xffff3b5c, {500, 1400}, {44, 60}, {12, 45}, {0.8f, 2.0f}, {0, 3},
          {220, 420}, {10, 60}, {0.8f, 2.2f}, {0.1f, 0.6f},
          {0.4f, 0.9f}, {2, 10}, {2500, 9000},
          {18, 34}, {0.8f, 1.0f}, {5000, 14000}, {0.3f, 0.5f, 0.1f, 0.1f},
          0.65f, {0.25f, 0.7f}, {0.55f, 0.85f}, {10, 26}, {90, 200}, {0.5f, 0.9f},
          0.3f, {0.25f, 0.5f}, {50, 120}, {400, 1400}, {0.4f, 0.6f, 0.0f, 0.0f}, {0.5f, 0.5f} },
        { "PSYTRANCE", 0xff35e0c4, {300, 900}, {44, 56}, {18, 60}, {0.9f, 1.8f}, {0, 0.5f},
          {140, 260}, {5, 35}, {1.2f, 3.0f}, {0.0f, 0.15f},
          {0.2f, 0.6f}, {1.5f, 6}, {3000, 11000},
          {0, 8}, {0.3f, 0.8f}, {7000, 18000}, {0.9f, 0.1f, 0.0f, 0.0f},
          0.0f, {0, 0}, {0, 0}, {0, 0}, {100, 100}, {0, 0},
          1.0f, {0.5f, 0.8f}, {60, 140}, {350, 1600}, {0.6f, 0.0f, 0.4f, 0.0f}, {0.7f, 0.3f} },
        { "HARD BOUNCE", 0xffffb02e, {700, 1800}, {46, 62}, {20, 70}, {0.7f, 1.6f}, {2, 9},
          {420, 750}, {40, 160}, {0.5f, 1.4f}, {0.2f, 0.7f},
          {0.4f, 0.9f}, {3, 12}, {2000, 7000},
          {16, 32}, {0.8f, 1.0f}, {4000, 11000}, {0.4f, 0.3f, 0.3f, 0.0f},
          0.2f, {0.1f, 0.3f}, {0.4f, 0.7f}, {8, 16}, {120, 220}, {0.7f, 0.9f},
          0.85f, {0.4f, 0.7f}, {90, 200}, {700, 2000}, {0.0f, 0.5f, 0.0f, 0.5f}, {0.6f, 0.4f} },
        { "HARDSTYLE", 0xff9b5cff, {800, 2000}, {44, 58}, {25, 80}, {0.7f, 1.5f}, {1, 7},
          {500, 1000}, {60, 200}, {0.5f, 1.3f}, {0.2f, 0.6f},
          {0.5f, 1.0f}, {4, 15}, {1500, 6000},
          {18, 34}, {0.9f, 1.0f}, {3500, 9000}, {0.5f, 0.2f, 0.3f, 0.0f},
          0.1f, {0.1f, 0.25f}, {0.4f, 0.6f}, {8, 14}, {120, 200}, {0.7f, 0.9f},
          0.75f, {0.45f, 0.7f}, {100, 200}, {700, 1600}, {0.0f, 0.2f, 0.0f, 0.8f}, {1.0f, 0.0f} },
        { "HARD TECHNO", 0xff4da3ff, {400, 1200}, {42, 58}, {15, 55}, {0.8f, 2.0f}, {0, 2},
          {280, 600}, {15, 80}, {0.7f, 1.8f}, {0.05f, 0.5f},
          {0.3f, 0.8f}, {2, 9}, {2500, 10000},
          {10, 26}, {0.6f, 1.0f}, {4000, 13000}, {0.5f, 0.3f, 0.1f, 0.1f},
          1.0f, {0.4f, 0.8f}, {0.7f, 0.92f}, {14, 26}, {80, 180}, {0.6f, 0.95f},
          0.3f, {0.25f, 0.45f}, {60, 140}, {400, 1500}, {0.3f, 0.7f, 0.0f, 0.0f}, {0.4f, 0.6f} },
        { "GABBER", 0xffb6ff3b, {900, 2000}, {50, 70}, {15, 50}, {1.0f, 2.5f}, {0, 4},
          {250, 500}, {20, 90}, {0.7f, 1.6f}, {0.3f, 0.9f},
          {0.5f, 1.0f}, {2, 8}, {3000, 9000},
          {26, 36}, {0.9f, 1.0f}, {5000, 12000}, {0.1f, 0.6f, 0.1f, 0.2f},
          0.3f, {0.15f, 0.4f}, {0.4f, 0.7f}, {10, 20}, {140, 220}, {0.5f, 0.8f},
          0.2f, {0.2f, 0.35f}, {50, 90}, {600, 1500}, {0.3f, 0.7f, 0.0f, 0.0f}, {0.3f, 0.7f} },
    }};
    return g;
}

inline KickParams randomKick (int genre, std::mt19937& rng, const KickParams& current)
{
    const auto& g = genreProfiles()[(size_t) std::clamp (genre, 0, 5)];
    std::uniform_real_distribution<float> u (0.0f, 1.0f);
    auto pick = [&] (Range r) { return r.lo + (r.hi - r.lo) * u (rng); };
    auto pickLog = [&] (Range r) { return r.lo * std::pow (r.hi / r.lo, u (rng)); };

    KickParams p = current; // keeps volume & key track
    p.pitchStart = pickLog (g.pitchStart);
    p.pitchEnd   = pick (g.pitchEnd);
    p.pitchDecay = pickLog (g.pitchDecay);
    p.pitchCurve = pick (g.pitchCurve);
    p.tailBend   = pick (g.tailBend);
    p.length     = pick (g.length);
    p.hold       = pick (g.hold);
    p.shape      = pick (g.shape);
    p.wave       = pick (g.wave);
    p.clickLevel = pick (g.clickLevel);
    p.clickDecay = pickLog (g.clickDecay);
    p.clickTone  = pickLog (g.clickTone);
    p.drive      = pick (g.drive);
    p.distMix    = pick (g.distMix);
    p.tone       = pickLog (g.tone);
    std::discrete_distribution<int> dt (g.distWeights.begin(), g.distWeights.end());
    p.distType = dt (rng);

    if (u (rng) < g.rumbleChance)
    {
        p.rumbleAmt   = pick (g.rumbleAmt);
        p.rumbleSize  = pick (g.rumbleSize);
        p.rumbleDrive = pick (g.rumbleDrive);
        p.rumbleTone  = pickLog (g.rumbleTone);
        p.rumbleDuck  = pick (g.rumbleDuck);
    }
    else p.rumbleAmt = 0.0f;

    if (u (rng) < g.bassChance)
    {
        std::discrete_distribution<int> bp (g.bassWeights.begin(), g.bassWeights.end());
        std::discrete_distribution<int> oc (g.octaveWeights.begin(), g.octaveWeights.end());
        p.bassPattern = bp (rng);
        p.bassLevel   = pick (g.bassLevel);
        p.bassSemi    = oc (rng) == 1 ? 12.0f : 0.0f;
        p.bassDecay   = pick (g.bassDecay);
        p.bassCutoff  = pickLog (g.bassCutoff);
    }
    else p.bassLevel = 0.0f;
    return p;
}

//==============================================================================
// Presets: 4 sound archetypes per genre (inspired by classic sounds of each
// scene), 5 named variations each = 20 per genre.

inline KickParams makeKick (float pS, float pE, float pD, float pC, float bend,
                            float len, float hold, float shape, float wave,
                            float cL, float cD, float cT,
                            float drv, int dT, float dMix, float tone,
                            float rA, float rS, float rD, float rT, float rDk,
                            int bP, float bL, float bS, float bD, float bC)
{
    KickParams k;
    k.pitchStart = pS; k.pitchEnd = pE; k.pitchDecay = pD; k.pitchCurve = pC; k.tailBend = bend;
    k.length = len; k.hold = hold; k.shape = shape; k.wave = wave;
    k.clickLevel = cL; k.clickDecay = cD; k.clickTone = cT;
    k.drive = drv; k.distType = dT; k.distMix = dMix; k.tone = tone;
    k.rumbleAmt = rA; k.rumbleSize = rS; k.rumbleDrive = rD; k.rumbleTone = rT; k.rumbleDuck = rDk;
    k.bassPattern = bP; k.bassLevel = bL; k.bassSemi = bS; k.bassDecay = bD; k.bassCutoff = bC;
    return k;
}

struct PresetBank
{
    std::array<std::array<KickParams, 4>, 6> archetypes;
    std::array<std::array<const char*, 20>, 6> names;
};

inline const PresetBank& presetBank()
{
    static const PresetBank bank = []
    {
        PresetBank b;
        //            pS    pE  pD  pC  bend len  hold shp  wav   cL   cD  cT     drv dT mix  tone    rA   rS   rD  rT   rDk   bP bL   bS  bD   bC
        b.archetypes[0] = {{  // SCHRANZ
            makeKick (900,  52, 22, 1.4f, 1, 320, 30, 1.4f, .3f, .7f, 4, 6000,  28, 1, 1,   9000,  .45f, .75f, 18, 140, .7f, 1, 0,   0,  90,  600),
            makeKick (700,  48, 30, 1.2f, 0, 420, 40, 1.1f, .2f, .5f, 6, 4000,  22, 0, .9f, 7000,  .7f,  .85f, 22, 110, .8f, 1, 0,   0,  90,  600),
            makeKick (1100, 55, 15, 1.8f, 2, 260, 15, 1.8f, .5f, .8f, 3, 8000,  30, 3, .8f, 11000, .3f,  .6f,  15, 180, .6f, 1, .35f, 0, 90,  600),
            makeKick (1300, 58, 12, 2.0f, 3, 230, 10, 2.0f, .6f, .9f, 2.5f, 9000, 32, 1, 1, 12000, .2f,  .5f,  12, 200, .5f, 0, .3f, 12, 70,  900) }};
        b.archetypes[1] = {{  // PSYTRANCE
            makeKick (650,  47, 35, 1.3f, 0, 210, 20, 1.8f, .05f, .4f, 3, 7000, 4, 0, .5f, 16000,  0, .5f, 10, 100, .5f,  0, .7f,  0, 110, 800),
            makeKick (500,  45, 25, 1.6f, 0, 170, 10, 2.4f, .1f, .55f, 2, 10000, 7, 0, .7f, 14000, 0, .5f, 10, 100, .5f,  2, .75f, 0, 80,  1300),
            makeKick (800,  50, 50, 1.1f, .3f, 250, 25, 1.5f, 0, .3f, 4, 5000,  2, 0, .4f, 18000,  0, .5f, 10, 100, .5f,  0, .6f, 12, 130, 600),
            makeKick (400,  48, 40, 1.2f, 0, 230, 30, 1.6f, .08f, .25f, 5, 4000, 3, 0, .4f, 15000, 0, .5f, 10, 100, .5f,  2, .6f,  0, 140, 500) }};
        b.archetypes[2] = {{  // HARD BOUNCE
            makeKick (1400, 52, 40, 1.1f, 6, 600, 90,  .9f, .5f,  .7f, 8, 4000,  26, 0, 1,   8000, .1f,  .5f, 10, 150, .8f,  3, .6f,  0, 150, 1200),
            makeKick (1200, 55, 50, 1.0f, 8, 650, 120, .8f, .4f,  .6f, 10, 3500, 22, 2, 1,   7000, 0,    .5f, 10, 150, .8f,  3, .5f,  0, 150, 1500),
            makeKick (1600, 58, 30, 1.3f, 4, 480, 60, 1.2f, .35f, .8f, 6, 5000,  28, 1, .95f, 9000, 0,   .5f, 10, 150, .8f,  1, .5f, 12, 120, 1000),
            makeKick (1800, 50, 45, 1.0f, 5, 700, 140, .7f, .45f, .9f, 12, 3000, 30, 0, 1,   6500, .15f, .6f, 12, 130, .85f, 3, .55f, 0, 150, 1000) }};
        b.archetypes[3] = {{  // HARDSTYLE
            makeKick (1500, 50, 50, 1.0f, 3, 800, 150, .7f, .35f, .8f, 10, 3000, 24, 0, 1, 7000, 0,   .5f, 10, 150, .8f, 3, .5f,  0,  150, 900),
            makeKick (2000, 48, 35, 1.2f, 6, 900, 180, .6f, .6f,  .9f, 8, 4000,  34, 2, 1, 6000, 0,   .5f, 10, 150, .8f, 3, .65f, 0,  150, 1400),
            makeKick (1000, 46, 60, 0.9f, 2, 600, 100, .9f, .25f, .6f, 12, 2500, 20, 0, 1, 8000, 0,   .5f, 10, 150, .8f, 1, .4f, 12,  150, 700),
            makeKick (1800, 58, 25, 1.4f, 4, 500, 60, 1.1f, .5f, 1.0f, 5, 5000,  32, 1, 1, 9000, .1f, .4f, 12, 180, .7f, 0, 0,    0,  100, 800) }};
        b.archetypes[4] = {{  // HARD TECHNO
            makeKick (600,  48, 30, 1.2f, 0, 380, 30, 1.3f, .15f, .45f, 5, 5000, 14, 0, .8f, 9000,  .75f, .88f, 20, 120, .85f, 0, 0,   0,  100, 800),
            makeKick (900,  52, 20, 1.6f, 1, 300, 20, 1.6f, .3f,  .6f, 3, 8000,  22, 1, .9f, 11000, .55f, .8f,  25, 160, .7f,  1, .3f, 12, 80,  700),
            makeKick (750,  50, 28, 1.3f, 0, 420, 50, 1.1f, .2f,  .5f, 6, 4000,  16, 0, .9f, 8000,  .6f,  .82f, 18, 100, .9f,  1, .4f,  0, 120, 500),
            makeKick (1000, 55, 18, 1.7f, 2, 280, 15, 1.7f, .25f, .7f, 3, 9000,  20, 2, .7f, 12000, .4f,  .7f,  15, 180, .6f,  0, .45f, 12, 90, 1500) }};
        b.archetypes[5] = {{  // GABBER
            makeKick (1700, 62, 20, 1.6f, 2, 380, 40, 1.2f, .6f, .7f, 4, 6000,  34, 0, 1,   9000,  .2f,  .5f,  10, 180, .6f, 0, 0,    0, 100, 800),
            makeKick (2000, 66, 15, 2.0f, 3, 320, 30, 1.3f, .8f, .9f, 3, 8000,  36, 1, 1,   11000, 0,    .5f,  10, 180, .6f, 1, .25f, 12, 60, 1000),
            makeKick (1200, 58, 30, 1.3f, 1, 450, 60, 1.0f, .5f, .6f, 6, 5000,  28, 2, 1,   8000,  .35f, .7f,  14, 150, .7f, 0, 0,    0, 100, 800),
            makeKick (2000, 70, 12, 2.4f, 4, 260, 20, 1.6f, .9f, 1.0f, 2, 9000, 36, 3, .9f, 12000, .25f, .55f, 20, 200, .5f, 0, 0,    0, 100, 800) }};

        b.names[0] = {{ "Frankfurt Hammer", "Bunker Stomp", "Steel Press", "Loop Machine", "Iron Fist",
                        "Basement Wall", "Dark Tunnel", "Concrete Rumble", "Smoke Room", "Subway Pressure",
                        "Crushed Loop", "Broken Tape", "Grind Cycle", "Rust Belt", "Static Mill",
                        "Metal Tok", "Drill Line", "Shrapnel", "Anvil Snap", "Factory Floor" }};
        b.names[1] = {{ "Full-On Roll", "Sunrise Stage", "Desert Flow", "Neon Rider", "Twilight Drive",
                        "Dark Forest", "Night Owl", "Swamp Gallop", "Spore Snap", "Midnight Lab",
                        "Goa Bounce", "Beach Temple", "Retro Goa", "Spiral Dance", "Chai Shop",
                        "Prog Groove", "Deep Journey", "Clean Pulse", "Morning Glory", "Liquid Path" }};
        b.names[2] = {{ "Bounce Zaag", "Jump Tail", "Rubber Kick", "Bouncy Castle", "Hype Train",
                        "Pitched Tail", "Slide Down", "Melodic Drop", "Glide Bomb", "Tone Bender",
                        "Offbeat Hop", "Pogo", "Skip Step", "Hop Scotch", "Spring Coil",
                        "Festival Punch", "Main Stage", "Crowd Lift", "Confetti", "Big Room Bounce" }};
        b.names[3] = {{ "Euphoric Lead", "Golden Hour", "Anthem Kick", "Hands Up", "Sky Line",
                        "Raw Zaag", "Screech", "Grime Tail", "Scar Tissue", "Black Mass",
                        "Classic Hard", "Old School Gate", "Golden Era", "Reverse Rider", "Gate Keeper",
                        "Uptempo Tok", "Rapid Fire", "Speed Freak", "Overdrive", "Red Line" }};
        b.names[4] = {{ "Warehouse Rumble", "Berlin Night", "Dark Room", "Fog Machine", "After Hours",
                        "Industrial", "Machine Shop", "Power Plant", "Steel Mill", "Pipe Line",
                        "Peak Time", "Strobe Rush", "Main Floor", "Hard Groove", "Laser Grid",
                        "Acid Punch", "303 Bite", "Acid Rain", "Squelch", "Toxic" }};
        b.names[5] = {{ "Rotterdam", "Harbor Kick", "Hardcore Pride", "Mainport", "Old Skool Gabber",
                        "Hakken", "Distort Drive", "Hard Pressure", "Shell Suit", "Gabber Gate",
                        "Early Hardcore", "Rave Tape", "Warehouse 93", "Strobe 94", "Club Classic",
                        "Terror Crush", "Speedcore Snap", "Chaos Engine", "Bone Breaker", "Overload" }};
        return b;
    }();
    return bank;
}

// Preset = archetype + small deterministic variation (variation 0 is the archetype itself).
inline KickParams presetParams (int genre, int index, const KickParams& current)
{
    genre = std::clamp (genre, 0, 5);
    index = std::clamp (index, 0, 19);
    KickParams p = presetBank().archetypes[(size_t) genre][(size_t) (index / 5)];
    p.volume = current.volume;
    p.keyTrack = current.keyTrack;
    const int variation = index % 5;
    if (variation == 0) return p;

    std::mt19937 rng ((uint32_t) (genre * 1000 + index * 7 + 1));
    std::uniform_real_distribution<float> u (-1.0f, 1.0f);
    auto vary = [&] (float& v, float amount, float lo, float hi) { v = std::clamp (v * (1.0f + amount * u (rng)), lo, hi); };
    vary (p.pitchStart, 0.2f, 80, 3000);
    vary (p.pitchEnd, 0.06f, 25, 120);
    vary (p.pitchDecay, 0.25f, 2, 400);
    vary (p.pitchCurve, 0.15f, 0.3f, 4);
    vary (p.length, 0.15f, 60, 2000);
    vary (p.hold, 0.25f, 0, 400);
    vary (p.shape, 0.15f, 0.3f, 4);
    p.wave = std::clamp (p.wave + 0.12f * u (rng), 0.0f, 1.0f);
    vary (p.clickLevel, 0.2f, 0, 1);
    vary (p.clickDecay, 0.3f, 0.5f, 40);
    vary (p.clickTone, 0.3f, 500, 16000);
    p.drive = std::clamp (p.drive + 4.0f * u (rng), 0.0f, 40.0f);
    vary (p.tone, 0.25f, 300, 20000);
    p.tailBend = std::clamp (p.tailBend + 1.5f * u (rng), -12.0f, 12.0f);
    if (p.rumbleAmt > 0) { vary (p.rumbleAmt, 0.2f, 0, 1); vary (p.rumbleTone, 0.2f, 40, 400); }
    if (p.bassLevel > 0) { vary (p.bassDecay, 0.2f, 20, 400); vary (p.bassCutoff, 0.25f, 80, 4000); }
    return p;
}
