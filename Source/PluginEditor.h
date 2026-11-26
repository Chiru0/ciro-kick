#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui
{
    const juce::Colour bg      { 0xff111114 };
    const juce::Colour panel   { 0xff1a1a1f };
    const juce::Colour track   { 0xff2a2a31 };
    const juce::Colour text    { 0xffe9e9ee };
    const juce::Colour dim     { 0xff7c7c88 };
}

// Spring used for the squishy press/pop animations.
struct Spring
{
    float value = 1.0f, velocity = 0.0f, target = 1.0f;
    bool step()
    {
        const float force = (target - value) * 0.28f;
        velocity = (velocity + force) * 0.72f;
        value += velocity;
        return std::abs (velocity) > 0.0005f || std::abs (target - value) > 0.0005f;
    }
    void kick (float amount) { velocity -= amount; }
};

class CiroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CiroLookAndFeel();
    // knobs are painted by FunKnob itself
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
};

// Rotary knob that eases towards its value, flashes when changed by RANDOM/presets.
class FunKnob : public juce::Component
{
public:
    FunKnob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name,
             std::function<juce::String (float)> fmt);
    void resized() override;
    void paint (juce::Graphics&) override;
    bool animate (juce::Colour accent);
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
    void mouseUp (const juce::MouseEvent&) override    { repaint(); }

    juce::Slider slider;
private:
    juce::String name;
    std::function<juce::String (float)> format;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    Spring spring;
    float shown = -1.0f, flash = 0.0f;
    juce::Colour accent { 0xffff3b5c };
};

// Squishy button with an optional custom painter.
class FunButton : public juce::Component
{
public:
    juce::String label;
    juce::Colour colour { 0xffff3b5c };
    bool filled = false;
    bool selected = false;
    float glow = 0.0f;
    float fontSize = 15.0f;
    std::function<void()> onClick;
    std::function<void()> onRightClick;
    std::function<void (juce::Graphics&, juce::Rectangle<float>)> drawIcon;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; spring.target = 1.04f; }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; spring.target = 1.0f; }
    virtual bool animate();
    void pop (float amount = 0.18f) { spring.kick (amount); }

protected:
    Spring spring;
    bool hover = false;
};

// Big pad with expanding rings on every hit.
class PlayPad : public FunButton
{
public:
    void paint (juce::Graphics&) override;
    bool animate() override;
    void hit();
private:
    std::vector<float> rings;
};

// Confetti-like particles drawn over everything.
class Particles : public juce::Component
{
public:
    Particles() { setInterceptsMouseClicks (false, false); }
    void burst (juce::Point<float> at, juce::Colour c, int count, float speed);
    bool step();
    void paint (juce::Graphics&) override;
private:
    struct P { juce::Point<float> pos, vel; float life, size, spin, angle; juce::Colour colour; int shape; };
    std::vector<P> ps;
    juce::Random rnd;
};

// Cartoon of Ciro that spins when clicked.
class FaceLogo : public juce::Component
{
public:
    juce::Colour accent { 0xffff3b5c };
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool animate();
private:
    float angle = 0.0f, spinVel = 0.0f, squash = 0.0f, tongue = 0.0f;
};

// Drag the puck: X = dark..bright, Y = clean..dirty.
class XYPad : public juce::Component
{
public:
    explicit XYPad (CiroKickProcessor& p) : proc (p) {}
    juce::Colour accent { 0xffff3b5c };
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool animate();
private:
    void apply (juce::Point<float>);
    juce::Point<float> puckFromParams() const;
    CiroKickProcessor& proc;
    std::vector<juce::Point<float>> trail;
    juce::Point<float> shown { 0.5f, 0.5f };
    bool dragging = false;
    float pulse = 0.0f;
};

// 16-step pattern for the LOOP preview.
class StepStrip : public juce::Component
{
public:
    explicit StepStrip (CiroKickProcessor& p) : proc (p) {}
    juce::Colour accent { 0xffff3b5c };
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    bool animate();
private:
    int stepAt (float x) const;
    CiroKickProcessor& proc;
    int paintValue = 1, lastPainted = -1, lastStep = -2;
    float flashes[32] {};
};

class WaveDisplay : public juce::Component
{
public:
    explicit WaveDisplay (CiroKickProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverNode = -1; repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void setMode (int m) { mode = m; hasCache = false; repaint(); }
    int mode = 0;
    void update (juce::Colour accent, double nowMs);
    void hit (double nowMs) { lastHitMs = nowMs; shake = 1.0f; }

private:
    void render (const KickParams&, double bpm);
    juce::Rectangle<float> waveArea() const;
    juce::Point<float> nodePos (int i) const;
    const float* nodeT (const KickParams&) const;
    const float* nodeG (const KickParams&) const;
    juce::String nodeId (bool gain) const;
    float nodeMs (const KickParams&, float t) const;
    float msToNode (const KickParams&, float ms) const;
    int nodeAt (juce::Point<float>) const;
    void setNode (int i, float t, float g);
    CiroKickProcessor& proc;
    int dragNode = -1, hoverNode = -1;
    KickParams cached;
    double cachedBpm = 0;
    bool hasCache = false;
    std::vector<float> kickT, bassT, rumbleT;   // targets
    std::vector<float> kickS, bassS, rumbleS;   // shown (morphing)
    juce::Colour accent;
    double lastHitMs = -1.0e9, nowMs = 0.0;
    float spanMs = 450.0f, shake = 0.0f;
};

class CiroKickEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CiroKickEditor (CiroKickProcessor&);
    ~CiroKickEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutRow (std::vector<std::vector<juce::Component*>> groups, int y);
    void showPresetMenu();
    void stepPreset (int delta);
    std::unique_ptr<juce::ParameterAttachment> attachCycle (FunButton&, const char* id, int count);

    CiroKickProcessor& proc;
    CiroLookAndFeel lnf;
    WaveDisplay display;
    std::vector<std::unique_ptr<FunKnob>> knobs;
    std::map<juce::String, FunKnob*> knobById;
    FaceLogo face;
    XYPad xyPad;
    StepStrip stepStrip;
    FunButton loopLessBtn, loopMoreBtn;
    FunButton mutateBtn, loopBtn, rRateBtn, bOscBtn;
    FunButton tabBtns[3];
    int tab = 0, frame = 0;
    void setTab (int);
    std::unique_ptr<juce::ParameterAttachment> rRateAtt, bOscAtt;
    std::unique_ptr<juce::ParameterAttachment> loopAtt;
    FunButton randomBtn, undoBtn, distTypeBtn, keyTrackBtn, bassPatBtn, presetBtn, prevBtn, nextBtn;
    PlayPad playBtn;
    Particles particles;
    std::vector<std::unique_ptr<FunButton>> genreBtns;
    std::unique_ptr<juce::ParameterAttachment> distTypeAtt, keyTrackAtt, genreAtt, bassPatAtt;
    juce::Colour accent { 0xffff3b5c };
    int lastHit = 0, dieFace = 5;
    float dieSpin = 0.0f, bgFlash = 0.0f;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> sectionLabels;
};
