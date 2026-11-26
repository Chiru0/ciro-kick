#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <vector>

// Zero-delay-feedback state-variable filter.
struct SVF
{
    double ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    double lowpass (double x, double fc, double sr, double q = 0.8)
    {
        const double g = std::tan (3.14159265358979 * std::clamp (fc, 10.0, sr * 0.45) / sr);
        const double k = 1.0 / q;
        const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const double v3 = x - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        return v2;
    }
    // returns low-pass, writes band-pass and high-pass
    double multi (double x, double fc, double sr, double q, double& bp, double& hp)
    {
        const double g = std::tan (3.14159265358979 * std::clamp (fc, 10.0, sr * 0.45) / sr);
        const double k = 1.0 / q;
        const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const double v3 = x - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        bp = v1; hp = x - k * v1 - v2;
        return v2;
    }
};

// Kick builder layers stacked on the main kick body.
// slot 0 = PUNCH, 1 = TAIL, 2/3 = EXTRA (enhancers)
constexpr int numLayers = 4;
struct LayerParams
{
    int   type  = 0;
    float level = 0.0f;     // 0..1 (0 = off)
    float tune  = 0.0f;     // semitones
    float decay = 200.0f;   // ms
    float drive = 12.0f;    // dB
    float tone  = 3000.0f;  // Hz
    float start = 0.0f;     // ms offset
};

inline int layerRole (int slot) { return slot < 2 ? slot : 2; }
inline const std::vector<const char*>& layerTypeNames (int role)
{
    static const std::vector<const char*> punch { "HARDSTYLISH", "INDUSTRIAL", "TOP KICK", "TRANSIENT", "TOKYO SNAP" };
    static const std::vector<const char*> tail  { "BIGROOM", "IND CRUNCH", "PSY ROLL", "PSY SHOT", "RAW CRUNCH", "REVERSE", "ROBOT", "RUMBLE" };
    static const std::vector<const char*> extra { "CLICK", "LOW MID", "NOISE", "NOISE 2", "RESONATOR", "REVERB", "SCHRANZ TOP" };
    return role == 0 ? punch : role == 1 ? tail : extra;
}

// Plain snapshot of every sound parameter (units are the ones shown in the UI).
struct KickParams
{
    float pitchStart = 600.0f;  // Hz
    float pitchEnd   = 50.0f;   // Hz
    float pitchDecay = 40.0f;   // ms
    float pitchCurve = 1.0f;    // >1 = snappier drop
    float tailBend   = 0.0f;    // semitones the tail glides down over the length

    float length     = 450.0f;  // ms
    float hold       = 30.0f;   // ms at full level before the decay
    float shape      = 1.5f;    // decay curve exponent
    float wave       = 0.0f;    // 0 sine .. 1 squarish
    float sub        = 0.0f;    // clean sine layer under the distortion

    float clickLevel = 0.4f;    // 0..1
    float clickDecay = 6.0f;    // ms
    float clickTone  = 4000.0f; // Hz
    float metal      = 0.0f;    // metallic clank layer
    float metalTone  = 500.0f;  // Hz

    float drive      = 6.0f;    // dB
    int   distType   = 0;       // 0 Soft, 1 Hard, 2 Fold, 3 Crush
    float distMix    = 1.0f;    // 0..1
    float tone       = 12000.0f;// Hz low-pass after drive
    float reso       = 0.7f;    // filter resonance (Q)
    float punch      = 0.0f;    // ms of clean attack before the distortion kicks in

    float rumbleAmt   = 0.0f;   // 0..1 (0 = off)
    float rumbleSize  = 0.75f;  // 0..1 reverb size
    float rumbleDrive = 16.0f;  // dB
    float rumbleTone  = 140.0f; // Hz low-pass
    float rumbleDuck  = 0.7f;   // 0..1 sidechain under the kick
    float rumbleRoll  = 0.0f;   // 0..1 rhythmic gating
    int   rumbleRate  = 1;      // 0 = 1/8, 1 = 1/16, 2 = 1/32
    float rumblePre   = 0.0f;   // ms pre-delay
    float rumbleDamp  = 0.35f;  // reverb damping
    float rumbleLow   = 25.0f;  // Hz low cut
    float rumbleGrit  = 0.3f;   // noise texture
    float rEnvT[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    float rEnvG[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    int   bassPattern = 0;      // 0 Rolling, 1 Offbeat, 2 Gallop, 3 Reverse
    float bassLevel   = 0.0f;   // 0..1 (0 = off)
    float bassSemi    = 0.0f;   // semitones from the kick tune
    float bassDecay   = 110.0f; // ms
    float bassCutoff  = 800.0f; // Hz
    int   bassOsc     = 0;      // 0 Saw, 1 Square, 2 Sine, 3 Reese
    float bassAttack  = 1.0f;   // ms
    float bassEnvAmt  = 5.0f;   // filter envelope amount
    float bassReso    = 0.9f;   // filter Q
    float bassDrive   = 4.0f;   // dB
    float bassGlide   = 0.0f;   // semitones
    float bEnvT[5] = { 0.0f, 0.1f, 0.3f, 0.6f, 1.0f };
    float bEnvG[5] = { 1.0f, 0.85f, 0.5f, 0.18f, 0.0f };

    // transient shaper: gain nodes over the kick length
    float envT[5] = { 0.0f, 0.04f, 0.12f, 0.35f, 0.7f };
    float envG[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    LayerParams layers[numLayers];
    int bodyCat = 0;            // builder body category (only used by the generator)

    float volume     = -3.0f;   // dB
    bool  keyTrack   = false;
};

// One monophonic kick voice, rendered sample by sample.
class KickVoice
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }

    void trigger (const KickParams& p, float velocity, float baseFreqOverride = -1.0f)
    {
        params = p;
        vel = velocity;
        endHz = baseFreqOverride > 0.0f ? baseFreqOverride : p.pitchEnd;
        startHz = p.pitchStart * (endHz / std::max (1.0f, p.pitchEnd));
        t = 0.0;
        phase = 0.0;
        active = true;
        fade = 1.0f;
        fadeStep = 0.0f;
        bp1 = bp2 = 0.0f;
        lp = 0.0f;
        toneFilter.reset();
        for (auto& m : metalPh) m = 0.0;
        crushHold = 0.0f;
        crushCount = 0;
    }

    void release() // quick fade used when a new note cuts this one
    {
        if (active) fadeStep = 1.0f / (float) (0.004 * sampleRate);
    }

    bool isActive() const { return active; }
    float getBaseHz() const { return (float) endHz; }

    float next()
    {
        if (! active) return 0.0f;

        const double sec = t;
        const double ms  = sec * 1000.0;
        const double lenMs = std::max (10.0, (double) params.length);

        // ---- pitch
        const double pd = std::max (0.5, (double) params.pitchDecay) / 1000.0;
        double pEnv = std::exp (-sec / pd);
        pEnv = std::pow (pEnv, (double) params.pitchCurve);
        double base = endHz;
        if (params.tailBend != 0.0f)
        {
            const double x = std::min (1.0, ms / lenMs);
            base *= std::pow (2.0, -(double) params.tailBend * x / 12.0);
        }
        const double freq = base + (startHz - endHz) * pEnv;
        phase += freq / sampleRate;
        phase -= std::floor (phase);

        // ---- oscillator (sine morphing to a soft square)
        const double s = std::sin (phase * 2.0 * 3.14159265358979);
        double osc = s;
        if (params.wave > 0.001f)
        {
            const double k = 1.0 + params.wave * 9.0;
            osc = std::tanh (k * s) / std::tanh (k);
        }

        // ---- amplitude envelope
        const double attack = 0.6;
        double amp;
        if (ms < attack) amp = ms / attack;
        else if (ms < params.hold) amp = 1.0;
        else
        {
            const double decLen = std::max (1.0, lenMs - std::max ((double) params.hold, attack));
            const double x = (ms - std::max ((double) params.hold, attack)) / decLen;
            amp = x >= 1.0 ? 0.0 : std::pow (1.0 - x, (double) params.shape);
        }

        // transient shaper nodes
        {
            const double x = ms / lenMs;
            double gain = params.envG[4];
            if (x <= params.envT[0]) gain = params.envG[0];
            else
                for (int i = 0; i < 4; ++i)
                {
                    const double t0 = params.envT[i], t1 = std::max (params.envT[i + 1], params.envT[i] + 1.0e-4f);
                    if (x < t1) { gain = params.envG[i] + (params.envG[i + 1] - params.envG[i]) * (x - t0) / (t1 - t0); break; }
                }
            amp *= gain;
        }

        double body = osc * amp;
        const double cleanSub = std::sin (phase * 2.0 * 3.14159265358979) * amp;

        // ---- metal: inharmonic square partials (clank) before the distortion
        if (params.metal > 0.001f && ms < 400.0)
        {
            static const double ratios[5] = { 1.0, 1.47, 2.09, 2.56, 3.31 };
            double m = 0.0;
            for (int i = 0; i < 5; ++i)
            {
                metalPh[i] += params.metalTone * ratios[i] / sampleRate;
                metalPh[i] -= std::floor (metalPh[i]);
                m += (metalPh[i] < 0.5 ? 1.0 : -1.0);
            }
            const double mEnv = std::exp (-ms / (25.0 + 60.0 * params.metal));
            body += m * 0.12 * mEnv * params.metal;
        }

        // ---- distortion on the body
        const double g = std::pow (10.0, params.drive / 20.0);
        double d = body * g;
        switch (params.distType)
        {
            case 0: d = std::tanh (d); break;
            case 1: d = std::clamp (d, -1.0, 1.0); break;
            case 2: d = std::sin (d * 1.5707963); break;
            default:
            {
                d = std::clamp (d, -1.0, 1.0);
                const double steps = std::max (2.0, 48.0 - params.drive);
                if (crushCount <= 0) { crushHold = (float) (std::round (d * steps) / steps); crushCount = 2 + (int) (params.drive / 6.0f); }
                --crushCount;
                d = crushHold;
                break;
            }
        }
        // keep loudness roughly even as drive rises
        const double comp = params.distType == 0 ? 1.0 : 0.9;
        double mix = params.distMix;
        if (params.punch > 0.0f) mix *= std::clamp ((ms - params.punch * 0.5) / (params.punch * 0.5 + 0.001), 0.0, 1.0);
        double shaped = body + (d * comp - body) * mix;

        // ---- tone: resonant low-pass, resonance tamed on the clean punch
        double out = toneFilter.lowpass (shaped, params.tone, sampleRate, std::max (0.5f, params.reso));
        if (params.reso > 1.0f) out /= std::sqrt (params.reso);
        if (params.sub > 0.001f) out = out * (1.0 - 0.35 * params.sub) + cleanSub * params.sub * 0.75;

        // ---- click / transient: filtered noise burst
        if (params.clickLevel > 0.0f && ms < params.clickDecay * 8.0)
        {
            seed = seed * 1664525u + 1013904223u;
            const float noise = (float) ((seed >> 9) * (1.0 / 4194304.0) - 1.0);
            // TPT state-variable band-pass
            const double g = std::tan (3.14159265358979 * std::min ((double) params.clickTone, sampleRate * 0.45) / sampleRate);
            const double k = 1.4;
            const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
            const double v3 = noise - bp2;
            const double v1 = a1 * bp1 + a2 * v3;
            const double v2 = bp2 + a2 * bp1 + a3 * v3;
            bp1 = (float) (2 * v1 - bp1);
            bp2 = (float) (2 * v2 - bp2);
            const double band = v1;
            const double cEnv = std::exp (-ms / std::max (0.3f, params.clickDecay));
            // 909-style tick: short sine burst at the click tone plus a bit of noise
            const double tick = std::sin (6.283185307 * params.clickTone * 0.5 * sec) * std::exp (-ms / std::max (0.2f, params.clickDecay * 0.4f));
            out += (band * 0.6 * cEnv + tick * 0.5) * params.clickLevel;
        }

        out *= std::pow (10.0, params.volume / 20.0) * vel;

        // ---- fade for retrigger
        if (fadeStep > 0.0f)
        {
            fade -= fadeStep;
            if (fade <= 0.0f) { active = false; return 0.0f; }
            out *= fade;
        }

        t += 1.0 / sampleRate;
        if (ms > lenMs + 20.0) active = false;
        return (float) out;
    }

private:
    KickParams params;
    double sampleRate = 44100.0;
    double t = 0.0, phase = 0.0;
    double startHz = 600.0, endHz = 50.0;
    float vel = 1.0f;
    bool active = false;
    float fade = 1.0f, fadeStep = 0.0f;
    float bp1 = 0.0f, bp2 = 0.0f, lp = 0.0f;
    float crushHold = 0.0f;
    int crushCount = 0;
    uint32_t seed = 22222u;
    SVF toneFilter;
    double metalPh[5] {};
};


// One builder layer, synthesised per category.
class LayerVoice
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }
    void trigger (const LayerParams& p, int slotIndex, double baseHz, double beatSamples, float velocity)
    {
        lp = p; role = layerRole (slotIndex); vel = velocity;
        base = baseHz * std::pow (2.0, p.tune / 12.0);
        beat = beatSamples;
        t = 0; ph = ph2 = ph3 = 0; held = 0; holdCount = 0;
        f1.reset(); f2.reset(); f3.reset(); f4.reset(); nlp = 0;
        fade = 1.0f; fadeStep = 0.0f;
        active = p.level > 0.0001f;
    }
    void release() { if (active) fadeStep = 1.0f / (float) (0.004 * sampleRate); }

    float next()
    {
        if (! active) return 0.0f;
        const double ms = t / sampleRate * 1000.0 - lp.start;
        t += 1.0;
        if (ms < 0.0) return 0.0f;
        const double sec = ms * 0.001;
        const double dec = std::max (2.0f, lp.decay);
        const double g = std::pow (10.0, lp.drive / 20.0);
        double out = 0.0;
        bool done = false;

        auto noise = [this] { seed = seed * 1664525u + 1013904223u; return (double) ((seed >> 9) * (1.0 / 4194304.0) - 1.0); };
        auto osc = [this] (double& phase, double f) { phase += f / sampleRate; phase -= std::floor (phase); return std::sin (phase * 6.283185307179586); };
        auto saw = [this] (double& phase, double f)
        {
            const double dt = f / sampleRate;
            phase += dt; phase -= std::floor (phase);
            double o = 2.0 * phase - 1.0;
            if (phase < dt) { const double x = phase / dt; o -= x + x - x * x - 1.0; }
            else if (phase > 1.0 - dt) { const double x = (phase - 1.0) / dt; o -= x * x + x + x + 1.0; }
            return o;
        };
        auto decayEnv = [&] (double hold, double shape)
        {
            if (ms < 0.4) return ms / 0.4;
            if (ms < hold) return 1.0;
            const double x = (ms - hold) / std::max (1.0, dec - hold);
            if (x >= 1.0) { done = true; return 0.0; }
            return std::pow (1.0 - x, shape);
        };
        auto fadeIn = [&] (double len) { return std::min (1.0, ms / len); };
        auto crush = [this] (double x, int every, double steps)
        {
            if (holdCount <= 0) { held = std::round (x * steps) / steps; holdCount = every; }
            --holdCount; return held;
        };
        double bp, hp;

        if (role == 0) // ---------------- PUNCH
        {
            switch (lp.type)
            {
                case 0: // HARDSTYLISH: screaming mid punch, distorted and band-limited
                {
                    const double f = base * 3.0 * (1.0 + 5.0 * std::exp (-ms / 12.0));
                    double x = std::tanh (osc (ph, f) * g * 1.5);
                    x = std::tanh (x * 2.5 + 0.5 * osc (ph2, f * 2.01) + 0.25 * (osc (ph3, f * 3.02) > 0 ? 1.0 : -1.0));
                    f1.multi (x, 520.0, sampleRate, 0.7, bp, hp); x = hp;
                    x = f2.lowpass (x, lp.tone, sampleRate, 1.2);
                    out = x * decayEnv (6.0, 2.0) * 0.45;
                    break;
                }
                case 1: // INDUSTRIAL: hard-clipped square body with clank
                {
                    const double f = base * 2.0 * (1.0 + 3.0 * std::exp (-ms / 8.0));
                    double x = std::tanh (4.0 * osc (ph, f)) + 0.35 * (osc (ph2, f * 2.73) > 0 ? 1.0 : -1.0) * std::exp (-ms / 30.0);
                    x = std::clamp (x * g * 0.5, -1.0, 1.0);
                    x = f1.lowpass (x, lp.tone, sampleRate, 1.6);
                    f2.multi (x, 30.0, sampleRate, 0.7, bp, hp);
                    out = hp * decayEnv (12.0, 1.4) * 0.4;
                    break;
                }
                case 2: // TOP KICK: short higher kick layered on top
                {
                    const double f = base * 2.3 * (1.0 + 6.0 * std::exp (-ms / 6.0));
                    double x = std::tanh (osc (ph, f) * (1.0 + g * 0.3));
                    x += noise() * std::exp (-ms / 1.5) * 0.5;
                    x = f1.lowpass (x, lp.tone, sampleRate, 0.8);
                    f2.multi (x, 80.0, sampleRate, 0.7, bp, hp);
                    out = hp * decayEnv (2.0, 2.5) * 0.7;
                    break;
                }
                case 3: // TRANSIENT: very fast pitch drop burst
                {
                    const double f = base * 2.0 * (1.0 + 2.5 * std::exp (-ms / 5.0));
                    double x = std::tanh (osc (ph, f) * (1.0 + g * 0.2)) + noise() * std::exp (-ms / 4.0) * 0.6;
                    x = f1.lowpass (x, lp.tone, sampleRate, 0.9);
                    f2.multi (x, 50.0, sampleRate, 0.7, bp, hp);
                    out = hp * decayEnv (1.0, 3.0) * 0.9;
                    break;
                }
                default: // TOKYO SNAP: folded, nasal punch
                {
                    const double f = base * 4.0 * (1.0 + 2.0 * std::exp (-ms / 10.0));
                    double x = std::sin (osc (ph, f) * (1.0 + g * 0.25) * 1.5707963);
                    x = f1.multi (x, lp.tone, sampleRate, 3.0, bp, hp) * 0.4 + bp * 0.8;
                    out = std::tanh (x * 1.5) * decayEnv (4.0, 2.0) * 0.7;
                    break;
                }
            }
        }
        else if (role == 1) // ---------------- TAIL
        {
            const double f = base;
            switch (lp.type)
            {
                case 0: // BIGROOM: round long sine bass
                {
                    const double fr = f * (1.0 + 0.5 * std::exp (-ms / 25.0));
                    double x = std::tanh (osc (ph, fr) * (1.0 + g * 0.25)) + 0.12 * saw (ph2, fr);
                    x = f1.lowpass (x, lp.tone, sampleRate, 0.7);
                    out = x * decayEnv (dec * 0.3, 1.5) * fadeIn (4.0) * 0.8;
                    break;
                }
                case 1: // IND CRUNCH: clipped, crushed, noisy low end
                {
                    double x = osc (ph, f) + 0.5 * (osc (ph2, f) > 0 ? 1.0 : -1.0);
                    nlp += (noise() - nlp) * 0.004;
                    x = std::clamp (x * g * (1.0 + nlp * 6.0), -1.0, 1.0);
                    x = crush (x, 3, 10.0);
                    x = f1.lowpass (x, lp.tone, sampleRate, 1.1);
                    f2.multi (x, 55.0, sampleRate, 0.7, bp, hp);
                    out = hp * decayEnv (dec * 0.2, 1.0) * fadeIn (3.0) * 0.35;
                    break;
                }
                case 2: // PSY ROLL: three 1/16 plucks after the kick
                {
                    const double pos = (t - lp.start * 0.001 * sampleRate) / std::max (1.0, beat);
                    const int step = (int) std::floor (pos * 4.0);
                    if (step >= 4) { done = true; break; }
                    if (step >= 1)
                    {
                        const double nms = (pos * 4.0 - step) * beat / 4.0 / sampleRate * 1000.0;
                        const double e = nms < 1.0 ? nms : std::max (0.0, 1.0 - (nms - 1.0) / dec);
                        double x = saw (ph, f * (1.0 + 0.04 * std::exp (-nms / 6.0)));
                        x = f1.lowpass (x, lp.tone * (1.0 + 5.0 * std::exp (-nms / 15.0)), sampleRate, 1.4);
                        out = std::tanh (x * (1.0 + g * 0.2)) * e * e * 1.1;
                    }
                    break;
                }
                case 3: // PSY SHOT: one filtered pluck
                {
                    double x = saw (ph, f) * 0.7 + 0.3 * saw (ph2, f * 1.005);
                    x = f1.lowpass (x, lp.tone * (1.0 + 6.0 * std::exp (-ms / 20.0)), sampleRate, 1.3);
                    out = std::tanh (x * (1.0 + g * 0.2)) * decayEnv (2.0, 2.0) * fadeIn (1.0) * 1.1;
                    break;
                }
                case 4: // RAW CRUNCH: screeching distorted tail with a vocal formant
                {
                    const double len = std::max (1.0, (double) dec);
                    const double fr = f * std::pow (2.0, -2.0 * std::min (1.0, ms / len) / 12.0);
                    double x = std::tanh (osc (ph, fr) * g * 2.0);
                    x = std::sin (x * 2.2);
                    const double low = f1.multi (x, lp.tone, sampleRate, 3.0, bp, hp);
                    x = low * 0.5 + bp * 0.9;
                    f2.multi (x, 80.0, sampleRate, 0.7, bp, hp);
                    out = std::tanh (hp * 1.5) * decayEnv (dec * 0.5, 0.8) * fadeIn (5.0) * 0.9;
                    break;
                }
                case 5: // REVERSE: swells up into the next kick
                {
                    const double total = std::max (1.0, beat * 0.95 - lp.start * 0.001 * sampleRate);
                    const double u = (t - lp.start * 0.001 * sampleRate) / total;
                    if (u >= 1.0) { done = true; break; }
                    double x = std::tanh ((1.0 + g * 0.2) * osc (ph, f));
                    x = f1.lowpass (x, lp.tone * (0.3 + u), sampleRate, 0.9);
                    out = x * std::pow (u, 2.2) * std::min (1.0, (1.0 - u) * 40.0) * 0.9;
                    break;
                }
                case 6: // ROBOT: FM + ring mod, bit crushed
                {
                    const double idx = 3.0 * std::exp (-ms / 80.0) + 0.6;
                    const double m = osc (ph2, f * 2.01);
                    ph += (f * (1.0 + idx * m * 0.5)) / sampleRate; ph -= std::floor (ph);
                    double x = std::sin (ph * 6.283185307179586) * (0.7 + 0.3 * (osc (ph3, f * 7.0) > 0 ? 1.0 : -1.0));
                    x = crush (std::tanh (x * g * 0.6), 4, 8.0);
                    x = f1.lowpass (x, lp.tone, sampleRate, 1.0);
                    out = x * decayEnv (dec * 0.25, 1.2) * fadeIn (3.0) * 0.6;
                    break;
                }
                default: // RUMBLE: dark, wobbling, saturated sub
                {
                    nlp += (noise() - nlp) * 0.002;
                    double x = osc (ph, f * (1.0 + nlp * 0.3)) * (1.0 + nlp * 2.0);
                    x = std::tanh (x * (1.0 + g * 0.5));
                    x = f1.lowpass (f2.lowpass (x, lp.tone, sampleRate, 0.7), lp.tone, sampleRate, 0.7);
                    out = x * decayEnv (dec * 0.2, 1.0) * fadeIn (12.0) * 0.55;
                    break;
                }
            }
        }
        else // ---------------- EXTRA / ENHANCERS
        {
            switch (lp.type)
            {
                case 0: // CLICK
                {
                    double x = noise() * std::exp (-ms / std::max (0.3, dec * 0.15));
                    f1.multi (x, lp.tone, sampleRate, 1.5, bp, hp);
                    const double tick = std::sin (6.283185307 * lp.tone * 0.5 * sec) * std::exp (-ms / std::max (0.2, dec * 0.08));
                    out = std::tanh ((bp * 1.2 + tick * 0.7) * (1.0 + g * 0.1)) * (ms < dec ? 1.0 : (done = true, 0.0));
                    break;
                }
                case 1: // LOW MID: thick 100-200 Hz bump
                {
                    const double f = 175.0 * std::pow (2.0, lp.tune / 12.0) * (1.0 + 0.6 * std::exp (-ms / 10.0));
                    double x = std::tanh (osc (ph, f) * (1.0 + g * 0.3));
                    x = f1.lowpass (x, lp.tone, sampleRate, 0.8);
                    out = x * decayEnv (dec * 0.15, 1.6) * 0.6;
                    break;
                }
                case 2: // NOISE: body-band noise smear
                {
                    double x = f1.multi (noise(), lp.tone, sampleRate, 1.2, bp, hp);
                    x = std::tanh (bp * (1.5 + g * 0.3));
                    out = x * decayEnv (5.0, 1.8) * 1.4;
                    break;
                }
                case 3: // NOISE 2: crunchy mid noise
                {
                    double x = noise();
                    x = crush (std::tanh (x * (1.0 + g * 0.4)), 2, 6.0);
                    f1.multi (x, lp.tone, sampleRate, 0.9, bp, hp);
                    out = bp * decayEnv (3.0, 2.2) * 1.2;
                    break;
                }
                case 4: // RESONATOR: ringing metallic body
                {
                    const double exc = noise() * (ms < 2.0 ? 1.0 : 0.0) + (ms < 0.1 ? 4.0 : 0.0);
                    f1.multi (exc, lp.tone, sampleRate, 40.0, bp, hp); double a = bp;
                    f2.multi (exc, lp.tone * 1.51, sampleRate, 30.0, bp, hp); double b = bp;
                    f3.multi (exc, lp.tone * 2.37, sampleRate, 25.0, bp, hp);
                    double x = (a + b * 0.35 + bp * 0.15) * 2.5;
                    out = std::tanh (x * (1.0 + g * 0.2)) * decayEnv (0.5, 1.5) * 0.55;
                    break;
                }
                case 5: // REVERB: short room smear after the hit
                {
                    double x = noise() * (0.6 + 0.4 * std::sin (ms * 0.37));
                    x = f1.lowpass (x, lp.tone, sampleRate, 0.7);
                    f2.multi (x, 180.0, sampleRate, 0.7, bp, hp);
                    out = hp * decayEnv (2.0, 2.5) * fadeIn (6.0) * 2.4 * (1.0 + g * 0.02);
                    break;
                }
                default: // SCHRANZ TOP: driven 1/16 metallic stabs over the beat
                {
                    const double pos = (t - lp.start * 0.001 * sampleRate) / std::max (1.0, beat);
                    if (pos >= 1.0) { done = true; break; }
                    const double nms = (pos * 4.0 - std::floor (pos * 4.0)) * beat / 4.0 / sampleRate * 1000.0;
                    const int step = (int) std::floor (pos * 4.0);
                    const double acc = step == 2 ? 1.0 : 0.7;
                    double x = (osc (ph, lp.tone * 0.25) > 0 ? 1.0 : -1.0) * 0.5 + (osc (ph2, lp.tone * 0.37) > 0 ? 0.5 : -0.5) + noise() * 0.6;
                    x = std::tanh (x * (1.0 + g * 0.5));
                    f1.multi (x, lp.tone, sampleRate, 1.2, bp, hp);
                    f2.multi (bp + hp * 0.3, 900.0, sampleRate, 0.7, bp, hp);
                    const double e = std::exp (-nms / std::max (5.0, dec * 0.3)) * std::min (1.0, nms / 0.5);
                    out = hp * e * acc * 1.0;
                    break;
                }
            }
        }

        out *= lp.level * vel;
        if (fadeStep > 0.0f)
        {
            fade -= fadeStep;
            if (fade <= 0.0f) { active = false; return 0.0f; }
            out *= fade;
        }
        if (done || ms > 4000.0) active = false;
        return (float) out;
    }

private:
    LayerParams lp;
    int role = 0;
    double sampleRate = 44100.0, t = 0, ph = 0, ph2 = 0, ph3 = 0, base = 50, beat = 20000, held = 0, nlp = 0;
    int holdCount = 0;
    float vel = 1.0f, fade = 1.0f, fadeStep = 0.0f;
    bool active = false;
    uint32_t seed = 4242u;
    SVF f1, f2, f3, f4;
};

// Linear interpolation through 5 (time, gain) nodes; holds the last gain afterwards.
inline double envNodes (const float* T, const float* G, double x)
{
    if (x <= T[0]) return G[0];
    for (int i = 0; i < 4; ++i)
    {
        const double t0 = T[i], t1 = std::max (T[i + 1], T[i] + 1.0e-4f);
        if (x < t1) return G[i] + (G[i + 1] - G[i]) * (x - t0) / (t1 - t0);
    }
    return G[4];
}

// Bass notes that play between kicks.
class BassVoice
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }
    void noteOn (double hz, bool reverseMode, double lenSamples, const KickParams& p)
    {
        freq = hz; reverse = reverseMode; len = std::max (1.0, lenSamples);
        params = p;
        t = 0; active = true;
    }
    void stop() { active = false; filter.reset(); }

    float next()
    {
        if (! active) return 0.0f;
        const double ms = t / sampleRate * 1000.0;
        const double decay = std::max (5.0f, params.bassDecay);
        double amp, fenv;
        if (! reverse)
        {
            const double x = ms / decay;
            if (x >= 1.0) { active = false; return 0.0f; }
            amp = std::min (1.0, ms / std::max (0.3f, params.bassAttack)) * std::max (0.0, envNodes (params.bEnvT, params.bEnvG, x));
            fenv = std::exp (-ms / std::max (2.0, decay * 0.3));
        }
        else
        {
            const double u = t / len;
            if (u >= 1.0) { active = false; return 0.0f; }
            amp = std::pow (u, 2.2) * std::min (1.0, (len - t) / (0.003 * sampleRate));
            fenv = u * 0.6;
        }
        // glide: starts above and drops to the note
        const double f = freq * std::pow (2.0, params.bassGlide * std::exp (-ms / 18.0) / 12.0);
        const double dt = f / sampleRate;
        phase += dt; phase -= std::floor (phase);
        phase2 += dt * 1.007; phase2 -= std::floor (phase2);

        auto saw = [dt] (double ph)
        {
            double o = 2.0 * ph - 1.0;
            if (ph < dt) { const double x = ph / dt; o -= x + x - x * x - 1.0; }
            else if (ph > 1.0 - dt) { const double x = (ph - 1.0) / dt; o -= x * x + x + x + 1.0; }
            return o;
        };
        double osc;
        if (reverse) osc = std::tanh (3.0 * std::sin (phase * 6.283185307));
        else switch (params.bassOsc)
        {
            case 1:  { double ph2 = phase + 0.5; ph2 -= std::floor (ph2); osc = 0.5 * (saw (phase) - saw (ph2)); break; } // square
            case 2:  osc = std::tanh (1.5 * std::sin (phase * 6.283185307)); break;                                        // sine
            case 3:  osc = 0.5 * (saw (phase) + saw (phase2)); break;                                                     // reese
            default: osc = saw (phase); break;
        }
        const double y = filter.lowpass (osc, params.bassCutoff * (1.0 + params.bassEnvAmt * fenv), sampleRate, std::max (0.5f, params.bassReso));
        const double g = std::pow (10.0, params.bassDrive / 20.0);
        t += 1.0;
        return (float) (std::tanh (y * g) / std::sqrt (g) * 1.6 * amp * params.bassLevel * 1.5);
    }

private:
    KickParams params;
    double sampleRate = 44100.0, freq = 50.0, phase = 0, phase2 = 0, t = 0, len = 1;
    bool reverse = false, active = false;
    SVF filter;
};

// Rumble: kick -> pre-delay -> reverb -> distortion -> filters -> sidechain/roll/shape.
class Rumble
{
public:
    void setSampleRate (double sr)
    {
        sampleRate = sr;
        const int combLen[4] = { 1116, 1188, 1277, 1356 };
        const int apLen[2] = { 556, 441 };
        for (int i = 0; i < 4; ++i) { combs[i].assign ((size_t) (combLen[i] * sr / 44100.0), 0.0f); combIdx[i] = 0; store[i] = 0; }
        for (int i = 0; i < 2; ++i) { aps[i].assign ((size_t) (apLen[i] * sr / 44100.0), 0.0f); apIdx[i] = 0; }
        pre.assign ((size_t) (0.11 * sr) + 1, 0.0f); preIdx = 0;
        lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); env = 0;
    }

    float process (float in, const KickParams& p, double beatFrac, double rollPhase)
    {
        if (p.rumbleAmt <= 0.0001f) { env = 0; return 0.0f; }
        const float fb = 0.70f + 0.28f * p.rumbleSize, damp = std::clamp (p.rumbleDamp, 0.0f, 0.95f);

        // pre-delay
        pre[(size_t) preIdx] = in;
        const int d = std::clamp ((int) (p.rumblePre * 0.001 * sampleRate), 0, (int) pre.size() - 1);
        int readIdx = preIdx - d; if (readIdx < 0) readIdx += (int) pre.size();
        const float delayed = pre[(size_t) readIdx];
        if (++preIdx >= (int) pre.size()) preIdx = 0;

        // grit: noise riding the signal
        seed = seed * 1664525u + 1013904223u;
        const float noise = (float) ((seed >> 9) * (1.0 / 4194304.0) - 1.0);
        const float x = delayed * 0.2f + noise * std::abs (delayed) * 0.12f * p.rumbleGrit;
        float sum = 0;
        for (int i = 0; i < 4; ++i)
        {
            auto& b = combs[i];
            const float o = b[(size_t) combIdx[i]];
            store[i] = o * (1.0f - damp) + store[i] * damp;
            b[(size_t) combIdx[i]] = x + store[i] * fb;
            if (++combIdx[i] >= (int) b.size()) combIdx[i] = 0;
            sum += o;
        }
        for (int i = 0; i < 2; ++i)
        {
            auto& b = aps[i];
            const float o = b[(size_t) apIdx[i]];
            b[(size_t) apIdx[i]] = sum + o * 0.5f;
            if (++apIdx[i] >= (int) b.size()) apIdx[i] = 0;
            sum = o - sum;
        }
        const double g = std::pow (10.0, p.rumbleDrive / 20.0);
        double y = std::tanh (sum * g * 2.0);
        y = lp1.lowpass (y, p.rumbleTone, sampleRate, 0.7);
        y = lp2.lowpass (y, p.rumbleTone, sampleRate, 0.7);
        y -= hp1.lowpass (y, p.rumbleLow, sampleRate, 0.7);
        y -= hp2.lowpass (y, p.rumbleLow, sampleRate, 0.7);

        // sidechain follower from the kick
        const float a = std::abs (in);
        env = a > env ? a : env * (float) std::exp (-1.0 / (0.08 * sampleRate));
        const double duck = 1.0 - p.rumbleDuck * std::min (1.0f, env * 1.5f);
        // roll: pumping notes
        const double ph = rollPhase - std::floor (rollPhase);
        const double roll = 1.0 - p.rumbleRoll * (1.0 - std::pow (1.0 - ph, 1.5));
        const double shape = std::max (0.0, envNodes (p.rEnvT, p.rEnvG, std::min (1.0, beatFrac)));
        return (float) (y * duck * roll * shape * p.rumbleAmt * 1.4);
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> combs[4], aps[2], pre;
    int combIdx[4] {}, apIdx[2] {}, preIdx = 0;
    float store[4] {};
    SVF lp1, lp2, hp1, hp2;
    float env = 0;
    uint32_t seed = 777u;
};

// Kick + bass pattern + rumble, used by the plugin and the display.
class GrooveEngine
{
public:
    void setSampleRate (double sr)
    {
        sampleRate = sr;
        for (auto& v : voices) v.setSampleRate (sr);
        bass.setSampleRate (sr);
        rumble.setSampleRate (sr);
        for (auto& b : layerVoices) for (auto& l : b) l.setSampleRate (sr);
    }
    void setParams (const KickParams& p) { params = p; }

    void trigger (const KickParams& p, float vel, float baseHz, double bpm)
    {
        params = p;
        voices[cur].release();
        cur ^= 1;
        voices[cur].trigger (p, vel, baseHz);
        bass.stop();
        beat = 60.0 / std::clamp (bpm, 60.0, 300.0) * sampleRate;
        sinceKick = 0;
        eventIdx = 0;
        bassHz = voices[cur].getBaseHz() * std::pow (2.0, p.bassSemi / 12.0);
        bassVel = vel;
        for (auto& l : layerVoices[curL]) l.release();
        curL ^= 1;
        for (int i = 0; i < numLayers; ++i)
            layerVoices[curL][i].trigger (p.layers[i], i, voices[cur].getBaseHz(), beat, vel * (float) std::pow (10.0, p.volume / 20.0));
    }

    void next (float& kick, float& bassOut, float& rumbleOut)
    {
        kick = voices[0].next() + voices[1].next();
        for (auto& b : layerVoices) for (auto& l : b) kick += l.next();
        if (params.bassLevel > 0.0001f)
        {
            static const double ev[4][3] = { { 0.25, 0.5, 0.75 }, { 0.5, -1, -1 }, { 0.5, 0.75, -1 }, { 0.12, -1, -1 } };
            const int pat = std::clamp (params.bassPattern, 0, 3);
            if (eventIdx < 3 && ev[pat][eventIdx] >= 0 && sinceKick >= ev[pat][eventIdx] * beat)
            {
                KickParams q = params;
                q.bassLevel *= bassVel;
                bass.noteOn (bassHz, pat == 3, (0.97 - 0.12) * beat, q);
                ++eventIdx;
            }
        }
        bassOut = bass.next() * (float) std::pow (10.0, params.volume / 20.0);
        static const double rateDiv[3] = { 2.0, 4.0, 8.0 };
        rumbleOut = rumble.process (kick, params, sinceKick / beat, sinceKick / (beat / rateDiv[std::clamp (params.rumbleRate, 0, 2)]));
        sinceKick += 1.0;
    }

    float next()
    {
        float k, b, r;
        next (k, b, r);
        return k + b + r;
    }

private:
    double sampleRate = 44100.0, beat = 20000, sinceKick = 1e9, bassHz = 50;
    float bassVel = 1.0f;
    int cur = 0, eventIdx = 3, curL = 0;
    KickVoice voices[2];
    LayerVoice layerVoices[2][numLayers];
    BassVoice bass;
    Rumble rumble;
    KickParams params;
};
