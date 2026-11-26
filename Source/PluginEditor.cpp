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

    // glow when touched or freshly changed
    const float glowAmt = std::max (flash, active ? 0.35f : 0.0f);
    if (glowAmt > 0.01f)
    {
        juce::ColourGradient grad (accent.withAlpha (0.45f * glowAmt), c.x, c.y, accent.withAlpha (0.0f), c.x + r + 10, c.y, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r - 10, c.y - r - 10, (r + 10) * 2, (r + 10) * 2);
    }

    juce::Path trackArc, valueArc;
    trackArc.addCentredArc (c.x, c.y, r - 3, r - 3, 0, start, end, true);
    g.setColour (ui::track);
    g.strokePath (trackArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = slider.getName() == "bipolar";
    const float from = bipolar ? start + (float) slider.valueToProportionOfLength (0.0) * (end - start) : start;
    valueArc.addCentredArc (c.x, c.y, r - 3, r - 3, 0, std::min (from, angle), std::max (from, angle), true);
    g.setColour (accent.interpolatedWith (juce::Colours::white, flash * 0.5f));
    g.strokePath (valueArc, juce::PathStrokeType (4.0f + 2.0f * flash, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = r - 10.0f;
    g.setColour (active ? ui::track.brighter (0.15f) : ui::track);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);

    const auto dot = c.getPointOnCircumference (br - 7.0f, angle);
    g.setColour (active || flash > 0.2f ? accent : ui::text);
    g.fillEllipse (dot.x - 3.5f, dot.y - 3.5f, 7.0f, 7.0f);

    g.setColour (active ? ui::text : ui::dim);
    g.setFont (uiFont (11.5f, active));
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
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    const float sc = spring.value;
    g.addTransform (juce::AffineTransform::scale (sc, sc, b.getCentreX(), b.getCentreY()));
    const float corner = std::min (b.getHeight() * 0.5f, 14.0f);

    if (glow > 0.01f)
    {
        g.setColour (colour.withAlpha (0.35f * glow));
        g.fillRoundedRectangle (b.expanded (4.0f * glow), corner + 4.0f);
    }

    const bool on = filled || selected;
    juce::Colour fill = on ? colour : ui::panel;
    if (hover) fill = fill.brighter (on ? 0.12f : 0.25f);
    fill = fill.interpolatedWith (juce::Colours::white, glow * 0.25f);
    g.setColour (fill);
    g.fillRoundedRectangle (b, corner);

    if (! on)
    {
        g.setColour (hover ? colour : ui::track.brighter (0.3f));
        g.drawRoundedRectangle (b, corner, 1.5f);
    }

    const juce::Colour fg = on ? ui::bg : (hover ? colour : ui::text);
    g.setColour (fg);
    const auto font = uiFont (std::min (fontSize, b.getHeight() * 0.42f), true);
    if (drawIcon && label.isEmpty())
    {
        drawIcon (g, b.reduced (9.0f));
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
    for (auto r : rings)
    {
        const float rad = 18.0f + r * b.getWidth() * 0.55f;
        g.setColour (colour.withAlpha ((1.0f - r) * 0.8f));
        g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, 3.0f * (1.0f - r) + 0.5f);
    }
    juce::Path tri;
    const float s = 14.0f;
    tri.addTriangle (c.x - s * 0.6f, c.y - 26 - s, c.x - s * 0.6f, c.y - 26 + s, c.x + s, c.y - 26);
    g.setColour (glow > 0.3f ? juce::Colours::white : colour);
    g.fillPath (tri);
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

    // badge
    g.setColour (accent);
    g.fillEllipse (R (0, 0, 1, 1));
    g.setColour (ui::bg);
    g.drawEllipse (R (0.03f, 0.03f, 0.94f, 0.94f), S * 0.03f);

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
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (b, 16.0f);
    auto area = b.reduced (14.0f);

    // grid that warps towards the puck
    const juce::Point<float> puck (area.getX() + shown.x * area.getWidth(), area.getBottom() - shown.y * area.getHeight());
    g.setColour (ui::track);
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

    const float r = 11.0f + 5.0f * pulse + (dragging ? 3.0f : 0.0f);
    g.setColour (accent.withAlpha (0.25f));
    g.fillEllipse (puck.x - r * 1.8f, puck.y - r * 1.8f, r * 3.6f, r * 3.6f);
    g.setColour (accent);
    g.fillEllipse (puck.x - r, puck.y - r, r * 2, r * 2);
    g.setColour (juce::Colours::white);
    g.fillEllipse (puck.x - 3, puck.y - 3, 6, 6);
}

//==============================================================================
int StepStrip::stepAt (float x) const
{
    return std::clamp ((int) (x / ((float) getWidth() / 16.0f)), 0, 15);
}

void StepStrip::mouseDown (const juce::MouseEvent& e)
{
    const int s = stepAt (e.position.x);
    const int mask = proc.getSteps();
    paintValue = ((mask >> s) & 1) ? 0 : 1;
    proc.setSteps (paintValue ? (mask | (1 << s)) : (mask & ~(1 << s)));
    lastPainted = s;
    flashes[s] = 1.0f;
    repaint();
}

void StepStrip::mouseDrag (const juce::MouseEvent& e)
{
    const int s = stepAt (e.position.x);
    if (s == lastPainted) return;
    lastPainted = s;
    const int mask = proc.getSteps();
    proc.setSteps (paintValue ? (mask | (1 << s)) : (mask & ~(1 << s)));
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
    const int mask = proc.getSteps();
    const int cur = proc.currentStep.load();
    const float w = (float) getWidth() / 16.0f;
    for (int i = 0; i < 16; ++i)
    {
        auto r = juce::Rectangle<float> (i * w, 0, w, (float) getHeight()).reduced (3.0f, 2.0f);
        const bool on = (mask >> i) & 1;
        const float f = flashes[i];
        r = r.expanded (2.0f * f);
        juce::Colour c = on ? accent : (i % 4 == 0 ? ui::track.brighter (0.15f) : ui::panel);
        if (i == cur) c = c.brighter (on ? 0.5f : 0.3f);
        g.setColour (c.interpolatedWith (juce::Colours::white, f * 0.5f));
        g.fillRoundedRectangle (r, 6.0f);
        if (! on)
        {
            g.setColour (ui::track.brighter (0.3f));
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
        }
        if (i == cur)
        {
            g.setColour (juce::Colours::white);
            g.fillRoundedRectangle (r.withTop (r.getBottom() - 3.0f).reduced (4.0f, 0), 1.5f);
        }
    }
}

//==============================================================================
juce::Rectangle<float> WaveDisplay::waveArea() const { return getLocalBounds().toFloat().reduced (12.0f, 24.0f); }

juce::Point<float> WaveDisplay::nodePos (int i) const
{
    const auto area = waveArea();
    const float frac = cached.envT[i] * cached.length / std::max (1.0f, spanMs);
    return { area.getX() + frac * area.getWidth(), area.getCentreY() - cached.envG[i] * 0.5f * area.getHeight() * 0.5f };
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
    set ("envT" + juce::String (i), t);
    set ("envG" + juce::String (i), gain);
}

void WaveDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragNode = nodeAt (e.position);
    if (dragNode < 0) { proc.triggerFromUI(); return; }
    for (auto* id : { "envT", "envG" })
        if (auto* p = proc.apvts.getParameter (id + juce::String (dragNode))) p->beginChangeGesture();
}

void WaveDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNode < 0) return;
    const auto area = waveArea();
    const auto k = proc.readParams();
    float t = (e.position.x - area.getX()) / area.getWidth() * spanMs / std::max (1.0f, k.length);
    if (dragNode == 0) t = 0.0f;
    else
    {
        const float lo = k.envT[dragNode - 1] + 0.005f;
        const float hi = dragNode < 4 ? k.envT[dragNode + 1] - 0.005f : 1.0f;
        t = std::clamp (t, lo, std::max (lo, hi));
    }
    const float gain = std::clamp ((area.getCentreY() - e.position.y) / (area.getHeight() * 0.25f), 0.0f, 2.0f);
    setNode (dragNode, t, gain);
}

void WaveDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragNode >= 0)
    {
        for (auto* id : { "envT", "envG" })
            if (auto* p = proc.apvts.getParameter (id + juce::String (dragNode))) p->endChangeGesture();
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
    if (n >= 0) setNode (n, cached.envT[n], 1.0f);
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
    spanMs = (float) std::max ((double) p.length + 20.0, extras ? beatMs : 0.0);
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

    g.setColour (ui::panel.interpolatedWith (accent, flash * 0.08f));
    g.fillRoundedRectangle (b, 16.0f);
    if (flash > 0.01f)
    {
        g.setColour (accent.withAlpha (flash * 0.6f));
        g.drawRoundedRectangle (b.reduced (1.0f), 16.0f, 2.0f);
    }

    auto area = b.reduced (12.0f, 24.0f);
    const float mid = area.getCentreY();
    const float half = area.getHeight() * 0.5f;
    g.setColour (ui::track);
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
    g.setColour (rumbleCol.withAlpha (0.55f)); g.fillPath (rumblePath);
    g.setColour (bassCol.withAlpha (0.7f + 0.3f * flash)); g.fillPath (bassPath);

    juce::ColourGradient grad (accent.brighter (0.3f + flash * 0.4f), area.getX(), mid - half,
                               accent.darker (0.3f), area.getX(), mid + half, false);
    g.setGradientFill (grad);
    g.setOpacity (0.75f + 0.25f * flash);
    g.fillPath (kickPath);
    g.setOpacity (1.0f);

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
        const float endX = area.getX() + std::min (1.0f, cached.length / std::max (1.0f, spanMs)) * area.getWidth();
        line.lineTo (endX, nodePos (4).y);
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
                const float db = 20.0f * std::log10 (std::max (0.001f, cached.envG[i]));
                g.setFont (uiFont (11.0f, true));
                g.setColour (ui::text);
                g.drawText ((db > 0 ? "+" : "") + juce::String (db, 1) + " dB  " + juce::String (juce::roundToInt (cached.envT[i] * cached.length)) + " ms",
                            juce::Rectangle<float> (pt.x - 60, pt.y < area.getY() + 24 ? pt.y + 12 : pt.y - 28, 120, 14).toNearestInt(), juce::Justification::centred);
            }
        }
    }

    g.setFont (uiFont (12.0f, true));
    auto info = b.reduced (16.0f, 6.0f);
    g.setColour (ui::dim);
    g.drawText (juce::String (juce::roundToInt (cached.length)) + " ms", info.toNearestInt(), juce::Justification::topLeft);
    g.drawText (noteName (cached.pitchEnd) + "  " + juce::String (cached.pitchEnd, 1) + " Hz  -  "
                + juce::String (juce::roundToInt (cachedBpm)) + " BPM", info.toNearestInt(), juce::Justification::topRight);

    int lx = (int) info.getX();
    auto legend = [&] (const juce::String& t, juce::Colour c)
    {
        g.setColour (c);
        g.fillEllipse ((float) lx, info.getBottom() - 9.0f, 7.0f, 7.0f);
        g.setColour (ui::dim);
        g.drawText (t, lx + 11, (int) info.getBottom() - 13, 70, 14, juce::Justification::centredLeft);
        lx += 80;
    };
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
        { "volume", "VOLUME", fmtDb },
        { "rAmt", "AMOUNT", fmtPct }, { "rSize", "SIZE", fmtPct }, { "rDrive", "DIRT", fmtDb },
        { "rTone", "TONE", fmtHz }, { "rDuck", "DUCK", fmtPct },
        { "bLevel", "LEVEL", fmtPct }, { "bSemi", "NOTE", fmtSemi }, { "bDecay", "DECAY", fmtMs }, { "bCut", "CUTOFF", fmtHz },
    };
    for (auto& d : defs)
    {
        auto k = std::make_unique<FunKnob> (p.apvts, d.id, d.name, d.fmt);
        addAndMakeVisible (*k);
        knobById[d.id] = k.get();
        knobs.push_back (std::move (k));
    }

    randomBtn.label = "RANDOM";
    randomBtn.filled = true;
    randomBtn.drawIcon = [this] (juce::Graphics& g, juce::Rectangle<float> r) { drawDie (g, r, dieFace, dieSpin, ui::bg); };
    randomBtn.onClick = [this]
    {
        proc.randomize();
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

    // face logo: spins and kicks
    face.onClick = [this]
    {
        proc.triggerFromUI();
        particles.burst (face.getBounds().getCentre().toFloat(), accent, 26, 6.0f);
    };
    addAndMakeVisible (face);

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
        mutateBtn.glow = 1.0f;
        particles.burst (mutateBtn.getBounds().getCentre().toFloat(), accent.withRotatedHue (0.15f), 16, 4.0f);
    };
    addAndMakeVisible (mutateBtn);

    // LOOP + steps
    loopBtn.label = "LOOP";
    loopAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("loop"), [this] (float v)
    {
        loopBtn.selected = v > 0.5f;
        loopBtn.repaint();
    });
    loopAtt->sendInitialUpdate();
    loopBtn.onClick = [this] { loopAtt->setValueAsCompleteGesture (loopBtn.selected ? 0.0f : 1.0f); loopBtn.glow = 1.0f; };
    addAndMakeVisible (loopBtn);
    addAndMakeVisible (stepStrip);
    addAndMakeVisible (xyPad);

    addAndMakeVisible (display);
    addAndMakeVisible (particles);
    lastHit = proc.hitCounter.load();
    setSize (1060, 760);
    startTimerHz (60);
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
    g.fillAll (ui::bg);
    if (bgFlash > 0.01f)
    {
        juce::ColourGradient grad (accent.withAlpha (0.18f * bgFlash), (float) getWidth() * 0.5f, 210.0f,
                                   accent.withAlpha (0.0f), (float) getWidth() * 0.5f, 700.0f, true);
        g.setGradientFill (grad);
        g.fillRect (getLocalBounds());
    }

    const auto titleFont = uiFont (30.0f, true);
    const int tx = 92;
    g.setFont (titleFont);
    g.setColour (ui::text);
    g.drawText ("C.I.R.O", tx, 18, 140, 40, juce::Justification::centredLeft);
    g.setColour (accent);
    g.drawText ("KICK", tx + juce::roundToInt (juce::GlyphArrangement::getStringWidth (titleFont, "C.I.R.O ")), 18, 100, 40,
                juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (uiFont (11.0f));
    g.drawText ("kick + bass + rumble generator", tx + 2, 52, 240, 14, juce::Justification::centredLeft);

    g.setFont (uiFont (11.0f, true));
    for (auto& [name, r] : sectionLabels)
    {
        g.setColour (ui::dim);
        g.drawText (name, r.withHeight (14), juce::Justification::centredLeft);
        g.setColour (ui::track);
        g.drawHorizontalLine (r.getY() + 18, (float) r.getX(), (float) r.getRight());
    }
}

void CiroKickEditor::layoutRow (std::vector<std::vector<juce::Component*>> groups, int y)
{
    const int cell = 86, gap = 30;
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
    face.setBounds (24, 12, 60, 60);
    randomBtn.setBounds (getWidth() - 24 - 160, 18, 160, 50);
    mutateBtn.setBounds (randomBtn.getX() - 56, 21, 50, 44);
    undoBtn.setBounds (mutateBtn.getX() - 56, 21, 50, 44);

    const int pw = 290;
    const int px = (getWidth() - pw) / 2 - 10;
    prevBtn.setBounds (px, 24, 40, 38);
    presetBtn.setBounds (px + 42, 24, pw - 84, 38);
    nextBtn.setBounds (px + pw - 40, 24, 40, 38);

    const int pillW = 138, pillGap = 8;
    int gx = (getWidth() - (pillW * numGenres + pillGap * (numGenres - 1))) / 2;
    for (auto& b : genreBtns) { b->setBounds (gx, 84, pillW, 34); gx += pillW + pillGap; }

    const int top = 130, h = 190;
    const int playW = 130, xyW = 170;
    display.setBounds (24, top, getWidth() - 48 - playW - xyW - 24, h);
    xyPad.setBounds (display.getRight() + 12, top, xyW, h);
    playBtn.setBounds (xyPad.getRight() + 12, top, playW, h);

    loopBtn.setBounds (21, top + h + 6, 86, 40);
    stepStrip.setBounds (112, top + h + 10, getWidth() - 112 - 24, 32);

    auto k = [this] (const char* id) -> juce::Component* { return knobById[id]; };
    const int y1 = 372, y2 = 498, y3 = 624;
    layoutRow ({ { k ("pStart"), k ("pEnd"), k ("pDecay"), k ("pCurve"), k ("bend") },
                 { k ("length"), k ("hold"), k ("shape"), k ("wave") } }, y1);
    layoutRow ({ { k ("cLevel"), k ("cDecay"), k ("cTone") },
                 { &distTypeBtn, k ("drive"), k ("dMix"), k ("punch"), k ("tone"), k ("reso") },
                 { k ("volume"), &keyTrackBtn } }, y2);
    layoutRow ({ { k ("rAmt"), k ("rSize"), k ("rDrive"), k ("rTone"), k ("rDuck"), k ("rRoll") },
                 { &bassPatBtn, k ("bLevel"), k ("bSemi"), k ("bDecay"), k ("bCut") } }, y3);

    sectionLabels.clear();
    auto label = [&] (const char* name, juce::Component* first, juce::Component* last, int y)
    {
        const int x0 = dynamic_cast<FunButton*> (first) ? first->getX() - 6 : first->getX();
        const int x1 = dynamic_cast<FunButton*> (last) ? last->getRight() + 6 : last->getRight();
        sectionLabels.push_back ({ name, juce::Rectangle<int> (x0 + 6, y, x1 - x0 - 12, 20) });
    };
    label ("PITCH", k ("pStart"), k ("bend"), y1);
    label ("BODY", k ("length"), k ("wave"), y1);
    label ("CLICK", k ("cLevel"), k ("cTone"), y2);
    label ("DRIVE", &distTypeBtn, k ("reso"), y2);
    label ("OUT", k ("volume"), &keyTrackBtn, y2);
    label ("RUMBLE", k ("rAmt"), k ("rRoll"), y3);
    label ("BASS", &bassPatBtn, k ("bCut"), y3);
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
        for (auto* b : { &randomBtn, &undoBtn, &distTypeBtn, &keyTrackBtn, &bassPatBtn, &presetBtn, &prevBtn, &nextBtn, &mutateBtn, &loopBtn })
        { b->colour = accent; b->repaint(); }
        face.accent = xyPad.accent = stepStrip.accent = accent;
        face.repaint(); xyPad.repaint(); stepStrip.repaint();
        playBtn.colour = accent; playBtn.repaint();
        repaint (0, 0, 340, 76);
    }

    const auto name = proc.getPresetName();
    if (presetBtn.label != name) { presetBtn.label = name; presetBtn.repaint(); }

    const int hits = proc.hitCounter.load();
    if (hits != lastHit)
    {
        lastHit = hits;
        display.hit (now);
        playBtn.hit();
        particles.burst (playBtn.getBounds().getCentre().toFloat(), accent, 10, 3.5f);
    }

    if (dieSpin > 0.0f) { dieSpin = std::max (0.0f, dieSpin * 0.82f - 0.01f); randomBtn.repaint(); }
    if (bgFlash > 0.0f) { bgFlash = std::max (0.0f, bgFlash - 0.04f); repaint(); }

    display.update (accent, now);
    for (auto* b : { &randomBtn, &undoBtn, &distTypeBtn, &keyTrackBtn, &bassPatBtn, &presetBtn, &prevBtn, &nextBtn, &mutateBtn, &loopBtn })
        b->animate();
    face.animate(); xyPad.animate(); stepStrip.animate();
    playBtn.animate();
    for (auto& b : genreBtns) b->animate();
    for (auto& kn : knobs) kn->animate (accent);
    particles.step();
}
