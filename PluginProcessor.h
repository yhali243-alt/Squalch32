#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SquelchEngine.h"
#include "ParamTable.h"
#include "Presets.h"

class Squalch32Processor : public juce::AudioProcessor
{
public:
    Squalch32Processor();
    ~Squalch32Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Squalch32"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- used by the editor ----
    juce::AudioProcessorValueTreeState apvts;
    sq::Engine engine;
    std::vector<sq::Preset> presets;
    std::atomic<int> currentProgram { 0 };

    void applyPreset (const sq::Params&);
    void randomizeSteps();
    void mutateSteps();
    void clearSteps();
    void setParam (const juce::String& id, float plainValue);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Bound { const sq::Desc* d; std::atomic<float>* v; };
    std::vector<Bound> bound;
    std::array<std::atomic<float>*, sq::kMaxSteps> stRate {}, stShape {}, stDepth {}, stOff {};
    sq::Params params;
    juce::AudioBuffer<float> scratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Squalch32Processor)
};
