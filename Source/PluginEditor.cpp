#include "PluginEditor.h"

static juce::Font uiFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

CiroLookAndFeel::CiroLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, ui::panel);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::headerTextColourId, ui::dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::track.brighter (0.2f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

//==============================================================================
FunKnob::FunKnob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& n,
                  std::function<juce::String (float)> fmt)
    : name (n), format (std::move (fmt))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    if (id == "bend" || id == "bSemi") slider.setName ("bipolar");
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    slider.addMouseListener (this, false);
}

void FunKnob::resized()
{
    const int size = 62;
    slider.setBounds (getLocalBounds().getCentreX() - size / 2, 4, size, size);
}

void FunKnob::paint (juce::Graphics& g)
{
    const bool active = slider.isMouseOverOrDragging();
    const float pos = shown < 0 ? (float) slider.valueToProportionOfLength (slider.getValue()) : shown;
    auto b = slider.getBounds().toFloat().reduced (4.0f);
    const float sc = spring.value;
    const auto c = b.getCentre();
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f * sc;
    const auto rp = slider.getRotaryParameters();
    const float start = rp.startAngleRadians, end = rp.endAngleRadians;
    const float angle = start + pos * (end - start);
    uint32_t seed = paper::seedOf (getBounds());

    if (flash > 0.01f || active)
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f * std::max (flash, active ? 0.6f : 0.0f)));
        g.fillPath (paper::cutCircle (c, r + 6.0f, seed, 1.5f));
    }

    // paper ring that holds the value strip
    auto ring = paper::cutCircle (c, r, seed, 1.0f);
    paper::shadow (g, ring, active ? 1.5f : 1.0f);
    g.setColour (ui::track);
    g.fillPath (ring);

    juce::Path valueArc;
    const bool bipolar = slider.getName() == "bipolar";
    const float from = bipolar ? start + (float) slider.valueToProportionOfLength (0.0) * (end - start) : start;
    valueArc.addCentredArc (c.x, c.y, r - 5, r - 5, 0, std::min (from, angle), std::max (from, angle), true);
    g.setColour (accent.interpolatedWith (juce::Colours::white, flash * 0.4f));
    g.strokePath (valueArc, juce::PathStrokeType (7.0f + 2.0f * flash, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    // sewn-on button in the middle
    const float br = r - 11.0f;
    auto button = paper::cutCircle (c, br, seed ^ 77u, 0.8f);
    paper::shadow (g, button, 0.7f);
    g.setColour (ui::panel);
    g.fillPath (button);
    paper::fibres (g, button, button.getBounds(), seed);
    juce::Path stitch;
    stitch.addEllipse (c.x - br + 4, c.y - br + 4, (br - 4) * 2, (br - 4) * 2);
    juce::Path dashed;
    const float dashes[] = { 3.0f, 3.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, stitch, dashes, 2);
    g.setColour (paper::ink.withAlpha (0.45f));
    g.fillPath (dashed);

    // pointer: a little paper triangle
    juce::Path tri;
    tri.addTriangle (0, -br + 5, -4.5f, -br + 14, 4.5f, -br + 14);
    g.setColour (active || flash > 0.2f ? accent.darker (0.3f) : paper::ink);
    g.fillPath (tri, juce::AffineTransform::rotation (angle).translated (c));

    g.setColour (active ? paper::ink : ui::dim);
    g.setFont (uiFont (11.5f, true));
    g.drawText (active ? format ((float) slider.getValue()) : name, getLocalBounds().withTop (70).withHeight (16),
                juce::Justification::centred);
}

bool FunKnob::animate (juce::Colour newAccent)
{
    bool dirty = newAccent != accent;
    accent = newAccent;

    const bool dragging = slider.isMouseButtonDown();
    spring.target = dragging ? 1.1f : 1.0f;
    dirty |= spring.step();

    const float target = (float) slider.valueToProportionOfLength (slider.getValue());
    if (shown < 0) shown = target;
    const float diff = target - shown;
    if (std::abs (diff) > 0.0005f)
    {
        if (dragging) shown = target;
        else
        {
            if (std::abs (diff) > 0.08f) flash = std::max (flash, std::min (1.0f, std::abs (diff) * 3.0f));
            shown += diff * 0.16f;
        }
        dirty = true;
    }
    else shown = target;

    if (flash > 0.0f) { flash = std::max (0.0f, flash - 0.03f); dirty = true; }
    if (dirty) repaint();
    return dirty;
}

//==============================================================================
void FunButton::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (5.0f);
    const float sc = spring.value;
    uint32_t seed = paper::seedOf (getBounds());
    const float tilt = (paper::rnd (seed) - 0.5f) * 0.035f;
    g.addTransform (juce::AffineTransform::scale (sc, sc, b.getCentreX(), b.getCentreY())
                        .rotated (tilt, b.getCentreX(), b.getCentreY()));
    const float corner = std::min (b.getHeight() * 0.3f, 10.0f);

    if (glow > 0.01f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.5f * glow));
        g.fillPath (paper::cutRect (b.expanded (5.0f * glow), seed, 2.0f, corner + 4));
    }

    const bool on = filled || selected;
    juce::Colour fill = on ? colour : ui::panel;
    if (hover) fill = fill.brighter (0.08f);
    fill = fill.interpolatedWith (juce::Colours::white, glow * 0.3f);
    const float lift = hover ? 1.6f : (sc < 0.97f ? 0.4f : 1.0f);
    auto shape = paper::piece (g, b.translated (0, hover ? -1.5f : 0.0f), fill, seed, lift, corner);
    if (on)
    {
        g.setColour (paper::ink.withAlpha (0.65f));
        g.strokePath (shape, juce::PathStrokeType (1.6f));
    }

    const juce::Colour fg = on ? (colour.getPerceivedBrightness() > 0.55f ? paper::ink : juce::Colours::white) : paper::ink;
    g.setColour (fg);
    const auto font = uiFont (std::min (fontSize, b.getHeight() * 0.42f), true);
    if (drawIcon && label.isEmpty())
    {
        drawIcon (g, b.reduced (8.0f));
        return;
    }
    auto content = b;
    if (drawIcon)
    {
        const float icon = b.getHeight() * 0.5f, gap = 10.0f;
        const float textW = juce::GlyphArrangement::getStringWidth (font, label);
        const float x0 = b.getCentreX() - (icon + gap + textW) * 0.5f;
        drawIcon (g, { x0, b.getCentreY() - icon * 0.5f, icon, icon });
        content = { x0 + icon + gap, b.getY(), textW + 4.0f, b.getHeight() };
    }
    g.setColour (fg);
    g.setFont (font);
    g.drawFittedText (label, content.toNearestInt().reduced (4, 0), drawIcon ? juce::Justification::centredLeft : juce::Justification::centred, 1, 0.8f);
}

void ToothButton::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f, 1.0f);
    const float sc = spring.value;
    g.addTransform (juce::AffineTransform::scale (sc, sc, b.getCentreX(), upper ? b.getY() : b.getBottom()));
    if (hover) b = b.translated (0, upper ? 2.0f : -2.0f);
    const float rr = std::min (b.getWidth() * 0.45f, b.getHeight() * 0.8f);
    juce::Path t;
    t.addRoundedRectangle (b.getX(), b.getY(), b.getWidth(), b.getHeight(), rr, rr, ! upper, ! upper, upper, upper);
    const bool on = selected || filled;
    const juce::Colour enamel = on ? juce::Colour (0xffeec25a) : juce::Colour (0xfff6efdf);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillPath (t, juce::AffineTransform::translation (1.5f, 2.5f));
    juce::ColourGradient grad (enamel.brighter (0.15f), b.getX(), b.getY(), enamel.darker (on ? 0.25f : 0.12f), b.getRight(), b.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (t);
    if (glow > 0.01f) { g.setColour (juce::Colours::white.withAlpha (0.5f * glow)); g.fillPath (t); }
    // shine
    g.setColour (juce::Colours::white.withAlpha (on ? 0.55f : 0.7f));
    g.fillRoundedRectangle (b.getX() + b.getWidth() * 0.14f, b.getY() + (upper ? 4.0f : b.getHeight() * 0.3f), 4.0f, b.getHeight() * 0.42f, 2.0f);
    g.setColour (juce::Colour (0xff9a7f62).withAlpha (0.8f));
    g.strokePath (t, juce::PathStrokeType (1.2f));
    if (on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        juce::Path star;
        star.addStar ({ b.getRight() - 12.0f, b.getY() + (upper ? 9.0f : b.getHeight() - 9.0f) }, 4, 1.5f, 5.0f, 0.3f);
        g.fillPath (star);
    }
    g.setColour (paper::ink);
    auto content = b.reduced (4.0f, 2.0f);
    if (drawIcon && label.isEmpty()) { drawIcon (g, content.reduced (6.0f)); return; }
    if (drawIcon)
    {
        auto ic = content.removeFromLeft (content.getHeight() * 0.6f).reduced (1.0f, content.getHeight() * 0.2f);
        drawIcon (g, ic);
    }
    g.setFont (uiFont (std::min (fontSize, b.getHeight() * 0.42f), true));
    g.drawFittedText (label, content.toNearestInt(), juce::Justification::centred, 1, 0.75f);
}

void FunButton::mouseDown (const juce::MouseEvent& e)
{
    spring.target = 0.86f;
    spring.kick (0.05f);
    if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); }
    else if (onClick) onClick();
}

void FunButton::mouseUp (const juce::MouseEvent&)
{
    spring.target = hover ? 1.04f : 1.0f;
    spring.velocity += 0.06f;
}

bool FunButton::animate()
{
    bool moving = spring.step();
    if (glow > 0.0f) { glow = std::max (0.0f, glow - 0.05f); moving = true; }
    if (moving) repaint();
    return moving;
}

//==============================================================================
void PlayPad::hit()
{
    glow = 1.0f;
    pop (0.12f);
    rings.push_back (0.0f);
    if (rings.size() > 6) rings.erase (rings.begin());
}

bool PlayPad::animate()
{
    bool moving = FunButton::animate();
    for (auto& r : rings) r += 0.035f;
    rings.erase (std::remove_if (rings.begin(), rings.end(), [] (float r) { return r >= 1.0f; }), rings.end());
    if (! rings.empty()) { moving = true; repaint(); }
    return moving;
}

void PlayPad::paint (juce::Graphics& g)
{
    FunButton::paint (g);
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    const auto c = b.getCentre();
    uint32_t seed = 99;
    for (auto r : rings)
    {
        const float rad = 18.0f + r * b.getWidth() * 0.55f;
        g.setColour (paper::ink.withAlpha ((1.0f - r) * 0.5f));
        g.strokePath (paper::cutCircle (c, rad, seed, 2.0f), juce::PathStrokeType (3.0f * (1.0f - r) + 0.5f));
    }
    juce::Path tri;
    const float s = 16.0f;
    tri.addTriangle (c.x - s * 0.6f, c.y - 28 - s, c.x - s * 0.6f, c.y - 28 + s, c.x + s, c.y - 28);
    paper::shadow (g, tri, 0.8f);
    g.setColour (glow > 0.3f ? juce::Colours::white : colour);
    g.fillPath (tri);
    g.setColour (paper::ink.withAlpha (0.7f));
    g.strokePath (tri, juce::PathStrokeType (1.5f));
}

//==============================================================================
void Particles::burst (juce::Point<float> at, juce::Colour c, int count, float speed)
{
    for (int i = 0; i < count; ++i)
    {
        const float a = rnd.nextFloat() * juce::MathConstants<float>::twoPi;
        const float v = speed * (0.35f + rnd.nextFloat());
        auto col = c.withRotatedHue ((rnd.nextFloat() - 0.5f) * 0.25f).brighter (rnd.nextFloat() * 0.5f);
        if (rnd.nextFloat() < 0.25f) col = juce::Colours::white;
        ps.push_back ({ at, { std::cos (a) * v, std::sin (a) * v - speed * 0.4f }, 1.0f,
                        3.0f + rnd.nextFloat() * 5.0f, (rnd.nextFloat() - 0.5f) * 0.4f, rnd.nextFloat() * 6.0f, col, rnd.nextInt (3) });
    }
}

bool Particles::step()
{
    if (ps.empty()) return false;
    for (auto& p : ps)
    {
        p.pos += p.vel;
        p.vel = { p.vel.x * 0.96f, p.vel.y * 0.96f + 0.35f };
        p.angle += p.spin;
        p.life -= 0.022f;
    }
    ps.erase (std::remove_if (ps.begin(), ps.end(), [] (const P& p) { return p.life <= 0.0f; }), ps.end());
    repaint();
    return true;
}

void Particles::paint (juce::Graphics& g)
{
    for (auto& p : ps)
    {
        g.setColour (p.colour.withAlpha (std::min (1.0f, p.life * 1.5f)));
        const float s = p.size * (0.5f + 0.5f * p.life);
        if (p.shape == 0) g.fillEllipse (p.pos.x - s / 2, p.pos.y - s / 2, s, s);
        else
        {
            juce::Path r;
            if (p.shape == 1) r.addRectangle (-s / 2, -s / 4, s, s / 2);
            else r.addTriangle (0, -s / 2, s / 2, s / 2, -s / 2, s / 2);
            g.fillPath (r, juce::AffineTransform::rotation (p.angle).translated (p.pos));
        }
    }
}

//==============================================================================
void FaceLogo::mouseDown (const juce::MouseEvent&)
{
    spinVel += 0.55f;
    squash = 1.0f;
    tongue = 1.0f;
    if (onClick) onClick();
}

bool FaceLogo::animate()
{
    bool moving = false;
    if (std::abs (spinVel) > 0.002f)
    {
        angle += spinVel;
        spinVel *= 0.93f;
        moving = true;
    }
    else
    {
        // settle upright
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float targetA = std::round (angle / twoPi) * twoPi;
        if (std::abs (targetA - angle) > 0.002f) { angle += (targetA - angle) * 0.2f; moving = true; }
        else angle = 0.0f;
        spinVel = 0.0f;
    }
    if (squash > 0.0f) { squash = std::max (0.0f, squash - 0.06f); moving = true; }
    if (tongue > 0.0f) { tongue = std::max (0.0f, tongue - 0.03f); moving = true; }
    if (moving) repaint();
    return moving;
}

void FaceLogo::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    const float S = std::min (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float wobble = 1.0f + 0.12f * squash * std::sin (squash * 12.0f);
    g.addTransform (juce::AffineTransform::rotation (angle, c.x, c.y)
                        .scaled (wobble, 2.0f - wobble, c.x, c.y));
    const float x0 = c.x - S / 2, y0 = c.y - S / 2;
    auto P = [&] (float x, float y) { return juce::Point<float> (x0 + x * S, y0 + y * S); };
    auto R = [&] (float x, float y, float w, float h) { return juce::Rectangle<float> (x0 + x * S, y0 + y * S, w * S, h * S); };

    // die-cut sticker
    auto sticker = paper::cutCircle (c, S * 0.5f, 31u, 1.0f);
    paper::shadow (g, sticker, 1.2f);
    g.setColour (juce::Colours::white);
    g.fillPath (sticker);
    g.setColour (accent);
    g.fillEllipse (R (0.07f, 0.07f, 0.86f, 0.86f));

    const juce::Colour skin (0xfff1c3a1), skinShade (0xffd99d7c), hair (0xff241812), ink (0xff1a1010);

    // ears + head
    g.setColour (skinShade);
    g.fillEllipse (R (0.15f, 0.43f, 0.11f, 0.17f));
    g.fillEllipse (R (0.74f, 0.43f, 0.11f, 0.17f));
    g.setColour (skin);
    juce::Path head;
    head.addRoundedRectangle (R (0.22f, 0.22f, 0.56f, 0.66f), S * 0.24f);
    g.fillPath (head);

    // hair: short, dark, swept fringe
    juce::Path h;
    h.startNewSubPath (P (0.20f, 0.46f));
    h.cubicTo (P (0.16f, 0.20f), P (0.38f, 0.08f), P (0.56f, 0.12f));
    h.cubicTo (P (0.74f, 0.14f), P (0.86f, 0.26f), P (0.80f, 0.46f));
    h.lineTo (P (0.76f, 0.34f));
    h.cubicTo (P (0.66f, 0.30f), P (0.58f, 0.26f), P (0.52f, 0.30f));
    h.lineTo (P (0.47f, 0.25f));
    h.cubicTo (P (0.40f, 0.30f), P (0.30f, 0.30f), P (0.25f, 0.36f));
    h.closeSubPath();
    g.setColour (hair);
    g.fillPath (h);

    // thick furrowed eyebrows
    g.setColour (hair);
    juce::Path brows;
    brows.startNewSubPath (P (0.28f, 0.42f)); brows.quadraticTo (P (0.36f, 0.38f), P (0.46f, 0.45f));
    brows.startNewSubPath (P (0.54f, 0.45f)); brows.quadraticTo (P (0.64f, 0.38f), P (0.72f, 0.42f));
    g.strokePath (brows, juce::PathStrokeType (S * 0.055f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // eyes (right one squinting)
    g.setColour (juce::Colours::white);
    g.fillEllipse (R (0.31f, 0.47f, 0.13f, 0.08f));
    g.fillEllipse (R (0.56f, 0.48f, 0.13f, 0.06f));
    g.setColour (ink);
    g.fillEllipse (R (0.355f, 0.475f, 0.06f, 0.07f));
    g.fillEllipse (R (0.595f, 0.48f, 0.055f, 0.055f));

    // scrunched nose
    juce::Path nose;
    nose.startNewSubPath (P (0.50f, 0.50f));
    nose.quadraticTo (P (0.47f, 0.60f), P (0.45f, 0.63f));
    nose.quadraticTo (P (0.50f, 0.67f), P (0.56f, 0.63f));
    nose.startNewSubPath (P (0.44f, 0.52f)); nose.lineTo (P (0.47f, 0.54f));
    nose.startNewSubPath (P (0.56f, 0.52f)); nose.lineTo (P (0.53f, 0.54f));
    g.setColour (skinShade.darker (0.25f));
    g.strokePath (nose, juce::PathStrokeType (S * 0.022f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // mouth + tongue out
    g.setColour (juce::Colour (0xff8a3a3a));
    g.fillEllipse (R (0.40f, 0.70f, 0.20f, 0.07f));
    const float tl = 0.13f + 0.05f * tongue;
    juce::Path t;
    t.addRoundedRectangle (R (0.43f, 0.72f, 0.14f, tl), S * 0.07f);
    g.setColour (juce::Colour (0xffef7d8e));
    g.fillPath (t);
    g.setColour (juce::Colour (0xffc85a6c));
    g.drawLine (juce::Line<float> (P (0.50f, 0.75f), P (0.50f, 0.72f + tl - 0.03f)), S * 0.015f);
}

//==============================================================================
static float xyBright (float tone) { return std::log (std::clamp (tone, 1500.0f, 16000.0f) / 1500.0f) / std::log (16000.0f / 1500.0f); }

juce::Point<float> XYPad::puckFromParams() const
{
    const auto k = proc.readParams();
    return { xyBright (k.tone), std::clamp (k.drive / 40.0f, 0.0f, 1.0f) };
}

void XYPad::apply (juce::Point<float> pos)
{
    const auto area = getLocalBounds().toFloat().reduced (14.0f);
    const float x = std::clamp ((pos.x - area.getX()) / area.getWidth(), 0.0f, 1.0f);
    const float y = std::clamp (1.0f - (pos.y - area.getY()) / area.getHeight(), 0.0f, 1.0f);
    auto set = [&] (const char* id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("tone", 1500.0f * std::pow (16000.0f / 1500.0f, x));
    set ("cTone", 2000.0f * std::pow (4.5f, x));
    set ("drive", 40.0f * y);
    if (proc.readParams().rumbleAmt > 0.0f) set ("rDrive", 30.0f * y);
    trail.push_back ({ x, y });
    if (trail.size() > 24) trail.erase (trail.begin());
}

void XYPad::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    pulse = 1.0f;
    for (auto* id : { "tone", "cTone", "drive", "rDrive" }) if (auto* p = proc.apvts.getParameter (id)) p->beginChangeGesture();
    apply (e.position);
}

void XYPad::mouseDrag (const juce::MouseEvent& e) { apply (e.position); }

void XYPad::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    for (auto* id : { "tone", "cTone", "drive", "rDrive" }) if (auto* p = proc.apvts.getParameter (id)) p->endChangeGesture();
    proc.triggerFromUI();
}

bool XYPad::animate()
{
    const auto target = puckFromParams();
    const auto d = target - shown;
    bool moving = std::abs (d.x) + std::abs (d.y) > 0.001f;
    shown += d * (dragging ? 1.0f : 0.18f);
    if (! dragging && ! trail.empty()) { trail.erase (trail.begin()); moving = true; }
    if (pulse > 0.0f) { pulse = std::max (0.0f, pulse - 0.04f); moving = true; }
    if (moving || dragging) repaint();
    return moving;
}

void XYPad::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (4.0f);
    paper::piece (g, b, juce::Colour (0xffd9ecf2), 4242u, 1.0f, 4.0f);
    paper::tapeStrip (g, { b.getCentreX(), b.getY() + 2 }, -0.05f, 54, 15, 5u);
    auto area = b.reduced (12.0f);

    // grid that warps towards the puck
    const juce::Point<float> puck (area.getX() + shown.x * area.getWidth(), area.getBottom() - shown.y * area.getHeight());
    g.setColour (juce::Colour (0x553c7a99));
    for (int i = 0; i <= 6; ++i)
    {
        const float f = (float) i / 6.0f;
        const float gx = area.getX() + f * area.getWidth(), gy = area.getY() + f * area.getHeight();
        g.drawLine (gx + (puck.x - gx) * 0.08f, area.getY(), gx + (puck.x - gx) * 0.08f, area.getBottom(), 1.0f);
        g.drawLine (area.getX(), gy + (puck.y - gy) * 0.08f, area.getRight(), gy + (puck.y - gy) * 0.08f, 1.0f);
    }

    g.setFont (uiFont (10.0f, true));
    g.setColour (ui::dim);
    g.drawText ("DIRTY", area.toNearestInt(), juce::Justification::centredTop);
    g.drawText ("CLEAN", area.toNearestInt(), juce::Justification::centredBottom);
    g.drawText ("DARK", area.toNearestInt(), juce::Justification::centredLeft);
    g.drawText ("BRIGHT", area.toNearestInt(), juce::Justification::centredRight);

    for (size_t i = 0; i < trail.size(); ++i)
    {
        const float a = (float) (i + 1) / (float) trail.size();
        const float tx = area.getX() + trail[i].x * area.getWidth(), ty = area.getBottom() - trail[i].y * area.getHeight();
        g.setColour (accent.withAlpha (0.35f * a));
        g.fillEllipse (tx - 5 * a, ty - 5 * a, 10 * a, 10 * a);
    }

    const float r = 12.0f + 5.0f * pulse + (dragging ? 3.0f : 0.0f);
    auto outer = paper::cutCircle (puck, r + 3, 11u, 0.8f);
    paper::shadow (g, outer, dragging ? 2.0f : 1.0f);
    g.setColour (juce::Colours::white);
    g.fillPath (outer);
    g.setColour (accent);
    g.fillEllipse (puck.x - r, puck.y - r, r * 2, r * 2);
    g.setColour (paper::ink);
    g.fillEllipse (puck.x - 3, puck.y - 3, 6, 6);
}

//==============================================================================
juce::Rectangle<float> StepStrip::cell (int i, int n) const
{
    const int cols = 8, rows = std::max (1, (n + cols - 1) / cols);
    const float w = (float) getWidth() / (float) cols, h = std::min (44.0f, (float) getHeight() / (float) rows);
    const float y0 = ((float) getHeight() - h * (float) rows) * 0.5f;
    return { (float) (i % cols) * w, y0 + (float) (i / cols) * h, w, h };
}

int StepStrip::stepAt (juce::Point<float> p) const
{
    const int n = proc.getLoopLength();
    int best = 0; float bd = 1.0e9f;
    for (int i = 0; i < n; ++i) { const float d = cell (i, n).getCentre().getDistanceSquaredFrom (p); if (d < bd) { bd = d; best = i; } }
    return best;
}

void StepStrip::mouseDown (const juce::MouseEvent& e)
{
    const int s = stepAt (e.position);
    const auto mask = proc.getSteps();
    paintValue = ((mask >> s) & 1) ? 0 : 1;
    proc.setSteps (paintValue ? (mask | ((juce::int64) 1 << s)) : (mask & ~((juce::int64) 1 << s)));
    lastPainted = s;
    flashes[s] = 1.0f;
    repaint();
}

void StepStrip::mouseDrag (const juce::MouseEvent& e)
{
    const int s = stepAt (e.position);
    if (s == lastPainted) return;
    lastPainted = s;
    const auto mask = proc.getSteps();
    proc.setSteps (paintValue ? (mask | ((juce::int64) 1 << s)) : (mask & ~((juce::int64) 1 << s)));
    flashes[s] = 1.0f;
    repaint();
}

bool StepStrip::animate()
{
    bool moving = false;
    const int cur = proc.currentStep.load();
    if (cur != lastStep)
    {
        lastStep = cur;
        if (cur >= 0 && ((proc.getSteps() >> cur) & 1)) flashes[cur] = 1.0f;
        moving = true;
    }
    for (auto& f : flashes) if (f > 0.0f) { f = std::max (0.0f, f - 0.06f); moving = true; }
    if (moving) repaint();
    return moving;
}

void StepStrip::paint (juce::Graphics& g)
{
    const auto mask = proc.getSteps();
    const int cur = proc.currentStep.load();
    const int n = proc.getLoopLength();
    for (int i = 0; i < n; ++i)
    {
        auto r = cell (i, n).reduced (3.0f, 3.0f);
        const bool on = (mask >> i) & 1;
        const float f = flashes[i];
        r = r.expanded (2.0f * f);
        juce::Colour c = on ? accent : (i % 4 == 0 ? ui::track : ui::panel);
        if (i == cur) c = c.brighter (0.25f);
        uint32_t seed = (uint32_t) (i * 7919 + 13);
        const float tilt = (paper::rnd (seed) - 0.5f) * 0.08f;
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (tilt, r.getCentreX(), r.getCentreY()).translated (0, on ? -1.0f : 0.0f));
        auto shape = paper::piece (g, r, c.interpolatedWith (juce::Colours::white, f * 0.5f), seed, on ? 1.2f : 0.6f, 4.0f, 1.0f);
        if (i == cur)
        {
            g.setColour (paper::ink);
            g.strokePath (shape, juce::PathStrokeType (2.0f));
        }
        g.restoreState();
    }
}

//==============================================================================
juce::Rectangle<float> WaveDisplay::waveArea() const { return getLocalBounds().toFloat().reduced (throat ? 16.0f : 12.0f, throat ? 18.0f : 24.0f); }

const float* WaveDisplay::nodeT (const KickParams& k) const { return mode == 0 ? k.envT : mode == 1 ? k.rEnvT : k.bEnvT; }
const float* WaveDisplay::nodeG (const KickParams& k) const { return mode == 0 ? k.envG : mode == 1 ? k.rEnvG : k.bEnvG; }
juce::String WaveDisplay::nodeId (bool gain) const
{
    return juce::String (mode == 0 ? "env" : mode == 1 ? "rEnv" : "bEnv") + (gain ? "G" : "T");
}

// node time (0..1) <-> milliseconds from the kick, per mode
float WaveDisplay::nodeMs (const KickParams& k, float t) const
{
    const float beatMs = (float) (60000.0 / std::max (20.0, cachedBpm));
    if (mode == 0) return t * k.length;
    if (mode == 1) return t * beatMs;
    static const float firstEv[4] = { 0.25f, 0.5f, 0.5f, 0.12f };
    return firstEv[std::clamp (k.bassPattern, 0, 3)] * beatMs + t * k.bassDecay;
}

float WaveDisplay::msToNode (const KickParams& k, float ms) const
{
    const float beatMs = (float) (60000.0 / std::max (20.0, cachedBpm));
    if (mode == 0) return ms / std::max (1.0f, k.length);
    if (mode == 1) return ms / beatMs;
    static const float firstEv[4] = { 0.25f, 0.5f, 0.5f, 0.12f };
    return (ms - firstEv[std::clamp (k.bassPattern, 0, 3)] * beatMs) / std::max (1.0f, k.bassDecay);
}

juce::Point<float> WaveDisplay::nodePos (int i) const
{
    const auto area = waveArea();
    const float frac = nodeMs (cached, nodeT (cached)[i]) / std::max (1.0f, spanMs);
    return { area.getX() + frac * area.getWidth(), area.getCentreY() - nodeG (cached)[i] * 0.5f * area.getHeight() * 0.5f };
}

int WaveDisplay::nodeAt (juce::Point<float> p) const
{
    int best = -1;
    float bestD = 14.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float d = nodePos (i).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void WaveDisplay::setNode (int i, float t, float gain)
{
    auto set = [&] (const juce::String& id, float v) { if (auto* p = proc.apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set (nodeId (false) + juce::String (i), t);
    set (nodeId (true) + juce::String (i), gain);
}

void WaveDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragNode = nodeAt (e.position);
    if (dragNode < 0) { proc.triggerFromUI(); return; }
    for (bool gain : { false, true })
        if (auto* p = proc.apvts.getParameter (nodeId (gain) + juce::String (dragNode))) p->beginChangeGesture();
}

void WaveDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNode < 0) return;
    const auto area = waveArea();
    const auto k = proc.readParams();
    float t = msToNode (k, (e.position.x - area.getX()) / area.getWidth() * spanMs);
    const float* T = nodeT (k);
    if (dragNode == 0) t = 0.0f;
    else
    {
        const float lo = T[dragNode - 1] + 0.005f;
        const float hi = dragNode < 4 ? T[dragNode + 1] - 0.005f : 1.0f;
        t = std::clamp (t, lo, std::max (lo, hi));
    }
    const float gain = std::clamp ((area.getCentreY() - e.position.y) / (area.getHeight() * 0.25f), 0.0f, 2.0f);
    setNode (dragNode, t, gain);
}

void WaveDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragNode >= 0)
    {
        for (bool gain : { false, true })
            if (auto* p = proc.apvts.getParameter (nodeId (gain) + juce::String (dragNode))) p->endChangeGesture();
        proc.triggerFromUI();
    }
    dragNode = -1;
}

void WaveDisplay::mouseMove (const juce::MouseEvent& e)
{
    const int n = nodeAt (e.position);
    if (n != hoverNode) { hoverNode = n; repaint(); }
    setMouseCursor (n >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::PointingHandCursor);
}

void WaveDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int n = nodeAt (e.position);
    if (n >= 0) setNode (n, nodeT (cached)[n], mode == 2 ? KickParams().bEnvG[n] : 1.0f);
}

//==============================================================================
static juce::String noteName (float hz)
{
    const int midi = (int) std::round (12.0 * std::log2 (hz / 440.0) + 69.0);
    return juce::MidiMessage::getMidiNoteName (midi, true, true, 4);
}

void WaveDisplay::render (const KickParams& p, double bpm)
{
    GrooveEngine e;
    const double sr = 22050.0;
    e.setSampleRate (sr);
    KickParams q = p;
    q.volume = 0.0f;
    e.setParams (q);
    e.trigger (q, 1.0f, -1.0f, bpm);
    const double beatMs = 60000.0 / bpm;
    const bool extras = p.bassLevel > 0.0001f || p.rumbleAmt > 0.0001f;
    spanMs = (float) std::max ((double) p.length + 20.0, (extras || mode != 0) ? beatMs : 0.0);
    const int total = (int) (sr * spanMs / 1000.0);
    const int w = std::max (1, getWidth() - 24);
    kickT.assign ((size_t) w, 0.0f); bassT.assign ((size_t) w, 0.0f); rumbleT.assign ((size_t) w, 0.0f);
    for (int i = 0; i < total; ++i)
    {
        float k, b, r;
        e.next (k, b, r);
        const auto idx = (size_t) std::min (w - 1, (int) ((int64_t) i * w / std::max (1, total)));
        kickT[idx] = std::max (kickT[idx], std::abs (k));
        bassT[idx] = std::max (bassT[idx], std::abs (b));
        rumbleT[idx] = std::max (rumbleT[idx], std::abs (r));
    }
    if (kickS.size() != kickT.size()) { kickS = kickT; bassS = bassT; rumbleS = rumbleT; }
}

void WaveDisplay::update (juce::Colour a, double now)
{
    nowMs = now;
    const auto p = proc.readParams();
    const double bpm = proc.bpm.load();
    const bool changed = ! hasCache || std::memcmp (&p, &cached, offsetof (KickParams, keyTrack)) != 0
                         || (int) kickT.size() != getWidth() - 24 || bpm != cachedBpm;
    if (changed) { cached = p; cachedBpm = bpm; hasCache = true; render (p, bpm); }

    // morph shown waveform towards the new one
    bool morphing = false;
    auto morph = [&] (std::vector<float>& s, const std::vector<float>& t)
    {
        if (s.size() != t.size()) { s = t; return; }
        for (size_t i = 0; i < s.size(); ++i)
        {
            const float d = t[i] - s[i];
            if (std::abs (d) > 0.002f) { s[i] += d * 0.2f; morphing = true; }
            else s[i] = t[i];
        }
    };
    morph (kickS, kickT); morph (bassS, bassT); morph (rumbleS, rumbleT);

    const bool playing = now - lastHitMs < spanMs + 300.0;
    if (shake > 0.0f) shake = std::max (0.0f, shake - 0.08f);
    if (shake > 0.0f)
    {
        juce::Random r;
        setTransform (juce::AffineTransform::translation ((r.nextFloat() - 0.5f) * 6.0f * shake, (r.nextFloat() - 0.5f) * 6.0f * shake));
    }
    else if (! getTransform().isIdentity()) setTransform ({});

    if (changed || morphing || playing || a != accent || shake > 0.0f) { accent = a; repaint(); }
}

void WaveDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const double since = nowMs - lastHitMs;
    const float flash = (float) juce::jlimit (0.0, 1.0, 1.0 - since / 300.0);

    const juce::Colour dimC = throat ? juce::Colour (0xffeaa6b0) : ui::dim;
    const juce::Colour textC = throat ? juce::Colours::white : ui::text;
    if (throat)
    {
        g.setColour (juce::Colour (0x18ffffff));
        for (float x = b.getX() + 8; x < b.getRight(); x += 16) g.drawVerticalLine ((int) x, b.getY() + 4, b.getBottom() - 4);
        if (flash > 0.0f) { g.setColour (accent.withAlpha (0.12f * flash)); g.fillRoundedRectangle (b, 20.0f); }
    }
    else
    {
    // graph-paper sheet taped on the cardboard
    const auto sheet = paper::piece (g, b.reduced (4.0f), ui::panel.interpolatedWith (juce::Colours::white, flash * 0.25f), 555u, 1.0f, 3.0f);
    {
        g.saveState();
        g.reduceClipRegion (sheet);
        g.setColour (juce::Colour (0x223c7a99));
        for (float x = b.getX() + 8; x < b.getRight(); x += 14) g.drawVerticalLine ((int) x, b.getY(), b.getBottom());
        for (float y = b.getY() + 8; y < b.getBottom(); y += 14) g.drawHorizontalLine ((int) y, b.getX(), b.getRight());
        g.restoreState();
    }
    paper::tapeStrip (g, { b.getX() + 22, b.getY() + 10 }, -0.6f, 50, 15, 3u);
    paper::tapeStrip (g, { b.getRight() - 22, b.getY() + 10 }, 0.6f, 50, 15, 9u);
    }

    auto area = waveArea();
    const float mid = area.getCentreY();
    const float half = area.getHeight() * 0.5f;
    g.setColour (paper::ink.withAlpha (0.25f));
    g.drawHorizontalLine ((int) mid, area.getX(), area.getRight());

    const float progress = (float) (since / std::max (1.0f, spanMs));

    auto makePath = [&] (const std::vector<float>& pk)
    {
        juce::Path wave;
        const int n = (int) pk.size();
        if (n < 2) return wave;
        wave.startNewSubPath (area.getX(), mid);
        for (int i = 0; i < n; ++i) wave.lineTo (area.getX() + (float) i, mid - std::min (1.0f, pk[(size_t) i]) * half);
        for (int i = n - 1; i >= 0; --i) wave.lineTo (area.getX() + (float) i, mid + std::min (1.0f, pk[(size_t) i]) * half);
        wave.closeSubPath();
        return wave;
    };

    const auto rumbleCol = accent.withRotatedHue (-0.08f).darker (0.4f);
    const auto bassCol = accent.withRotatedHue (0.45f).brighter (0.2f);
    const auto rumblePath = makePath (rumbleS), bassPath = makePath (bassS), kickPath = makePath (kickS);
    const float kickA = mode == 0 ? 1.0f : 0.35f;
    auto cutout = [&] (const juce::Path& p, juce::Colour c, float alpha)
    {
        g.setColour (juce::Colours::black.withAlpha (0.22f * alpha));
        g.fillPath (p, juce::AffineTransform::translation (2.0f, 3.0f));
        g.setColour (c.withAlpha (alpha));
        g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.8f * alpha));
        g.strokePath (p, juce::PathStrokeType (1.5f));
    };
    cutout (rumblePath, rumbleCol, mode == 1 ? 1.0f : 0.5f);
    cutout (bassPath, bassCol, mode == 2 ? 1.0f : 0.65f);
    cutout (kickPath, accent.interpolatedWith (juce::Colours::white, flash * 0.3f), kickA);

    if (progress >= 0.0f && progress <= 1.0f)
    {
        g.saveState();
        g.reduceClipRegion (area.withWidth (area.getWidth() * progress).toNearestInt());
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.fillPath (kickPath); g.fillPath (bassPath);
        g.restoreState();
        const float x = area.getX() + area.getWidth() * progress;
        g.setColour (juce::Colours::white);
        g.fillRect (x - 1.0f, area.getY() - 6, 2.0f, area.getHeight() + 12);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillRect (x - 6.0f, area.getY() - 6, 12.0f, area.getHeight() + 12);
    }

    // transient shaper line + draggable nodes
    {
        juce::Path line;
        line.startNewSubPath (nodePos (0));
        for (int i = 1; i < 5; ++i) line.lineTo (nodePos (i));
        const float endMs = mode == 0 ? cached.length : mode == 1 ? spanMs : nodeMs (cached, 1.0f);
        const float endX = area.getX() + std::min (1.0f, endMs / std::max (1.0f, spanMs)) * area.getWidth();
        line.lineTo (std::max (endX, nodePos (4).x), nodePos (4).y);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.strokePath (line, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.strokePath (line, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        for (int i = 0; i < 5; ++i)
        {
            const auto pt = nodePos (i);
            const bool hot = i == hoverNode || i == dragNode;
            const float r = hot ? 8.0f : 5.5f;
            if (hot)
            {
                g.setColour (accent.withAlpha (0.35f));
                g.fillEllipse (pt.x - r * 2, pt.y - r * 2, r * 4, r * 4);
            }
            g.setColour (ui::bg);
            g.fillEllipse (pt.x - r, pt.y - r, r * 2, r * 2);
            g.setColour (hot ? accent : juce::Colours::white);
            g.drawEllipse (pt.x - r, pt.y - r, r * 2, r * 2, 2.0f);
            if (i == dragNode)
            {
                const float db = 20.0f * std::log10 (std::max (0.001f, nodeG (cached)[i]));
                g.setFont (uiFont (11.0f, true));
                g.setColour (textC);
                g.drawText ((db > 0 ? "+" : "") + juce::String (db, 1) + " dB  " + juce::String (juce::roundToInt (nodeMs (cached, nodeT (cached)[i]))) + " ms",
                            juce::Rectangle<float> (pt.x - 60, pt.y < area.getY() + 24 ? pt.y + 12 : pt.y - 28, 120, 14).toNearestInt(), juce::Justification::centred);
            }
        }
    }

    g.setFont (uiFont (12.0f, true));
    auto info = b.reduced (16.0f, 6.0f);
    g.setColour (dimC);
    g.drawText (juce::String (juce::roundToInt (cached.length)) + " ms", info.toNearestInt(), juce::Justification::topLeft);
    g.drawText (noteName (cached.pitchEnd) + "  " + juce::String (cached.pitchEnd, 1) + " Hz  -  "
                + juce::String (juce::roundToInt (cachedBpm)) + " BPM", info.toNearestInt(), juce::Justification::topRight);

    int lx = (int) info.getX();
    auto legend = [&] (const juce::String& t, juce::Colour c)
    {
        g.setColour (c);
        g.fillEllipse ((float) lx, info.getBottom() - 9.0f, 7.0f, 7.0f);
        g.setColour (dimC);
        g.drawText (t, lx + 11, (int) info.getBottom() - 13, 70, 14, juce::Justification::centredLeft);
        lx += 80;
    };
    g.setColour (dimC);
    if (! throat) g.drawText (mode == 0 ? "drag points: kick transient" : mode == 1 ? "drag points: rumble shape over the beat" : "drag points: bass note envelope",
                info.toNearestInt(), juce::Justification::bottomRight);
    legend ("KICK", accent);
    if (cached.bassLevel > 0.0001f) legend ("BASS", bassCol);
    if (cached.rumbleAmt > 0.0001f) legend ("RUMBLE", rumbleCol);
}

//==============================================================================
static juce::String fmtHz (float v) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }
static juce::String fmtMs (float v) { return v < 10.0f ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms"; }
static juce::String fmtDb (float v) { return juce::String (v, 1) + " dB"; }
static juce::String fmtPct (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
static juce::String fmtX (float v) { return juce::String (v, 2); }
static juce::String fmtSt (float v) { return (v > 0 ? "-" : (v < 0 ? "+" : "")) + juce::String (std::abs (v), 1) + " st"; }
static juce::String fmtSemi (float v) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; }

static void drawDie (juce::Graphics& g, juce::Rectangle<float> r, int face, float spin, juce::Colour fg)
{
    const float s = std::min (r.getWidth(), r.getHeight());
    auto box = juce::Rectangle<float> (s, s).withCentre (r.getCentre());
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (spin, box.getCentreX(), box.getCentreY()));
    g.setColour (fg);
    g.drawRoundedRectangle (box, s * 0.22f, 2.2f);
    const float d = s * 0.17f;
    auto pip = [&] (float px, float py) { g.fillEllipse (box.getX() + px * s - d / 2, box.getY() + py * s - d / 2, d, d); };
    const float a = 0.28f, m = 0.5f, z = 0.72f;
    if (face % 2 == 1) pip (m, m);
    if (face >= 2) { pip (a, a); pip (z, z); }
    if (face >= 4) { pip (z, a); pip (a, z); }
    if (face == 6) { pip (a, m); pip (z, m); }
    g.restoreState();
}

static void drawArrow (juce::Graphics& g, juce::Rectangle<float> r, bool right)
{
    juce::Path p;
    const auto c = r.getCentre();
    const float s = std::min (r.getWidth(), r.getHeight()) * 0.35f;
    if (right) p.addTriangle (c.x - s * 0.6f, c.y - s, c.x - s * 0.6f, c.y + s, c.x + s * 0.8f, c.y);
    else       p.addTriangle (c.x + s * 0.6f, c.y - s, c.x + s * 0.6f, c.y + s, c.x - s * 0.8f, c.y);
    g.fillPath (p);
}

std::unique_ptr<juce::ParameterAttachment> CiroKickEditor::attachCycle (FunButton& btn, const char* id, int count)
{
    auto* param = proc.apvts.getParameter (id);
    auto att = std::make_unique<juce::ParameterAttachment> (*param, [&btn, param] (float)
    {
        btn.label = param->getCurrentValueAsText();
        btn.repaint();
    });
    att->sendInitialUpdate();
    auto* a = att.get();
    btn.onClick = [a, param, count, &btn]
    {
        const int cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
        a->setValueAsCompleteGesture ((float) ((cur + 1) % count));
        btn.glow = 0.6f;
    };
    btn.onRightClick = [a, param, count]
    {
        const int cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
        a->setValueAsCompleteGesture ((float) ((cur + count - 1) % count));
    };
    return att;
}

CiroKickEditor::CiroKickEditor (CiroKickProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p), xyPad (p), stepStrip (p)
{
    setLookAndFeel (&lnf);

    struct K { const char* id; const char* name; juce::String (*fmt) (float); };
    const K defs[] = {
        { "pStart", "START", fmtHz }, { "pEnd", "TUNE", fmtHz }, { "pDecay", "SWEEP", fmtMs },
        { "pCurve", "CURVE", fmtX },  { "bend", "BEND", fmtSt },
        { "length", "LENGTH", fmtMs }, { "hold", "HOLD", fmtMs }, { "shape", "SHAPE", fmtX }, { "wave", "WAVE", fmtPct },
        { "cLevel", "LEVEL", fmtPct }, { "cDecay", "SNAP", fmtMs }, { "cTone", "TONE", fmtHz },
        { "drive", "DRIVE", fmtDb }, { "dMix", "MIX", fmtPct }, { "tone", "FILTER", fmtHz },
        { "reso", "RESO", fmtX }, { "punch", "PUNCH", fmtMs }, { "rRoll", "ROLL", fmtPct },
        { "sub", "SUB", fmtPct }, { "metal", "METAL", fmtPct }, { "mTone", "M.TONE", fmtHz },
        { "rPre", "PRE-DLY", fmtMs }, { "rDamp", "DAMP", fmtPct }, { "rLow", "LOW CUT", fmtHz }, { "rGrit", "GRIT", fmtPct },
        { "bAtk", "ATTACK", fmtMs }, { "bEnv", "ENV", fmtX }, { "bReso", "RESO", fmtX }, { "bDrive", "DRIVE", fmtDb },
        { "bGlide", "GLIDE", fmtSemi },
        { "volume", "VOLUME", fmtDb },
        { "rAmt", "AMOUNT", fmtPct }, { "rSize", "SIZE", fmtPct }, { "rDrive", "DIRT", fmtDb },
        { "rTone", "TONE", fmtHz }, { "rDuck", "DUCK", fmtPct },
        { "bLevel", "LEVEL", fmtPct }, { "bSemi", "NOTE", fmtSemi }, { "bDecay", "DECAY", fmtMs }, { "bCut", "CUTOFF", fmtHz },
    };
    std::vector<std::pair<juce::String, K>> all;
    for (auto& d : defs) all.push_back ({ d.id, d });
    static const K layerKnobs[] = { { "Lvl", "LEVEL", fmtPct }, { "Tune", "TUNE", fmtSemi }, { "Dec", "DECAY", fmtMs },
                                    { "Drv", "DRIVE", fmtDb }, { "Tone", "TONE", fmtHz }, { "Start", "START", fmtMs } };
    for (int i = 0; i < numLayers; ++i)
        for (auto& lk : layerKnobs) all.push_back ({ "L" + juce::String (i) + lk.id, lk });
    for (auto& [id, d] : all)
    {
        auto k = std::make_unique<FunKnob> (p.apvts, id, d.name, d.fmt);
        addAndMakeVisible (*k);
        knobById[id] = k.get();
        knobs.push_back (std::move (k));
    }

    randomBtn.label = "RANDOM";
    randomBtn.filled = true;
    randomBtn.drawIcon = [this] (juce::Graphics& g, juce::Rectangle<float> r) { drawDie (g, r, dieFace, dieSpin, ui::bg); };
    randomBtn.onClick = [this]
    {
        proc.randomize();
        head.dizzy();
        dieFace = 1 + juce::Random::getSystemRandom().nextInt (6);
        dieSpin = juce::MathConstants<float>::pi * 1.5f;
        randomBtn.pop (0.25f);
        randomBtn.glow = 1.0f;
        particles.burst (randomBtn.getBounds().getCentre().toFloat(), accent, 40, 7.0f);
        bgFlash = 1.0f;
    };
    addAndMakeVisible (randomBtn);

    undoBtn.drawIcon = [] (juce::Graphics& g, juce::Rectangle<float> r)
    {
        juce::Path a;
        auto c = r.getCentre();
        const float s = std::min (r.getWidth(), r.getHeight()) * 0.45f;
        a.addCentredArc (c.x, c.y, s, s, 0, -2.2f, 1.6f, true);
        g.strokePath (a, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const auto tip = c.getPointOnCircumference (s, -2.2f);
        juce::Path head;
        head.addTriangle (tip.x - 5, tip.y - 1, tip.x + 3, tip.y - 5, tip.x + 2, tip.y + 4);
        g.fillPath (head);
    };
    undoBtn.onClick = [this] { if (! proc.undoRandom()) undoBtn.pop (0.3f); };
    addAndMakeVisible (undoBtn);

    presetBtn.fontSize = 14.0f;
    presetBtn.onClick = [this] { showPresetMenu(); };
    prevBtn.drawIcon = [] (juce::Graphics& g, juce::Rectangle<float> r) { drawArrow (g, r, false); };
    nextBtn.drawIcon = [] (juce::Graphics& g, juce::Rectangle<float> r) { drawArrow (g, r, true); };
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (presetBtn); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);

    playBtn.label = "PLAY";
    playBtn.onClick = [this] { proc.triggerFromUI(); };
    addAndMakeVisible (playBtn);

    distTypeAtt = attachCycle (distTypeBtn, "dType", 4);
    bassPatAtt = attachCycle (bassPatBtn, "bPat", 4);
    bassPatBtn.fontSize = 12.0f;
    addAndMakeVisible (distTypeBtn);
    addAndMakeVisible (bassPatBtn);

    keyTrackBtn.label = "KEY";
    keyTrackAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("keyTrack"), [this] (float v)
    {
        keyTrackBtn.selected = v > 0.5f;
        keyTrackBtn.repaint();
    });
    keyTrackAtt->sendInitialUpdate();
    keyTrackBtn.onClick = [this] { keyTrackAtt->setValueAsCompleteGesture (keyTrackBtn.selected ? 0.0f : 1.0f); };
    addAndMakeVisible (keyTrackBtn);

    const auto& profiles = genreProfiles();
    for (int i = 0; i < (int) profiles.size(); ++i)
    {
        auto b = std::make_unique<FunButton>();
        b->label = profiles[(size_t) i].name;
        b->colour = juce::Colour (profiles[(size_t) i].colour);
        auto* raw = b.get();
        b->onClick = [this, i, raw]
        {
            genreAtt->setValueAsCompleteGesture ((float) i);
            particles.burst (raw->getBounds().getCentre().toFloat(), raw->colour, 18, 4.5f);
        };
        addAndMakeVisible (*b);
        genreBtns.push_back (std::move (b));
    }
    genreAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("genre"), [this] (float v)
    {
        for (int i = 0; i < (int) genreBtns.size(); ++i)
        {
            genreBtns[(size_t) i]->selected = i == juce::roundToInt (v);
            genreBtns[(size_t) i]->repaint();
        }
    });
    genreAtt->sendInitialUpdate();

    // Ciro's head: click it to spin and kick
    head.onClick = [this]
    {
        proc.triggerFromUI();
        particles.burst (head.getBounds().getCentre().toFloat().translated (0, -120), accent, 30, 6.0f);
    };
    addAndMakeVisible (head);

    // MUTATE: small nudge of the current sound
    mutateBtn.drawIcon = [] (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto c = r.getCentre();
        const float s = std::min (r.getWidth(), r.getHeight()) * 0.5f;
        juce::Path star;
        star.addStar (c, 4, s * 0.35f, s, 0.0f);
        g.fillPath (star);
        star.clear();
        star.addStar (c.translated (s * 0.85f, -s * 0.75f), 4, s * 0.12f, s * 0.35f, 0.0f);
        g.fillPath (star);
    };
    mutateBtn.onClick = [this]
    {
        proc.mutate();
        head.shake();
        mutateBtn.glow = 1.0f;
        particles.burst (mutateBtn.getBounds().getCentre().toFloat(), accent.withRotatedHue (0.15f), 16, 4.0f);
    };
    addAndMakeVisible (mutateBtn);

    // LOOP + steps

    loopAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("loop"), [this] (float v)
    {
        loopBtn.selected = v > 0.5f;
        loopBtn.repaint();
    });
    loopAtt->sendInitialUpdate();
    loopBtn.onClick = [this] { loopAtt->setValueAsCompleteGesture (loopBtn.selected ? 0.0f : 1.0f); loopBtn.glow = 1.0f; };
    addAndMakeVisible (loopBtn);
    loopLessBtn.label = "-";
    loopMoreBtn.label = "+";
    loopLessBtn.fontSize = loopMoreBtn.fontSize = 20.0f;
    auto changeLen = [this] (int d)
    {
        const int n = proc.getLoopLength();
        const int next = d > 0 ? (n < 4 ? n + 1 : std::min (32, n + 4)) : (n <= 4 ? std::max (1, n - 1) : n - 4);
        proc.setLoopLength (next);
        loopBtn.label = "LOOP " + juce::String (next);
        loopBtn.glow = 0.6f;
        stepStrip.repaint();
    };
    loopLessBtn.onClick = [changeLen] { changeLen (-1); };
    loopMoreBtn.onClick = [changeLen] { changeLen (1); };
    loopBtn.label = "LOOP " + juce::String (proc.getLoopLength());
    addAndMakeVisible (loopLessBtn);
    addAndMakeVisible (loopMoreBtn);
    addAndMakeVisible (stepStrip);
    addAndMakeVisible (xyPad);

    rRateAtt = attachCycle (rRateBtn, "rRate", 3);
    bOscAtt = attachCycle (bOscBtn, "bOsc", 4);
    bOscBtn.fontSize = 12.0f;
    addAndMakeVisible (rRateBtn);
    addAndMakeVisible (bOscBtn);

    const char* tabNames[4] = { "KICK", "RUMBLE", "BASS", "BUILDER" };
    for (int i = 0; i < 4; ++i)
    {
        tabBtns[i].label = tabNames[i];
        tabBtns[i].drawIcon = [this, i] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            const auto k = proc.readParams();
            bool anyLayer = false;
            for (auto& l : k.layers) anyLayer = anyLayer || l.level > 0.0001f;
            const bool on = i == 0 || (i == 1 ? k.rumbleAmt > 0.0001f : i == 2 ? k.bassLevel > 0.0001f : anyLayer);
            const auto c = r.getCentre();
            const float rad = std::min (r.getWidth(), r.getHeight()) * 0.32f;
            if (on) g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
            else g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, 1.5f);
        };
        tabBtns[i].onClick = [this, i] { setTab (i); };
        addAndMakeVisible (tabBtns[i]);
    }

    // kick builder: layer type (click = next, right click = previous) + dice
    auto dieIcon = [] (juce::Graphics& g, juce::Rectangle<float> r) { drawDie (g, r, 5, 0.0f, paper::ink); };
    for (int i = 0; i < numLayers; ++i)
    {
        auto& st = layerStacks[i];
        st.top.fontSize = 10.0f;
        st.bottom.drawIcon = dieIcon;
        const int n = (int) layerTypeNames (layerRole (i)).size();
        auto cycle = [this, i, n] (int d)
        {
            const int cur = proc.readParams().layers[i].type;
            proc.setLayerType (i, (cur + d + n) % n);
            flashLayer (layerStacks[i].top);
        };
        st.top.onClick = [cycle] { cycle (1); };
        st.top.onRightClick = [cycle] { cycle (-1); };
        st.bottom.onClick = [this, i] { proc.rollLayer (i); flashLayer (layerStacks[i].bottom); };
        addChildComponent (st);
    }
    {
        const int n = (int) bodyCatNames().size();
        bodyBtn.fontSize = 11.0f;
        bodyBtn.onClick = [this, n] { proc.setLayerType (-1, (proc.readParams().bodyCat + 1) % n); flashLayer (bodyBtn); };
        bodyBtn.onRightClick = [this, n] { proc.setLayerType (-1, (proc.readParams().bodyCat + n - 1) % n); flashLayer (bodyBtn); };
        bodyDice.drawIcon = dieIcon;
        bodyDice.onClick = [this] { proc.rollLayer (-1); flashLayer (bodyDice); };
        buildBtn.label = "BUILD!";
        buildBtn.filled = true;
        buildBtn.fontSize = 14.0f;
        buildBtn.onClick = [this]
        {
            proc.buildAll();
            head.dizzy();
            flashLayer (buildBtn);
            particles.burst (buildBtn.getBounds().getCentre().toFloat(), accent, 40, 7.0f);
            bgFlash = 1.0f;
        };
        addChildComponent (bodyStack); addChildComponent (buildBtn);
    }

    // lower teeth: loop controls + hit
    hitBtn.label = "HIT!";
    hitBtn.onClick = [this] { proc.triggerFromUI(); };
    for (auto* t : { &loopBtn, &loopLessBtn, &loopMoreBtn, &hitBtn }) { t->upper = false; t->fontSize = 13.0f; }
    loopLessBtn.fontSize = loopMoreBtn.fontSize = 20.0f;
    addAndMakeVisible (hitBtn);
    for (auto& t : tabBtns) t.fontSize = 13.0f;

    display.throat = true;
    addAndMakeVisible (display);
    for (ToothButton* t : { &loopBtn, &loopLessBtn, &loopMoreBtn, &hitBtn, &tabBtns[0], &tabBtns[1], &tabBtns[2], &tabBtns[3] }) t->toFront (false);
    addAndMakeVisible (particles);
    head.toBack();
    lastHit = proc.hitCounter.load();
    setSize (1200, 800);
    setTab (0);
    startTimerHz (60);
}

void CiroKickEditor::flashLayer (juce::Component& c)
{
    if (auto* b = dynamic_cast<FunButton*> (&c)) { b->glow = 1.0f; b->pop (0.2f); }
    particles.burst (c.getScreenBounds().getCentre().toFloat() - getScreenPosition().toFloat(), accent, 16, 4.5f);
}

CiroKickEditor::~CiroKickEditor()
{
    setLookAndFeel (nullptr);
}

void CiroKickEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& bank = presetBank();
    const int curGenre = proc.getGenre();
    for (int gi = 0; gi < numGenres; ++gi)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < 20; ++i)
        {
            if (i % 5 == 0 && i > 0) sub.addSeparator();
            sub.addItem (gi * 100 + i + 1, bank.names[(size_t) gi][(size_t) i], true,
                         gi == curGenre && i == proc.presetIndex);
        }
        menu.addSubMenu (genreProfiles()[(size_t) gi].name, sub, true, nullptr, gi == curGenre);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetBtn).withMinimumWidth (presetBtn.getWidth()),
                        [this] (int r)
                        {
                            if (r <= 0) return;
                            proc.loadPreset ((r - 1) / 100, (r - 1) % 100);
                            presetBtn.glow = 1.0f;
                            particles.burst (presetBtn.getBounds().getCentre().toFloat(), accent, 24, 5.0f);
                        });
}

void CiroKickEditor::stepPreset (int delta)
{
    const int idx = proc.presetIndex < 0 ? (delta > 0 ? 0 : 19) : (proc.presetIndex + delta + 20) % 20;
    proc.loadPreset (proc.getGenre(), idx);
    presetBtn.glow = 0.8f;
    presetBtn.pop (0.1f);
}

void CiroKickEditor::paint (juce::Graphics& g)
{
    if (background.getWidth() != getWidth() || background.getHeight() != getHeight())
        background = paper::cardboard (getWidth(), getHeight()), paper::scraps (background);
    g.drawImageAt (background, 0, 0);

    // paper sheets behind each section
    for (auto& [name, r] : sectionLabels)
    {
        auto sheet = r.toFloat().withHeight (126.0f).expanded (8.0f, 2.0f).withTrimmedTop (-2.0f);
        uint32_t seed = paper::seedOf (r);
        paper::piece (g, sheet, ui::panel.darker (0.02f * (float) (seed % 3)), seed, 1.0f, 4.0f, 1.8f);
    }
    if (bgFlash > 0.01f)
    {
        juce::ColourGradient grad (accent.withAlpha (0.18f * bgFlash), (float) getWidth() * 0.5f, 210.0f,
                                   accent.withAlpha (0.0f), (float) getWidth() * 0.5f, 700.0f, true);
        g.setGradientFill (grad);
        g.fillRect (getLocalBounds());
    }

    const auto titleFont = uiFont (30.0f, true);
    const int tx = 34;
    {
        const float tw = juce::GlyphArrangement::getStringWidth (titleFont, "C.I.R.O KICK") + 24.0f;
        paper::piece (g, { (float) tx - 10, 16.0f, tw, 54.0f }, ui::panel, 4711u, 1.2f, 3.0f, 2.2f);
    }
    g.setFont (titleFont);
    g.setColour (ui::text);
    g.drawText ("C.I.R.O", tx, 18, 140, 40, juce::Justification::centredLeft);
    g.setColour (accent);
    g.drawText ("KICK", tx + juce::roundToInt (juce::GlyphArrangement::getStringWidth (titleFont, "C.I.R.O ")), 18, 100, 40,
                juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (uiFont (11.0f));
    g.drawText ("kick + bass + rumble generator", tx + 2, 52, 240, 14, juce::Justification::centredLeft);

    g.setFont (uiFont (12.0f, true));
    for (auto& [name, r] : sectionLabels)
    {
        // masking-tape label
        const float w = juce::GlyphArrangement::getStringWidth (uiFont (12.0f, true), name) + 22.0f;
        uint32_t seed = paper::seedOf (r) ^ 5u;
        const float tilt = (paper::rnd (seed) - 0.5f) * 0.08f;
        paper::tapeStrip (g, { (float) r.getX() + w / 2, (float) r.getY() + 6 }, tilt, w, 18, seed);
        g.setColour (paper::ink);
        g.drawText (name, juce::Rectangle<int> (r.getX(), r.getY() - 3, (int) w, 18), juce::Justification::centred);
    }
}

void CiroKickEditor::layoutRow (std::vector<std::vector<juce::Component*>> groups, int y, int cell)
{
    const int gap = 24;
    int total = -gap;
    for (auto& grp : groups) total += (int) grp.size() * cell + gap;
    int x = (getWidth() - total) / 2;
    for (auto& grp : groups)
    {
        for (auto* c : grp)
        {
            if (dynamic_cast<FunButton*> (c) != nullptr) c->setBounds (x + 6, y + 40, cell - 12, 36);
            else c->setBounds (x, y + 22, cell, 92);
            x += cell;
        }
        x += gap;
    }
}

void CiroKickEditor::resized()
{
    particles.setBounds (getLocalBounds());
    const int W = getWidth();

    // left column: title, genres, XY pad
    const int colW = 280;
    const int pillW = (colW - 8) / 2;
    for (int i = 0; i < (int) genreBtns.size(); ++i)
        genreBtns[(size_t) i]->setBounds (20 + (i % 2) * (pillW + 8), 88 + (i / 2) * 40, pillW, 34);
    xyPad.setBounds (20, 256, colW, 214);

    // right column: preset bar, undo/mutate/random, play pad, step grid
    const int rx = W - 20 - colW;
    prevBtn.setBounds (rx, 18, 40, 38);
    presetBtn.setBounds (rx + 42, 18, colW - 84, 38);
    nextBtn.setBounds (rx + colW - 40, 18, 40, 38);
    undoBtn.setBounds (rx, 64, 52, 46);
    mutateBtn.setBounds (rx + 56, 64, 52, 46);
    randomBtn.setBounds (rx + 112, 62, colW - 112, 50);
    playBtn.setBounds (rx, 120, colW, 168);
    stepStrip.setBounds (rx, 296, colW, 174);

    // centre: the head, with the UI inside its mouth
    head.setBounds (20 + colW + 6, 0, rx - 26 - colW - 6 + 20, 540);
    mouthRect = juce::Rectangle<int> (0, 0, 380, 196).withCentre ({ W / 2, 0 }).withY (284);
    head.setMouth (mouthRect.translated (-head.getX(), -head.getY()).toFloat());
    placeMouth();

    loopBtn.label = "LOOP " + juce::String (proc.getLoopLength());

    // hide every tab control, then lay out the current tab
    for (auto& kn : knobs) kn->setVisible (false);
    for (auto* b : { &distTypeBtn, &keyTrackBtn, &bassPatBtn, &rRateBtn, &bOscBtn }) b->setVisible (false);
    for (auto& st : layerStacks) st.setVisible (false);
    bodyStack.setVisible (false); buildBtn.setVisible (false);

    auto k = [this] (const char* id) -> juce::Component* { return knobById[id]; };
    using Group = std::pair<const char*, std::vector<juce::Component*>>;
    auto row = [&] (std::vector<Group> groups, int y, int cell = 80)
    {
        std::vector<std::vector<juce::Component*>> comps;
        for (auto& g : groups) { comps.push_back (g.second); for (auto* c : g.second) c->setVisible (true); }
        layoutRow (comps, y, cell);
        for (auto& g : groups)
        {
            auto* first = g.second.front(); auto* last = g.second.back();
            const int x0 = dynamic_cast<FunButton*> (first) ? first->getX() - 6 : first->getX();
            const int x1 = dynamic_cast<FunButton*> (last) ? last->getRight() + 6 : last->getRight();
            sectionLabels.push_back ({ g.first, juce::Rectangle<int> (x0 + 6, y, x1 - x0 - 12, 20) });
        }
    };
    sectionLabels.clear();
    const int y1 = 540, y2 = 666;
    if (tab == 0)
    {
        row ({ { "PITCH", { k ("pStart"), k ("pEnd"), k ("pDecay"), k ("pCurve"), k ("bend") } },
               { "BODY", { k ("sub"), k ("length"), k ("hold"), k ("shape"), k ("wave") } },
               { "OUT", { k ("volume"), &keyTrackBtn } } }, y1);
        row ({ { "CLICK", { k ("cLevel"), k ("cDecay"), k ("cTone"), k ("metal"), k ("mTone") } },
               { "DRIVE", { &distTypeBtn, k ("drive"), k ("dMix"), k ("punch"), k ("tone"), k ("reso") } } }, y2);
    }
    else if (tab == 1)
    {
        row ({ { "REVERB", { k ("rAmt"), k ("rSize"), k ("rPre"), k ("rDamp") } },
               { "FILTER", { k ("rTone"), k ("rLow") } } }, y1);
        row ({ { "TEXTURE", { k ("rDrive"), k ("rGrit") } },
               { "MOTION", { k ("rDuck"), k ("rRoll"), &rRateBtn } } }, y2);
    }
    else if (tab == 3)
    {
        auto layer = [&] (int i) -> std::vector<juce::Component*>
        {
            const auto L = "L" + juce::String (i);
            return { &layerStacks[i], k ((L + "Lvl").toRawUTF8()), k ((L + "Tune").toRawUTF8()), k ((L + "Dec").toRawUTF8()),
                     k ((L + "Drv").toRawUTF8()), k ((L + "Tone").toRawUTF8()), k ((L + "Start").toRawUTF8()) };
        };
        row ({ { "BODY", { &bodyStack } }, { "PUNCH", layer (0) }, { "TAIL", layer (1) } }, y1, 70);
        row ({ { "BUILD", { &buildBtn } }, { "EXTRA A", layer (2) }, { "EXTRA B", layer (3) } }, y2, 70);
    }
    else
    {
        row ({ { "PATTERN", { &bassPatBtn, &bOscBtn } },
               { "NOTE", { k ("bLevel"), k ("bSemi"), k ("bGlide") } },
               { "ENVELOPE", { k ("bAtk"), k ("bDecay") } } }, y1);
        row ({ { "FILTER", { k ("bCut"), k ("bReso"), k ("bEnv") } },
               { "DRIVE", { k ("bDrive") } } }, y2);
    }
    repaint();
}

void CiroKickEditor::placeMouth()
{
    const int ox = juce::roundToInt (head.overlayOffsetX());
    const int jaw = juce::roundToInt (head.jawOffset());
    const auto m = mouthRect.translated (ox, 0);
    const int tw = m.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        tabBtns[i].setBounds (m.getX() + 14 + i * (tw - 7), m.getY() - 2, tw - 9, 40);
    const int bw = juce::roundToInt (m.getWidth() * 0.78f), bx = m.getCentreX() - bw / 2, lw = bw / 4;
    ToothButton* lower[4] = { &loopLessBtn, &loopBtn, &loopMoreBtn, &hitBtn };
    const int widths[4] = { lw - 22, lw + 34, lw - 22, lw + 10 };
    int x = bx + 2;
    for (int i = 0; i < 4; ++i) { lower[i]->setBounds (x, m.getBottom() - 38 + jaw, widths[i], 38); x += widths[i] + 3; }
    display.setBounds (m.getX() + 26, m.getY() + 42, m.getWidth() - 52, std::max (10, m.getHeight() - 84 + jaw));
    const float a = head.overlayAlpha();
    display.setAlpha (a);
    for (ToothButton* t : { &loopBtn, &loopLessBtn, &loopMoreBtn, &hitBtn, &tabBtns[0], &tabBtns[1], &tabBtns[2], &tabBtns[3] }) t->setAlpha (std::max (0.0f, (a - 0.2f) / 0.8f));
}

void CiroKickEditor::setTab (int t)
{
    if (t != tab) head.chomp();
    tab = t;
    for (int i = 0; i < 4; ++i) { tabBtns[i].selected = i == t; tabBtns[i].repaint(); }
    tabBtns[t].glow = 1.0f;
    tabBtns[t].pop (0.15f);
    display.setMode (t == 3 ? 0 : t);
    particles.burst (tabBtns[t].getBounds().getCentre().toFloat(), accent, 14, 4.0f);
    resized();
}

void CiroKickEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();

    const auto target = juce::Colour (genreProfiles()[(size_t) proc.getGenre()].colour);
    if (accent != target)
    {
        accent = accent.interpolatedWith (target, 0.18f);
        if (std::abs (accent.getFloatRed() - target.getFloatRed()) + std::abs (accent.getFloatGreen() - target.getFloatGreen())
            + std::abs (accent.getFloatBlue() - target.getFloatBlue()) < 0.01f)
            accent = target;
        for (FunButton* b : std::initializer_list<FunButton*> { &randomBtn, &undoBtn, &distTypeBtn, &keyTrackBtn, &bassPatBtn, &presetBtn, &prevBtn, &nextBtn, &mutateBtn, &loopBtn,
                         &rRateBtn, &bOscBtn, &tabBtns[0], &tabBtns[1], &tabBtns[2], &tabBtns[3], &loopLessBtn, &loopMoreBtn,
                         &bodyBtn, &bodyDice, &buildBtn, &layerStacks[0].top, &layerStacks[1].top, &layerStacks[2].top, &layerStacks[3].top,
                         &layerStacks[0].bottom, &layerStacks[1].bottom, &layerStacks[2].bottom, &layerStacks[3].bottom })
        { b->colour = accent; b->repaint(); }
        head.accent = xyPad.accent = stepStrip.accent = accent;
        xyPad.repaint(); stepStrip.repaint();
        playBtn.colour = accent; playBtn.repaint();
        repaint (0, 0, 340, 76);
    }

    if (tab == 3 && (frame % 6) == 0)
    {
        const auto kp = proc.readParams();
        for (int i = 0; i < numLayers; ++i)
        {
            auto& names = layerTypeNames (layerRole (i));
            const juce::String lbl = names[(size_t) std::clamp (kp.layers[i].type, 0, (int) names.size() - 1)];
            const bool on = kp.layers[i].level > 0.0001f;
            auto& b = layerStacks[i].top;
            if (b.label != lbl || b.selected != on) { b.label = lbl; b.selected = on; b.repaint(); }
        }
        const juce::String bl (bodyCatNames()[(size_t) std::clamp (kp.bodyCat, 0, (int) bodyCatNames().size() - 1)]);
        if (bodyBtn.label != bl) { bodyBtn.label = bl; bodyBtn.repaint(); }
    }

    const auto name = proc.getPresetName();
    if (presetBtn.label != name) { presetBtn.label = name; presetBtn.repaint(); }

    const int hits = proc.hitCounter.load();
    if (hits != lastHit)
    {
        lastHit = hits;
        display.hit (now);
        head.hit();
        playBtn.hit();
        particles.burst (playBtn.getBounds().getCentre().toFloat(), accent, 10, 3.5f);
    }

    if (dieSpin > 0.0f) { dieSpin = std::max (0.0f, dieSpin * 0.82f - 0.01f); randomBtn.repaint(); }
    if (bgFlash > 0.0f) { bgFlash = std::max (0.0f, bgFlash - 0.04f); repaint(); }

    display.update (accent, now);
    for (FunButton* b : std::initializer_list<FunButton*> { &randomBtn, &undoBtn, &distTypeBtn, &keyTrackBtn, &bassPatBtn, &presetBtn, &prevBtn, &nextBtn, &mutateBtn, &loopBtn,
                     &rRateBtn, &bOscBtn, &tabBtns[0], &tabBtns[1], &tabBtns[2], &tabBtns[3], &loopLessBtn, &loopMoreBtn, &hitBtn,
                     &bodyBtn, &bodyDice, &buildBtn, &layerStacks[0].top, &layerStacks[1].top, &layerStacks[2].top, &layerStacks[3].top,
                     &layerStacks[0].bottom, &layerStacks[1].bottom, &layerStacks[2].bottom, &layerStacks[3].bottom })
        b->animate();
    if ((++frame % 10) == 0) for (auto& t : tabBtns) t.repaint();
    {
        const auto kp = proc.readParams();
        head.drive01 = juce::jlimit (0.0f, 1.0f, kp.drive / 34.0f);
        head.rumble01 = kp.rumbleAmt;
        head.lookAt (getMouseXYRelative().toFloat() - head.getPosition().toFloat());
        if (head.animate()) placeMouth();
    }
    xyPad.animate(); stepStrip.animate();
    playBtn.animate();
    for (auto& b : genreBtns) b->animate();
    for (auto& kn : knobs) kn->animate (accent);
    particles.step();
}
