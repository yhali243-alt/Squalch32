#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui
{
namespace col
{
    const juce::Colour bg0    { 0xff07040d };
    const juce::Colour bg1    { 0xff130b22 };
    const juce::Colour panel  { 0xff160d27 };
    const juce::Colour border { 0xff2c1d4a };
    const juce::Colour pink   { 0xffff3fb4 };
    const juce::Colour cyan   { 0xff33e6ff };
    const juce::Colour lime   { 0xffb8ff3c };
    const juce::Colour gold   { 0xffffc53f };
    const juce::Colour violet { 0xffa275ff };
    const juce::Colour text   { 0xffe9e0ff };
    const juce::Colour dim    { 0xff8a7aa8 };
}

inline juce::Font font (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

class DiscoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DiscoLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (12.f, true); }
    juce::Font getPopupMenuFont() override { return font (13.f); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (11.5f, true); }
    juce::Font getSliderPopupFont (juce::Slider&) override { return font (12.f, true); }
};

// one knob / combo / toggle bound to a parameter
class ControlCell : public juce::Component
{
public:
    ControlCell (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& label,
                 juce::Colour accent, juce::Component* popupParent);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::String paramId;
    void setLabel (const juce::String& l) { if (l != labelText) { labelText = l; repaint(); } }
private:
    juce::String labelText; juce::Colour accent;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

// ---------------------------------------------------------- step sequencer
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (SqueFMProcessor&);
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
    enum Lane { None, Range, Shape, Mode, Rate };
    enum Handle { HNone, HLo, HHi, HBoth };
    SqueFMProcessor& proc;
    std::array<juce::RangedAudioParameter*, sq::kMaxSteps> pLo {}, pHi {}, pShape {}, pMode {}, pSync {}, pHz {};
    juce::RangedAudioParameter *pSteps = nullptr, *pSpeed = nullptr, *pDiv = nullptr;
    juce::Rectangle<float> grid, laneRange, laneShape, laneMode, laneRate, laneNums, gutter;
    Lane dragLane = None; Handle dragHandle = HNone; int dragStep = -1, hoverStep = -1; Lane hoverLane = None;
    float dragStartY = 0.f, dragStartVal = 0.f, dragStartLo = 0.f, dragStartHi = 0.f;
    std::vector<juce::RangedAudioParameter*> touched;

    int stepAt (float x) const;
    Lane laneAt (juce::Point<float>) const;
    float colX (int i) const { return grid.getX() + grid.getWidth() * (float) i / sq::kMaxSteps; }
    float colW() const { return grid.getWidth() / sq::kMaxSteps; }
    float yOf (float v) const { return laneRange.getBottom() - 4.f - v * (laneRange.getHeight() - 8.f); }
    float valOfY (float y) const { return juce::jlimit (0.f, 1.f, (laneRange.getBottom() - 4.f - y) / (laneRange.getHeight() - 8.f)); }
    float val (juce::RangedAudioParameter* p) const { return p->convertFrom0to1 (p->getValue()); }
    void setVal (juce::RangedAudioParameter*, float);
    void endAll();
    void showMenu (Lane, int step);
    juce::String rateText (int step) const;
    static void drawShapeIcon (juce::Graphics&, int shape, juce::Rectangle<float>);
};

class FilterDisplay : public juce::Component
{
public:
    explicit FilterDisplay (SqueFMProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    SqueFMProcessor& proc;
};

class ModScope : public juce::Component
{
public:
    explicit ModScope (SqueFMProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    SqueFMProcessor& proc;
};

class LfoView : public juce::Component
{
public:
    LfoView (SqueFMProcessor& p, int idx) : proc (p), index (idx) {}
    void paint (juce::Graphics&) override;
private:
    SqueFMProcessor& proc; int index;
};

class MatrixRow : public juce::Component
{
public:
    MatrixRow (SqueFMProcessor&, int slot, juce::Component* popupParent);
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    SqueFMProcessor& proc; int slot;
    juce::ComboBox src, via, dst; juce::Slider amount { juce::Slider::LinearBar, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aSrc, aVia, aDst;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aAmt;
};
} // namespace ui

// =================================================================== EDITOR
class SqueFMEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SqueFMEditor (SqueFMProcessor&);
    ~SqueFMEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int W = 1280, H = 860;

private:
    enum Page { Synth, Seq, Mod, Fx, Always };
    struct Panel
    {
        juce::String title; juce::Rectangle<int> r; int cols; juce::Colour accent; Page page;
        std::vector<ui::ControlCell*> items; int topExtra = 0; int leftWidth = 0;
        juce::Component* extra = nullptr;
    };
    class Content : public juce::Component
    {
    public:
        explicit Content (SqueFMEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
    private:
        SqueFMEditor& ed;
    };

    void timerCallback() override;
    void paintContent (juce::Graphics&);
    void buildBackground();
    Panel& addPanel (const juce::String& title, juce::Rectangle<int> r, int cols, juce::Colour accent, Page page,
                     std::initializer_list<std::pair<const char*, const char*>> controls);
    void layoutPanels();
    void showPage (Page);
    ui::ControlCell* findCell (const juce::String& id);

    SqueFMProcessor& proc;
    ui::DiscoLookAndFeel lnf;
    Content content { *this };
    std::vector<std::unique_ptr<ui::ControlCell>> cells;
    std::vector<Panel> panels;
    Page page = Seq;

    ui::StepGrid grid;
    ui::FilterDisplay filterDisplay;
    ui::ModScope scope;
    std::array<std::unique_ptr<ui::LfoView>, sq::kNumLfos> lfoViews;
    std::array<std::unique_ptr<ui::MatrixRow>, sq::kMatrixSlots> matrixRows;
    juce::Rectangle<int> matrixArea;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::TextButton randBtn { "RANDOM" }, mutateBtn { "MUTATE" }, clearBtn { "CLEAR" };
    std::array<juce::TextButton, 4> tabs;
    std::array<juce::String, sq::kNumMacros> macroHints;
    juce::Image background;
    juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;
    int lastProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueFMEditor)
};
