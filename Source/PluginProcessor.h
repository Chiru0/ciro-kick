#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KickEngine.h"
#include "Genres.h"
#include <random>

class CiroKickProcessor : public juce::AudioProcessor
{
public:
    CiroKickProcessor();

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "C.I.R.O KICK"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // UI helpers
    KickParams readParams() const;
    void writeParams (const KickParams&);
    void randomize();
    bool undoRandom();
    void triggerFromUI() { uiTrigger = true; }
    int getGenre() const;
    void loadPreset (int genre, int index);
    void mutate();
    void setSteps (int mask);
    int getSteps() const { return steps.load(); }
    std::atomic<int> currentStep { -1 };
    juce::String getPresetName() const;
    int presetIndex = -1;
    std::atomic<double> bpm { 145.0 };

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> hitCounter { 0 }; // bumps on every kick, UI uses it to flash

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void startKick (float velocity, int note);

    void setPresetName (const juce::String&);
    GrooveEngine engine;
    int randomCount = 0;
    std::atomic<int> steps { 0x1111 };
    double loopPpq = 0.0, sampleRateHz = 44100.0;
    int lastStep = -1;
    std::atomic<bool> uiTrigger { false };
    std::mt19937 rng { std::random_device{}() };
    std::vector<std::pair<KickParams, juce::String>> history;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CiroKickProcessor)
};
