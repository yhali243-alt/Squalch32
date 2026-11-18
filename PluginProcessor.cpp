#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static String formatValue (float v, const std::string& u)
{
    if (u == "Hz")  return v >= 1000.f ? String (v / 1000.f, 2) + " kHz" : String (v, v < 10.f ? 2 : 1) + " Hz";
    if (u == "s")   return v < 1.f ? String (v * 1000.f, 0) + " ms" : String (v, 2) + " s";
    if (u == "dB")  return String (v, 1) + " dB";
    if (u == "st")  return String (v, 1) + " st";
    if (u == "ct")  return String (v, 0) + " ct";
    if (u == "x")   return String (v, 2) + "x";
    if (u == "oct") return String (v, 2) + " oct";
    return String (v, 3);
}

AudioProcessorValueTreeState::ParameterLayout SqueFMProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : sq::paramTable())
    {
        const ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case sq::Desc::Float:
            {
                NormalisableRange<float> r (d.min, d.max);
                if (d.centre > d.min) r.setSkewForCentre (d.centre);
                const std::string unit = d.unit;
                layout.add (std::make_unique<AudioParameterFloat> (pid, d.name, r, d.def,
                    AudioParameterFloatAttributes().withLabel (unit)
                        .withStringFromValueFunction ([unit] (float v, int) { return formatValue (v, unit); })));
                break;
            }
            case sq::Desc::Choice:
            {
                StringArray c; for (auto& s : d.choices) c.add (String::fromUTF8 (s.c_str()));
                layout.add (std::make_unique<AudioParameterChoice> (pid, d.name, c, (int) d.def));
                break;
            }
            case sq::Desc::Bool: layout.add (std::make_unique<AudioParameterBool> (pid, d.name, d.def > 0.5f)); break;
            case sq::Desc::Int:  layout.add (std::make_unique<AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def)); break;
        }
    }
    return layout;
}

SqueFMProcessor::SqueFMProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SQUEFM", createLayout()),
      presets (sq::factoryPresets())
{
    for (const auto& d : sq::paramTable())
        bound.push_back ({ &d, apvts.getRawParameterValue (d.id) });
    int start = presets.size() > 1 ? 1 : 0;
    for (int i = 0; i < (int) presets.size(); ++i) if (presets[(size_t) i].name == "A Fat WOW") start = i;
    applyPreset (presets[(size_t) start].p); currentProgram = start;
}

bool SqueFMProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::stereo() || out == AudioChannelSet::mono();
}

void SqueFMProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    scratch.setSize (2, jmax (1, samplesPerBlock));
}

void SqueFMProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    for (auto& b : bound) b.d->set (params, b.v->load (std::memory_order_relaxed));
    engine.setParams (params);

    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            playing = pos->getIsPlaying();
        }
    hostBpm.store ((float) bpm);
    engine.setTransport (bpm, ppq, playing);

    const int n = buffer.getNumSamples();
    if (scratch.getNumSamples() < n) scratch.setSize (2, n, false, false, true);
    float* L = scratch.getWritePointer (0);
    float* R = scratch.getWritePointer (1);

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = jlimit (0, n, meta.samplePosition);
        if (t > pos) { engine.render (L + pos, R + pos, t - pos); pos = t; }
        if (m.isNoteOn())                engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())          engine.noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())       engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.f);
        else if (m.isController() && m.getControllerNumber() == 1) engine.modWheel ((float) m.getControllerValue() / 127.f);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    buffer.clear();
    if (buffer.getNumChannels() >= 2) { buffer.copyFrom (0, 0, L, n); buffer.copyFrom (1, 0, R, n); }
    else if (buffer.getNumChannels() == 1) { buffer.copyFrom (0, 0, L, n, 0.5f); buffer.addFrom (0, 0, R, n, 0.5f); }
}

// ------------------------------------------------------------ presets ------
int SqueFMProcessor::getNumPrograms() { return (int) presets.size(); }

const String SqueFMProcessor::getProgramName (int index)
{
    return isPositiveAndBelow (index, (int) presets.size()) ? String (presets[(size_t) index].name) : String();
}

void SqueFMProcessor::setCurrentProgram (int index)
{
    if (! isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;
    applyPreset (presets[(size_t) index].p);
}

void SqueFMProcessor::setParam (const String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

float SqueFMProcessor::getParam (const String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load();
    return 0.f;
}

void SqueFMProcessor::applyPreset (const sq::Params& p)
{
    for (const auto& d : sq::paramTable()) setParam (d.id, d.get (p));
}

void SqueFMProcessor::applyFmFamily (int family)
{
    sq::Params cur;
    for (auto& b : bound) b.d->set (cur, b.v->load());
    sq::applyFmFamily (cur, family);
    for (const auto& d : sq::paramTable())
        if (d.id.rfind ("op", 0) == 0 || d.id.rfind ("fv", 0) == 0) setParam (d.id, d.get (cur));
}

void SqueFMProcessor::randomizeSteps()
{
    auto& r = Random::getSystemRandom();
    static const int syncBag[]  = { 0, 0, 3, 3, 5, 5, 5, 2, 4, 1, 8, 7 };
    static const int shapeBag[] = { 0, 0, 1, 2, 3, 4, 5, 6, 6, 7, 7, 7, 8, 9 };
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const float lo = 0.15f + r.nextFloat() * 0.3f;
        const float hi = jmin (0.95f, lo + 0.08f + r.nextFloat() * 0.4f);
        setParam (sq::stepId (i, "lo"), lo);
        setParam (sq::stepId (i, "hi"), hi);
        setParam (sq::stepId (i, "shape"), (float) shapeBag[r.nextInt ((int) std::size (shapeBag))]);
        const bool hz = r.nextInt (5) == 0;
        setParam (sq::stepId (i, "mode"), hz ? 1.f : 0.f);
        setParam (sq::stepId (i, "sync"), (float) syncBag[r.nextInt ((int) std::size (syncBag))]);
        setParam (sq::stepId (i, "hz"), 1.f + r.nextFloat() * r.nextFloat() * 30.f);
    }
}

void SqueFMProcessor::mutateSteps()
{
    auto& r = Random::getSystemRandom();
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        for (auto* w : { "lo", "hi" })
        {
            const auto id = sq::stepId (i, w);
            setParam (id, jlimit (0.f, 1.f, getParam (id) + (r.nextFloat() - 0.5f) * 0.12f));
        }
        if (r.nextInt (6) == 0)
        {
            const auto id = sq::stepId (i, "sync");
            setParam (id, (float) jlimit (0, sq::kNumStepSync - 1, (int) getParam (id) + (r.nextBool() ? 1 : -1)));
        }
        if (r.nextInt (8) == 0) setParam (sq::stepId (i, "shape"), (float) r.nextInt (sq::kNumStepShapes));
    }
}

void SqueFMProcessor::clearSteps()
{
    const sq::StepParams d;
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        setParam (sq::stepId (i, "lo"), d.lo); setParam (sq::stepId (i, "hi"), d.hi);
        setParam (sq::stepId (i, "shape"), (float) d.shape); setParam (sq::stepId (i, "mode"), (float) d.mode);
        setParam (sq::stepId (i, "sync"), (float) d.sync); setParam (sq::stepId (i, "hz"), d.hz);
    }
}

void SqueFMProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void SqueFMProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            // a project saved by an older version lacks newer parameters: give those their
            // defaults instead of leaving whatever value the instance currently holds
            for (const auto& d : sq::paramTable())
                if (! tree.getChildWithProperty ("id", String (d.id)).isValid())
                    if (auto* p = apvts.getParameter (d.id))
                    {
                        ValueTree c ("PARAM"); c.setProperty ("id", String (d.id), nullptr);
                        c.setProperty ("value", p->convertFrom0to1 (p->getDefaultValue()), nullptr);
                        tree.appendChild (c, nullptr);
                    }
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* SqueFMProcessor::createEditor() { return new SqueFMEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SqueFMProcessor(); }
