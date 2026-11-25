#pragma once
#include "KickEngine.h"
#include <random>
#include <array>

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
};

// Ranges tuned by ear for each style.
inline const std::array<GenreProfile, 6>& genreProfiles()
{
    static const std::array<GenreProfile, 6> g {{
        // name         colour       pStart        pEnd         pDecay       pCurve      bend
        { "SCHRANZ",    0xffff3b5c, {500, 1400}, {44, 60},   {12, 45},   {0.8f, 2.0f}, {0, 3},
                                     /*len*/{220, 420}, {10, 60}, {0.8f, 2.2f}, {0.1f, 0.6f},
                                     /*click*/{0.4f, 0.9f}, {2, 10}, {2500, 9000},
                                     /*drive*/{18, 34}, {0.8f, 1.0f}, {5000, 14000}, {0.3f, 0.5f, 0.1f, 0.1f} },
        { "PSYTRANCE",  0xff35e0c4, {300, 900},  {44, 56},   {18, 60},   {0.9f, 1.8f}, {0, 0.5f},
                                     {140, 260}, {5, 35},   {1.2f, 3.0f}, {0.0f, 0.15f},
                                     {0.2f, 0.6f}, {1.5f, 6}, {3000, 11000},
                                     {0, 8},   {0.3f, 0.8f}, {7000, 18000}, {0.9f, 0.1f, 0.0f, 0.0f} },
        { "HARD BOUNCE",0xffffb02e, {700, 1800}, {46, 62},   {20, 70},   {0.7f, 1.6f}, {2, 9},
                                     {420, 750}, {40, 160}, {0.5f, 1.4f}, {0.2f, 0.7f},
                                     {0.4f, 0.9f}, {3, 12}, {2000, 7000},
                                     {16, 32}, {0.8f, 1.0f}, {4000, 11000}, {0.4f, 0.3f, 0.3f, 0.0f} },
        { "HARDSTYLE",  0xff9b5cff, {800, 2000}, {44, 58},   {25, 80},   {0.7f, 1.5f}, {1, 7},
                                     {500, 1000},{60, 200}, {0.5f, 1.3f}, {0.2f, 0.6f},
                                     {0.5f, 1.0f}, {4, 15}, {1500, 6000},
                                     {18, 34}, {0.9f, 1.0f}, {3500, 9000}, {0.5f, 0.2f, 0.3f, 0.0f} },
        { "HARD TECHNO",0xff4da3ff, {400, 1200}, {42, 58},   {15, 55},   {0.8f, 2.0f}, {0, 2},
                                     {280, 600}, {15, 80},  {0.7f, 1.8f}, {0.05f, 0.5f},
                                     {0.3f, 0.8f}, {2, 9}, {2500, 10000},
                                     {10, 26}, {0.6f, 1.0f}, {4000, 13000}, {0.5f, 0.3f, 0.1f, 0.1f} },
        { "GABBER",     0xffb6ff3b, {900, 2000}, {50, 70},   {15, 50},   {1.0f, 2.5f}, {0, 4},
                                     {250, 500}, {20, 90},  {0.7f, 1.6f}, {0.3f, 0.9f},
                                     {0.5f, 1.0f}, {2, 8}, {3000, 9000},
                                     {26, 36}, {0.9f, 1.0f}, {5000, 12000}, {0.1f, 0.6f, 0.1f, 0.2f} },
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
    return p;
}
