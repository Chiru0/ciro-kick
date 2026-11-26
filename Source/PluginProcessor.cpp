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
    add ("sub",    "Sub",         juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f);
    add ("metal",  "Metal",       juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f);
    add ("mTone",  "Metal Tone",  skewed (150.0f, 3000.0f, 600.0f), 500.0f, "Hz");
    add ("reso",   "Resonance",   skewed (0.5f, 8.0f, 1.5f), 0.7f);
    add ("punch",  "Punch",       juce::NormalisableRange<float> (0.0f, 120.0f), 0.0f, "ms");
    add ("rAmt",   "Rumble Amount", juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f);
    add ("rSize",  "Rumble Size",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.75f);
    add ("rDrive", "Rumble Drive",  juce::NormalisableRange<float> (0.0f, 30.0f), 16.0f, "dB");
    add ("rTone",  "Rumble Tone",   skewed (40.0f, 400.0f, 140.0f), 140.0f, "Hz");
    add ("rDuck",  "Rumble Duck",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f);
    add ("rRoll",  "Rumble Roll",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "rRate", 1 }, "Rumble Rate",
                                                               juce::StringArray { "1/8", "1/16", "1/32" }, 1));
    add ("rPre",   "Rumble Pre-Delay", juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f, "ms");
    add ("rDamp",  "Rumble Damp",   juce::NormalisableRange<float> (0.0f, 0.95f), 0.35f);
    add ("rLow",   "Rumble Low Cut", skewed (20.0f, 250.0f, 50.0f), 25.0f, "Hz");
    add ("rGrit",  "Rumble Grit",   juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "bOsc", 1 }, "Bass Osc",
                                                               juce::StringArray { "SAW", "SQUARE", "SINE", "REESE" }, 0));
    add ("bAtk",   "Bass Attack",   skewed (0.3f, 40.0f, 4.0f), 1.0f, "ms");
    add ("bEnv",   "Bass Filter Env", juce::NormalisableRange<float> (0.0f, 10.0f), 5.0f);
    add ("bReso",  "Bass Resonance", skewed (0.5f, 6.0f, 1.2f), 0.9f);
    add ("bDrive", "Bass Drive",    juce::NormalisableRange<float> (0.0f, 24.0f), 4.0f, "dB");
    add ("bGlide", "Bass Glide",    juce::NormalisableRange<float> (0.0f, 24.0f), 0.0f, "st");
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "bPat", 1 }, "Bass Pattern",
                                                               juce::StringArray { "ROLLING", "OFFBEAT", "GALLOP", "REVERSE" }, 0));
    add ("bLevel", "Bass Level",    juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f);
    add ("bSemi",  "Bass Note",     juce::NormalisableRange<float> (-12.0f, 24.0f, 1.0f), 0.0f, "st");
    add ("bDecay", "Bass Decay",    skewed (20.0f, 400.0f, 110.0f), 110.0f, "ms");
    add ("bCut",   "Bass Cutoff",   skewed (80.0f, 4000.0f, 800.0f), 800.0f, "Hz");
    add ("volume", "Volume",      juce::NormalisableRange<float> (-36.0f, 6.0f), -3.0f, "dB");
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "keyTrack", 1 }, "Key Track", false));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "genre", 1 }, "Genre",
        juce::StringArray { "SCHRANZ", "PSYTRANCE", "HARD BOUNCE", "HARDSTYLE", "HARD TECHNO", "GABBER", "TECHNO", "INDUSTRIAL" }, 0));
    const KickParams defaults;
    for (int i = 0; i < 5; ++i)
    {
        const auto n = juce::String (i);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "rEnvT" + n, 1 }, "Rumble Shape Time " + n,
                                                                  juce::NormalisableRange<float> (0.0f, 1.0f), defaults.rEnvT[i]));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "rEnvG" + n, 1 }, "Rumble Shape Gain " + n,
                                                                  juce::NormalisableRange<float> (0.0f, 2.0f), defaults.rEnvG[i]));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "bEnvT" + n, 1 }, "Bass Shape Time " + n,
                                                                  juce::NormalisableRange<float> (0.0f, 1.0f), defaults.bEnvT[i]));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "bEnvG" + n, 1 }, "Bass Shape Gain " + n,
                                                                  juce::NormalisableRange<float> (0.0f, 2.0f), defaults.bEnvG[i]));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "envT" + n, 1 }, "Shaper Time " + n,
                                                                  juce::NormalisableRange<float> (0.0f, 1.0f), defaults.envT[i]));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "envG" + n, 1 }, "Shaper Gain " + n,
                                                                  juce::NormalisableRange<float> (0.0f, 2.0f), 1.0f));
    }
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "loop", 1 }, "Loop", false));

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
    engine.setSampleRate (sampleRate);
    sampleRateHz = sampleRate;
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
    k.reso       = val (apvts, "reso");
    k.sub        = val (apvts, "sub");
    k.metal      = val (apvts, "metal");
    k.metalTone  = val (apvts, "mTone");
    k.punch      = val (apvts, "punch");
    k.rumbleRoll = val (apvts, "rRoll");
    for (int i = 0; i < 5; ++i)
    {
        k.envT[i] = val (apvts, ("envT" + juce::String (i)).toRawUTF8());
        k.envG[i] = val (apvts, ("envG" + juce::String (i)).toRawUTF8());
        k.rEnvT[i] = val (apvts, ("rEnvT" + juce::String (i)).toRawUTF8());
        k.rEnvG[i] = val (apvts, ("rEnvG" + juce::String (i)).toRawUTF8());
        k.bEnvT[i] = val (apvts, ("bEnvT" + juce::String (i)).toRawUTF8());
        k.bEnvG[i] = val (apvts, ("bEnvG" + juce::String (i)).toRawUTF8());
    }
    k.rumbleRate = (int) val (apvts, "rRate");
    k.rumblePre  = val (apvts, "rPre");
    k.rumbleDamp = val (apvts, "rDamp");
    k.rumbleLow  = val (apvts, "rLow");
    k.rumbleGrit = val (apvts, "rGrit");
    k.bassOsc    = (int) val (apvts, "bOsc");
    k.bassAttack = val (apvts, "bAtk");
    k.bassEnvAmt = val (apvts, "bEnv");
    k.bassReso   = val (apvts, "bReso");
    k.bassDrive  = val (apvts, "bDrive");
    k.bassGlide  = val (apvts, "bGlide");
    k.rumbleAmt   = val (apvts, "rAmt");
    k.rumbleSize  = val (apvts, "rSize");
    k.rumbleDrive = val (apvts, "rDrive");
    k.rumbleTone  = val (apvts, "rTone");
    k.rumbleDuck  = val (apvts, "rDuck");
    k.bassPattern = (int) val (apvts, "bPat");
    k.bassLevel   = val (apvts, "bLevel");
    k.bassSemi    = val (apvts, "bSemi");
    k.bassDecay   = val (apvts, "bDecay");
    k.bassCutoff  = val (apvts, "bCut");
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
    setParam (apvts, "reso",   k.reso);
    setParam (apvts, "sub",    k.sub);
    setParam (apvts, "metal",  k.metal);
    setParam (apvts, "mTone",  k.metalTone);
    setParam (apvts, "punch",  k.punch);
    setParam (apvts, "rRoll",  k.rumbleRoll);
    for (int i = 0; i < 5; ++i)
    {
        setParam (apvts, ("envT" + juce::String (i)).toRawUTF8(), k.envT[i]);
        setParam (apvts, ("envG" + juce::String (i)).toRawUTF8(), k.envG[i]);
        setParam (apvts, ("rEnvT" + juce::String (i)).toRawUTF8(), k.rEnvT[i]);
        setParam (apvts, ("rEnvG" + juce::String (i)).toRawUTF8(), k.rEnvG[i]);
        setParam (apvts, ("bEnvT" + juce::String (i)).toRawUTF8(), k.bEnvT[i]);
        setParam (apvts, ("bEnvG" + juce::String (i)).toRawUTF8(), k.bEnvG[i]);
    }
    setParam (apvts, "rRate",  (float) k.rumbleRate);
    setParam (apvts, "rPre",   k.rumblePre);
    setParam (apvts, "rDamp",  k.rumbleDamp);
    setParam (apvts, "rLow",   k.rumbleLow);
    setParam (apvts, "rGrit",  k.rumbleGrit);
    setParam (apvts, "bOsc",   (float) k.bassOsc);
    setParam (apvts, "bAtk",   k.bassAttack);
    setParam (apvts, "bEnv",   k.bassEnvAmt);
    setParam (apvts, "bReso",  k.bassReso);
    setParam (apvts, "bDrive", k.bassDrive);
    setParam (apvts, "bGlide", k.bassGlide);
    setParam (apvts, "rAmt",   k.rumbleAmt);
    setParam (apvts, "rSize",  k.rumbleSize);
    setParam (apvts, "rDrive", k.rumbleDrive);
    setParam (apvts, "rTone",  k.rumbleTone);
    setParam (apvts, "rDuck",  k.rumbleDuck);
    setParam (apvts, "bPat",   (float) k.bassPattern);
    setParam (apvts, "bLevel", k.bassLevel);
    setParam (apvts, "bSemi",  k.bassSemi);
    setParam (apvts, "bDecay", k.bassDecay);
    setParam (apvts, "bCut",   k.bassCutoff);
}

juce::String CiroKickProcessor::getPresetName() const
{
    return apvts.state.getProperty ("presetName", "INIT").toString();
}

void CiroKickProcessor::setPresetName (const juce::String& n)
{
    apvts.state.setProperty ("presetName", n, nullptr);
}

void CiroKickProcessor::loadPreset (int genre, int index)
{
    history.push_back ({ readParams(), getPresetName() });
    if (history.size() > 50) history.erase (history.begin());
    setParam (apvts, "genre", (float) genre);
    writeParams (presetParams (genre, index, readParams()));
    setPresetName (presetBank().names[(size_t) genre][(size_t) index]);
    presetIndex = index;
    triggerFromUI();
}

int CiroKickProcessor::getGenre() const { return (int) val (apvts, "genre"); }

void CiroKickProcessor::randomize()
{
    const auto cur = readParams();
    history.push_back ({ cur, getPresetName() });
    if (history.size() > 50) history.erase (history.begin());
    writeParams (randomKick (getGenre(), rng, cur));
    setPresetName ("RANDOM #" + juce::String (++randomCount));
    triggerFromUI();
}

void CiroKickProcessor::mutate()
{
    const auto cur = readParams();
    history.push_back ({ cur, getPresetName() });
    if (history.size() > 50) history.erase (history.begin());
    writeParams (mutateKick (cur, rng));
    if (! getPresetName().endsWith ("*")) setPresetName (getPresetName() + " *");
    triggerFromUI();
}

void CiroKickProcessor::setSteps (juce::int64 mask)
{
    steps = mask;
    apvts.state.setProperty ("steps", mask, nullptr);
}

void CiroKickProcessor::setLoopLength (int len)
{
    loopLen = std::clamp (len, 1, 32);
    apvts.state.setProperty ("loopLen", loopLen.load(), nullptr);
}

bool CiroKickProcessor::undoRandom()
{
    if (history.empty()) return false;
    writeParams (history.back().first);
    setPresetName (history.back().second);
    history.pop_back();
    triggerFromUI();
    return true;
}

void CiroKickProcessor::startKick (float velocity, int note)
{
    const auto k = readParams();
    const float baseHz = (k.keyTrack && note >= 0) ? (float) juce::MidiMessage::getMidiNoteInHertz (note) : -1.0f;
    engine.trigger (k, velocity, baseHz, bpm.load());
    ++hitCounter;
}

void CiroKickProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int n = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);

    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                if (*b > 20.0) bpm = *b;
            if (pos->getIsPlaying())
                if (auto ppq = pos->getPpqPosition()) { hostPlaying = true; loopPpq = *ppq; }
        }
    const bool looping = val (apvts, "loop") > 0.5f;
    const double ppqPerSample = bpm.load() / 60.0 / sampleRateHz;
    const juce::int64 stepMask = steps.load();
    const int len = loopLen.load();

    engine.setParams (readParams());

    if (uiTrigger.exchange (false))
        startKick (1.0f, -1);

    int pos = 0;
    auto render = [&] (int end)
    {
        for (; pos < end; ++pos)
        {
            if (looping)
            {
                const int step = (int) (((juce::int64) std::floor (loopPpq * 4.0)) % len);
                if (step != lastStep)
                {
                    lastStep = step;
                    currentStep = step;
                    if ((stepMask >> step) & 1) startKick (1.0f, -1);
                }
                loopPpq += ppqPerSample;
            }
            else currentStep = -1;
            float s = engine.next();
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
    if (! looping) { lastStep = -1; if (! hostPlaying) loopPpq = 0.0; }

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
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            steps = (juce::int64) apvts.state.getProperty ("steps", (juce::int64) 0x11111111);
            loopLen = std::clamp ((int) apvts.state.getProperty ("loopLen", 16), 1, 32);
        }
}

juce::AudioProcessorEditor* CiroKickProcessor::createEditor() { return new CiroKickEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CiroKickProcessor(); }
