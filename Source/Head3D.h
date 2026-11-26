#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PaperStyle.h"

// Low-poly cartoon head of Ciro, flat shaded, with a giant open mouth that hosts the UI.
class Head3D : public juce::Component
{
public:
    juce::Colour accent { 0xffff3b5c };
    std::function<void()> onClick;
    float drive01 = 0.0f;    // 0..1, makes the face angrier
    float rumble01 = 0.0f;   // 0..1, makes the head tremble

    Head3D() { buildMesh(); }

    void setMouth (juce::Rectangle<float> r) { mouth = r; repaint(); }
    // how far the lower jaw is currently dropped below its open position (px, negative = closing)
    float jawOffset() const { return (open.value - 1.0f) * mouth.getHeight() * 0.8f + bounce * 6.0f; }
    float overlayOffsetX() const { return std::sin (yaw) * radius * 0.92f; }
    float overlayAlpha() const { return juce::jlimit (0.0f, 1.0f, open.value * 1.4f - 0.4f) * std::max (0.0f, std::cos (spin)); }

    void hit()    { bounce = 1.0f; browJump = 1.0f; }
    void chomp()  { open.value = 0.08f; open.velocity = 0.0f; }
    void dizzy()  { dizzyT = 1.0f; shakeVel += 0.09f; }
    void shake()  { shakeVel += 0.14f; }
    void lookAt (juce::Point<float> p) { lookTarget = p; }

    bool animate()
    {
        const float prevYaw = yaw;
        // jaw spring
        open.target = 1.0f;
        const bool jawMoving = stepSpring (open, 0.16f, 0.72f);
        bounce *= 0.82f; browJump *= 0.88f; dizzyT = std::max (0.0f, dizzyT - 0.012f);
        // shake (damped oscillator on yaw)
        shakeVel += -shakeYaw * 0.18f; shakeVel *= 0.86f; shakeYaw += shakeVel;
        yaw = shakeYaw + (rumble01 > 0.01f ? std::sin ((float) frame * 0.9f) * 0.004f * rumble01 : 0.0f);
        if (spin > 0.0f) { spin = std::max (0.0f, spin - 0.22f); }
        // eyes
        const auto c = centre();
        const auto d = lookTarget - c;
        const juce::Point<float> want (juce::jlimit (-1.0f, 1.0f, d.x / 300.0f), juce::jlimit (-1.0f, 1.0f, d.y / 220.0f));
        look += (want - look) * 0.2f;
        if (--blinkCountdown <= 0) { blink = 1.0f; blinkCountdown = 120 + juce::Random::getSystemRandom().nextInt (240); }
        blink = std::max (0.0f, blink - 0.12f);
        ++frame;
        const bool busy = jawMoving || bounce > 0.01f || browJump > 0.01f || dizzyT > 0.0f || std::abs (shakeYaw) > 0.0005f
                          || std::abs (shakeVel) > 0.0005f || spin > 0.0f || blink > 0.0f || (want - look).getDistanceFromOrigin() > 0.004f
                          || rumble01 > 0.01f || yaw != prevYaw;
        if (busy) repaint();
        return busy;
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        spin = juce::MathConstants<float>::twoPi;
        bounce = 1.0f;
        if (onClick) onClick();
    }

    bool hitTest (int x, int y) override
    {
        // only the head itself is clickable (not the mouth, which is covered by the UI anyway)
        const auto c = centre();
        const float dx = ((float) x - c.x) / radius, dy = ((float) y - c.y) / (radius * 1.05f);
        return dx * dx + dy * dy < 1.0f && ! mouth.expanded (6.0f).contains ((float) x, (float) y);
    }

    void paint (juce::Graphics& g) override
    {
        const float sc = 1.0f + 0.025f * bounce;
        const auto c = centre();
        const float yawAll = yaw + spin, pitch = -0.04f + 0.03f * bounce;
        const float cy = std::cos (yawAll), sy = std::sin (yawAll), cp = std::cos (pitch), sp = std::sin (pitch);
        const float jawModel = std::max (0.0f, (mouth.getHeight() + jawOffset()) / radius);

        struct V3 { float x, y, z; };
        auto xform = [&] (V3 v) -> V3
        {
            // yaw around Y, then pitch around X
            V3 a { v.x * cy + v.z * sy, v.y, -v.x * sy + v.z * cy };
            return { a.x, a.y * cp - a.z * sp, a.y * sp + a.z * cp };
        };
        auto project = [&] (V3 v) -> juce::Point<float>
        {
            const float f = 6.0f, s = radius * sc * f / (f - v.z);
            return { c.x + v.x * s, c.y - v.y * s };
        };
        auto surf = [&] (float th, float ph, float push, bool jaw) -> V3
        {
            const P3 p = surface (th, ph, jaw ? jawModel : 0.0f);
            return { p.x * push, p.y * push, p.z * push };
        };

        struct Poly { juce::Path path; float depth; juce::Colour col; };
        std::vector<Poly> polys;
        polys.reserve (tris.size() + 64);
        const V3 light { -0.45f, 0.55f, 0.75f };
        auto addPoly = [&] (std::initializer_list<V3> pts, juce::Colour base, float bias, bool cull = true)
        {
            std::vector<V3> v;
            V3 cen { 0, 0, 0 };
            for (auto p : pts) { auto q = xform (p); v.push_back (q); cen.x += q.x; cen.y += q.y; cen.z += q.z; }
            const float n = (float) v.size();
            cen = { cen.x / n, cen.y / n, cen.z / n };
            V3 a { v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z }, b { v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z };
            V3 nr { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
            const float len = std::sqrt (nr.x * nr.x + nr.y * nr.y + nr.z * nr.z) + 1e-6f;
            nr = { nr.x / len, nr.y / len, nr.z / len };
            if (nr.x * cen.x + nr.y * cen.y + nr.z * cen.z < 0) nr = { -nr.x, -nr.y, -nr.z };
            if (cull && nr.z < -0.05f) return;
            const float diff = std::max (0.0f, nr.x * light.x + nr.y * light.y + nr.z * light.z);
            const float shade = 0.5f + 0.62f * diff + 0.12f * std::max (0.0f, nr.z);
            juce::Path path;
            bool first = true;
            for (auto& q : v) { auto s2 = project (q); if (first) path.startNewSubPath (s2); else path.lineTo (s2); first = false; }
            path.closeSubPath();
            polys.push_back ({ std::move (path), cen.z + bias,
                               juce::Colour::fromFloatRGBA (std::min (1.0f, base.getFloatRed() * shade), std::min (1.0f, base.getFloatGreen() * shade),
                                                            std::min (1.0f, base.getFloatBlue() * shade), base.getFloatAlpha()) });
        };

        // ---- head shell
        for (auto& t : tris)
        {
            V3 p[3];
            for (int k = 0; k < 3; ++k)
            {
                const auto& vx = verts[(size_t) t.v[k]];
                p[k] = surf (vx.th, vx.ph, vx.jit, vx.jaw);
            }
            addPoly ({ p[0], p[1], p[2] }, t.colour, 0.0f);
        }

        const juce::Colour skin (0xffeab28f), skinDk (0xffcf9474), hair (0xff2a1d16), brow (0xff1d140f);

        // ---- ears
        for (float s : { -1.0f, 1.0f })
        {
            const float ph = s * 1.52f;
            V3 top = surf (1.47f, ph, 0.98f, false), bot = surf (1.80f, ph, 0.98f, false), mid = surf (1.62f, ph, 1.0f, false);
            V3 out { mid.x * 1.1f, mid.y + 0.01f, mid.z - 0.1f };
            addPoly ({ top, out, mid }, skinDk, 0.02f, false);
            addPoly ({ mid, out, bot }, skinDk.darker (0.1f), 0.02f, false);
        }

        // ---- nose (big and proud)
        {
            V3 bridge = surf (1.30f, 0.0f, 1.03f, false);
            V3 lu = surf (1.47f, -0.13f, 1.01f, false), ru = surf (1.47f, 0.13f, 1.01f, false);
            V3 ll = surf (1.66f, -0.17f, 1.0f, false), rl = surf (1.66f, 0.17f, 1.0f, false);
            V3 tipBase = surf (1.60f, 0.0f, 1.0f, false);
            V3 tip { tipBase.x, tipBase.y - 0.02f, tipBase.z + 0.24f };
            addPoly ({ bridge, lu, tip }, skin, 0.05f);
            addPoly ({ bridge, tip, ru }, skin, 0.05f);
            addPoly ({ lu, ll, tip }, skinDk, 0.05f);
            addPoly ({ ru, tip, rl }, skinDk, 0.05f);
            addPoly ({ ll, rl, tip }, skinDk.darker (0.35f), 0.04f);
        }

        // ---- eyes + brows
        const float angry = juce::jlimit (0.0f, 1.0f, 0.35f + drive01 * 0.65f);
        for (float s : { -1.0f, 1.0f })
        {
            const float ph = s * 0.37f, th = 1.36f;
            const float openEye = (dizzyT > 0.0f ? 0.085f : 0.066f - 0.014f * angry) * (1.0f - blink * 0.85f);
            const float w = 0.18f;
            addPoly ({ surf (th - openEye, ph - w, 1.012f, false), surf (th - openEye * 1.3f, ph + w * 0.2f, 1.012f, false),
                       surf (th - openEye, ph + w, 1.012f, false), surf (th + openEye, ph + w, 1.012f, false),
                       surf (th + openEye * 1.2f, ph, 1.012f, false), surf (th + openEye, ph - w, 1.012f, false) },
                     juce::Colour (0xfff6efe4), 0.06f);
            if (blink < 0.6f)
            {
                float lx = look.x * 0.07f, ly = look.y * 0.025f;
                if (dizzyT > 0.0f)
                {
                    const float a = (float) frame * 0.35f * s;
                    lx = std::cos (a) * 0.06f; ly = std::sin (a) * 0.025f;
                }
                const float r = 0.055f, rt = std::min (r * 0.75f, openEye * 0.98f);
                {
                    std::vector<V3> ring;
                    for (int k = 0; k < 8; ++k)
                    {
                        const float a = (float) k * juce::MathConstants<float>::twoPi / 8.0f;
                        ring.push_back (surf (th + ly + std::sin (a) * rt, ph + lx + std::cos (a) * r * 0.8f, 1.02f, false));
                    }
                    addPoly ({ ring[0], ring[1], ring[2], ring[3], ring[4], ring[5], ring[6], ring[7] }, juce::Colour (0xff3a2416), 0.07f);
                }
                addPoly ({ surf (th + ly - rt * 0.4f, ph + lx - 0.01f, 1.026f, false), surf (th + ly - rt * 0.4f, ph + lx + 0.012f, 1.026f, false),
                           surf (th + ly - rt * 0.05f, ph + lx + 0.012f, 1.026f, false) }, juce::Colours::white, 0.08f);
            }
            // thick furrowed brow, inner end pulled down when angry
            const float jump = browJump * 0.07f + (dizzyT > 0.0f ? 0.05f : 0.0f);
            const float inTh = th - 0.10f + angry * 0.05f - jump, outTh = th - 0.16f - jump * 0.6f;
            const float thick = 0.055f;
            addPoly ({ surf (inTh, s * 0.07f, 1.04f, false), surf (outTh - 0.02f, s * 0.33f, 1.04f, false), surf (outTh, s * 0.56f, 1.04f, false),
                       surf (outTh + thick * 0.7f, s * 0.55f, 1.04f, false), surf (outTh + thick, s * 0.33f, 1.04f, false), surf (inTh + thick, s * 0.08f, 1.04f, false) },
                     brow, 0.09f);
            // blush when the drive is high
            if (drive01 > 0.45f)
                addPoly ({ surf (1.58f, s * 0.52f, 1.012f, false), surf (1.58f, s * 0.74f, 1.012f, false),
                           surf (1.70f, s * 0.70f, 1.012f, false), surf (1.70f, s * 0.54f, 1.012f, false) },
                         juce::Colour (0xffe8707a).withAlpha (juce::jlimit (0.0f, 0.7f, (drive01 - 0.45f) * 2.0f)), 0.03f);
        }

        std::sort (polys.begin(), polys.end(), [] (const Poly& a, const Poly& b) { return a.depth < b.depth; });

        // soft drop shadow of the whole head on the cardboard
        {
            g.setColour (juce::Colours::black.withAlpha (0.22f));
            g.fillEllipse (c.x - radius * 0.95f + 10, c.y - radius * 0.9f + 18, radius * 1.9f, radius * 2.0f + jawModel * radius * 0.6f);
        }
        for (auto& p : polys)
        {
            g.setColour (p.col);
            g.fillPath (p.path);
            g.strokePath (p.path, juce::PathStrokeType (0.8f));
        }

        // sackboy-ish cross stitches on the temples
        if (std::cos (yawAll) > 0.3f)
            for (float s : { -1.0f, 1.0f })
            {
                const auto pt = project (xform (surf (1.15f, s * 1.05f, 1.01f, false)));
                g.setColour (paper::cream.withAlpha (0.85f));
                for (int i = 0; i < 3; ++i)
                {
                    const float yy = pt.y + (float) i * 9.0f;
                    g.drawLine (pt.x - 4, yy - 3, pt.x + 4, yy + 3, 1.8f);
                    g.drawLine (pt.x - 4, yy + 3, pt.x + 4, yy - 3, 1.8f);
                }
            }

        // ---- the mouth: dark throat + tongue (UI lives inside)
        const float ox = overlayOffsetX();
        if (std::cos (spin) > 0.0f)
        {
            auto path = mouthPath (ox);
            g.setColour (juce::Colour (0xff8d3a3f));
            g.strokePath (path, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            juce::ColourGradient throat (juce::Colour (0xff5a1620), mouth.getCentreX() + ox, mouth.getY(),
                                         juce::Colour (0xff12040a), mouth.getCentreX() + ox, mouth.getCentreY() + 10, true);
            g.setGradientFill (throat);
            g.fillPath (path);
            g.saveState();
            g.reduceClipRegion (path);
            // uvula + tongue
            const float bottom = mouth.getBottom() + jawOffset();
            g.setColour (juce::Colour (0xffd9606e));
            g.fillEllipse (mouth.getCentreX() + ox - 70, bottom - 34, 140, 70);
            g.setColour (juce::Colour (0xffb94352));
            g.drawLine (mouth.getCentreX() + ox, bottom - 28, mouth.getCentreX() + ox, bottom - 8, 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            for (int i = 0; i < 5; ++i)
                g.drawRoundedRectangle (mouth.translated (ox, 0).reduced (10.0f + (float) i * 14.0f, 8.0f + (float) i * 8.0f), 30.0f, 2.0f);
            g.restoreState();
            g.setColour (juce::Colour (0xffc9707a));
            g.strokePath (path, juce::PathStrokeType (3.0f));
        }
    }

    juce::Path mouthPath (float ox) const
    {
        const float bottom = mouth.getBottom() + jawOffset();
        const float top = mouth.getY();
        const float l = mouth.getX() + ox, r = mouth.getRight() + ox, cx = mouth.getCentreX() + ox;
        const float inset = mouth.getWidth() * 0.11f;
        juce::Path p;
        p.startNewSubPath (l, top + 8);
        p.quadraticTo (cx, top - 14, r, top + 8);
        p.quadraticTo (r + 6, std::max (top + 30, bottom - 40), r - inset, bottom);
        p.quadraticTo (cx, bottom + 22, l + inset, bottom);
        p.quadraticTo (l - 6, std::max (top + 30, bottom - 40), l, top + 8);
        p.closeSubPath();
        return p;
    }

private:
    struct Vert { float th, ph, jit; bool jaw; };
    struct Tri { int v[3]; juce::Colour colour; };
    std::vector<Vert> verts;
    std::vector<Tri> tris;
    juce::Rectangle<float> mouth { 100, 250, 380, 200 };
    float radius = 250.0f;
    struct S { float value = 1.0f, velocity = 0.0f, target = 1.0f; } open;
    float bounce = 0.0f, browJump = 0.0f, dizzyT = 0.0f, yaw = 0.0f, shakeYaw = 0.0f, shakeVel = 0.0f, spin = 0.0f, blink = 0.0f;
    int blinkCountdown = 90, frame = 0;
    juce::Point<float> look, lookTarget;

    static bool stepSpring (S& s, float k, float damp)
    {
        s.velocity = (s.velocity + (s.target - s.value) * k) * damp;
        s.value += s.velocity;
        return std::abs (s.velocity) > 0.0005f || std::abs (s.target - s.value) > 0.0005f;
    }

    juce::Point<float> centre() const { return { getWidth() * 0.5f, mouth.getY() - radius * 0.14f }; }

    void resized() override { radius = std::min (getWidth() * 0.40f, 222.0f); }

    static constexpr float mouthTh = 1.70f;

    // head surface in model units; jaw > 0 drops the front of the lower face
    struct P3 { float x, y, z; };
    static P3 surface (float th, float ph, float jaw)
    {
        float x = std::sin (th) * std::sin (ph), y = std::cos (th), z = std::sin (th) * std::cos (ph);
        y *= 1.02f; z *= 0.9f;
        if (y < 0.0f)
        {
            const float t = -y;
            x *= 1.0f - 0.2f * t * t;
            z *= 1.0f - 0.08f * t * t;
        }
        if (y > 0.55f) { x *= 1.03f; z *= 1.02f; } // hair volume
        if (jaw > 0.0f && th > mouthTh - 0.001f)
        {
            const float front = juce::jlimit (0.0f, 1.0f, (std::cos (ph) - 0.05f) / 0.55f);
            y -= jaw * front;
            z += jaw * front * 0.08f;
        }
        return P3 { x, y, z };
    }

    void buildMesh()
    {
        // latitudes (top -> bottom); the mouth line is duplicated so the jaw can drop open
        const std::vector<std::pair<float, bool>> lats {
            { 0.0f, false }, { 0.38f, false }, { 0.72f, false }, { 1.0f, false }, { 1.22f, false }, { 1.45f, false }, { 1.62f, false },
            { mouthTh - 0.002f, false }, { mouthTh, true }, { 1.98f, true }, { 2.28f, true }, { 2.62f, true }, { 2.95f, true }, { 3.1416f, true } };
        const int lon = 18;
        uint32_t seed = 8080u;
        std::vector<std::vector<int>> idx (lats.size());
        for (size_t i = 0; i < lats.size(); ++i)
            for (int j = 0; j < lon; ++j)
            {
                const float ph = -juce::MathConstants<float>::pi + (float) j * juce::MathConstants<float>::twoPi / (float) lon;
                const bool pole = i == 0 || i + 1 == lats.size();
                const bool mouthRow = std::abs (lats[i].first - mouthTh) < 0.01f;
                const float jit = (pole || mouthRow) ? 1.0f : 1.0f + (paper::rnd (seed) - 0.5f) * 0.05f;
                idx[i].push_back ((int) verts.size());
                verts.push_back ({ lats[i].first, ph, jit, lats[i].second });
            }
        const juce::Colour skin (0xffeab28f), hair (0xff2a1d16), hairHi (0xff3b2a20);
        uint32_t cs = 99u;
        for (size_t i = 0; i + 1 < lats.size(); ++i)
            for (int j = 0; j < lon; ++j)
            {
                const int j2 = (j + 1) % lon;
                const float thm = (lats[i].first + lats[i + 1].first) * 0.5f;
                const float phm = -juce::MathConstants<float>::pi + ((float) j + 0.5f) * juce::MathConstants<float>::twoPi / (float) lon;
                const float yv = std::cos (thm), front = std::cos (phm);
                bool isHair = yv > 0.62f
                              || (front < 0.25f && yv > -0.45f)
                              || (front < 0.7f && yv > 0.15f)
                              || (front >= 0.7f && yv > 0.42f + 0.07f * std::sin (phm * 11.0f + 1.0f));
                for (int half = 0; half < 2; ++half)
                {
                    juce::Colour col = isHair ? (paper::rnd (cs) > 0.7f ? hairHi : hair) : skin;
                    col = col.withMultipliedBrightness (0.95f + 0.1f * paper::rnd (cs));
                    if (half == 0) tris.push_back ({ { idx[i][(size_t) j], idx[i][(size_t) j2], idx[i + 1][(size_t) j2] }, col });
                    else           tris.push_back ({ { idx[i][(size_t) j], idx[i + 1][(size_t) j2], idx[i + 1][(size_t) j] }, col });
                }
            }
    }
};
