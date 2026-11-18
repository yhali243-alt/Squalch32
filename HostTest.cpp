#include <functional>
// Headless smoke test: processor + editor snapshot + state round-trip.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String outDir = argc > 1 ? argv[1] : ".";
    std::unique_ptr<SqueFMProcessor> proc (new SqueFMProcessor());
    proc->setPlayConfigDetails (0, 2, 48000.0, 512);
    proc->prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    double peak = 0; bool bad = false;
    for (int prog = 0; prog < proc->getNumPrograms(); ++prog)
    {
        proc->setCurrentProgram (prog);
        for (int b = 0; b < 200; ++b)
        {
            juce::MidiBuffer m;
            if (b % 20 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 40 + (b / 20) % 5, (juce::uint8) 100), 17);
            if (b % 20 == 15) m.addEvent (juce::MidiMessage::noteOff (1, 40 + (b / 20) % 5), 300);
            if (b == 50) m.addEvent (juce::MidiMessage::pitchWheel (1, 12000), 0);
            proc->processBlock (buf, m);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 512; ++i)
                {
                    const float v = buf.getSample (c, i);
                    if (! std::isfinite (v)) bad = true;
                    peak = std::max (peak, (double) std::abs (v));
                }
        }
        std::cout << "program " << prog << " " << proc->getProgramName (prog) << " ok\n";
    }
    std::cout << "peak " << peak << (bad ? " NaN!" : " finite") << "\n";

    // state round trip
    proc->setCurrentProgram (3);
    proc->randomizeSteps();
    juce::MemoryBlock mb; proc->getStateInformation (mb);
    const float before = proc->apvts.getRawParameterValue ("s5hi")->load();
    proc->setCurrentProgram (0);
    proc->setStateInformation (mb.getData(), (int) mb.getSize());
    const float after = proc->apvts.getRawParameterValue ("s5hi")->load();
    std::cout << "state roundtrip " << (std::abs (before - after) < 1e-6 ? "ok" : "FAIL") << " (" << before << " / " << after << ")\n";
    std::cout << "num params " << proc->getParameters().size() << "\n";
    {   // old-project compatibility: a state without "quality" must load it as default
        juce::MemoryBlock m2; proc->getStateInformation (m2);
        auto xml = juce::AudioProcessor::getXmlFromBinary (m2.getData(), (int) m2.getSize());
        auto tree = juce::ValueTree::fromXml (*xml);
        tree.removeChild (tree.getChildWithProperty ("id", "quality"), nullptr);
        auto* q = dynamic_cast<juce::RangedAudioParameter*> (proc->getParameters()[0]);
        for (auto* p : proc->getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p)) if (r->getParameterID() == "quality") q = r;
        q->setValueNotifyingHost (0.f);
        juce::MemoryBlock old; juce::AudioProcessor::copyXmlToBinary (*tree.createXml(), old);
        proc->setStateInformation (old.getData(), (int) old.getSize());
        std::cout << "old-state default fill " << (q->getValue() > 0.5f ? "ok" : "FAIL") << "\n";
    }

    proc->setCurrentProgram (1);
    // let the sequencer run so the GUI shows a playhead
    for (int b = 0; b < 37; ++b) { juce::MidiBuffer m; if (b == 0) m.addEvent (juce::MidiMessage::noteOn (1, 40, (juce::uint8) 100), 0); proc->processBlock (buf, m); }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
    ed->setSize (SqueFMEditor::W, SqueFMEditor::H);
    const char* tabNames[] = { "MAIN", "FM", "SYNTH", "SEQUENCER", "MOD", "FX" };
    for (int t = 0; t < 6; ++t)
    {
        // click the tab button by name
        std::function<juce::Button*(juce::Component*)> findBtn = [&] (juce::Component* c) -> juce::Button* {
            if (auto* b = dynamic_cast<juce::TextButton*> (c)) if (b->getButtonText() == tabNames[t]) return b;
            for (auto* ch : c->getChildren()) if (auto* r = findBtn (ch)) return r;
            return nullptr; };
        if (auto* b = findBtn (ed.get())) if (b->onClick) b->onClick();
        ed->repaint();
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (outDir + "/gui_" + juce::String (tabNames[t]) + ".png"); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    std::cout << "gui snapshot written\n";
    ed.reset();
    proc.reset();
    return bad ? 1 : 0;
}
