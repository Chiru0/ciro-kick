#include "PluginEditor.h"

static juce::Font uiFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    const bool active = s.isMouseOverOrDragging();

    // arcs
    juce::Path trackArc, valueArc;
    trackArc.addCentredArc (c.x, c.y, r - 3, r - 3, 0, start, end, true);
    g.setColour (ui::track);
    g.strokePath (trackArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0 && s.getName() == "bipolar";
    const float from = bipolar ? (start + end) * 0.5f : start;
    valueArc.addCentredArc (c.x, c.y, r - 3, r - 3, 0, std::min (from, angle), std::max (from, angle), true);
    g.setColour (accent);
    g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // body
    const float br = r - 10.0f;
    g.setColour (active ? ui::track.brighter (0.15f) : ui::track);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);

    // pointer dot
    const auto dot = c.getPointOnCircumference (br - 7.0f, angle);
    g.setColour (active ? accent : ui::text);
    g.fillEllipse (dot.x - 3.5f, dot.y - 3.5f, 7.0f, 7.0f);
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
    slider.setVelocityBasedMode (false);
    if (id == "bend") slider.setName ("bipolar");
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    slider.onValueChange = [this] { repaint(); };
    slider.addMouseListener (this, false);
}

void FunKnob::resized()
{
    auto b = getLocalBounds();
    const int size = 62;
    slider.setBounds (b.getCentreX() - size / 2, 4, size, size);
}

void FunKnob::paint (juce::Graphics& g)
{
    const bool active = slider.isMouseOverOrDragging();
    g.setColour (active ? ui::text : ui::dim);
    g.setFont (uiFont (11.5f, active));
    auto r = getLocalBounds().withTop (70).withHeight (16);
    g.drawText (active ? format ((float) slider.getValue()) : name, r, juce::Justification::centred);
}

bool FunKnob::animate()
{
    const bool active = slider.isMouseButtonDown();
    if (active != wasActive) { spring.target = active ? 1.12f : 1.0f; wasActive = active; }
    if (! spring.step() && spring.value == lastScale) return false;
    lastScale = spring.value;
    const auto c = slider.getBounds().getCentre().toFloat();
    slider.setTransform (juce::AffineTransform::scale (lastScale, lastScale, c.x, c.y));
    return true;
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
        g.fillRoundedRectangle (b.expanded (3.0f * glow), corner + 3.0f);
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
    const auto font = uiFont (std::min (15.0f, b.getHeight() * 0.42f), true);
    if (drawIcon && label.isEmpty())
    {
        drawIcon (g, b.reduced (9.0f));
        return;
    }
    auto content = b;
    if (drawIcon)
    {
        // centre icon + text together
        const float icon = b.getHeight() * 0.5f, gap = 10.0f;
        const float textW = juce::GlyphArrangement::getStringWidth (font, label);
        const float x0 = b.getCentreX() - (icon + gap + textW) * 0.5f;
        drawIcon (g, { x0, b.getCentreY() - icon * 0.5f, icon, icon });
        content = { x0 + icon + gap, b.getY(), textW + 4.0f, b.getHeight() };
    }
    g.setColour (fg);
    g.setFont (font);
    g.drawText (label, content.toNearestInt(), drawIcon ? juce::Justification::centredLeft : juce::Justification::centred);
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
    spring.velocity += 0.06f; // bounce back with overshoot
}

bool FunButton::animate()
{
    bool moving = spring.step();
    if (glow > 0.0f) { glow = std::max (0.0f, glow - 0.06f); moving = true; }
    if (moving) repaint();
    return moving;
}

//==============================================================================
static juce::String noteName (float hz)
{
    const int midi = (int) std::round (12.0 * std::log2 (hz / 440.0) + 69.0);
    return juce::MidiMessage::getMidiNoteName (midi, true, true, 4);
}

void WaveDisplay::render (const KickParams& p)
{
    KickVoice v;
    const double sr = 22050.0;
    v.setSampleRate (sr);
    KickParams q = p;
    q.volume = 0.0f;
    v.trigger (q, 1.0f);
    const int total = (int) (sr * (p.length + 20.0f) / 1000.0f);
    const int w = std::max (1, getWidth() - 24);
    peaks.assign ((size_t) w, 0.0f);
    for (int i = 0; i < total; ++i)
    {
        const float s = std::abs (v.next());
        auto& pk = peaks[(size_t) std::min (w - 1, i * w / std::max (1, total))];
        pk = std::max (pk, s);
    }
    lengthMs = p.length;
}

bool WaveDisplay::update (juce::Colour a, double now)
{
    nowMs = now;
    const auto p = proc.readParams();
    bool changed = ! hasCache || std::memcmp (&p, &cached, offsetof (KickParams, keyTrack)) != 0 || (int) peaks.size() != getWidth() - 24;
    if (changed) { cached = p; hasCache = true; render (p); }
    const bool playing = now - lastHitMs < lengthMs + 250.0;
    if (changed || playing || a != accent) { accent = a; repaint(); }
    return playing;
}

void WaveDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (b, 16.0f);

    auto area = b.reduced (12.0f, 22.0f);
    const float mid = area.getCentreY();
    const float half = area.getHeight() * 0.5f;

    g.setColour (ui::track);
    g.drawHorizontalLine ((int) mid, area.getX(), area.getRight());

    const double since = nowMs - lastHitMs;
    const float flash = (float) juce::jlimit (0.0, 1.0, 1.0 - since / 300.0);
    const float progress = (float) (since / std::max (1.0f, lengthMs + 20.0f));

    juce::Path wave;
    const int n = (int) peaks.size();
    if (n > 1)
    {
        wave.startNewSubPath (area.getX(), mid);
        for (int i = 0; i < n; ++i)
            wave.lineTo (area.getX() + (float) i, mid - std::min (1.0f, peaks[(size_t) i]) * half);
        for (int i = n - 1; i >= 0; --i)
            wave.lineTo (area.getX() + (float) i, mid + std::min (1.0f, peaks[(size_t) i]) * half);
        wave.closeSubPath();

        g.setColour (accent.withAlpha (0.55f + 0.45f * flash));
        g.fillPath (wave);

        if (progress >= 0.0f && progress <= 1.0f)
        {
            // light up the part already played
            g.saveState();
            g.reduceClipRegion (area.withWidth (area.getWidth() * progress).toNearestInt());
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillPath (wave);
            g.restoreState();
            const float x = area.getX() + area.getWidth() * progress;
            g.setColour (ui::text);
            g.fillRect (x - 1.0f, area.getY(), 2.0f, area.getHeight());
        }
    }

    g.setFont (uiFont (12.0f, true));
    g.setColour (ui::dim);
    auto info = b.reduced (16.0f, 6.0f);
    g.drawText (juce::String (juce::roundToInt (cached.length)) + " ms", info.toNearestInt(), juce::Justification::topLeft);
    g.drawText (noteName (cached.pitchEnd) + "  " + juce::String (cached.pitchEnd, 1) + " Hz",
                info.toNearestInt(), juce::Justification::topRight);
}

//==============================================================================
static juce::String fmtHz (float v) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }
static juce::String fmtMs (float v) { return v < 10.0f ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms"; }
static juce::String fmtDb (float v) { return juce::String (v, 1) + " dB"; }
static juce::String fmtPct (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
static juce::String fmtX (float v) { return juce::String (v, 2); }
static juce::String fmtSt (float v) { return (v > 0 ? "-" : (v < 0 ? "+" : "")) + juce::String (std::abs (v), 1) + " st"; }

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

CiroKickEditor::CiroKickEditor (CiroKickProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p)
{
    struct K { const char* id; const char* name; juce::String (*fmt) (float); };
    const K defs[] = {
        { "pStart", "START", fmtHz }, { "pEnd", "TUNE", fmtHz }, { "pDecay", "SWEEP", fmtMs },
        { "pCurve", "CURVE", fmtX },  { "bend", "BEND", fmtSt },
        { "length", "LENGTH", fmtMs }, { "hold", "HOLD", fmtMs }, { "shape", "SHAPE", fmtX }, { "wave", "WAVE", fmtPct },
        { "cLevel", "LEVEL", fmtPct }, { "cDecay", "SNAP", fmtMs }, { "cTone", "TONE", fmtHz },
        { "drive", "DRIVE", fmtDb }, { "dMix", "MIX", fmtPct }, { "tone", "FILTER", fmtHz },
        { "volume", "VOLUME", fmtDb },
    };
    for (auto& d : defs)
    {
        auto k = std::make_unique<FunKnob> (p.apvts, d.id, d.name, d.fmt);
        k->slider.setLookAndFeel (&lnf);
        addAndMakeVisible (*k);
        knobById[d.id] = k.get();
        knobs.push_back (std::move (k));
    }

    // RANDOM
    randomBtn.label = "RANDOM";
    randomBtn.filled = true;
    randomBtn.drawIcon = [this] (juce::Graphics& g, juce::Rectangle<float> r) { drawDie (g, r, dieFace, dieSpin, ui::bg); };
    randomBtn.onClick = [this]
    {
        proc.randomize();
        dieFace = 1 + juce::Random::getSystemRandom().nextInt (6);
        dieSpin = juce::MathConstants<float>::pi * 1.5f;
        randomBtn.pop (0.25f);
        for (auto& k : knobs) k->repaint();
    };
    addAndMakeVisible (randomBtn);

    // UNDO
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

    // PLAY pad
    playBtn.label = "PLAY";
    playBtn.onClick = [this] { proc.triggerFromUI(); };
    addAndMakeVisible (playBtn);

    // drive type: click = next, right-click = previous
    auto* dType = p.apvts.getParameter ("dType");
    distTypeAtt = std::make_unique<juce::ParameterAttachment> (*dType, [this, dType] (float)
    {
        distTypeBtn.label = dType->getCurrentValueAsText();
        distTypeBtn.repaint();
    });
    distTypeAtt->sendInitialUpdate();
    distTypeBtn.onClick = [this, dType]
    {
        const int cur = juce::roundToInt (dType->convertFrom0to1 (dType->getValue()));
        distTypeAtt->setValueAsCompleteGesture ((float) ((cur + 1) % 4));
    };
    distTypeBtn.onRightClick = [this, dType]
    {
        const int cur = juce::roundToInt (dType->convertFrom0to1 (dType->getValue()));
        distTypeAtt->setValueAsCompleteGesture ((float) ((cur + 3) % 4));
    };
    addAndMakeVisible (distTypeBtn);

    // key track toggle
    keyTrackBtn.label = "KEY";
    keyTrackAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("keyTrack"), [this] (float v)
    {
        keyTrackBtn.selected = v > 0.5f;
        keyTrackBtn.repaint();
    });
    keyTrackAtt->sendInitialUpdate();
    keyTrackBtn.onClick = [this] { keyTrackAtt->setValueAsCompleteGesture (keyTrackBtn.selected ? 0.0f : 1.0f); };
    addAndMakeVisible (keyTrackBtn);

    // genres
    const auto& profiles = genreProfiles();
    for (int i = 0; i < (int) profiles.size(); ++i)
    {
        auto b = std::make_unique<FunButton>();
        b->label = profiles[(size_t) i].name;
        b->colour = juce::Colour (profiles[(size_t) i].colour);
        b->onClick = [this, i] { genreAtt->setValueAsCompleteGesture ((float) i); };
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

    addAndMakeVisible (display);
    lastHit = proc.hitCounter.load();
    setSize (920, 600);
    startTimerHz (60);
}

CiroKickEditor::~CiroKickEditor()
{
    for (auto& k : knobs) k->slider.setLookAndFeel (nullptr);
}

void CiroKickEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    g.setColour (ui::text);
    g.setFont (uiFont (30.0f, true));
    g.drawText ("CIRO", 28, 18, 90, 40, juce::Justification::centredLeft);
    g.setColour (accent);
    g.drawText ("KICK", 108, 18, 100, 40, juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (uiFont (11.0f));
    g.drawText ("kick generator", 30, 52, 200, 14, juce::Justification::centredLeft);

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
    const int cell = 88, gap = 34;
    int total = -gap;
    for (auto& grp : groups) total += (int) grp.size() * cell + gap;
    int x = (getWidth() - total) / 2;
    for (auto& grp : groups)
    {
        for (auto* c : grp)
        {
            if (dynamic_cast<FunButton*> (c) != nullptr)
                c->setBounds (x + 8, y + 40, cell - 16, 36);
            else
                c->setBounds (x, y + 22, cell, 92);
            x += cell;
        }
        x += gap;
    }
}

void CiroKickEditor::resized()
{
    randomBtn.setBounds (getWidth() - 28 - 170, 18, 170, 50);
    undoBtn.setBounds (randomBtn.getX() - 58, 21, 50, 44);

    const int pillW = 132, pillGap = 8;
    int px = (getWidth() - (pillW * 6 + pillGap * 5)) / 2;
    for (auto& b : genreBtns) { b->setBounds (px, 86, pillW, 34); px += pillW + pillGap; }

    display.setBounds (28, 134, getWidth() - 56 - 150, 170);
    playBtn.setBounds (display.getRight() + 12, 134, 138, 170);

    auto k = [this] (const char* id) -> juce::Component* { return knobById[id]; };
    layoutRow ({ { k ("pStart"), k ("pEnd"), k ("pDecay"), k ("pCurve"), k ("bend") },
                 { k ("length"), k ("hold"), k ("shape"), k ("wave") } }, 318);
    layoutRow ({ { k ("cLevel"), k ("cDecay"), k ("cTone") },
                 { &distTypeBtn, k ("drive"), k ("dMix"), k ("tone") },
                 { k ("volume"), &keyTrackBtn } }, 452);

    // section labels follow the first and last component of each group
    sectionLabels.clear();
    auto label = [&] (const char* name, juce::Component* first, juce::Component* last, int y)
    {
        sectionLabels.push_back ({ name, juce::Rectangle<int> (first->getX() + 6, y, last->getRight() - first->getX() - 12, 20) });
    };
    label ("PITCH", k ("pStart"), k ("bend"), 318);
    label ("BODY", k ("length"), k ("wave"), 318);
    label ("CLICK", k ("cLevel"), k ("cTone"), 452);
    label ("DRIVE", &distTypeBtn, k ("tone"), 452);
    label ("OUT", k ("volume"), &keyTrackBtn, 452);
    // buttons in a row sit inside a cell, use the cell edge for labels
    sectionLabels[3].second.setX (distTypeBtn.getX() - 2);
    sectionLabels[4].second.setRight (keyTrackBtn.getRight() + 2);
}

void CiroKickEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();

    // accent glides towards the selected genre colour
    const auto target = juce::Colour (genreProfiles()[(size_t) proc.getGenre()].colour);
    if (accent != target)
    {
        accent = accent.interpolatedWith (target, 0.18f);
        if (std::abs (accent.getFloatRed() - target.getFloatRed()) + std::abs (accent.getFloatGreen() - target.getFloatGreen())
            + std::abs (accent.getFloatBlue() - target.getFloatBlue()) < 0.01f)
            accent = target;
        lnf.accent = accent;
        randomBtn.colour = playBtn.colour = distTypeBtn.colour = keyTrackBtn.colour = undoBtn.colour = accent;
        randomBtn.repaint(); playBtn.repaint(); distTypeBtn.repaint(); keyTrackBtn.repaint(); undoBtn.repaint();
        for (auto& kn : knobs) kn->repaint();
        repaint (0, 0, 240, 70);
    }

    const int hits = proc.hitCounter.load();
    if (hits != lastHit)
    {
        lastHit = hits;
        display.hit (now);
        playBtn.glow = 1.0f;
        playBtn.pop (0.12f);
    }

    if (dieSpin > 0.0f) { dieSpin = std::max (0.0f, dieSpin * 0.82f - 0.01f); randomBtn.repaint(); }

    display.update (accent, now);
    randomBtn.animate(); undoBtn.animate(); playBtn.animate(); distTypeBtn.animate(); keyTrackBtn.animate();
    for (auto& b : genreBtns) b->animate();
    for (auto& kn : knobs) kn->animate();
}
