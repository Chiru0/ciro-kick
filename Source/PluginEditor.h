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

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour accent { 0xffff3b5c };
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
};

// Rotary knob + label. Shows its value while hovered or dragged.
class FunKnob : public juce::Component
{
public:
    FunKnob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name,
             std::function<juce::String (float)> fmt);
    void resized() override;
    void paint (juce::Graphics&) override;
    bool animate();
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
    void mouseUp (const juce::MouseEvent&) override    { repaint(); }

    juce::Slider slider;
private:
    juce::String name;
    std::function<juce::String (float)> format;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    Spring spring;
    bool wasActive = false;
    float lastScale = 1.0f;
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
    std::function<void()> onClick;
    std::function<void()> onRightClick;
    std::function<void (juce::Graphics&, juce::Rectangle<float>)> drawIcon;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; spring.target = 1.04f; }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; spring.target = 1.0f; }
    bool animate();
    void pop (float amount = 0.18f) { spring.kick (amount); }

private:
    Spring spring;
    bool hover = false;
};

class WaveDisplay : public juce::Component
{
public:
    explicit WaveDisplay (CiroKickProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    bool update (juce::Colour accent, double nowMs);
    void hit (double nowMs) { lastHitMs = nowMs; }

private:
    void render (const KickParams&);
    CiroKickProcessor& proc;
    KickParams cached;
    bool hasCache = false;
    std::vector<float> peaks;
    juce::Colour accent;
    double lastHitMs = -1.0e9, nowMs = 0.0;
    float lengthMs = 450.0f;
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

    CiroKickProcessor& proc;
    KnobLookAndFeel lnf;
    WaveDisplay display;
    std::vector<std::unique_ptr<FunKnob>> knobs;
    std::map<juce::String, FunKnob*> knobById;
    FunButton randomBtn, undoBtn, playBtn, distTypeBtn, keyTrackBtn;
    std::vector<std::unique_ptr<FunButton>> genreBtns;
    std::unique_ptr<juce::ParameterAttachment> distTypeAtt, keyTrackAtt, genreAtt;
    juce::Colour accent { 0xffff3b5c };
    int lastHit = 0, dieFace = 5;
    float dieSpin = 0.0f;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> sectionLabels;
};
