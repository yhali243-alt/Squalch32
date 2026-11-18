#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui
{
namespace col
{
    const juce::Colour bg0     { 0xff060b09 };
    const juce::Colour bg1     { 0xff0d1813 };
    const juce::Colour panel   { 0xff0f1c16 };
    const juce::Colour border  { 0xff1d3a2d };
    const juce::Colour lime    { 0xffb8ff3c };
    const juce::Colour toxic   { 0xff2dffa0 };
    const juce::Colour magenta { 0xffff3fa4 };
    const juce::Colour violet  { 0xff9b6bff };
    const juce::Colour amber   { 0xffffb341 };
    const juce::Colour text    { 0xffd2eadf };
    const juce::Colour dim     { 0xff6c8c7d };
}

inline juce::Font font (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

// ------------------------------------------------------------------ L&F ----
class SwampLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SwampLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (12.f, true); }
    juce::Font getPopupMenuFont() override { return font (13.f); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (11.f, true); }
    juce::Font getSliderPopupFont (juce::Slider&) override { return font (12.f, true); }
};

// ------------------------------------------------------------ one control ---
class ControlCell : public juce::Component
{
public:
    ControlCell (juce::AudioProcessorValueTreeState&, const juce::String& paramId,
                 const juce::String& label, juce::Colour accent, juce::Component* popupParent);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Colour accent;

private:
    juce::String labelText;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

// -------------------------------------------------------- step sequencer ---
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (Squalch32Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum Lane { None, Curve, Depth, Shape, Rate };
    Squalch32Processor& proc;
    std::array<juce::RangedAudioParameter*, sq::kMaxSteps> pRate {}, pShape {}, pDepth {}, pOff {};
    juce::RangedAudioParameter* pSteps = nullptr;
    juce::RangedAudioParameter* pSpeed = nullptr;
    juce::Rectangle<float> gridArea, laneCurve, laneDepth, laneShape, laneRate, laneNums, gutter;
    Lane dragLane = None; int dragStep = -1; int hoverStep = -1; Lane hoverLane = None;
    float rateDragStartY = 0.f; int rateDragStartVal = 0;
    std::vector<juce::RangedAudioParameter*> touched;

    int stepAt (float x) const;
    Lane laneAt (juce::Point<float>) const;
    float colX (int i) const { return gridArea.getX() + gridArea.getWidth() * (float) i / sq::kMaxSteps; }
    float colW() const { return gridArea.getWidth() / sq::kMaxSteps; }
    float val (juce::RangedAudioParameter* p) const { return p->convertFrom0to1 (p->getValue()); }
    void setVal (juce::RangedAudioParameter* p, float v);
    void endAll();
    void setFromY (Lane lane, int step, float y);
    void showMenu (Lane lane, int step);
    static void drawShapeIcon (juce::Graphics&, int shape, juce::Rectangle<float>);
    static float previewShape (int shape, float p, int seed);
};

// ---------------------------------------------------------- filter scope ---
class FilterDisplay : public juce::Component
{
public:
    explicit FilterDisplay (Squalch32Processor& p);
    void paint (juce::Graphics&) override;
private:
    Squalch32Processor& proc;
    std::atomic<float>* res; std::atomic<float>* slope; std::atomic<float>* mode;
};

class ModScope : public juce::Component
{
public:
    explicit ModScope (Squalch32Processor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    Squalch32Processor& proc;
};
} // namespace ui

// ================================================================ EDITOR ====
class Squalch32Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Squalch32Editor (Squalch32Processor&);
    ~Squalch32Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1240, H = 820;

private:
    struct Panel
    {
        juce::String title; juce::Rectangle<int> r; int cols; juce::Colour accent;
        std::vector<ui::ControlCell*> items; int topExtra = 0; int leftWidth = 0;
    };

    // Everything is laid out at 1240x820 inside `content` and scaled as a whole.
    class Content : public juce::Component
    {
    public:
        explicit Content (Squalch32Editor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
    private:
        Squalch32Editor& ed;
    };

    void timerCallback() override;
    void paintContent (juce::Graphics&);
    void buildBackground();
    Panel& addPanel (const juce::String& title, juce::Rectangle<int> r, int cols, juce::Colour accent,
                     std::initializer_list<std::pair<const char*, const char*>> controls);
    void layoutPanels();

    Squalch32Processor& proc;
    ui::SwampLookAndFeel lnf;
    Content content { *this };
    std::vector<std::unique_ptr<ui::ControlCell>> cells;
    std::vector<Panel> panels;
    ui::StepGrid grid;
    ui::FilterDisplay filterDisplay;
    ui::ModScope scope;
    juce::ComboBox presetBox;
    juce::TextButton randBtn { "RANDOM" }, mutateBtn { "MUTATE" }, clearBtn { "CLEAR" };
    juce::Image background;
    juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;
    int lastProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Squalch32Editor)
};
