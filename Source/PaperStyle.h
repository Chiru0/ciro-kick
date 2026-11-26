#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Paper-cut collage helpers: torn/cut edges, drop shadows, tape, cardboard.
namespace paper
{
    const juce::Colour kraft   { 0xffb98d5e };
    const juce::Colour kraftDk { 0xff8f6840 };
    const juce::Colour cream   { 0xfff4ead3 };
    const juce::Colour cream2  { 0xffe6d6b4 };
    const juce::Colour ink     { 0xff2b1d14 };
    const juce::Colour tape    { 0xc8efe2b8 };

    inline float rnd (uint32_t& s) { s = s * 1664525u + 1013904223u; return (float) ((s >> 8) & 0xffff) / 65535.0f; }

    // Rounded rectangle with slightly irregular, scissor-cut edges.
    inline juce::Path cutRect (juce::Rectangle<float> r, uint32_t seed, float jag = 1.6f, float corner = 6.0f)
    {
        juce::Path base;
        base.addRoundedRectangle (r, corner);
        juce::Path out;
        juce::PathFlatteningIterator it (base, {}, 0.5f);
        bool first = true;
        float acc = 0.0f;
        juce::Point<float> last;
        while (it.next())
        {
            const juce::Point<float> p2 ((float) it.x2, (float) it.y2);
            if (first) { out.startNewSubPath ((float) it.x1, (float) it.y1); last = { (float) it.x1, (float) it.y1 }; first = false; }
            acc += last.getDistanceFrom (p2);
            last = p2;
            if (acc > 9.0f)
            {
                acc = 0.0f;
                const auto c = r.getCentre();
                auto dir = (p2 - c);
                const float len = std::max (1.0f, dir.getDistanceFromOrigin());
                const float o = (rnd (seed) - 0.5f) * 2.0f * jag;
                out.lineTo (p2 + dir / len * o);
            }
        }
        out.closeSubPath();
        return out;
    }

    inline juce::Path cutCircle (juce::Point<float> c, float radius, uint32_t seed, float jag = 1.2f)
    {
        juce::Path p;
        const int n = std::max (16, (int) (radius * 0.9f));
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) n;
            const float r = radius + (rnd (seed) - 0.5f) * 2.0f * jag;
            const auto pt = c.getPointOnCircumference (r, a);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    inline void shadow (juce::Graphics& g, const juce::Path& p, float lift = 1.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillPath (p, juce::AffineTransform::translation (2.0f * lift, 3.5f * lift));
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillPath (p, juce::AffineTransform::translation (3.5f * lift, 6.0f * lift));
    }

    // Light paper fibres so flat colours read as paper.
    inline void fibres (juce::Graphics& g, const juce::Path& clip, juce::Rectangle<float> r, uint32_t seed, float alpha = 0.06f)
    {
        g.saveState();
        g.reduceClipRegion (clip);
        const int n = (int) (r.getWidth() * r.getHeight() / 260.0f);
        for (int i = 0; i < n; ++i)
        {
            const float x = r.getX() + rnd (seed) * r.getWidth();
            const float y = r.getY() + rnd (seed) * r.getHeight();
            const float l = 3.0f + rnd (seed) * 9.0f;
            const float a = rnd (seed) * 3.1416f;
            g.setColour ((rnd (seed) > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (alpha));
            g.drawLine (x, y, x + std::cos (a) * l, y + std::sin (a) * l, 1.0f);
        }
        g.restoreState();
    }

    // Faint low-poly creases, like paper that was crumpled and flattened again.
    inline void crumple (juce::Graphics& g, const juce::Path& clip, juce::Rectangle<float> r, uint32_t seed, float alpha = 0.03f)
    {
        if (r.getWidth() < 30.0f || r.getHeight() < 20.0f) return;
        g.saveState();
        g.reduceClipRegion (clip);
        const int n = 3 + (int) (r.getWidth() * r.getHeight() / 9000.0f);
        for (int i = 0; i < std::min (n, 14); ++i)
        {
            juce::Path t;
            const juce::Point<float> c0 (r.getX() + rnd (seed) * r.getWidth(), r.getY() + rnd (seed) * r.getHeight());
            const float span = std::min (70.0f, std::min (r.getWidth(), r.getHeight()) * 0.8f);
            auto pt = [&] { return c0 + juce::Point<float> ((rnd (seed) - 0.5f) * span, (rnd (seed) - 0.5f) * span); };
            t.addTriangle (pt(), pt(), pt());
            g.setColour ((rnd (seed) > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (alpha * (0.5f + rnd (seed))));
            g.fillPath (t);
        }
        g.restoreState();
    }

    // Full paper piece: shadow, fill, fibres, faint lighter edge.
    inline juce::Path piece (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, uint32_t seed,
                             float lift = 1.0f, float corner = 6.0f, float jag = 1.6f)
    {
        auto p = cutRect (r, seed, jag, corner);
        shadow (g, p, lift);
        g.setColour (c);
        g.fillPath (p);
        fibres (g, p, r, seed ^ 0x9e3779b9u);
        crumple (g, p, r, seed ^ 0x51ed27u);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.strokePath (p, juce::PathStrokeType (1.0f));
        return p;
    }

    inline void tapeStrip (juce::Graphics& g, juce::Point<float> c, float angle, float w = 46.0f, float h = 16.0f, uint32_t seed = 7)
    {
        juce::Path t;
        t.startNewSubPath (-w / 2, -h / 2);
        t.lineTo (w / 2, -h / 2);
        for (int i = 0; i <= 4; ++i) t.lineTo (w / 2 + (rnd (seed) - 0.5f) * 3.0f, -h / 2 + h * (float) i / 4.0f);
        t.lineTo (-w / 2, h / 2);
        for (int i = 4; i >= 0; --i) t.lineTo (-w / 2 + (rnd (seed) - 0.5f) * 3.0f, -h / 2 + h * (float) i / 4.0f);
        t.closeSubPath();
        const auto tr = juce::AffineTransform::rotation (angle).translated (c);
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillPath (t, tr.translated (1.0f, 1.5f));
        g.setColour (tape);
        g.fillPath (t, tr);
    }

    // Cardboard background with speckles and corrugation stripes.
    inline juce::Image cardboard (int w, int h)
    {
        juce::Image img (juce::Image::RGB, std::max (1, w), std::max (1, h), true);
        juce::Graphics g (img);
        g.setGradientFill (juce::ColourGradient (kraft.brighter (0.08f), 0, 0, kraft.darker (0.12f), (float) w, (float) h, false));
        g.fillAll();
        for (int x = 0; x < w; x += 9)
        {
            g.setColour (juce::Colours::black.withAlpha (0.035f));
            g.fillRect ((float) x, 0.0f, 4.0f, (float) h);
        }
        uint32_t s = 1234;
        for (int i = 0; i < w * h / 40; ++i)
        {
            const float x = rnd (s) * (float) w, y = rnd (s) * (float) h;
            g.setColour ((rnd (s) > 0.5f ? juce::Colours::black : juce::Colours::white).withAlpha (0.05f + 0.05f * rnd (s)));
            g.fillRect (x, y, 1.0f + rnd (s) * 1.5f, 1.0f);
        }
        juce::ColourGradient vig (juce::Colours::transparentBlack, (float) w / 2, (float) h / 2,
                                  juce::Colours::black.withAlpha (0.25f), 0, 0, true);
        g.setGradientFill (vig);
        g.fillAll();
        return img;
    }

    // Scraps glued on the cardboard: notebook paper, newsprint, halftone, felt, stamps and doodles.
    inline void scraps (juce::Image& img)
    {
        juce::Graphics g (img);
        const float w = (float) img.getWidth(), h = (float) img.getHeight();
        uint32_t s = 4242u;
        const juce::Colour cols[] = { cream, cream2, juce::Colour (0xffd9e6f2), juce::Colour (0xfff2d9de), juce::Colour (0xffdfeccf),
                                      juce::Colour (0xffece7da), juce::Colour (0xff6d8fb3), juce::Colour (0xffc75d4a), juce::Colour (0xff3f6b4d) };
        for (int i = 0; i < 34; ++i)
        {
            const float pw = 50.0f + rnd (s) * 150.0f, ph = 36.0f + rnd (s) * 110.0f;
            const juce::Rectangle<float> r (rnd (s) * (w - pw * 0.5f) - pw * 0.25f, rnd (s) * (h - ph * 0.5f) - ph * 0.25f, pw, ph);
            const float ang = (rnd (s) - 0.5f) * 0.6f;
            const int kind = (int) (rnd (s) * 6.0f);
            const auto col = cols[(int) (rnd (s) * 8.99f)];
            g.saveState();
            g.addTransform (juce::AffineTransform::rotation (ang, r.getCentreX(), r.getCentreY()));
            auto p = cutRect (r, s ^ (uint32_t) i * 977u, kind == 4 ? 3.0f : 2.2f, kind == 3 ? r.getHeight() * 0.5f : 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.22f));
            g.fillPath (p, juce::AffineTransform::translation (2.0f, 3.0f));
            g.setColour (col.withMultipliedSaturation (0.85f));
            g.fillPath (p);
            g.saveState();
            g.reduceClipRegion (p);
            const auto ink2 = (col.getPerceivedBrightness() > 0.5f ? ink : cream).withAlpha (0.25f);
            g.setColour (ink2);
            switch (kind)
            {
                case 0: // notebook lines + margin
                    for (float y = r.getY() + 10; y < r.getBottom(); y += 9) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
                    g.setColour (juce::Colour (0x66d0464b)); g.drawVerticalLine ((int) (r.getX() + 14), r.getY(), r.getBottom());
                    break;
                case 1: // newsprint text blocks
                    for (float y = r.getY() + 8; y < r.getBottom() - 6; y += 6)
                        g.fillRect (r.getX() + 6, y, (r.getWidth() - 12) * (0.6f + 0.4f * rnd (s)), 2.5f);
                    break;
                case 2: // halftone dots
                    for (float y = r.getY() + 4; y < r.getBottom(); y += 7)
                        for (float x = r.getX() + 4 + std::fmod (y, 14.0f) * 0.5f; x < r.getRight(); x += 7)
                        { const float d = 1.0f + 2.2f * (x - r.getX()) / r.getWidth(); g.fillEllipse (x, y, d, d); }
                    break;
                case 3: // felt patch with running stitch
                {
                    fibres (g, p, r, s, 0.12f);
                    juce::Path st; juce::Path inner; inner.addRoundedRectangle (r.reduced (6.0f), r.getHeight() * 0.4f);
                    const float dash[] = { 5.0f, 4.0f };
                    juce::PathStrokeType (1.6f).createDashedStroke (st, inner, dash, 2);
                    g.setColour (cream.withAlpha (0.8f)); g.fillPath (st);
                    break;
                }
                case 4: // torn kraft with a doodle spiral
                {
                    juce::Path sp; const auto c = r.getCentre();
                    for (int k = 0; k < 60; ++k) { const float a = (float) k * 0.35f, rad = (float) k * 0.45f; auto q = c.getPointOnCircumference (rad, a); if (k == 0) sp.startNewSubPath (q); else sp.lineTo (q); }
                    g.strokePath (sp, juce::PathStrokeType (1.5f));
                    break;
                }
                default: // stamp: perforated edge + star
                {
                    g.setColour (juce::Colours::white.withAlpha (0.35f));
                    for (float x = r.getX(); x < r.getRight(); x += 8) { g.fillEllipse (x, r.getY() - 3, 6, 6); g.fillEllipse (x, r.getBottom() - 3, 6, 6); }
                    juce::Path star; star.addStar (r.getCentre(), 5, std::min (pw, ph) * 0.12f, std::min (pw, ph) * 0.3f, rnd (s));
                    g.setColour (ink2); g.fillPath (star);
                    break;
                }
            }
            g.restoreState();
            fibres (g, p, r, s ^ 77u, 0.05f);
            g.restoreState();
            if (rnd (s) > 0.55f) tapeStrip (g, { r.getCentreX() + (rnd (s) - 0.5f) * pw * 0.5f, r.getY() + 4 }, ang + (rnd (s) - 0.5f) * 0.8f, 40, 14, s);
        }
    }

    inline uint32_t seedOf (juce::Rectangle<int> r) { return (uint32_t) (r.getX() * 73856093) ^ (uint32_t) (r.getY() * 19349663) ^ (uint32_t) r.getWidth(); }
}
