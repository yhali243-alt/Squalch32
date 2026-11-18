#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SqueFMEngine.h"
#include "ParamTable.h"
#include "Presets.h"

class SqueFMProcessor : public juce::AudioProcessor
{
public:
    SqueFMProcessor();
    ~SqueFMProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SqueFM"; }
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

    juce::AudioProcessorValueTreeState apvts;
    sq::Engine engine;
    std::vector<sq::Preset> presets;
    std::atomic<int> currentProgram { 0 };
    std::atomic<float> hostBpm { 130.f };

    void applyPreset (const sq::Params&);
    void randomizeSteps();
    void mutateSteps();
    void clearSteps();
    void setParam (const juce::String& id, float plainValue);
    float getParam (const juce::String& id) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    struct Bound { const sq::Desc* d; std::atomic<float>* v; };
    std::vector<Bound> bound;
    sq::Params params;
    juce::AudioBuffer<float> scratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueFMProcessor)
};
