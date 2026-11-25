#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>

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

    float clickLevel = 0.4f;    // 0..1
    float clickDecay = 6.0f;    // ms
    float clickTone  = 4000.0f; // Hz

    float drive      = 6.0f;    // dB
    int   distType   = 0;       // 0 Soft, 1 Hard, 2 Fold, 3 Crush
    float distMix    = 1.0f;    // 0..1
    float tone       = 12000.0f;// Hz low-pass after drive

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
        crushHold = 0.0f;
        crushCount = 0;
    }

    void release() // quick fade used when a new note cuts this one
    {
        if (active) fadeStep = 1.0f / (float) (0.004 * sampleRate);
    }

    bool isActive() const { return active; }

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

        double body = osc * amp;

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
        double shaped = body + (d * comp - body) * params.distMix;

        // ---- tone (one-pole low-pass)
        const double a = 1.0 - std::exp (-2.0 * 3.14159265358979 * params.tone / sampleRate);
        lp += (float) (a * (shaped - lp));
        double out = lp;

        // ---- click / transient: filtered noise burst
        if (params.clickLevel > 0.0f && ms < params.clickDecay * 8.0)
        {
            seed = seed * 1664525u + 1013904223u;
            const float noise = (float) ((seed >> 9) * (1.0 / 4194304.0) - 1.0);
            // state-variable band-pass
            const float f = (float) (2.0 * std::sin (3.14159265358979 * std::min (params.clickTone, (float) sampleRate * 0.2f) / sampleRate));
            const float q = 0.6f;
            bp2 += f * bp1;
            const float hp = noise - bp2 - q * bp1;
            bp1 += f * hp;
            const double cEnv = std::exp (-ms / std::max (0.3f, params.clickDecay));
            out += bp1 * 1.6 * cEnv * params.clickLevel;
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
};
