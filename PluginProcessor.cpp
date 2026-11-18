#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static String formatValue (float v, const char* unit)
{
    const String u (unit);
    if (u == "Hz")  return v >= 1000.f ? String (v / 1000.f, 2) + " kHz" : String (v, v < 10.f ? 2 : 0) + " Hz";
    if (u == "s")   return v < 1.f ? String (v * 1000.f, 0) + " ms" : String (v, 2) + " s";
    if (u == "dB")  return String (v, 1) + " dB";
    if (u == "st")  return String (v, 1) + " st";
    if (u == "ct")  return String (v, 0) + " ct";
    if (u == "oct") return String (v, 2) + " oct";
    if (u == "x")   return String (v, 2) + "x";
    return String (v, 3);
}

AudioProcessorValueTreeState::ParameterLayout Squalch32Processor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    const sq::Params defaults;

    for (const auto& d : sq::paramTable())
    {
        const float def = sq::getField (defaults, d);
        const ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case sq::Desc::Float:
            {
                NormalisableRange<float> r (d.min, d.max);
                if (d.centre > d.min) r.setSkewForCentre (d.centre);
                const char* unit = d.unit;
                layout.add (std::make_unique<AudioParameterFloat> (pid, d.name, r, def,
                    AudioParameterFloatAttributes().withLabel (unit)
                        .withStringFromValueFunction ([unit] (float v, int) { return formatValue (v, unit); })));
                break;
            }
            case sq::Desc::Choice:
            {
                StringArray c; for (int i = 0; i < d.numChoices; ++i) c.add (d.choices[i]);
                layout.add (std::make_unique<AudioParameterChoice> (pid, d.name, c, (int) def));
                break;
            }
            case sq::Desc::Bool:
                layout.add (std::make_unique<AudioParameterBool> (pid, d.name, def > 0.5f));
                break;
            case sq::Desc::Int:
                layout.add (std::make_unique<AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) def));
                break;
        }
    }

    StringArray rates, shapes;
    for (auto* n : sq::kStepRateNames) rates.add (n);
    for (auto* n : sq::kShapeNames) shapes.add (n);
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const auto& s = defaults.steps[(size_t) i];
        const String n = "Step " + String (i + 1) + " ";
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { sq::stepId (i, "rate"), 1 },  n + "Rate",  rates,  s.rate));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { sq::stepId (i, "shape"), 1 }, n + "Shape", shapes, s.shape));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { sq::stepId (i, "depth"), 1 }, n + "Depth", NormalisableRange<float> (-1.f, 1.f), s.depth));
        layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { sq::stepId (i, "off"), 1 },   n + "Offset", NormalisableRange<float> (-1.f, 1.f), s.offset));
    }
    return layout;
}

Squalch32Processor::Squalch32Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SQUALCH32", createLayout()),
      presets (sq::factoryPresets())
{
    for (const auto& d : sq::paramTable())
        bound.push_back ({ &d, apvts.getRawParameterValue (d.id) });
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        stRate[(size_t) i]  = apvts.getRawParameterValue (sq::stepId (i, "rate"));
        stShape[(size_t) i] = apvts.getRawParameterValue (sq::stepId (i, "shape"));
        stDepth[(size_t) i] = apvts.getRawParameterValue (sq::stepId (i, "depth"));
        stOff[(size_t) i]   = apvts.getRawParameterValue (sq::stepId (i, "off"));
    }
    // start on a playable sound rather than Init
    if (presets.size() > 1) { applyPreset (presets[1].p); currentProgram = 1; }
}

bool Squalch32Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == AudioChannelSet::stereo() || out == AudioChannelSet::mono();
}

void Squalch32Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    scratch.setSize (2, jmax (1, samplesPerBlock));
}

void Squalch32Processor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;

    for (auto& b : bound) sq::setField (params, *b.d, b.v->load (std::memory_order_relaxed));
    for (size_t i = 0; i < (size_t) sq::kMaxSteps; ++i)
    {
        auto& s = params.steps[i];
        s.rate   = (int) stRate[i]->load();
        s.shape  = (int) stShape[i]->load();
        s.depth  = stDepth[i]->load();
        s.offset = stOff[i]->load();
    }
    engine.setParams (params);

    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            playing = pos->getIsPlaying();
        }
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

        if (m.isNoteOn())                      engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                engine.noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())             engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.f);
        else if (m.isController() && m.getControllerNumber() == 1) engine.modWheel ((float) m.getControllerValue() / 127.f);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    buffer.clear();
    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, L, n);
        buffer.copyFrom (1, 0, R, n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, L, n, 0.5f);
        buffer.addFrom  (0, 0, R, n, 0.5f);
    }
}

// ------------------------------------------------------------ presets ------
int Squalch32Processor::getNumPrograms() { return (int) presets.size(); }

const String Squalch32Processor::getProgramName (int index)
{
    return isPositiveAndBelow (index, (int) presets.size()) ? String (presets[(size_t) index].name) : String();
}

void Squalch32Processor::setCurrentProgram (int index)
{
    if (! isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;
    applyPreset (presets[(size_t) index].p);
}

void Squalch32Processor::setParam (const String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

void Squalch32Processor::applyPreset (const sq::Params& p)
{
    for (const auto& d : sq::paramTable()) setParam (d.id, sq::getField (p, d));
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const auto& s = p.steps[(size_t) i];
        setParam (sq::stepId (i, "rate"),  (float) s.rate);
        setParam (sq::stepId (i, "shape"), (float) s.shape);
        setParam (sq::stepId (i, "depth"), s.depth);
        setParam (sq::stepId (i, "off"),   s.offset);
    }
}

void Squalch32Processor::randomizeSteps()
{
    auto& r = Random::getSystemRandom();
    // musically weighted: mostly 1/2..4 cycles, plenty of squelch & smooth shapes, some static steps
    static const int rateBag[]  = { 0, 0, 2, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 12 };
    static const int shapeBag[] = { 0, 0, 1, 1, 2, 3, 4, 5, 6, 6, 7, 7, 7, 8, 8 };
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        setParam (sq::stepId (i, "rate"),  (float) rateBag[r.nextInt ((int) std::size (rateBag))]);
        setParam (sq::stepId (i, "shape"), (float) shapeBag[r.nextInt ((int) std::size (shapeBag))]);
        setParam (sq::stepId (i, "depth"), (r.nextFloat() * 0.7f + 0.3f) * (r.nextInt (5) == 0 ? -1.f : 1.f));
        setParam (sq::stepId (i, "off"),   (r.nextFloat() - 0.5f) * 1.0f);
    }
}

void Squalch32Processor::mutateSteps()
{
    auto& r = Random::getSystemRandom();
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const auto depthId = sq::stepId (i, "depth"), offId = sq::stepId (i, "off");
        setParam (depthId, jlimit (-1.f, 1.f, apvts.getRawParameterValue (depthId)->load() + (r.nextFloat() - 0.5f) * 0.3f));
        setParam (offId,   jlimit (-1.f, 1.f, apvts.getRawParameterValue (offId)->load()   + (r.nextFloat() - 0.5f) * 0.3f));
        if (r.nextInt (6) == 0)
        {
            const auto rateId = sq::stepId (i, "rate");
            const int cur = (int) apvts.getRawParameterValue (rateId)->load();
            setParam (rateId, (float) jlimit (0, sq::kNumStepRates - 1, cur + (r.nextBool() ? 1 : -1)));
        }
    }
}

void Squalch32Processor::clearSteps()
{
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        setParam (sq::stepId (i, "rate"), 4.f);
        setParam (sq::stepId (i, "shape"), 0.f);
        setParam (sq::stepId (i, "depth"), 0.f);
        setParam (sq::stepId (i, "off"), 0.f);
    }
}

// -------------------------------------------------------------- state ------
void Squalch32Processor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void Squalch32Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* Squalch32Processor::createEditor() { return new Squalch32Editor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Squalch32Processor(); }
