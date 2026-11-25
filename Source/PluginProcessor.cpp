#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
{
    juce::NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

APVTS::ParameterLayout CiroKickProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&] (const char* id, const char* name, juce::NormalisableRange<float> r, float def, const char* unit = "")
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                                                                  juce::AudioParameterFloatAttributes().withLabel (unit)));
    };

    add ("pStart", "Pitch Start", skewed (80.0f, 3000.0f, 600.0f), 600.0f, "Hz");
    add ("pEnd",   "Pitch End",   juce::NormalisableRange<float> (25.0f, 120.0f), 50.0f, "Hz");
    add ("pDecay", "Pitch Decay", skewed (2.0f, 400.0f, 50.0f), 40.0f, "ms");
    add ("pCurve", "Pitch Curve", skewed (0.3f, 4.0f, 1.0f), 1.0f);
    add ("bend",   "Tail Bend",   juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f, "st");
    add ("length", "Length",      skewed (60.0f, 2000.0f, 450.0f), 450.0f, "ms");
    add ("hold",   "Hold",        skewed (0.0f, 400.0f, 60.0f), 30.0f, "ms");
    add ("shape",  "Decay Shape", skewed (0.3f, 4.0f, 1.2f), 1.5f);
    add ("wave",   "Wave",        juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f);
    add ("cLevel", "Click Level", juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f);
    add ("cDecay", "Click Decay", skewed (0.5f, 40.0f, 6.0f), 6.0f, "ms");
    add ("cTone",  "Click Tone",  skewed (500.0f, 16000.0f, 4000.0f), 4000.0f, "Hz");
    add ("drive",  "Drive",       juce::NormalisableRange<float> (0.0f, 40.0f), 6.0f, "dB");
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "dType", 1 }, "Drive Type",
                                                               juce::StringArray { "SOFT", "HARD", "FOLD", "CRUSH" }, 0));
    add ("dMix",   "Drive Mix",   juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f);
    add ("tone",   "Tone",        skewed (300.0f, 20000.0f, 3000.0f), 12000.0f, "Hz");
    add ("volume", "Volume",      juce::NormalisableRange<float> (-36.0f, 6.0f), -3.0f, "dB");
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "keyTrack", 1 }, "Key Track", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "genre", 1 }, "Genre",
        juce::StringArray { "SCHRANZ", "PSYTRANCE", "HARD BOUNCE", "HARDSTYLE", "HARD TECHNO", "GABBER" }, 0));

    return { p.begin(), p.end() };
}

CiroKickProcessor::CiroKickProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool CiroKickProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void CiroKickProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& v : voices) v.setSampleRate (sampleRate);
}

static float val (const APVTS& s, const char* id) { return s.getRawParameterValue (id)->load(); }

KickParams CiroKickProcessor::readParams() const
{
    KickParams k;
    k.pitchStart = val (apvts, "pStart");
    k.pitchEnd   = val (apvts, "pEnd");
    k.pitchDecay = val (apvts, "pDecay");
    k.pitchCurve = val (apvts, "pCurve");
    k.tailBend   = val (apvts, "bend");
    k.length     = val (apvts, "length");
    k.hold       = val (apvts, "hold");
    k.shape      = val (apvts, "shape");
    k.wave       = val (apvts, "wave");
    k.clickLevel = val (apvts, "cLevel");
    k.clickDecay = val (apvts, "cDecay");
    k.clickTone  = val (apvts, "cTone");
    k.drive      = val (apvts, "drive");
    k.distType   = (int) val (apvts, "dType");
    k.distMix    = val (apvts, "dMix");
    k.tone       = val (apvts, "tone");
    k.volume     = val (apvts, "volume");
    k.keyTrack   = val (apvts, "keyTrack") > 0.5f;
    return k;
}

static void setParam (APVTS& s, const char* id, float plainValue)
{
    if (auto* p = s.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

void CiroKickProcessor::writeParams (const KickParams& k)
{
    setParam (apvts, "pStart", k.pitchStart);
    setParam (apvts, "pEnd",   k.pitchEnd);
    setParam (apvts, "pDecay", k.pitchDecay);
    setParam (apvts, "pCurve", k.pitchCurve);
    setParam (apvts, "bend",   k.tailBend);
    setParam (apvts, "length", k.length);
    setParam (apvts, "hold",   k.hold);
    setParam (apvts, "shape",  k.shape);
    setParam (apvts, "wave",   k.wave);
    setParam (apvts, "cLevel", k.clickLevel);
    setParam (apvts, "cDecay", k.clickDecay);
    setParam (apvts, "cTone",  k.clickTone);
    setParam (apvts, "drive",  k.drive);
    setParam (apvts, "dType",  (float) k.distType);
    setParam (apvts, "dMix",   k.distMix);
    setParam (apvts, "tone",   k.tone);
}

int CiroKickProcessor::getGenre() const { return (int) val (apvts, "genre"); }

void CiroKickProcessor::randomize()
{
    const auto cur = readParams();
    history.push_back (cur);
    if (history.size() > 50) history.erase (history.begin());
    writeParams (randomKick (getGenre(), rng, cur));
    triggerFromUI();
}

bool CiroKickProcessor::undoRandom()
{
    if (history.empty()) return false;
    writeParams (history.back());
    history.pop_back();
    triggerFromUI();
    return true;
}

void CiroKickProcessor::startKick (float velocity, int note)
{
    const auto k = readParams();
    voices[currentVoice].release();
    currentVoice ^= 1;
    const float baseHz = (k.keyTrack && note >= 0) ? (float) juce::MidiMessage::getMidiNoteInHertz (note) : -1.0f;
    voices[currentVoice].trigger (k, velocity, baseHz);
    ++hitCounter;
}

void CiroKickProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int n = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);

    if (uiTrigger.exchange (false))
        startKick (1.0f, -1);

    int pos = 0;
    auto render = [&] (int end)
    {
        for (; pos < end; ++pos)
        {
            float s = voices[0].next() + voices[1].next();
            // safety soft limiter above -1 dBFS
            const float a = std::abs (s);
            if (a > 0.89f) s = std::copysign (0.89f + 0.1f * std::tanh ((a - 0.89f) / 0.1f), s);
            left[pos] = s;
        }
    };

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        render (std::min (meta.samplePosition, n));
        if (m.isNoteOn())
            startKick (m.getFloatVelocity(), m.getNoteNumber());
    }
    render (n);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, n);
}

void CiroKickProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void CiroKickProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* CiroKickProcessor::createEditor() { return new CiroKickEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CiroKickProcessor(); }
