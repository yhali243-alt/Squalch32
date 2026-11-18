#include "PluginEditor.h"
#include <complex>

using namespace juce;

namespace ui
{
// =========================================================== LookAndFeel ===
DiscoLookAndFeel::DiscoLookAndFeel()
{
    setColour (ComboBox::backgroundColourId, col::bg1);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::outlineColourId, col::border);
    setColour (ComboBox::arrowColourId, col::pink);
    setColour (PopupMenu::backgroundColourId, Colour (0xf4120a20));
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::pink.withAlpha (0.25f));
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (TextButton::textColourOffId, col::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (BubbleComponent::backgroundColourId, Colour (0xf0140b24));
    setColour (BubbleComponent::outlineColourId, col::pink.withAlpha (0.6f));
    setColour (TooltipWindow::textColourId, col::text);
    setColour (TooltipWindow::backgroundColourId, Colour (0xf0140b24));
}

void DiscoLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);
    const float size = (float) jmin (w, h) - 6.f;
    const auto c = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const float r = size * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    const bool bip = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float from = bip ? (a0 + a1) * 0.5f : a0;

    Path track; track.addCentredArc (c.x, c.y, r, r, 0.f, a0, a1, true);
    g.setColour (Colour (0xff261a3d));
    g.strokePath (track, PathStrokeType (3.4f, PathStrokeType::curved, PathStrokeType::rounded));
    if (std::abs (ang - from) > 0.001f)
    {
        Path arc; arc.addCentredArc (c.x, c.y, r, r, 0.f, jmin (from, ang), jmax (from, ang), true);
        const auto vc = accent.interpolatedWith (col::gold, bip ? std::abs (pos - 0.5f) * 0.8f : pos * 0.45f);
        g.setColour (vc.withAlpha (0.2f));
        g.strokePath (arc, PathStrokeType (9.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (vc);
        g.strokePath (arc, PathStrokeType (3.4f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    const float br = r - 6.5f;
    g.setGradientFill (ColourGradient (Colour (0xff35254f), c.x - br * 0.6f, c.y - br * 0.8f, Colour (0xff0d0817), c.x + br * 0.5f, c.y + br, true));
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.f);
    const Point<float> p0 (c.x + br * 0.25f * std::sin (ang), c.y - br * 0.25f * std::cos (ang));
    const Point<float> p1 (c.x + br * 0.92f * std::sin (ang), c.y - br * 0.92f * std::cos (ang));
    g.setColour (s.isMouseOverOrDragging() ? accent : col::text);
    g.drawLine ({ p0, p1 }, 2.3f);
}

void DiscoLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float, float, float, Slider::SliderStyle, Slider& s)
{
    auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.f, 5.f);
    g.setColour (Colour (0xff1d1230)); g.fillRoundedRectangle (r, 4.f);
    const double v = s.getValue(), mn = s.getMinimum(), mx = s.getMaximum();
    const float zero = r.getX() + r.getWidth() * (float) ((0.0 - mn) / (mx - mn));
    const float px = r.getX() + r.getWidth() * (float) ((v - mn) / (mx - mn));
    const auto c = v >= 0 ? col::cyan : col::pink;
    g.setColour (c.withAlpha (0.75f));
    g.fillRoundedRectangle (Rectangle<float> (jmin (zero, px), r.getY() + 2.f, std::abs (px - zero), r.getHeight() - 4.f), 3.f);
    g.setColour (Colours::white.withAlpha (0.25f)); g.drawVerticalLine ((int) zero, r.getY(), r.getBottom());
    g.setColour (col::text); g.setFont (font (11.f, true));
    g.drawText ((v > 0 ? "+" : "") + String ((int) std::round (v * 100.0)) + "%", r, Justification::centred);
}

void DiscoLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool hi, bool)
{
    const auto accent = b.findColour (ToggleButton::tickColourId);
    auto r = b.getLocalBounds().toFloat().withSizeKeepingCentre (36.f, 18.f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent.withAlpha (0.28f) : Colour (0xff1d1230)); g.fillRoundedRectangle (r, 9.f);
    g.setColour (on ? accent : (hi ? col::dim : col::border)); g.drawRoundedRectangle (r, 9.f, 1.2f);
    const float d = 12.f, cx = on ? r.getRight() - d - 3.f : r.getX() + 3.f;
    if (on) { g.setColour (accent.withAlpha (0.35f)); g.fillEllipse (cx - 3.f, r.getCentreY() - d * 0.5f - 3.f, d + 6.f, d + 6.f); }
    g.setColour (on ? accent : col::dim); g.fillEllipse (cx, r.getCentreY() - d * 0.5f, d, d);
}

void DiscoLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::bg1); g.fillRoundedRectangle (r, 5.f);
    g.setColour (box.isMouseOver (true) ? box.findColour (ComboBox::arrowColourId) : col::border);
    g.drawRoundedRectangle (r, 5.f, 1.f);
    Path p; const float ax = (float) w - 10.f, ay = (float) h * 0.5f;
    p.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (box.findColour (ComboBox::arrowColourId)); g.fillPath (p);
}

void DiscoLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    if (on) g.setGradientFill (ColourGradient (col::pink, r.getX(), r.getY(), col::violet, r.getRight(), r.getBottom(), false));
    else g.setColour (down ? col::pink.withAlpha (0.3f) : (hi ? col::pink.withAlpha (0.12f) : col::bg1));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (on ? col::pink.brighter (0.3f) : (hi ? col::pink : col::border));
    g.drawRoundedRectangle (r, 6.f, 1.f);
}

// =========================================================== ControlCell ===
ControlCell::ControlCell (AudioProcessorValueTreeState& s, const String& id, const String& label, Colour acc, Component* popupParent)
    : paramId (id), labelText (label), accent (acc)
{
    auto* p = s.getParameter (id);
    jassert (p != nullptr);
    if (auto* cp = dynamic_cast<AudioParameterChoice*> (p))
    {
        combo = std::make_unique<ComboBox>();
        combo->addItemList (cp->choices, 1);
        combo->setColour (ComboBox::arrowColourId, accent);
        combo->setJustificationType (Justification::centred);
        addAndMakeVisible (*combo);
        cAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *combo);
    }
    else if (dynamic_cast<AudioParameterBool*> (p) != nullptr)
    {
        toggle = std::make_unique<ToggleButton>();
        toggle->setColour (ToggleButton::tickColourId, accent);
        addAndMakeVisible (*toggle);
        bAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (s, id, *toggle);
    }
    else
    {
        slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
        slider->setColour (Slider::rotarySliderFillColourId, accent);
        slider->setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
        bool bip = false;
        for (const auto& d : sq::paramTable()) if (id == String (d.id)) bip = d.bipolar;
        slider->getProperties().set ("bipolar", bip);
        slider->setPopupDisplayEnabled (true, true, popupParent, 1200);
        slider->setMouseDragSensitivity (220);
        addAndMakeVisible (*slider);
        sAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, id, *slider);
        slider->setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        slider->setTooltip (p->getName (64) + "  (double-click = default)");
    }
}

void ControlCell::resized()
{
    auto r = getLocalBounds(); r.removeFromBottom (15);
    if (slider) { const int s = jmin (r.getWidth(), r.getHeight(), 62); slider->setBounds (r.withSizeKeepingCentre (s, s)); }
    if (combo)  combo->setBounds (r.withSizeKeepingCentre (jmin (r.getWidth() - 6, 120), 24));
    if (toggle) toggle->setBounds (r.withSizeKeepingCentre (46, 26));
}

void ControlCell::paint (Graphics& g)
{
    g.setFont (font (10.f, true));
    g.setColour (col::dim.brighter (0.35f));
    g.drawText (labelText, getLocalBounds().removeFromBottom (15), Justification::centred, true);
}

// ============================================================== StepGrid ===
StepGrid::StepGrid (SqueFMProcessor& p) : proc (p)
{
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const auto si = (size_t) i;
        pLo[si] = proc.apvts.getParameter (sq::stepId (i, "lo"));
        pHi[si] = proc.apvts.getParameter (sq::stepId (i, "hi"));
        pShape[si] = proc.apvts.getParameter (sq::stepId (i, "shape"));
        pMode[si] = proc.apvts.getParameter (sq::stepId (i, "mode"));
        pSync[si] = proc.apvts.getParameter (sq::stepId (i, "sync"));
        pHz[si] = proc.apvts.getParameter (sq::stepId (i, "hz"));
    }
    pSteps = proc.apvts.getParameter ("steps");
    pSpeed = proc.apvts.getParameter ("speed");
    pDiv = proc.apvts.getParameter ("stepDiv");
    setTooltip ("RANGE: drag the top line = how OPEN the filter gets, the bottom line = how CLOSED. Alt-drag moves both.\n"
                "SHAPE: click to change (right-click = menu).  BPM/Hz: click to switch.  RATE: drag up/down or scroll.\n"
                "Right-click any lane for 'apply to all steps'. Double-click resets.");
}

void StepGrid::resized()
{
    auto r = getLocalBounds().toFloat();
    gutter = r.removeFromLeft (52.f);
    grid = r;
    laneNums = r.removeFromBottom (15.f);
    laneRate = r.removeFromBottom (24.f);
    laneMode = r.removeFromBottom (22.f);
    laneShape = r.removeFromBottom (26.f);
    r.removeFromBottom (4.f);
    laneRange = r;
}

static float hashRnd (int n) { uint32_t x = (uint32_t) n * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return (float) (x & 0xffff) / 32767.5f - 1.f; }

void StepGrid::drawShapeIcon (Graphics& g, int shape, Rectangle<float> r)
{
    Path p; const int N = 24;
    for (int i = 0; i <= N; ++i)
    {
        const float t = (float) i / N;
        const float cyc = (shape == 5 || shape == 6 || shape == 8) ? 3.f : 1.f;
        const float pp = t * cyc; const int c = (int) pp;
        const float u = sq::Engine::stepShape (shape, pp - (float) c, hashRnd (c + 7), hashRnd (c + 6));
        const float x = r.getX() + t * r.getWidth(), y = r.getBottom() - u * r.getHeight();
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.strokePath (p, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
}

String StepGrid::rateText (int i) const
{
    if ((int) val (pMode[(size_t) i]) == 0) return sq::kStepSyncNames[jlimit (0, sq::kNumStepSync - 1, (int) val (pSync[(size_t) i]))];
    const float hz = val (pHz[(size_t) i]);
    return hz < 10.f ? String (hz, 1) : String ((int) std::round (hz));
}

void StepGrid::paint (Graphics& g)
{
    const int nSteps = (int) val (pSteps);
    const int cur = proc.engine.uiStep.load();
    const float frac = proc.engine.uiStepFrac.load();
    const float speed = val (pSpeed);
    const double stepBeats = sq::kDivBeats[jlimit (0, sq::kNumDivs - 1, (int) val (pDiv))];
    const double bpm = jmax (20.0, (double) proc.hostBpm.load());

    g.setColour (Colour (0xff0b0614)); g.fillRoundedRectangle (grid.expanded (2.f), 6.f);

    // gutter
    g.setFont (font (9.5f, true)); g.setColour (col::dim);
    g.drawText ("SHAPE", gutter.withY (laneShape.getY()).withHeight (laneShape.getHeight()), Justification::centredLeft);
    g.drawText ("BPM/HZ", gutter.withY (laneMode.getY()).withHeight (laneMode.getHeight()), Justification::centredLeft);
    g.drawText ("RATE", gutter.withY (laneRate.getY()).withHeight (laneRate.getHeight()), Justification::centredLeft);
    g.setColour (col::pink.withAlpha (0.8f));
    g.drawText ("OPEN", gutter.withY (laneRange.getY()).withHeight (16.f), Justification::centredLeft);
    g.setColour (col::cyan.withAlpha (0.8f));
    g.drawText ("CLOSED", gutter.withY (laneRange.getBottom() - 16.f).withHeight (16.f), Justification::centredLeft);
    for (float f : { 100.f, 1000.f, 10000.f })
    {
        const float y = yOf (sq::Engine::hzToNorm (f));
        g.setColour (Colours::white.withAlpha (0.07f)); g.drawHorizontalLine ((int) y, grid.getX(), grid.getRight());
        g.setColour (col::dim.withAlpha (0.8f)); g.setFont (font (9.f));
        g.drawText (f >= 1000.f ? String ((int) (f / 1000.f)) + "k" : String ((int) f), Rectangle<float> (gutter.getX(), y - 7.f, 46.f, 14.f), Justification::centredRight);
    }

    const float cw = colW();
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const auto si = (size_t) i;
        const float x = colX (i);
        const bool active = i < nSteps, isCur = active && i == cur;
        const float a = active ? 1.f : 0.25f;
        const float lo = val (pLo[si]), hi = val (pHi[si]);
        const int shape = (int) val (pShape[si]), mode = (int) val (pMode[si]), sync = (int) val (pSync[si]);
        const float hz = val (pHz[si]);

        auto colR = Rectangle<float> (x + 1.f, laneRange.getY(), cw - 2.f, laneRate.getBottom() - laneRange.getY());
        g.setColour (((i / 4) % 2 == 0 ? Colour (0xff160d27) : Colour (0xff100920)).withMultipliedAlpha (active ? 1.f : 0.5f));
        g.fillRect (colR);
        if (isCur) { g.setColour (col::pink.withAlpha (0.10f)); g.fillRect (colR); }
        if (i == hoverStep) { g.setColour (Colours::white.withAlpha (0.04f)); g.fillRect (colR); }

        // range band
        const float yLo = yOf (lo), yHi = yOf (hi);
        g.setGradientFill (ColourGradient (col::pink.withAlpha (0.28f * a), 0.f, jmin (yLo, yHi), col::cyan.withAlpha (0.22f * a), 0.f, jmax (yLo, yHi), false));
        g.fillRect (Rectangle<float> (x + 3.f, jmin (yLo, yHi), cw - 6.f, jmax (2.f, std::abs (yLo - yHi))));
        g.setColour (col::pink.withAlpha (a)); g.fillRect (Rectangle<float> (x + 2.f, yHi - 1.2f, cw - 4.f, 2.4f));
        g.setColour (col::cyan.withAlpha (a)); g.fillRect (Rectangle<float> (x + 2.f, yLo - 1.2f, cw - 4.f, 2.4f));

        // LFO preview inside the step
        double cycles = 0.0; bool hold = false;
        if (mode == 0) { if (sync == 0) hold = true; else cycles = stepBeats / sq::kRateBeats[jlimit (1, sq::kNumStepSync - 1, sync) - 1]; }
        else cycles = stepBeats * 60.0 / bpm * hz;
        cycles *= speed;
        Path curve; const int N = 40;
        for (int k = 0; k <= N; ++k)
        {
            const float t = (float) k / N;
            float u = 1.f;
            if (! hold)
            {
                const double pc = t * cycles; const int c = (int) pc;
                u = sq::Engine::stepShape (shape, (float) (pc - c), hashRnd (i * 97 + c), hashRnd (i * 97 + c - 1));
            }
            const float px = x + 2.f + t * (cw - 4.f), py = yOf (lo + (hi - lo) * u);
            if (k == 0) curve.startNewSubPath (px, py); else curve.lineTo (px, py);
        }
        const auto cc = isCur ? col::gold : Colours::white;
        g.setColour (cc.withAlpha (0.2f * a)); g.strokePath (curve, PathStrokeType (4.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (cc.withAlpha (0.9f * a)); g.strokePath (curve, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));

        // shape / mode / rate / number
        g.setColour ((isCur ? col::gold : col::violet).withAlpha (a));
        drawShapeIcon (g, shape, Rectangle<float> (x + 4.f, laneShape.getY() + 5.f, cw - 8.f, laneShape.getHeight() - 10.f));
        const auto mc = mode == 0 ? col::cyan : col::gold;
        auto mr = Rectangle<float> (x + 2.f, laneMode.getY() + 3.f, cw - 4.f, laneMode.getHeight() - 6.f);
        g.setColour (mc.withAlpha (0.18f * a)); g.fillRoundedRectangle (mr, 3.f);
        g.setColour (mc.withAlpha (a)); g.setFont (font (8.5f, true));
        g.drawText (mode == 0 ? "BPM" : "HZ", mr, Justification::centred, false);
        g.setFont (font (cw < 30.f ? 8.5f : 10.f, true));
        g.setColour ((mode == 0 && sync == 0 ? col::dim : mc).withAlpha (a));
        g.drawText (rateText (i), Rectangle<float> (x, laneRate.getY(), cw, laneRate.getHeight()), Justification::centred, false);
        g.setFont (font (9.f, isCur)); g.setColour (isCur ? col::gold : col::dim.withAlpha (a));
        g.drawText (String (i + 1), Rectangle<float> (x, laneNums.getY(), cw, laneNums.getHeight()), Justification::centred, false);

        if (isCur)
        {
            const float px = x + frac * cw;
            g.setColour (col::gold.withAlpha (0.3f)); g.fillRect (px - 2.f, laneRange.getY(), 4.f, laneRange.getHeight());
            g.setColour (col::gold); g.fillRect (px - 0.6f, laneRange.getY(), 1.2f, laneRange.getHeight());
        }
    }
    // live sequencer value marker
    const float sv = proc.engine.uiSeqValue.load();
    g.setColour (col::gold.withAlpha (0.8f));
    Path tri; const float ty = yOf (sv);
    tri.addTriangle (grid.getX() - 8.f, ty - 5.f, grid.getX() - 8.f, ty + 5.f, grid.getX() - 1.f, ty);
    g.fillPath (tri);

    for (int i = 4; i < sq::kMaxSteps; i += 4)
    {
        g.setColour (i % 16 == 0 ? col::pink.withAlpha (0.3f) : Colours::white.withAlpha (0.08f));
        g.drawVerticalLine ((int) colX (i), laneRange.getY(), laneRate.getBottom());
    }
    if (nSteps < sq::kMaxSteps) { g.setColour (col::gold.withAlpha (0.8f)); g.fillRect (colX (nSteps) - 1.f, laneRange.getY(), 2.f, laneRate.getBottom() - laneRange.getY()); }
}

int StepGrid::stepAt (float x) const { return jlimit (0, sq::kMaxSteps - 1, (int) ((x - grid.getX()) / colW())); }

StepGrid::Lane StepGrid::laneAt (Point<float> p) const
{
    if (p.x < grid.getX()) return None;
    if (laneRange.contains (p)) return Range;
    if (laneShape.contains (p)) return Shape;
    if (laneMode.contains (p))  return Mode;
    if (laneRate.contains (p))  return Rate;
    return None;
}

void StepGrid::setVal (RangedAudioParameter* p, float v)
{
    if (std::find (touched.begin(), touched.end(), p) == touched.end()) { p->beginChangeGesture(); touched.push_back (p); }
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}
void StepGrid::endAll() { for (auto* p : touched) p->endChangeGesture(); touched.clear(); }

void StepGrid::showMenu (Lane lane, int step)
{
    PopupMenu m;
    const auto si = (size_t) step;
    if (lane == Shape)
    {
        for (int i = 0; i < sq::kNumStepShapes; ++i) m.addItem (100 + i, sq::kStepShapeNames[i], true, (int) val (pShape[si]) == i);
        m.addSeparator(); m.addItem (1, "Apply this shape to ALL steps");
    }
    else if (lane == Rate || lane == Mode)
    {
        PopupMenu syncMenu;
        for (int i = 0; i < sq::kNumStepSync; ++i) syncMenu.addItem (200 + i, sq::kStepSyncNames[i], true, (int) val (pMode[si]) == 0 && (int) val (pSync[si]) == i);
        m.addSubMenu ("BPM sync", syncMenu);
        PopupMenu hzMenu;
        const float hzs[] = { 0.5f, 1.f, 2.f, 3.f, 4.f, 6.f, 8.f, 12.f, 16.f, 24.f, 32.f, 48.f };
        for (int i = 0; i < 12; ++i) hzMenu.addItem (300 + i, String (hzs[i], hzs[i] < 1.f ? 1 : 0) + " Hz");
        m.addSubMenu ("Free Hz", hzMenu);
        m.addSeparator(); m.addItem (2, "Apply this speed to ALL steps");
    }
    else if (lane == Range)
    {
        m.addItem (3, "Apply this range to ALL steps");
        m.addItem (4, "Copy steps 1-8 to the rest");
        m.addItem (5, "Flatten (no movement) this step");
    }
    SafePointer<StepGrid> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe, step] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& s = *safe; const auto si = (size_t) step;
        const float hzs[] = { 0.5f, 1.f, 2.f, 3.f, 4.f, 6.f, 8.f, 12.f, 16.f, 24.f, 32.f, 48.f };
        if (r >= 100 && r < 200) s.setVal (s.pShape[si], (float) (r - 100));
        else if (r >= 200 && r < 300) { s.setVal (s.pMode[si], 0.f); s.setVal (s.pSync[si], (float) (r - 200)); }
        else if (r >= 300) { s.setVal (s.pMode[si], 1.f); s.setVal (s.pHz[si], hzs[r - 300]); }
        else if (r == 1) { const float v = s.val (s.pShape[si]); for (auto* p : s.pShape) s.setVal (p, v); }
        else if (r == 2) { const float m = s.val (s.pMode[si]), y = s.val (s.pSync[si]), h = s.val (s.pHz[si]);
                           for (int i = 0; i < sq::kMaxSteps; ++i) { s.setVal (s.pMode[(size_t) i], m); s.setVal (s.pSync[(size_t) i], y); s.setVal (s.pHz[(size_t) i], h); } }
        else if (r == 3) { const float lo = s.val (s.pLo[si]), hi = s.val (s.pHi[si]); for (int i = 0; i < sq::kMaxSteps; ++i) { s.setVal (s.pLo[(size_t) i], lo); s.setVal (s.pHi[(size_t) i], hi); } }
        else if (r == 4) { for (int i = 8; i < sq::kMaxSteps; ++i) { const auto a = (size_t) i, b = (size_t) (i % 8);
                               s.setVal (s.pLo[a], s.val (s.pLo[b])); s.setVal (s.pHi[a], s.val (s.pHi[b])); s.setVal (s.pShape[a], s.val (s.pShape[b]));
                               s.setVal (s.pMode[a], s.val (s.pMode[b])); s.setVal (s.pSync[a], s.val (s.pSync[b])); s.setVal (s.pHz[a], s.val (s.pHz[b])); } }
        else if (r == 5) { s.setVal (s.pSync[si], 0.f); s.setVal (s.pMode[si], 0.f); s.setVal (s.pLo[si], s.val (s.pHi[si])); }
        s.endAll(); s.repaint();
    });
}

void StepGrid::mouseDown (const MouseEvent& e)
{
    const auto pt = e.position;
    const Lane lane = laneAt (pt);
    if (lane == None) return;
    const int step = stepAt (pt.x); const auto si = (size_t) step;
    dragLane = lane; dragStep = step; dragStartY = pt.y;
    if (e.mods.isPopupMenu()) { showMenu (lane, step); dragLane = None; return; }

    if (lane == Range)
    {
        const float lo = val (pLo[si]), hi = val (pHi[si]);
        dragStartLo = lo; dragStartHi = hi;
        if (e.mods.isAltDown()) dragHandle = HBoth;
        else dragHandle = std::abs (pt.y - yOf (hi)) <= std::abs (pt.y - yOf (lo)) ? HHi : HLo;
        if (dragHandle != HBoth) setVal (dragHandle == HHi ? pHi[si] : pLo[si], valOfY (pt.y));
    }
    else if (lane == Shape)
    {
        const int s = (int) val (pShape[si]);
        setVal (pShape[si], (float) ((s + (e.mods.isShiftDown() ? sq::kNumStepShapes - 1 : 1)) % sq::kNumStepShapes));
    }
    else if (lane == Mode) setVal (pMode[si], val (pMode[si]) > 0.5f ? 0.f : 1.f);
    else if (lane == Rate) dragStartVal = (int) val (pMode[si]) == 0 ? val (pSync[si]) : val (pHz[si]);
    repaint();
}

void StepGrid::mouseDrag (const MouseEvent& e)
{
    const auto pt = e.position;
    if (dragLane == Range)
    {
        if (dragHandle == HBoth)
        {
            const auto si = (size_t) dragStep;
            const float d = valOfY (pt.y) - valOfY (dragStartY);
            setVal (pLo[si], jlimit (0.f, 1.f, dragStartLo + d)); setVal (pHi[si], jlimit (0.f, 1.f, dragStartHi + d));
        }
        else
        {
            const int step = stepAt (pt.x);
            for (int s = jmin (step, dragStep); s <= jmax (step, dragStep); ++s)
                setVal (dragHandle == HHi ? pHi[(size_t) s] : pLo[(size_t) s], valOfY (pt.y));
            dragStep = step;
        }
        repaint();
    }
    else if (dragLane == Rate)
    {
        const auto si = (size_t) dragStep;
        const float dy = dragStartY - pt.y;
        if ((int) val (pMode[si]) == 0) setVal (pSync[si], (float) jlimit (0, sq::kNumStepSync - 1, (int) std::round (dragStartVal + dy / 10.f)));
        else setVal (pHz[si], jlimit (0.05f, 60.f, dragStartVal * std::exp2 (dy / 40.f)));
        repaint();
    }
}

void StepGrid::mouseUp (const MouseEvent&) { endAll(); dragLane = None; dragHandle = HNone; }

void StepGrid::mouseDoubleClick (const MouseEvent& e)
{
    const Lane lane = laneAt (e.position);
    const auto si = (size_t) stepAt (e.position.x);
    const sq::StepParams d;
    if (lane == Range) { setVal (pLo[si], d.lo); setVal (pHi[si], d.hi); }
    else if (lane == Rate) { setVal (pSync[si], (float) d.sync); setVal (pHz[si], d.hz); }
    endAll(); repaint();
}

void StepGrid::mouseMove (const MouseEvent& e)
{
    hoverLane = laneAt (e.position);
    hoverStep = hoverLane == None ? -1 : stepAt (e.position.x);
    setMouseCursor (hoverLane == Rate ? MouseCursor::UpDownResizeCursor
                    : (hoverLane == Shape || hoverLane == Mode) ? MouseCursor::PointingHandCursor
                    : hoverLane == Range ? MouseCursor::CrosshairCursor : MouseCursor::NormalCursor);
}
void StepGrid::mouseExit (const MouseEvent&) { hoverStep = -1; hoverLane = None; }

void StepGrid::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const Lane lane = laneAt (e.position);
    const auto si = (size_t) stepAt (e.position.x);
    const int dir = w.deltaY > 0 ? 1 : -1;
    if (lane == Rate)
    {
        if ((int) val (pMode[si]) == 0) setVal (pSync[si], (float) jlimit (0, sq::kNumStepSync - 1, (int) val (pSync[si]) + dir));
        else setVal (pHz[si], jlimit (0.05f, 60.f, val (pHz[si]) * (dir > 0 ? 1.12f : 1.f / 1.12f)));
    }
    else if (lane == Shape) setVal (pShape[si], (float) ((((int) val (pShape[si]) + dir) % sq::kNumStepShapes + sq::kNumStepShapes) % sq::kNumStepShapes));
    else if (lane == Range)
    {
        const float d = dir * 0.02f;
        setVal (pLo[si], jlimit (0.f, 1.f, val (pLo[si]) + d)); setVal (pHi[si], jlimit (0.f, 1.f, val (pHi[si]) + d));
    }
    endAll(); repaint();
}

// ========================================================= FilterDisplay ===
void FilterDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0614)); g.fillRoundedRectangle (r, 6.f);
    g.setColour (col::border); g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);
    g.setColour (Colours::white.withAlpha (0.05f));
    for (float f : { 100.f, 1000.f, 10000.f }) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * std::log10 (f / 20.f) / 3.f), r.getY(), r.getBottom());

    const float fc = proc.engine.uiCutoffHz.load();
    const float k = jlimit (0.f, 3.9f, proc.getParam ("res") * 4.1f);
    const float sl = proc.getParam ("slope");
    const int md = (int) proc.getParam ("fmode");
    Path p; const int N = 140; const float dbTop = 24.f, dbBot = -48.f;
    for (int i = 0; i <= N; ++i)
    {
        const float t = (float) i / N, f = 20.f * std::pow (1000.f, t);
        const std::complex<float> h1 = 1.f / std::complex<float> (1.f, f / fc);
        const auto h2 = h1 * h1, h3 = h2 * h1, h4 = h3 * h1;
        const auto u = 1.f / (1.f + k * h4);
        std::complex<float> y;
        if (md == 0) { const std::complex<float> taps[4] = { h1, h2, h3, h4 }; const int a = jmin (2, (int) sl); const float fr = sl - (float) a; y = u * (taps[a] + (taps[a + 1] - taps[a]) * fr); }
        else if (md == 1) { const auto b2 = 2.f * (h1 - h2), b4 = 4.f * (h2 - 2.f * h3 + h4); y = u * (b2 + (b4 - b2) * (sl / 3.f)); }
        else { const auto p2 = 1.f - 2.f * h1 + h2, p4 = 1.f - 4.f * h1 + 6.f * h2 - 4.f * h3 + h4; y = u * (p2 + (p4 - p2) * (sl / 3.f)); }
        const float db = jlimit (dbBot, dbTop, 20.f * std::log10 (std::abs (y) * (1.f + k * 0.28f) + 1e-6f));
        const float x = r.getX() + t * r.getWidth(), yy = r.getY() + 4.f + (dbTop - db) / (dbTop - dbBot) * (r.getHeight() - 8.f);
        if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
    }
    Path fill (p); fill.lineTo (r.getRight(), r.getBottom()); fill.lineTo (r.getX(), r.getBottom()); fill.closeSubPath();
    g.setGradientFill (ColourGradient (col::pink.withAlpha (0.4f), r.getX(), r.getY(), col::cyan.withAlpha (0.03f), r.getX(), r.getBottom(), false));
    g.fillPath (fill);
    g.setColour (col::pink.withAlpha (0.3f)); g.strokePath (p, PathStrokeType (5.f));
    g.setColour (col::pink.brighter (0.4f)); g.strokePath (p, PathStrokeType (1.6f));
    g.setFont (font (10.f, true)); g.setColour (col::text.withAlpha (0.85f));
    g.drawText (fc >= 1000.f ? String (fc / 1000.f, 2) + " kHz" : String ((int) fc) + " Hz", r.reduced (8.f, 4.f), Justification::topRight);
}

void ModScope::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0614)); g.fillRoundedRectangle (r, 6.f);
    const int n = sq::Engine::kScopeSize, w = proc.engine.scopeWrite.load();
    Path p;
    for (int i = 0; i < n; ++i)
    {
        const float v = jlimit (0.f, 1.f, proc.engine.scope[(w + i) % n]);
        const float x = r.getX() + r.getWidth() * (float) i / (n - 1), y = r.getBottom() - 6.f - v * (r.getHeight() - 22.f);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::pink.withAlpha (0.3f)); g.strokePath (p, PathStrokeType (4.f));
    g.setColour (col::pink); g.strokePath (p, PathStrokeType (1.4f));
    g.setFont (font (9.5f, true)); g.setColour (col::dim);
    g.drawText ("SEQUENCER > CUTOFF", r.reduced (8.f, 3.f), Justification::topLeft);
    const float fc = proc.engine.uiCutoffHz.load();
    g.setColour (col::gold);
    g.drawText (fc >= 1000.f ? String (fc / 1000.f, 2) + " kHz" : String ((int) fc) + " Hz", r.reduced (8.f, 3.f), Justification::topRight);
}

void LfoView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0614)); g.fillRoundedRectangle (r, 5.f);
    const int shape = (int) proc.getParam (sq::lfoId (index, "shape"));
    Path p; const int N = 80;
    for (int i = 0; i <= N; ++i)
    {
        const float t = (float) i / N * 2.f; const int c = (int) t; const float ph = t - (float) c;
        float v = 0.f;
        switch (shape)
        {
            case 0: v = std::sin (sq::kTwoPi * ph); break;
            case 1: v = ph < 0.5f ? 4.f * ph - 1.f : 3.f - 4.f * ph; break;
            case 2: v = 2.f * ph - 1.f; break;
            case 3: v = 1.f - 2.f * ph; break;
            case 4: v = ph < 0.5f ? 1.f : -1.f; break;
            case 5: v = hashRnd (c * 3 + 1 + (int) (ph * 4.f)); break;
            case 6: { const float a = hashRnd ((int) (t * 4.f)), b = hashRnd ((int) (t * 4.f) + 1); const float f = t * 4.f - std::floor (t * 4.f); v = a + (b - a) * (0.5f - 0.5f * std::cos (sq::kPi * f)); } break;
            default: v = hashRnd ((int) (t * 8.f) + 11); break;
        }
        const float x = r.getX() + 4.f + (float) i / N * (r.getWidth() - 8.f), y = r.getCentreY() - v * (r.getHeight() * 0.38f);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::violet.withAlpha (0.35f)); g.strokePath (p, PathStrokeType (4.f));
    g.setColour (col::violet.brighter (0.3f)); g.strokePath (p, PathStrokeType (1.5f));
    const float live = proc.engine.uiSrc[(size_t) (sq::SrcLfo1 + index)].load();
    const float ly = r.getCentreY() - live * (r.getHeight() * 0.38f);
    g.setColour (col::gold); g.fillEllipse (r.getRight() - 12.f, ly - 4.f, 8.f, 8.f);
}

// ============================================================= MatrixRow ===
MatrixRow::MatrixRow (SqueFMProcessor& p, int s, Component* popupParent) : proc (p), slot (s)
{
    StringArray srcNames, dstNames;
    for (int i = 0; i < sq::NumSrc; ++i) srcNames.add (String::fromUTF8 (sq::kSrcNames[i]));
    for (int i = 0; i < sq::NumDst; ++i) dstNames.add (String::fromUTF8 (sq::kDstNames[i]));
    src.addItemList (srcNames, 1); via.addItemList (srcNames, 1); dst.addItemList (dstNames, 1);
    src.setColour (ComboBox::arrowColourId, col::cyan); via.setColour (ComboBox::arrowColourId, col::violet); dst.setColour (ComboBox::arrowColourId, col::pink);
    for (auto* c : { &src, &via, &dst }) { c->setJustificationType (Justification::centredLeft); addAndMakeVisible (*c); }
    via.setTooltip ("Optional: a second source that scales this modulation (e.g. Mod Wheel)");
    amount.setRange (-1.0, 1.0); amount.setDoubleClickReturnValue (true, 0.0);
    amount.setTooltip ("Amount. Negative = inverted modulation. Double-click = 0");
    addAndMakeVisible (amount);
    aSrc = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, sq::slotId (slot, "src"), src);
    aVia = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, sq::slotId (slot, "via"), via);
    aDst = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, sq::slotId (slot, "dst"), dst);
    aAmt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, sq::slotId (slot, "amt"), amount);
    ignoreUnused (popupParent);
}

void MatrixRow::resized()
{
    auto r = getLocalBounds().reduced (0, 4);
    r.removeFromLeft (34);
    src.setBounds (r.removeFromLeft (190)); r.removeFromLeft (10);
    via.setBounds (r.removeFromLeft (150)); r.removeFromLeft (14);
    amount.setBounds (r.removeFromLeft (420)); r.removeFromLeft (34);
    dst.setBounds (r.removeFromLeft (220));
}

void MatrixRow::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (font (12.f, true)); g.setColour (col::dim);
    g.drawText (String (slot + 1), r.removeFromLeft (30.f), Justification::centred);
    const float arrowX = 34.f + 190.f + 10.f + 150.f + 14.f + 420.f + 8.f;
    g.setColour (col::pink.withAlpha (0.7f));
    Path a; a.addArrow (Line<float> (arrowX, r.getCentreY(), arrowX + 18.f, r.getCentreY()), 2.f, 9.f, 7.f); g.fillPath (a);
    // live meter
    const float live = jlimit (-1.f, 1.f, proc.engine.uiSlot[(size_t) slot].load());
    auto m = Rectangle<float> ((float) getWidth() - 130.f, r.getCentreY() - 5.f, 120.f, 10.f);
    g.setColour (Colour (0xff1d1230)); g.fillRoundedRectangle (m, 4.f);
    const float cx = m.getCentreX(), px = cx + live * m.getWidth() * 0.5f;
    g.setColour ((live >= 0 ? col::cyan : col::pink).withAlpha (0.9f));
    g.fillRoundedRectangle (Rectangle<float> (jmin (cx, px), m.getY() + 1.f, std::abs (px - cx), m.getHeight() - 2.f), 3.f);
}
} // namespace ui

// ================================================================= EDITOR ==

// =============================================================== v4 views ===
namespace ui
{
void FilterLfoView::push()
{
    hist[(size_t) w] = proc.engine.uiFilterLfo.load (std::memory_order_relaxed);
    w = (w + 1) % (int) hist.size();
}

void FilterLfoView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0716)); g.fillRoundedRectangle (r, 8.f);
    g.setColour (col::border); g.drawRoundedRectangle (r.reduced (0.5f), 8.f, 1.f);
    const bool on = proc.getParam ("flOn") > 0.5f;
    const bool bip = proc.getParam ("flBip") > 0.5f;
    auto plot = r.reduced (10.f, 8.f).withTrimmedRight (150.f);
    g.setColour (col::dim.withAlpha (0.25f)); g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());
    Path p; const int n = (int) hist.size();
    for (int i = 0; i < n; ++i)
    {
        float v = hist[(size_t) ((w + i) % n)];
        if (! bip) v = v * 2.f - 1.f;
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) (n - 1);
        const float y = plot.getCentreY() - v * plot.getHeight() * 0.45f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour ((on ? col::pink : col::dim).withAlpha (on ? 1.f : 0.4f)); g.strokePath (p, PathStrokeType (2.f));
    auto info = r.reduced (10.f).removeFromRight (140.f);
    g.setFont (font (11.f, true)); g.setColour (col::dim);
    g.drawText ("LIVE CUTOFF", info.removeFromTop (16.f), Justification::centredLeft);
    const float hz = proc.engine.uiCutoffHz.load (std::memory_order_relaxed);
    g.setFont (font (22.f, true)); g.setColour (on ? col::pink : col::text);
    g.drawText (hz >= 1000.f ? String (hz / 1000.f, 2) + " kHz" : String ((int) hz) + " Hz", info.removeFromTop (30.f), Justification::centredLeft);
    g.setFont (font (10.5f)); g.setColour (col::dim);
    g.drawFittedText (on ? "LFO drives cutoff directly (octaves). Envelope and LFO stay independent." : "Filter LFO is OFF - switch it on for WOW movement.",
                      info.toNearestInt(), Justification::topLeft, 3);
}

void FmAlgoView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0716)); g.fillRoundedRectangle (r, 8.f);
    const int a = jlimit (0, sq::kNumFmAlgos - 1, (int) proc.getParam ("fvAlgo"));
    g.setFont (font (13.f, true)); g.setColour (col::gold);
    g.drawText (sq::kFmAlgoNames[a], r.removeFromTop (24.f).reduced (8.f, 0.f), Justification::centredLeft);
    // place operators: modulators above their targets
    Point<float> pos[4];
    const float cx = r.getCentreX(), top = r.getY() + 20.f, bottom = r.getBottom() - 46.f;
    int depth[4] = { 0, 0, 0, 0 };
    for (int pass = 0; pass < 4; ++pass)
        for (int d = 0; d < 4; ++d) for (int s2 = 0; s2 < 4; ++s2)
            if (sq::Engine::kAlgoMods[a][d][s2] && s2 > d) depth[s2] = jmax (depth[s2], depth[d] + 1);
    int maxD = 0; for (int i : depth) maxD = jmax (maxD, i);
    int countAt[4] = { 0, 0, 0, 0 }, idxAt[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; ++i) ++countAt[depth[i]];
    for (int i = 0; i < 4; ++i)
    {
        const int d = depth[i]; const int k = idxAt[d]++;
        const float x = cx + ((float) k - (countAt[d] - 1) * 0.5f) * 86.f;
        const float y = maxD == 0 ? (top + bottom) * 0.5f : bottom - (bottom - top) * (float) d / (float) maxD;
        pos[i] = { x, y };
    }
    g.setColour (col::gold.withAlpha (0.6f));
    for (int d = 0; d < 4; ++d) for (int s2 = 0; s2 < 4; ++s2)
        if (sq::Engine::kAlgoMods[a][d][s2])
        {
            Path arrow; arrow.addArrow ({ pos[s2], pos[d].translated (0.f, s2 < d ? 0.f : -20.f) }, 2.f, 9.f, 8.f);
            g.fillPath (arrow);
        }
    for (int i = 0; i < 4; ++i)
    {
        const bool car = sq::Engine::kAlgoCarrier[a][i];
        const float env = proc.engine.uiOpEnv[(size_t) i].load (std::memory_order_relaxed);
        auto box = Rectangle<float> (60.f, 40.f).withCentre (pos[i]);
        g.setColour ((car ? col::cyan : col::gold).withAlpha (0.15f + 0.6f * jlimit (0.f, 1.f, env)));
        g.fillRoundedRectangle (box, 6.f);
        g.setColour (car ? col::cyan : col::gold); g.drawRoundedRectangle (box, 6.f, 1.5f);
        g.setFont (font (12.f, true)); g.setColour (col::text);
        g.drawText ("OP " + String (i + 1), box.withTrimmedBottom (14.f), Justification::centred);
        g.setFont (font (9.5f)); g.setColour (col::dim);
        g.drawText (car ? "carrier" : "mod", box.withTrimmedTop (22.f), Justification::centred);
    }
    g.setFont (font (10.5f)); g.setColour (col::dim);
    g.drawFittedText ("cyan = heard (carrier)   gold = modulator   brightness = live envelope   OP4 has feedback",
                      getLocalBounds().removeFromBottom (34).reduced (8, 2), Justification::centred, 2);
}

static const char* kLaneNames[] = { "FM TIMBRE", "CHARACTER", "RESONANCE", "DRIVE", "PROBABILITY", "ACCENT" };
static const char* kLaneIds[]   = { "fm", "chr", "rs", "dv", "pb", "ac" };
static const char* kLaneDepth[] = { "lnFm", "lnChar", "lnRes", "lnDrive", "", "accAmt" };

LaneEditor::LaneEditor (SqueFMProcessor& p) : proc (p) {}

RangedAudioParameter* LaneEditor::stepParam (int l, int step) const
{
    return dynamic_cast<RangedAudioParameter*> (proc.apvts.getParameter (sq::stepId (step, kLaneIds[l])));
}
RangedAudioParameter* LaneEditor::depthParam (int l) const
{
    return String (kLaneDepth[l]).isEmpty() ? nullptr : dynamic_cast<RangedAudioParameter*> (proc.apvts.getParameter (kLaneDepth[l]));
}

void LaneEditor::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0716)); g.fillRoundedRectangle (r, 8.f);
    tabsR = r.removeFromTop (24.f);
    const float tw = (tabsR.getWidth() - 220.f) / 6.f;
    for (int l = 0; l < 6; ++l)
    {
        auto t = Rectangle<float> (tabsR.getX() + tw * l, tabsR.getY(), tw, tabsR.getHeight()).reduced (2.f);
        g.setColour (l == lane ? col::lime.withAlpha (0.25f) : col::border.withAlpha (0.5f)); g.fillRoundedRectangle (t, 5.f);
        g.setColour (l == lane ? col::lime : col::dim); g.setFont (font (10.5f, true)); g.drawText (kLaneNames[l], t, Justification::centred);
    }
    depthR = tabsR.withTrimmedLeft (tw * 6.f + 6.f);
    if (auto* dp = depthParam (lane))
    {
        const float v = dp->convertFrom0to1 (dp->getValue());
        g.setFont (font (10.5f, true)); g.setColour (col::text);
        g.drawText ("LANE DEPTH  " + String (v, 2) + "  (drag)", depthR, Justification::centred);
    }
    else { g.setFont (font (10.5f)); g.setColour (col::dim); g.drawText ("chance that a step plays", depthR, Justification::centred); }
    bars = r.reduced (4.f);
    const int nSteps = (int) proc.getParam ("steps");
    const int cur = proc.engine.uiStep.load (std::memory_order_relaxed);
    const float cw = bars.getWidth() / sq::kMaxSteps;
    const bool centred = lane < 4;
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        auto* p = stepParam (lane, i); if (! p) continue;
        const float v = p->convertFrom0to1 (p->getValue());
        auto col = Rectangle<float> (bars.getX() + cw * i + 1.f, bars.getY(), cw - 2.f, bars.getHeight());
        g.setColour (i < nSteps ? Colour (0xff181030) : Colour (0xff100a1c)); g.fillRect (col);
        const float base = centred ? col.getCentreY() : col.getBottom();
        const float y = col.getBottom() - v * col.getHeight();
        g.setColour ((i == cur ? col::gold : col::lime).withAlpha (i < nSteps ? 0.85f : 0.25f));
        g.fillRect (Rectangle<float>::leftTopRightBottom (col.getX(), jmin (y, base), col.getRight(), jmax (y, base)));
    }
    if (centred) { g.setColour (col::dim.withAlpha (0.4f)); g.drawHorizontalLine ((int) bars.getCentreY(), bars.getX(), bars.getRight()); }
}

void LaneEditor::setAt (Point<float> pt, bool reset)
{
    const int i = jlimit (0, sq::kMaxSteps - 1, (int) ((pt.x - bars.getX()) / (bars.getWidth() / sq::kMaxSteps)));
    auto* p = stepParam (lane, i); if (! p) return;
    float v = jlimit (0.f, 1.f, (bars.getBottom() - pt.y) / bars.getHeight());
    if (reset) v = lane < 4 ? 0.5f : (lane == 4 ? 1.f : 0.f);
    if (std::find (touched.begin(), touched.end(), p) == touched.end()) { p->beginChangeGesture(); touched.push_back (p); }
    p->setValueNotifyingHost (p->convertTo0to1 (v));
    repaint();
}

void LaneEditor::mouseDown (const MouseEvent& e)
{
    const auto pt = e.position;
    if (tabsR.contains (pt) && ! depthR.contains (pt))
    {
        const float tw = (tabsR.getWidth() - 220.f) / 6.f;
        lane = jlimit (0, 5, (int) ((pt.x - tabsR.getX()) / tw)); repaint(); return;
    }
    if (depthR.contains (pt)) return;
    setAt (pt, e.mods.isAltDown());
}
void LaneEditor::mouseDrag (const MouseEvent& e)
{
    if (depthR.contains (e.mouseDownPosition))
    {
        if (auto* dp = depthParam (lane))
        {
            if (touched.empty()) { dp->beginChangeGesture(); touched.push_back (dp); }
            dp->setValueNotifyingHost (jlimit (0.f, 1.f, dp->getValue() - (float) e.getDistanceFromDragStartY() * 0.0008f));
            repaint();
        }
        return;
    }
    if (bars.contains (e.mouseDownPosition)) setAt (e.position, e.mods.isAltDown());
}
void LaneEditor::mouseUp (const MouseEvent&) { for (auto* p : touched) p->endChangeGesture(); touched.clear(); }
void LaneEditor::mouseDoubleClick (const MouseEvent& e) { if (bars.contains (e.position)) { setAt (e.position, true); mouseUp (e); } }
} // namespace ui

SqueFMEditor::SqueFMEditor (SqueFMProcessor& p)
    : AudioProcessorEditor (p), proc (p), grid (p), filterDisplay (p), scope (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setBounds (0, 0, W, H);
    using namespace ui;

    // ---------------- SYNTH page ----------------
    const int yA = 72, hA = 290, yB = 370, hB = 330;
    addPanel ("OSC A", { 10, yA, 170, hA }, 2, col::cyan, Synth, { { "waveA", "WAVE" }, { "octA", "OCTAVE" }, { "semiA", "SEMI" }, { "fineA", "FINE" } });
    addPanel ("OSC B", { 188, yA, 232, hA }, 3, col::cyan, Synth, { { "waveB", "WAVE" }, { "octB", "OCTAVE" }, { "semiB", "SEMI" }, { "fineB", "FINE" }, { "syncB", "SYNC A>B" }, { "oscReset", "PHASE RESET" } });
    addPanel ("FM OPERATOR", { 428, yA, 330, hA }, 4, col::gold, Synth, { { "fmMode", "MODE" }, { "fmRatio", "RATIO" }, { "fmFine", "FINE" }, { "fmHz", "FIXED HZ" },
                                                                         { "fmIndex", "INDEX" }, { "fmFb", "FEEDBACK" }, { "fmTarget", "TARGET" }, { "lvFm", "OP LEVEL" } });
    addPanel ("MIXER", { 766, yA, 252, hA }, 3, col::violet, Synth, { { "lvA", "OSC A" }, { "lvB", "OSC B" }, { "lvSub", "SUB" }, { "subOct", "SUB OCT" }, { "lvNoise", "NOISE" }, { "fdbk", "FEEDBACK" } });
    addPanel ("ZAP / VELOCITY", { 1026, yA, 244, hA }, 2, col::gold, Synth, { { "penv", "ZAP PITCH" }, { "pdec", "ZAP DECAY" }, { "velf", "VEL>FILTER" }, { "vela", "VEL>AMP" } });
    { auto& f = addPanel ("LADDER FILTER", { 10, yB, 420, hB }, 4, col::pink, Synth, { { "cutoff", "CUTOFF" }, { "res", "RESONANCE" }, { "drive", "DRIVE" }, { "slope", "SLOPE 6-24" },
                                                                                       { "fmode", "MODE" }, { "ktrk", "KEY TRACK" }, { "envamt", "ENV AMT" }, { "resenv", "RES ENV" } });
      f.topExtra = 110; f.extra = &filterDisplay; }
    addPanel ("COMB", { 438, yB, 250, hB }, 3, col::cyan, Synth, { { "combMix", "MIX" }, { "combFb", "FEEDBACK" }, { "combDamp", "DAMP" }, { "combSemi", "TUNE" }, { "combHz", "FREE HZ" }, { "combKey", "KEY TRACK" } });
    addPanel ("ENVELOPES   /   FILTER  /  AMP  /  MOD", { 696, yB, 574, hB }, 5, col::violet, Synth,
              { { "fa", "F.ATTACK" }, { "fd", "F.DECAY" }, { "fs", "F.SUSTAIN" }, { "fr", "F.RELEASE" }, { "floop", "F.LOOP" },
                { "aa", "A.ATTACK" }, { "ad", "A.DECAY" }, { "as", "A.SUSTAIN" }, { "ar", "A.RELEASE" }, { "", "" },
                { "ma", "M.ATTACK" }, { "md", "M.DECAY" }, { "ms", "M.SUSTAIN" }, { "mr", "M.RELEASE" }, { "mloop", "M.LOOP" } });

    // ---------------- SEQUENCER page ----------------
    addPanel ("SEQUENCER", { 10, yA, 250, 628 }, 2, col::pink, Seq, { { "seqOn", "SEQ ON" }, { "seqRetrig", "RETRIG" }, { "seqReset", "PHASE RESET" }, { "steps", "STEPS" },
                                                                       { "stepDiv", "STEP LENGTH" }, { "seqDepth", "DEPTH" }, { "seqSmooth", "SMOOTH" }, { "speed", "SPEED" },
                                                                       { "wander", "WANDER" }, { "swing", "SWING" }, { "chaos", "CHAOS" } });
    { auto& gp = addPanel ("32 STEPS  /  EVERY STEP = ITS OWN LFO  /  OPEN / CLOSED RANGE  /  BPM OR HZ", { 268, yA, 1002, 628 }, 1, col::pink, Seq, {});
      gp.extra = &grid; }

    // ---------------- MOD page ----------------
    for (int i = 0; i < sq::kNumLfos; ++i)
    {
        lfoViews[(size_t) i] = std::make_unique<LfoView> (proc, i);
        const String n = String (i + 1);
        auto& lp = addPanel ("LFO " + n, { 10 + i * 308, yA, 300, 270 }, 3, col::violet, Mod,
                             { { strdup (sq::lfoId (i, "mode").c_str()), "BPM/HZ" }, { strdup (sq::lfoId (i, "sync").c_str()), "SYNC RATE" },
                               { strdup (sq::lfoId (i, "hz").c_str()), "HZ RATE" }, { strdup (sq::lfoId (i, "shape").c_str()), "SHAPE" },
                               { strdup (sq::lfoId (i, "retrig").c_str()), "RETRIG" } });
        lp.topExtra = 74; lp.extra = lfoViews[(size_t) i].get();
    }
    addPanel ("SPIT  +  CHAOS", { 934, yA, 336, 270 }, 3, col::gold, Mod, { { "spitRate", "SPIT RATE" }, { "spitDecay", "SPIT DECAY" }, { "spitRnd", "SPIT RANDOM" },
                                                                             { "chaosRate", "CHAOS RATE" }, { "chaosAmt", "CHAOS AMT" } });
    { auto& mp = addPanel ("MOD MATRIX  /  ANY SOURCE > ANY DESTINATION  /  NEGATIVE AMOUNT = INVERT", { 10, 350, 1260, 350 }, 1, col::cyan, Mod, {});
      matrixArea = mp.r.reduced (12).withTrimmedTop (48); }
    for (int i = 0; i < sq::kMatrixSlots; ++i)
    {
        matrixRows[(size_t) i] = std::make_unique<MatrixRow> (proc, i, &content);
        content.addAndMakeVisible (*matrixRows[(size_t) i]);
    }

    // ---------------- FX page ----------------
    addPanel ("TALK", { 10, yA, 200, hA }, 1, col::cyan, Fx, { { "talkMix", "MIX" }, { "vowel", "A-E-I-O-U" } });
    addPanel ("BITE!  +  CRUSH", { 218, yA, 340, hA }, 3, col::pink, Fx, { { "biteType", "TYPE" }, { "bite", "AMOUNT" }, { "biteMix", "MIX" }, { "bitePost", "POST FILTER" }, { "crush", "CRUSH" } });
    addPanel ("PHASER", { 566, yA, 300, hA }, 2, col::violet, Fx, { { "phMix", "MIX" }, { "phRate", "RATE" }, { "phDepth", "DEPTH" }, { "phFb", "FEEDBACK" } });
    addPanel ("REVERB", { 874, yA, 396, hA }, 3, col::violet, Fx, { { "rvMix", "MIX" }, { "rvSize", "SIZE" }, { "rvDamp", "DAMP" } });
    addPanel ("QUICK GUIDE", { 538, yB, 732, hB }, 1, col::gold, Fx, {});
    addPanel ("DELAY", { 10, yB, 520, hB }, 3, col::cyan, Fx, { { "dlMix", "MIX" }, { "dlDiv", "TIME" }, { "dlFb", "FEEDBACK" }, { "dlTone", "TONE" }, { "dlMod", "WOBBLE" }, { "dlPP", "PING PONG" } });

    // ---------------- MAIN page (v4) ----------------
    const int yM = 72, hM = 300, yN = 380, hN = 320;
    addPanel ("MAIN SOUND  /  BODY", { 10, yM, 300, hM }, 4, col::cyan, Main, { { "lvA", "OSC A" }, { "waveA", "A WAVE" }, { "lvB", "OSC B" }, { "fineB", "B DETUNE" },
                                                                             { "lvSub", "SUB" }, { "subShape", "SUB SHAPE" }, { "subPost", "SUB CLEAN" }, { "subOct", "SUB OCT" } });
    { auto& fp = addPanel ("FM CHARACTER", { 318, yM, 330, hM }, 3, col::gold, Main, { { "fvLevel", "FM VOICE" }, { "fvBright", "BRIGHTNESS" }, { "fvChar", "CHARACTER" },
                                                                                    { "fvBody", "BODY" }, { "fvDecay", "FM DECAY" }, { "fvFb", "FEEDBACK" } });
      fp.topExtra = 40; fp.extra = &familyBox; }
    addPanel ("WOW  /  FILTER", { 656, yM, 300, hM }, 3, col::pink, Main, { { "cutoff", "CUTOFF" }, { "res", "RESONANCE" }, { "fBass", "BASS KEEP" },
                                                                        { "envamt", "ENV AMT" }, { "fd", "ENV DECAY" }, { "drive", "PRE DRIVE" } });
    addPanel ("BITE  /  GROWL", { 964, yM, 306, hM }, 2, col::pink, Main, { { "satDrive", "GROWL" }, { "satMix", "PARALLEL" }, { "satAsym", "ASYM" }, { "satFold", "FOLD" } });
    { auto& lp = addPanel ("FILTER LFO  >  CUTOFF   (dedicated, independent of envelope and sequencer)", { 10, yN, 800, hN }, 7, col::pink, Main,
                           { { "flOn", "ON" }, { "flMode", "BPM / HZ" }, { "flSync", "BPM RATE" }, { "flHz", "HZ RATE" }, { "flShape", "SHAPE" }, { "flAmt", "AMOUNT" }, { "flBip", "BIPOLAR" },
                             { "flPhase", "PHASE" }, { "flOffset", "OFFSET" }, { "flSmooth", "SMOOTH" }, { "flRise", "RISE" }, { "flRetrig", "RETRIG" }, { "flFm", "LFO > FM" }, { "slope", "SLOPE" } });
      lp.topExtra = 110; lp.extra = &flView; }
    addPanel ("MOVEMENT", { 818, yN, 220, hN }, 2, col::lime, Main, { { "seqOn", "SEQ ON" }, { "seqDepth", "SEQ > CUTOFF" }, { "lnFm", "SEQ > FM" }, { "accAmt", "ACCENT" } });
    addPanel ("SPACE", { 1046, yN, 224, hN }, 2, col::violet, Main, { { "dlMix", "DELAY" }, { "dlDiv", "TIME" }, { "rvMix", "REVERB" }, { "fvWidth", "FM WIDTH" } });

    // ---------------- FM page (v4) ----------------
    { auto& vp = addPanel ("FM VOICE  /  4 OPERATORS", { 10, yM, 470, hM }, 4, col::gold, Fm,
                           { { "fvLevel", "LEVEL" }, { "fvAlgo", "ALGORITHM" }, { "fvRoute", "ROUTE" }, { "fvBright", "BRIGHTNESS" },
                             { "fvChar", "CHARACTER" }, { "fvBody", "BODY" }, { "fvDecay", "DECAY" }, { "fvFb", "FEEDBACK" },
                             { "fvWidth", "WIDTH" }, { "fvVel", "VEL > TIMBRE" }, { "fvKey", "KEY SCALE" }, { "fvLowCut", "LOW CUT" } });
      vp.topExtra = 40; vp.extra = &familyBox2; }
    { auto& ap = addPanel ("ALGORITHM", { 488, yM, 400, hM }, 1, col::gold, Fm, {}); ap.extra = &algoView; }
    addPanel ("FM MOVEMENT", { 896, yM, 374, hM }, 2, col::lime, Fm, { { "lnFm", "SEQ > FM" }, { "lnChar", "SEQ > CHARACTER" }, { "flFm", "FILTER LFO > FM" }, { "accAmt", "ACCENT" } });
    for (int i = 0; i < 4; ++i)
    {
        const std::string o = "op" + std::to_string (i + 1);
        auto id = [&o] (const char* w2) { return strdup ((o + w2).c_str()); };
        addPanel ("OP " + String (i + 1) + (i == 0 ? "   (always a carrier)" : (i == 3 ? "   (feedback)" : "")), { 10 + i * 316, yN, 308, hN }, 4, i == 0 ? col::cyan : col::gold, Fm,
                  { { id ("ratio"), "RATIO" }, { id ("fine"), "FINE" }, { id ("level"), "LEVEL" }, { id ("s"), "SUSTAIN" },
                    { id ("a"), "ATTACK" }, { id ("d"), "DECAY" }, { id ("r"), "RELEASE" } });
    }

    // ---------------- always visible footer ----------------
    addPanel ("MACROS", { 10, 708, 560, 142 }, 4, col::gold, Always, { { "m1", "MACRO 1" }, { "m2", "MACRO 2" }, { "m3", "MACRO 3" }, { "m4", "MACRO 4" } });
    addPanel ("MASTER", { 578, 708, 340, 142 }, 4, col::pink, Always, { { "out", "OUTPUT" }, { "glide", "GLIDE" }, { "legato", "LEGATO" }, { "bend", "BEND" } });
    { auto& sp = addPanel ("LIVE", { 926, 708, 344, 142 }, 1, col::pink, Always, {}); sp.extra = &scope; }

    content.addAndMakeVisible (grid); content.addAndMakeVisible (filterDisplay); content.addAndMakeVisible (scope);
    for (auto* c : std::initializer_list<Component*> { &flView, &algoView, &laneEditor, &familyBox, &familyBox2 }) content.addAndMakeVisible (c);
    for (auto* fb : { &familyBox, &familyBox2 })
    {
        fb->setTextWhenNothingSelected ("FM SOUND: choose a family...");
        for (int f = 0; f < sq::kNumFmFamilies; ++f) fb->addItem (sq::kFmFamilyNames[f], f + 1);
        fb->setColour (ComboBox::arrowColourId, col::gold);
        fb->onChange = [this, fb] { const int f = fb->getSelectedId() - 1; if (f >= 0) proc.applyFmFamily (f); };
    }
    for (auto& v : lfoViews) content.addAndMakeVisible (*v);

    // header
    for (int i = 0; i < (int) proc.presets.size(); ++i) presetBox.addItem (proc.presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.setColour (ComboBox::arrowColourId, col::gold);
    presetBox.onChange = [this] { const int idx = presetBox.getSelectedId() - 1; if (idx >= 0) { proc.setCurrentProgram (idx); lastProgram = idx; proc.updateHostDisplay(); } };
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); presetBox.setSelectedId ((proc.getCurrentProgram() - 1 + n) % n + 1); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); presetBox.setSelectedId ((proc.getCurrentProgram() + 1) % n + 1); };
    for (auto* c : std::initializer_list<Component*> { &presetBox, &prevBtn, &nextBtn }) content.addAndMakeVisible (c);

    randBtn.setTooltip ("New random step pattern (musical)");
    mutateBtn.setTooltip ("Nudge the pattern a little so it evolves");
    clearBtn.setTooltip ("Reset all 32 steps");
    randBtn.onClick = [this] { proc.randomizeSteps(); };
    mutateBtn.onClick = [this] { proc.mutateSteps(); };
    clearBtn.onClick = [this] { proc.clearSteps(); };
    for (auto* b : { &randBtn, &mutateBtn, &clearBtn }) content.addAndMakeVisible (*b);

    const char* tabNames[] = { "MAIN", "FM", "SYNTH", "SEQUENCER", "MOD", "FX" };
    static const Page tabPages[] = { Main, Fm, Synth, Seq, Mod, Fx };
    for (int i = 0; i < 6; ++i)
    {
        tabs[(size_t) i].setButtonText (tabNames[i]);
        tabs[(size_t) i].onClick = [this, i] { showPage (tabPages[i]); };
        content.addAndMakeVisible (tabs[(size_t) i]);
    }

    layoutPanels();
    buildBackground();
    showPage (Main);

    setResizable (true, true);
    setResizeLimits (W / 2, H / 2, W * 2, H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / H);
    setSize (W, H);
    startTimerHz (30);
}

SqueFMEditor::~SqueFMEditor() { stopTimer(); setLookAndFeel (nullptr); }

SqueFMEditor::Panel& SqueFMEditor::addPanel (const String& title, Rectangle<int> r, int cols, Colour accent, Page pg,
                                             std::initializer_list<std::pair<const char*, const char*>> controls)
{
    Panel p { title, r, cols, accent, pg, {}, 0, 0, nullptr };
    for (auto& c : controls)
    {
        if (String (c.first).isEmpty()) { p.items.push_back (nullptr); continue; }
        cells.push_back (std::make_unique<ui::ControlCell> (proc.apvts, c.first, c.second, accent, &content));
        content.addAndMakeVisible (*cells.back());
        p.items.push_back (cells.back().get());
    }
    panels.push_back (p);
    return panels.back();
}

ui::ControlCell* SqueFMEditor::findCell (const String& id)
{
    for (auto& c : cells) if (c->paramId == id) return c.get();
    return nullptr;
}

void SqueFMEditor::layoutPanels()
{
    for (auto& p : panels)
    {
        auto area = p.r.reduced (10).withTrimmedTop (24);
        if (p.extra != nullptr && p.items.empty())
        {
            if (p.extra == &grid) { area.removeFromTop (8); grid.setBounds (area.removeFromTop (area.getHeight() - 170)); area.removeFromTop (8); laneEditor.setBounds (area); }
            else p.extra->setBounds (area);
            continue;
        }
        if (p.topExtra > 0 && p.extra != nullptr) { p.extra->setBounds (area.removeFromTop (p.topExtra - 8)); area.removeFromTop (8); }
        const int n = (int) p.items.size();
        if (n == 0) continue;
        const int rows = (n + p.cols - 1) / p.cols;
        const int cw = area.getWidth() / p.cols, ch = jmin (110, area.getHeight() / jmax (1, rows));
        const int yOff = (area.getHeight() - ch * rows) / 2;
        for (int i = 0; i < n; ++i)
            if (auto* c = p.items[(size_t) i])
                c->setBounds (area.getX() + (i % p.cols) * cw, area.getY() + yOff + (i / p.cols) * ch, cw, ch);
    }
    const int rowH = matrixArea.getHeight() / sq::kMatrixSlots;
    for (int i = 0; i < sq::kMatrixSlots; ++i)
        matrixRows[(size_t) i]->setBounds (matrixArea.getX(), matrixArea.getY() + i * rowH, matrixArea.getWidth(), rowH);

    prevBtn.setBounds (378, 18, 28, 30);
    presetBox.setBounds (410, 18, 266, 30);
    nextBtn.setBounds (680, 18, 28, 30);
    for (int i = 0; i < 6; ++i) tabs[(size_t) i].setBounds (W - 10 - (6 - i) * 92, 18, 88, 32);
    // sequencer tools sit in the grid panel's title bar
    randBtn.setBounds (1270 - 3 * 92 - 10, 72 + 4, 86, 22);
    mutateBtn.setBounds (1270 - 2 * 92 - 10, 72 + 4, 86, 22);
    clearBtn.setBounds (1270 - 1 * 92 - 10, 72 + 4, 86, 22);
}

void SqueFMEditor::showPage (Page pg)
{
    page = pg;
    static const Page tabPages[] = { Main, Fm, Synth, Seq, Mod, Fx };
    for (int i = 0; i < 6; ++i) tabs[(size_t) i].setToggleState (tabPages[i] == pg, dontSendNotification);
    for (auto& p : panels)
    {
        const bool vis = p.page == pg || p.page == Always;
        for (auto* c : p.items) if (c) c->setVisible (vis);
        if (p.extra) p.extra->setVisible (vis);
    }
    for (auto& r : matrixRows) r->setVisible (pg == Mod);
    laneEditor.setVisible (pg == Seq);
    for (auto* b : { &randBtn, &mutateBtn, &clearBtn }) b->setVisible (pg == Seq);
    content.repaint();
}

void SqueFMEditor::buildBackground()
{
    background = Image (Image::ARGB, W, H, true);
    Graphics g (background);
    using namespace ui;
    ColourGradient bg (Colour (0xff1a0e30), W * 0.25f, H * 0.1f, col::bg0, W * 0.9f, H * 1.1f, true);
    bg.addColour (0.55, Colour (0xff0e0819));
    g.setGradientFill (bg); g.fillAll();

    // disco-ball dot lattice
    Random rnd (777);
    for (int y = 8; y < H; y += 18)
        for (int x = (y / 18) % 2 ? 9 : 0; x < W; x += 18)
        {
            const float tw = rnd.nextFloat();
            g.setColour ((tw > 0.97f ? col::gold : (tw > 0.9f ? col::pink : col::violet)).withAlpha (tw > 0.9f ? 0.18f : 0.04f));
            g.fillEllipse ((float) x, (float) y, 2.f, 2.f);
        }
    // valley contour lines
    for (int l = 0; l < 14; ++l)
    {
        Path p; const float base = H * 0.55f + l * 22.f, amp = 30.f + l * 6.f;
        for (int x = 0; x <= W; x += 8)
        {
            const float t = (float) x / W;
            const float yy = base + amp * std::pow (std::abs (t - 0.5f) * 2.f, 1.6f) * -1.f + 8.f * std::sin (t * 9.f + l);
            if (x == 0) p.startNewSubPath (0.f, yy); else p.lineTo ((float) x, yy);
        }
        g.setColour ((l % 2 ? col::cyan : col::pink).withAlpha (0.035f));
        g.strokePath (p, PathStrokeType (1.f));
    }
    // logo
    g.setFont (font (11.f, true)); g.setColour (col::cyan);
    g.drawText ("D I S C O   V A L L E Y", 22, 8, 260, 16, Justification::centredLeft);
    auto f = font (36.f, true); g.setFont (f);
    for (int i = 6; i > 0; --i) { g.setColour (col::pink.withAlpha (0.03f * i)); g.drawText ("SqueFM", 20 - i / 2, 20 - i / 2, 250 + i, 44 + i, Justification::centredLeft); }
    g.setGradientFill (ColourGradient (col::pink, 20.f, 24.f, col::gold, 190.f, 60.f, false));
    g.drawText ("SqueFM", 20, 20, 250, 44, Justification::centredLeft);
    g.setFont (font (10.f, true)); g.setColour (col::dim);
    g.drawText ("ZENON / PROG / PSYTECH / DARK / HI-TECH", 170, 38, 250, 18, Justification::centredLeft);
    g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.45f, Colours::black.withAlpha (0.5f), 0.f, 0.f, true));
    g.fillAll();
}

void SqueFMEditor::paint (Graphics& g) { g.fillAll (ui::col::bg0); }

void SqueFMEditor::paintContent (Graphics& g)
{
    using namespace ui;
    g.drawImageAt (background, 0, 0);
    for (auto& p : panels)
    {
        if (! (p.page == page || p.page == Always)) continue;
        const auto r = p.r.toFloat();
        g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0.f, 3.f), 12.f);
        g.setGradientFill (ColourGradient (Colour (0xf01b1030), r.getX(), r.getY(), Colour (0xf00f0a1c), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 12.f);
        g.setColour (p.accent.withAlpha (0.28f)); g.drawRoundedRectangle (r.reduced (0.5f), 12.f, 1.f);
        g.setColour (p.accent); g.fillEllipse (r.getX() + 12.f, r.getY() + 11.f, 7.f, 7.f);
        g.setColour (p.accent.withAlpha (0.3f)); g.fillEllipse (r.getX() + 9.f, r.getY() + 8.f, 13.f, 13.f);
        g.setFont (font (11.5f, true)); g.setColour (p.accent);
        g.drawText (p.title, Rectangle<float> (r.getX() + 27.f, r.getY() + 5.f, r.getWidth() - 34.f, 19.f), Justification::centredLeft);
        g.setColour (p.accent.withAlpha (0.14f)); g.drawHorizontalLine ((int) r.getY() + 28, r.getX() + 10.f, r.getRight() - 10.f);
    }
    if (page == Mod)
    {
        g.setFont (font (10.f, true)); g.setColour (col::dim);
        const float y = (float) matrixArea.getY() - 20.f; const float x0 = (float) matrixArea.getX();
        g.drawText ("SOURCE", (int) x0 + 34, (int) y, 190, 16, Justification::centredLeft);
        g.drawText ("VIA  (scales amount)", (int) x0 + 234, (int) y, 150, 16, Justification::centredLeft);
        g.drawText ("AMOUNT", (int) x0 + 398, (int) y, 420, 16, Justification::centred);
        g.drawText ("DESTINATION", (int) x0 + 852, (int) y, 220, 16, Justification::centredLeft);
        g.drawText ("LIVE", matrixArea.getRight() - 130, (int) y, 120, 16, Justification::centred);
    }
    if (page == Fx)
    {
        static const char* tips[] = {
            "ZENON / PROG  -  Ladder 24dB, Res 0.7-0.9, Drive 0.4. Mix HOLD steps with SQUELCH steps at 1/16-1/32.",
            "PSYTECH  -  FM Ratio 2-5, Index 0.2-0.5, Feedback up. Route Mod Env + Sequencer to FM Index. Add Comb.",
            "HI-TECH  -  Zap Pitch +24..+48 with 20-40 ms decay, BLIP / S&H steps, Crush a little, 1/16D delay.",
            "DARK / FOREST  -  FM in Hz mode at 3-12 Hz = sub-audio ripping squelch. Chaos > Vowel, Talk 0.5.",
            "SPITS  -  Spit source > Cutoff + FM Index + Vowel. Short Spit Decay (10-40 ms) = wet spits.",
            "INVERT  -  any negative matrix amount flips the modulation. Low above High in a step = upside-down LFO.",
            "MACROS  -  map Macro 1-4 in the matrix, then automate them in Cubase for live tweaks.",
            "MOVEMENT  -  Seq Speed off 1.00x, a little Wander, Mutate every few bars: nothing stays still." };
        g.setFont (font (12.5f));
        int y = 370 + 42;
        for (auto* t : tips)
        {
            const String s (t); const int dash = s.indexOf ("  -  ");
            g.setColour (col::gold); g.setFont (font (12.5f, true));
            g.drawText (s.substring (0, dash), 556, y, 130, 30, Justification::centredLeft);
            g.setColour (col::text.withAlpha (0.85f)); g.setFont (font (12.f));
            g.drawFittedText (s.substring (dash + 5), 690, y, 568, 30, Justification::centredLeft, 2);
            y += 35;
        }
    }
    if (true)
    {
        g.setFont (font (9.5f, true)); g.setColour (col::gold.withAlpha (0.8f));
        g.drawText ("Macros: assign in MOD > Matrix (source Macro 1-4)", 90, 712, 470, 16, Justification::centredRight);
    }
}

void SqueFMEditor::resized() { content.setTransform (AffineTransform::scale ((float) getWidth() / (float) W)); }

void SqueFMEditor::timerCallback()
{
    flView.push();
    if (page == Main) flView.repaint();
    if (page == Fm) algoView.repaint();
    if (page == Seq) { grid.repaint(); laneEditor.repaint(); }
    if (page == Synth) filterDisplay.repaint();
    if (page == Mod) { for (auto& v : lfoViews) v->repaint(); for (auto& r : matrixRows) r->repaint(); }
    scope.repaint();

    const int prog = proc.getCurrentProgram();
    if (prog != lastProgram) { lastProgram = prog; presetBox.setSelectedId (prog + 1, dontSendNotification); }

    for (int m = 0; m < sq::kNumMacros; ++m)
    {
        String hint;
        for (int sl = 0; sl < sq::kMatrixSlots; ++sl)
            if ((int) proc.getParam (sq::slotId (sl, "src")) == sq::SrcMacro1 + m)
            {
                const int d = (int) proc.getParam (sq::slotId (sl, "dst"));
                if (d > 0) hint << (hint.isEmpty() ? "" : " + ") << String (sq::kDstNames[d]).toUpperCase();
            }
        if (auto* c = findCell (sq::macroId (m).c_str())) c->setLabel (hint.isEmpty() ? "MACRO " + String (m + 1) : hint);
    }
    // dim the control that is not active for the current mode
    auto dimIf = [this] (const char* id, bool active) { if (auto* c = findCell (id)) c->setAlpha (active ? 1.f : 0.35f); };
    const bool fmHz = proc.getParam ("fmMode") > 0.5f;
    dimIf ("fmRatio", ! fmHz); dimIf ("fmFine", ! fmHz); dimIf ("fmHz", fmHz);
    const bool flHz = proc.getParam ("flMode") > 0.5f;
    dimIf ("flSync", ! flHz); dimIf ("flHz", flHz);
    const bool ck = proc.getParam ("combKey") > 0.5f;
    dimIf ("combSemi", ck); dimIf ("combHz", ! ck);
    for (int i = 0; i < sq::kNumLfos; ++i)
    {
        const bool hz = proc.getParam (sq::lfoId (i, "mode")) > 0.5f;
        dimIf (sq::lfoId (i, "sync").c_str(), ! hz); dimIf (sq::lfoId (i, "hz").c_str(), hz);
    }
}
