#pragma once
#include "KickEngine.h"
#include <random>
#include <array>
#include <string>
#include <sstream>
#include <map>

constexpr int numGenres = 7;

struct GenreProfile
{
    const char* name;
    uint32_t colour;   // accent colour ARGB
    float tuneLo, tuneHi;
};

inline const std::array<GenreProfile, numGenres>& genreProfiles()
{
    static const std::array<GenreProfile, numGenres> g {{
        { "SCHRANZ",     0xffff3b5c, 46, 55 },
        { "PSYTRANCE",   0xff35e0c4, 44, 50 },
        { "HARD BOUNCE", 0xffffb02e, 48, 57 },
        { "HARDSTYLE",   0xff9b5cff, 43, 52 },
        { "HARD TECHNO", 0xff4da3ff, 45, 54 },
        { "GABBER",      0xffb6ff3b, 54, 68 },
        { "TECHNO",      0xffff6fd8, 45, 55 },
    }};
    return g;
}

// Builds a kick from a compact "key value key value ..." description.
inline KickParams kickSpec (const char* spec)
{
    KickParams k;
    std::map<std::string, float*> f {
        { "start", &k.pitchStart }, { "tune", &k.pitchEnd }, { "sweep", &k.pitchDecay }, { "curve", &k.pitchCurve },
        { "bend", &k.tailBend }, { "len", &k.length }, { "hold", &k.hold }, { "shape", &k.shape }, { "wave", &k.wave },
        { "click", &k.clickLevel }, { "snap", &k.clickDecay }, { "ctone", &k.clickTone },
        { "drive", &k.drive }, { "mix", &k.distMix }, { "filter", &k.tone }, { "reso", &k.reso }, { "punch", &k.punch },
        { "rum", &k.rumbleAmt }, { "size", &k.rumbleSize }, { "dirt", &k.rumbleDrive }, { "rtone", &k.rumbleTone },
        { "duck", &k.rumbleDuck }, { "roll", &k.rumbleRoll },
        { "blvl", &k.bassLevel }, { "bsemi", &k.bassSemi }, { "bdec", &k.bassDecay }, { "bcut", &k.bassCutoff } };
    std::istringstream in (spec);
    std::string key; float v;
    while (in >> key >> v)
    {
        if (key == "type") k.distType = (int) v;
        else if (key == "bpat") k.bassPattern = (int) v;
        else if (auto it = f.find (key); it != f.end()) *it->second = v;
    }
    return k;
}

struct PresetBank
{
    std::array<std::array<KickParams, 4>, numGenres> archetypes;
    std::array<std::array<const char*, 20>, numGenres> names;
};

// 4 sound archetypes per genre, 5 named variations each = 20 presets per genre.
inline const PresetBank& presetBank()
{
    static const PresetBank bank = []
    {
        PresetBank b;
        // SCHRANZ: aggressive, dirty, industrial punch + deep rolling, noisy rumble
        b.archetypes[0] = {{
            kickSpec ("start 700 tune 50 sweep 28 curve 1.3 len 300 hold 25 shape 1.3 wave .25 click .55 snap 3 ctone 3500 drive 22 type 0 mix 1 punch 8 filter 6500 reso 1.2 rum .7 size .85 dirt 22 rtone 170 duck .75 roll .6"),
            kickSpec ("start 550 tune 47 sweep 35 curve 1.2 len 360 hold 35 shape 1.1 wave .15 click .45 snap 4 ctone 3000 drive 18 type 0 mix .9 punch 10 filter 5000 reso 1 rum .85 size .9 dirt 26 rtone 140 duck .8 roll .8"),
            kickSpec ("start 900 tune 52 sweep 20 curve 1.6 len 260 hold 15 shape 1.6 wave .4 click .65 snap 2.5 ctone 5000 drive 28 type 1 mix .9 punch 5 filter 7000 reso 1.6 rum .6 size .8 dirt 28 rtone 200 duck .65 roll 1"),
            kickSpec ("start 800 tune 54 sweep 24 curve 1.4 len 280 hold 20 shape 1.4 wave .3 click .5 snap 3 ctone 4500 drive 24 type 3 mix .6 punch 6 filter 8000 reso 1.4 rum .65 size .82 dirt 20 rtone 180 duck .7 roll .7 bpat 0 blvl .25 bsemi 12 bdec 60 bcut 500") }};
        // PSYTRANCE: tight, clean kick + rolling bass
        b.archetypes[1] = {{
            kickSpec ("start 480 tune 48 sweep 32 curve 1.4 len 200 hold 12 shape 2 click .35 snap 2.5 ctone 6000 drive 4 mix .6 filter 14000 bpat 0 blvl .7 bdec 100 bcut 700"),
            kickSpec ("start 420 tune 46 sweep 26 curve 1.6 len 170 hold 8 shape 2.4 click .45 snap 1.8 ctone 8000 drive 6 mix .7 filter 12000 bpat 0 blvl .75 bdec 75 bcut 1200"),
            kickSpec ("start 600 tune 49 sweep 45 curve 1.1 len 230 hold 20 shape 1.6 click .3 snap 3 ctone 5000 drive 3 mix .5 filter 16000 bpat 2 blvl .65 bsemi 12 bdec 120 bcut 600"),
            kickSpec ("start 380 tune 47 sweep 38 curve 1.3 len 220 hold 18 shape 1.8 click .25 snap 4 ctone 4000 drive 3 mix .5 filter 13000 bpat 2 blvl .6 bdec 140 bcut 500") }};
        // HARD BOUNCE: round, clean, punchy, thick controlled sub, short cohesive tail
        b.archetypes[2] = {{
            kickSpec ("start 420 tune 52 sweep 38 curve 1.2 len 340 hold 60 shape 1.3 wave .05 click .35 snap 3 ctone 4000 drive 6 mix .8 filter 11000"),
            kickSpec ("start 520 tune 55 sweep 30 curve 1.4 len 300 hold 45 shape 1.5 wave .1 click .45 snap 2.5 ctone 5000 drive 8 mix .8 filter 10000"),
            kickSpec ("start 360 tune 49 sweep 45 curve 1.1 len 400 hold 80 shape 1.1 wave .08 click .3 snap 3.5 ctone 3500 drive 5 mix .7 filter 9000"),
            kickSpec ("start 650 tune 54 sweep 25 curve 1.6 len 280 hold 40 shape 1.6 wave .15 click .55 snap 2 ctone 6000 drive 9 mix .85 filter 12000") }};
        // HARDSTYLE: punch, then pitched distorted tonal tail that acts as the bassline
        b.archetypes[3] = {{
            kickSpec ("start 900 tune 49 sweep 40 curve 1.2 bend 1 len 700 hold 300 shape .8 wave .3 click .6 snap 4 ctone 3500 drive 26 type 0 mix 1 punch 45 filter 3500 reso 2.5"),
            kickSpec ("start 1100 tune 46 sweep 35 curve 1.3 bend 2 len 800 hold 350 shape .7 wave .5 click .7 snap 3 ctone 4000 drive 32 type 2 mix 1 punch 35 filter 2800 reso 3.5"),
            kickSpec ("start 800 tune 44 sweep 50 curve 1.1 bend .5 len 600 hold 220 shape 1 wave .25 click .5 snap 5 ctone 3000 drive 22 type 0 mix 1 punch 55 filter 3000 reso 2"),
            kickSpec ("start 1200 tune 51 sweep 30 curve 1.4 bend 3 len 750 hold 320 shape .75 wave .6 click .75 snap 3 ctone 5000 drive 34 type 1 mix 1 punch 30 filter 4500 reso 4.5") }};
        // HARD TECHNO: heavy, dark, hard transient; rumbling, dry, metallic or tonal
        b.archetypes[4] = {{
            kickSpec ("start 600 tune 48 sweep 30 curve 1.3 len 340 hold 30 shape 1.3 wave .15 click .5 snap 3 ctone 3500 drive 16 mix .9 punch 6 filter 7000 reso 1 rum .7 size .88 dirt 18 rtone 130 duck .85 roll .3"),
            kickSpec ("start 750 tune 50 sweep 24 curve 1.5 len 300 hold 25 shape 1.5 wave .2 click .6 snap 2.5 ctone 4500 drive 20 type 1 mix .85 punch 5 filter 8000 reso 1.2"),
            kickSpec ("start 900 tune 52 sweep 20 curve 1.6 len 280 hold 20 shape 1.6 wave .3 click .6 snap 2 ctone 6000 drive 22 type 2 mix .7 punch 4 filter 6000 reso 3 rum .4 size .7 dirt 16 rtone 200 duck .7"),
            kickSpec ("start 650 tune 46 sweep 35 curve 1.2 bend 1 len 420 hold 120 shape 1 wave .25 click .5 snap 3 ctone 3500 drive 18 mix 1 punch 20 filter 4000 reso 2 rum .35 size .75 dirt 14 rtone 120 duck .9 bpat 1 blvl .35 bsemi 12 bdec 110 bcut 700") }};
        // GABBER: brutal, clipped, explosive attack; the kick is the bass
        b.archetypes[5] = {{
            kickSpec ("start 1400 tune 58 sweep 22 curve 1.4 len 420 hold 160 shape .9 wave .5 click .7 snap 3 ctone 4000 drive 34 type 1 mix 1 filter 5500 reso 1.5"),
            kickSpec ("start 1700 tune 62 sweep 18 curve 1.6 len 380 hold 130 shape 1 wave .7 click .8 snap 2.5 ctone 5000 drive 38 type 1 mix 1 filter 6500 reso 2"),
            kickSpec ("start 1100 tune 55 sweep 28 curve 1.3 len 450 hold 180 shape .9 wave .4 click .6 snap 4 ctone 3500 drive 30 type 0 mix 1 punch 4 filter 4500 reso 1.8"),
            kickSpec ("start 2000 tune 66 sweep 14 curve 1.9 len 330 hold 110 shape 1.1 wave .8 click .9 snap 2 ctone 6000 drive 40 type 3 mix .8 filter 7000 reso 2.5") }};
        // TECHNO: classic 909-style kicks
        b.archetypes[6] = {{
            kickSpec ("start 330 tune 52 sweep 45 curve 1.2 len 380 hold 15 shape 1.8 click .55 snap 2.5 ctone 5000 drive 3 mix .5 filter 14000"),
            kickSpec ("start 220 tune 46 sweep 60 curve 1 len 500 hold 30 shape 1.5 click .3 snap 4 ctone 3000 drive 2 mix .4 filter 9000"),
            kickSpec ("start 350 tune 48 sweep 40 curve 1.2 len 360 hold 20 shape 1.6 click .45 snap 3 ctone 4000 drive 6 mix .6 filter 11000 rum .45 size .8 dirt 12 rtone 120 duck .8 roll .2"),
            kickSpec ("start 280 tune 54 sweep 35 curve 1.4 len 260 hold 10 shape 2.2 click .5 snap 2 ctone 7000 drive 2 mix .4 filter 15000") }};

        b.names[0] = {{ "Frankfurt Hammer", "Bunker Stomp", "Steel Press", "Loop Machine", "Iron Fist",
                        "Basement Wall", "Dark Tunnel", "Concrete Rumble", "Smoke Room", "Subway Pressure",
                        "Grinder", "Broken Tape", "Grind Cycle", "Rust Belt", "Static Mill",
                        "Machine Tok", "Drill Line", "Shrapnel", "Anvil Snap", "Factory Floor" }};
        b.names[1] = {{ "Full-On Roll", "Sunrise Stage", "Desert Flow", "Neon Rider", "Twilight Drive",
                        "Dark Forest", "Night Owl", "Swamp Roll", "Spore Snap", "Midnight Lab",
                        "Goa Gallop", "Beach Temple", "Retro Goa", "Spiral Dance", "Chai Shop",
                        "Prog Groove", "Deep Journey", "Clean Pulse", "Morning Glory", "Liquid Path" }};
        b.names[2] = {{ "Rubber Kick", "Bouncy Castle", "Pogo", "Spring Coil", "Jelly",
                        "Elastic", "Hop Scotch", "Skip Step", "Jump Tail", "Trampoline",
                        "Thick Sub", "Round One", "Fat Bounce", "Big Room Bounce", "Cushion",
                        "Snappy", "Crowd Lift", "Main Stage", "Confetti", "Hype Train" }};
        b.names[3] = {{ "Euphoric Lead", "Golden Hour", "Anthem Kick", "Hands Up", "Sky Line",
                        "Raw Zaag", "Grime Tail", "Scar Tissue", "Rawphoric", "Distortion Rider",
                        "Classic Hard", "Old School Gate", "Golden Era", "Reverse Rider", "Gate Keeper",
                        "Screech", "Metal Scream", "Razor", "Resonator", "Black Mass" }};
        b.names[4] = {{ "Warehouse Rumble", "Berlin Night", "Dark Room", "Fog Machine", "After Hours",
                        "Dry Hit", "Peak Time", "Strobe Rush", "Main Floor", "Hard Groove",
                        "Industrial", "Machine Shop", "Steel Mill", "Pipe Line", "Power Plant",
                        "Tonal Drive", "Deep Pressure", "Low Tone", "Bass Engine", "Night Shift" }};
        b.names[5] = {{ "Rotterdam", "Harbor Kick", "Hardcore Pride", "Mainport", "Old Skool Gabber",
                        "Hakken", "Distort Drive", "Hard Pressure", "Shell Suit", "Gabber Gate",
                        "Early Hardcore", "Rave Tape", "Warehouse 93", "Strobe 94", "Club Classic",
                        "Terror Crush", "Speedcore Snap", "Chaos Engine", "Bone Breaker", "Overload" }};
        b.names[6] = {{ "909 Punch", "Detroit Drive", "Warehouse Classic", "Steady Four", "Analog Thump",
                        "Deep Space", "Dub Chamber", "Night Drive", "Velvet", "Low Orbit",
                        "Peak Rumble", "Main Room", "Big Floor", "Pressure", "Strobe Line",
                        "Minimal Tick", "Clicky", "Tight Loop", "Micro", "Lean" }};
        return b;
    }();
    return bank;
}

// Varies a kick while keeping its character. scale 1 = gentle, 2+ = wild.
inline void varyKick (KickParams& p, std::mt19937& rng, float scale)
{
    std::uniform_real_distribution<float> u (-1.0f, 1.0f);
    auto mul = [&] (float& v, float amount, float lo, float hi) { v = std::clamp (v * (1.0f + amount * scale * u (rng)), lo, hi); };
    auto add = [&] (float& v, float amount, float lo, float hi) { v = std::clamp (v + amount * scale * u (rng), lo, hi); };
    mul (p.pitchStart, 0.15f, 80, 3000);
    mul (p.pitchDecay, 0.15f, 2, 400);
    mul (p.pitchCurve, 0.08f, 0.3f, 4);
    mul (p.length, 0.08f, 60, 2000);
    mul (p.hold, 0.15f, 0, 400);
    mul (p.shape, 0.08f, 0.3f, 4);
    add (p.wave, 0.06f, 0, 1);
    mul (p.clickLevel, 0.15f, 0, 1);
    mul (p.clickDecay, 0.2f, 0.5f, 40);
    mul (p.clickTone, 0.2f, 500, 16000);
    add (p.drive, 2.5f, 0, 40);
    mul (p.tone, 0.15f, 300, 20000);
    mul (p.reso, 0.12f, 0.5f, 8);
    if (p.punch > 0) mul (p.punch, 0.15f, 0, 120);
    if (std::abs (p.tailBend) > 0.01f) add (p.tailBend, 0.6f, -12, 12);
    if (p.rumbleAmt > 0)
    {
        mul (p.rumbleAmt, 0.12f, 0.05f, 1); mul (p.rumbleTone, 0.12f, 40, 400);
        add (p.rumbleRoll, 0.1f, 0, 1); add (p.rumbleDrive, 2.0f, 0, 30);
    }
    if (p.bassLevel > 0) { mul (p.bassDecay, 0.12f, 20, 400); mul (p.bassCutoff, 0.15f, 80, 4000); }
}

inline void keepUserSettings (KickParams& p, const KickParams& current)
{
    p.volume = current.volume;
    p.keyTrack = current.keyTrack;
}

inline KickParams presetParams (int genre, int index, const KickParams& current)
{
    genre = std::clamp (genre, 0, numGenres - 1);
    index = std::clamp (index, 0, 19);
    KickParams p = presetBank().archetypes[(size_t) genre][(size_t) (index / 5)];
    keepUserSettings (p, current);
    if (index % 5 == 0) return p;
    std::mt19937 rng ((uint32_t) (genre * 1000 + index * 7 + 1));
    varyKick (p, rng, 1.0f);
    // tune each variation to a different note in the genre range
    const auto& g = genreProfiles()[(size_t) genre];
    std::uniform_real_distribution<float> t (g.tuneLo, g.tuneHi);
    p.pitchEnd = t (rng);
    return p;
}

// RANDOM: start from one of the genre's archetypes and push it around.
inline KickParams randomKick (int genre, std::mt19937& rng, const KickParams& current)
{
    genre = std::clamp (genre, 0, numGenres - 1);
    std::uniform_int_distribution<int> pick (0, 3);
    KickParams p = presetBank().archetypes[(size_t) genre][(size_t) pick (rng)];
    keepUserSettings (p, current);
    varyKick (p, rng, 2.0f);
    const auto& g = genreProfiles()[(size_t) genre];
    std::uniform_real_distribution<float> t (g.tuneLo, g.tuneHi);
    p.pitchEnd = t (rng);
    return p;
}

// MUTATE: small nudge of the current sound.
inline KickParams mutateKick (const KickParams& current, std::mt19937& rng)
{
    KickParams p = current;
    varyKick (p, rng, 0.7f);
    return p;
}
