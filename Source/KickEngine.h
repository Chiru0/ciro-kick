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
};

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
    }

    void next (float& kick, float& bassOut, float& rumbleOut)
    {
        kick = voices[0].next() + voices[1].next();
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
    int cur = 0, eventIdx = 3;
    KickVoice voices[2];
    BassVoice bass;
    Rumble rumble;
    KickParams params;
};
