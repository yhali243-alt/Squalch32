#include "PluginEditor.h"
#include <complex>

using namespace juce;
namespace ui
{
// ============================================================ LookAndFeel ===
SwampLookAndFeel::SwampLookAndFeel()
{
    setColour (ComboBox::backgroundColourId, col::bg1);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::outlineColourId, col::border);
    setColour (ComboBox::arrowColourId, col::lime);
    setColour (PopupMenu::backgroundColourId, Colour (0xf20b1611));
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::lime.withAlpha (0.2f));
    setColour (PopupMenu::highlightedTextColourId, col::lime);
    setColour (TextButton::textColourOffId, col::text);
    setColour (TextButton::textColourOnId, col::lime);
    setColour (BubbleComponent::backgroundColourId, Colour (0xf0101d17));
    setColour (BubbleComponent::outlineColourId, col::lime.withAlpha (0.5f));
    setColour (TooltipWindow::textColourId, col::text);
}

void SwampLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float a0, float a1, Slider& s)
{
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);
    const float size = (float) jmin (w, h) - 6.f;
    const auto c = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const float r = size * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    const bool bip = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float from = bip ? (a0 + a1) * 0.5f : a0;

    // track
    Path track; track.addCentredArc (c.x, c.y, r, r, 0.f, a0, a1, true);
    g.setColour (Colour (0xff1a2c24));
    g.strokePath (track, PathStrokeType (3.2f, PathStrokeType::curved, PathStrokeType::rounded));

    // value arc + glow
    if (std::abs (ang - from) > 0.001f)
    {
        Path arc; arc.addCentredArc (c.x, c.y, r, r, 0.f, jmin (from, ang), jmax (from, ang), true);
        const auto vc = accent.interpolatedWith (col::magenta, bip ? std::abs (pos - 0.5f) : pos * 0.55f);
        g.setColour (vc.withAlpha (0.18f));
        g.strokePath (arc, PathStrokeType (8.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (vc);
        g.strokePath (arc, PathStrokeType (3.2f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // body
    const float br = r - 6.f;
    ColourGradient body (Colour (0xff22382e), c.x - br * 0.6f, c.y - br * 0.8f, Colour (0xff09120e), c.x + br * 0.5f, c.y + br, true);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.f);

    // pointer
    const float pr0 = br * 0.25f, pr1 = br * 0.92f;
    const Point<float> p0 (c.x + pr0 * std::sin (ang), c.y - pr0 * std::cos (ang));
    const Point<float> p1 (c.x + pr1 * std::sin (ang), c.y - pr1 * std::cos (ang));
    g.setColour (s.isMouseOverOrDragging() ? col::lime : col::text);
    g.drawLine ({ p0, p1 }, 2.2f);
}

void SwampLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool hi, bool)
{
    const auto accent = b.findColour (ToggleButton::tickColourId);
    auto r = b.getLocalBounds().toFloat().withSizeKeepingCentre (34.f, 17.f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent.withAlpha (0.25f) : Colour (0xff15241d));
    g.fillRoundedRectangle (r, 8.5f);
    g.setColour (on ? accent : (hi ? col::dim : col::border));
    g.drawRoundedRectangle (r, 8.5f, 1.2f);
    const float d = 11.f;
    const float cx = on ? r.getRight() - d - 3.f : r.getX() + 3.f;
    if (on) { g.setColour (accent.withAlpha (0.3f)); g.fillEllipse (cx - 3.f, r.getCentreY() - d * 0.5f - 3.f, d + 6.f, d + 6.f); }
    g.setColour (on ? accent : col::dim);
    g.fillEllipse (cx, r.getCentreY() - d * 0.5f, d, d);
}

void SwampLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::bg1);
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (box.isMouseOver (true) ? box.findColour (ComboBox::arrowColourId) : col::border);
    g.drawRoundedRectangle (r, 5.f, 1.f);
    Path p;
    const float ax = (float) w - 11.f, ay = (float) h * 0.5f;
    p.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (box.findColour (ComboBox::arrowColourId));
    g.fillPath (p);
}

void SwampLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? col::lime.withAlpha (0.3f) : (hi ? col::lime.withAlpha (0.12f) : col::bg1));
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (hi ? col::lime : col::border);
    g.drawRoundedRectangle (r, 5.f, 1.f);
}

// ============================================================ ControlCell ===
ControlCell::ControlCell (AudioProcessorValueTreeState& s, const String& id, const String& label,
                          Colour acc, Component* popupParent)
    : accent (acc), labelText (label)
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
        for (const auto& d : sq::paramTable()) if (id == d.id) bip = d.bipolar;
        if (id.endsWith ("depth") || id.endsWith ("off")) bip = true;
        slider->getProperties().set ("bipolar", bip);
        slider->setPopupDisplayEnabled (true, true, popupParent, 1200);
        slider->setVelocityBasedMode (false);
        slider->setMouseDragSensitivity (220);
        addAndMakeVisible (*slider);
        sAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, id, *slider);
        slider->setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        slider->setTooltip (p->getName (64));
    }
}

void ControlCell::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (15);
    if (slider)
    {
        const int s = jmin (r.getWidth(), r.getHeight(), 58);
        slider->setBounds (r.withSizeKeepingCentre (s, s));
    }
    if (combo)  combo->setBounds (r.withSizeKeepingCentre (jmin (r.getWidth() - 6, 92), 22));
    if (toggle) toggle->setBounds (r.withSizeKeepingCentre (44, 24));
}

void ControlCell::paint (Graphics& g)
{
    g.setFont (font (10.f, true));
    g.setColour (col::dim.brighter (0.25f));
    g.drawText (labelText, getLocalBounds().removeFromBottom (15), Justification::centred, false);
}

// =============================================================== StepGrid ===
StepGrid::StepGrid (Squalch32Processor& p) : proc (p)
{
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        pRate[(size_t) i]  = proc.apvts.getParameter (sq::stepId (i, "rate"));
        pShape[(size_t) i] = proc.apvts.getParameter (sq::stepId (i, "shape"));
        pDepth[(size_t) i] = proc.apvts.getParameter (sq::stepId (i, "depth"));
        pOff[(size_t) i]   = proc.apvts.getParameter (sq::stepId (i, "off"));
    }
    pSteps = proc.apvts.getParameter ("steps");
    pSpeed = proc.apvts.getParameter ("speed");
    setTooltip ("Drag in CURVE to draw step offsets, in DEPTH to draw LFO depth. "
                "Click SHAPE to cycle (right-click for menu). Drag or scroll RATE. Double-click resets.");
}

void StepGrid::resized()
{
    auto r = getLocalBounds().toFloat();
    gutter = r.removeFromLeft (46.f);
    gridArea = r;
    laneNums  = r.removeFromBottom (14.f);
    laneRate  = r.removeFromBottom (22.f);
    laneShape = r.removeFromBottom (22.f);
    r.removeFromBottom (4.f);
    laneDepth = r.removeFromBottom (r.getHeight() * 0.32f);
    r.removeFromBottom (4.f);
    laneCurve = r;
}

float StepGrid::previewShape (int shape, float p, int seed)
{
    auto rnd = [] (int n) { uint32_t x = (uint32_t) n * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return (float) (x & 0xffff) / 32767.5f - 1.f; };
    const int cyc = (int) std::floor (p);
    const float f = p - (float) cyc;
    switch (shape)
    {
        case 0: return std::sin (sq::kTwoPi * f);
        case 1: return f < 0.5f ? 4.f * f - 1.f : 3.f - 4.f * f;
        case 2: return 2.f * f - 1.f;
        case 3: return 1.f - 2.f * f;
        case 4: return f < 0.5f ? 1.f : -1.f;
        case 5: return rnd (seed * 97 + cyc);
        case 6: { const float a = rnd (seed * 97 + cyc - 1), b = rnd (seed * 97 + cyc); return a + (b - a) * (0.5f - 0.5f * std::cos (sq::kPi * f)); }
        case 7: return std::exp (-f * 7.f) * 1.6f - 0.35f;
        case 8: { const float a = rnd (seed * 131 + cyc - 1), b = rnd (seed * 131 + cyc); return a + (b - a) * (f * f * (3.f - 2.f * f)); }
        default: return 0.f;
    }
}

void StepGrid::drawShapeIcon (Graphics& g, int shape, Rectangle<float> r)
{
    Path p;
    const int N = 24;
    for (int i = 0; i <= N; ++i)
    {
        const float t = (float) i / N;
        float v = previewShape (shape, t * (shape >= 5 && shape != 7 ? 3.f : 1.f), 7);
        if (shape == 4 && i > 0 && i < N && std::abs (t - 0.5f) < 0.03f) v = 0.f;
        const float x = r.getX() + t * r.getWidth();
        const float y = r.getCentreY() - v * r.getHeight() * 0.42f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.strokePath (p, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
}

void StepGrid::paint (Graphics& g)
{
    const int nSteps = (int) val (pSteps);
    const int cur = proc.engine.uiStep.load();
    const float frac = proc.engine.uiStepFrac.load();
    const float speed = val (pSpeed);

    g.setColour (Colour (0xff08110d));
    g.fillRoundedRectangle (gridArea.expanded (2.f), 6.f);

    // gutter labels
    g.setFont (font (9.5f, true));
    g.setColour (col::dim);
    g.drawText ("CURVE", gutter.withY (laneCurve.getY()).withHeight (laneCurve.getHeight()), Justification::centredLeft);
    g.drawText ("DEPTH", gutter.withY (laneDepth.getY()).withHeight (laneDepth.getHeight()), Justification::centredLeft);
    g.drawText ("SHAPE", gutter.withY (laneShape.getY()).withHeight (laneShape.getHeight()), Justification::centredLeft);
    g.drawText ("RATE",  gutter.withY (laneRate.getY()).withHeight (laneRate.getHeight()), Justification::centredLeft);

    const float cw = colW();
    for (int i = 0; i < sq::kMaxSteps; ++i)
    {
        const float x = colX (i);
        const bool active = i < nSteps;
        const bool isCur = active && i == cur;
        const float a = active ? 1.f : 0.25f;
        const int rate = (int) val (pRate[(size_t) i]);
        const int shape = (int) val (pShape[(size_t) i]);
        const float depth = val (pDepth[(size_t) i]);
        const float off = val (pOff[(size_t) i]);

        // column backgrounds
        auto colR = Rectangle<float> (x + 1.f, laneCurve.getY(), cw - 2.f, laneRate.getBottom() - laneCurve.getY());
        const bool beat = (i / 4) % 2 == 0;
        g.setColour ((beat ? Colour (0xff0f1e17) : Colour (0xff0b1712)).withMultipliedAlpha (active ? 1.f : 0.5f));
        g.fillRect (colR);
        if (isCur)
        {
            g.setColour (col::lime.withAlpha (0.10f)); g.fillRect (colR);
        }
        if (i == hoverStep) { g.setColour (Colours::white.withAlpha (0.04f)); g.fillRect (colR); }

        // ---- curve lane: offset bar + live LFO preview
        const float cy = laneCurve.getCentreY(), hh = laneCurve.getHeight() * 0.5f - 3.f;
        {
            const float top = jmin (cy, cy - off * hh), bot = jmax (cy, cy - off * hh);
            g.setColour (col::lime.withAlpha (0.16f * a));
            g.fillRect (Rectangle<float> (x + 2.f, top, cw - 4.f, jmax (1.f, bot - top)));
            g.setColour (col::lime.withAlpha (0.55f * a));
            g.fillRect (Rectangle<float> (x + 2.f, cy - off * hh - 1.f, cw - 4.f, 2.f));

            Path curve;
            const float cycles = sq::kStepRates[jlimit (0, sq::kNumStepRates - 1, rate)] * speed;
            const int N = 28;
            for (int k = 0; k <= N; ++k)
            {
                const float t = (float) k / N;
                const float v = jlimit (-1.15f, 1.15f, off + (cycles > 0.f ? depth * previewShape (shape, t * cycles, i) : 0.f));
                const float px = x + 1.f + t * (cw - 2.f), py = cy - v * hh;
                if (k == 0) curve.startNewSubPath (px, py); else curve.lineTo (px, py);
            }
            const auto cc = isCur ? col::magenta : col::toxic;
            g.setColour (cc.withAlpha (0.25f * a));
            g.strokePath (curve, PathStrokeType (4.f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (cc.withAlpha (0.95f * a));
            g.strokePath (curve, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // ---- depth lane
        {
            const float dy = laneDepth.getCentreY(), dh = laneDepth.getHeight() * 0.5f - 2.f;
            const float top = jmin (dy, dy - depth * dh), bot = jmax (dy, dy - depth * dh);
            g.setColour (col::magenta.withAlpha (0.65f * a));
            g.fillRoundedRectangle (Rectangle<float> (x + 3.f, top, cw - 6.f, jmax (1.5f, bot - top)), 2.f);
        }

        // ---- shape icon
        g.setColour ((isCur ? col::lime : col::violet).withAlpha (a));
        drawShapeIcon (g, shape, Rectangle<float> (x + 4.f, laneShape.getY() + 4.f, cw - 8.f, laneShape.getHeight() - 8.f));

        // ---- rate text
        g.setFont (font (cw < 26.f ? 9.f : 10.f, true));
        g.setColour ((rate == 0 ? col::dim : col::amber).withAlpha (a));
        g.drawText (sq::kStepRateNames[jlimit (0, sq::kNumStepRates - 1, rate)],
                    Rectangle<float> (x, laneRate.getY(), cw, laneRate.getHeight()), Justification::centred, false);

        // ---- numbers
        g.setFont (font (9.f, isCur));
        g.setColour (isCur ? col::lime : col::dim.withAlpha (a));
        g.drawText (String (i + 1), Rectangle<float> (x, laneNums.getY(), cw, laneNums.getHeight()), Justification::centred, false);

        // playhead inside step
        if (isCur)
        {
            const float px = x + frac * cw;
            g.setColour (col::lime.withAlpha (0.25f)); g.fillRect (px - 2.f, laneCurve.getY(), 4.f, laneCurve.getHeight());
            g.setColour (col::lime); g.fillRect (px - 0.5f, laneCurve.getY(), 1.2f, laneCurve.getHeight());
        }
    }

    // zero lines and bar separators
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) laneCurve.getCentreY(), gridArea.getX(), gridArea.getRight());
    g.drawHorizontalLine ((int) laneDepth.getCentreY(), gridArea.getX(), gridArea.getRight());
    for (int i = 4; i < sq::kMaxSteps; i += 4)
    {
        g.setColour (i % 16 == 0 ? col::lime.withAlpha (0.25f) : Colours::white.withAlpha (0.08f));
        g.drawVerticalLine ((int) colX (i), laneCurve.getY(), laneRate.getBottom());
    }
    // end marker
    if (nSteps < sq::kMaxSteps)
    {
        g.setColour (col::magenta.withAlpha (0.8f));
        g.fillRect (colX (nSteps) - 1.f, laneCurve.getY(), 2.f, laneRate.getBottom() - laneCurve.getY());
    }
}

int StepGrid::stepAt (float x) const
{
    return jlimit (0, sq::kMaxSteps - 1, (int) ((x - gridArea.getX()) / colW()));
}

StepGrid::Lane StepGrid::laneAt (Point<float> p) const
{
    if (p.x < gridArea.getX()) return None;
    if (laneCurve.contains (p)) return Curve;
    if (laneDepth.contains (p)) return Depth;
    if (laneShape.contains (p)) return Shape;
    if (laneRate.contains (p))  return Rate;
    return None;
}

void StepGrid::setVal (RangedAudioParameter* p, float v)
{
    if (std::find (touched.begin(), touched.end(), p) == touched.end())
    {
        p->beginChangeGesture();
        touched.push_back (p);
    }
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void StepGrid::endAll()
{
    for (auto* p : touched) p->endChangeGesture();
    touched.clear();
}

void StepGrid::setFromY (Lane lane, int step, float y)
{
    if (lane == Curve)
    {
        const float v = jlimit (-1.f, 1.f, (laneCurve.getCentreY() - y) / (laneCurve.getHeight() * 0.5f - 3.f));
        setVal (pOff[(size_t) step], std::abs (v) < 0.04f ? 0.f : v);
    }
    else if (lane == Depth)
    {
        const float v = jlimit (-1.f, 1.f, (laneDepth.getCentreY() - y) / (laneDepth.getHeight() * 0.5f - 2.f));
        setVal (pDepth[(size_t) step], std::abs (v) < 0.05f ? 0.f : v);
    }
}

void StepGrid::showMenu (Lane lane, int step)
{
    PopupMenu m;
    auto* p = lane == Shape ? pShape[(size_t) step] : pRate[(size_t) step];
    const int cur = (int) val (p);
    const int n = lane == Shape ? sq::kNumShapes : sq::kNumStepRates;
    for (int i = 0; i < n; ++i)
        m.addItem (i + 1, lane == Shape ? String (sq::kShapeNames[i]) : String (sq::kStepRateNames[i]) + (i ? " cycles" : " (static)"), true, i == cur);
    m.addSeparator();
    m.addItem (1000, "Apply to ALL steps");
    SafePointer<StepGrid> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(),
        [safe, lane, step, cur] (int r)
        {
            if (safe == nullptr || r == 0) return;
            auto& self = *safe;
            auto& arr = lane == Shape ? self.pShape : self.pRate;
            if (r == 1000)
            {
                for (auto* q : arr) self.setVal (q, (float) cur);
            }
            else self.setVal (arr[(size_t) step], (float) (r - 1));
            self.endAll();
            self.repaint();
        });
}

void StepGrid::mouseDown (const MouseEvent& e)
{
    const auto pt = e.position;
    const Lane lane = laneAt (pt);
    if (lane == None) return;
    const int step = stepAt (pt.x);
    dragLane = lane; dragStep = step;

    if (e.mods.isPopupMenu() && (lane == Shape || lane == Rate)) { showMenu (lane, step); dragLane = None; return; }

    if (lane == Curve || lane == Depth)
    {
        if (e.mods.isAltDown()) { setVal (lane == Curve ? pOff[(size_t) step] : pDepth[(size_t) step], 0.f); return; }
        setFromY (lane, step, pt.y);
    }
    else if (lane == Shape)
    {
        const int curShape = (int) val (pShape[(size_t) step]);
        const int next = (curShape + (e.mods.isShiftDown() ? sq::kNumShapes - 1 : 1)) % sq::kNumShapes;
        setVal (pShape[(size_t) step], (float) next);
    }
    else if (lane == Rate)
    {
        rateDragStartY = pt.y;
        rateDragStartVal = (int) val (pRate[(size_t) step]);
    }
    repaint();
}

void StepGrid::mouseDrag (const MouseEvent& e)
{
    const auto pt = e.position;
    if (dragLane == Curve || dragLane == Depth)
    {
        // draw across steps, interpolating so fast drags don't skip columns
        const int step = stepAt (pt.x);
        const int a = jmin (step, dragStep), b = jmax (step, dragStep);
        for (int s = a; s <= b; ++s) setFromY (dragLane, s, pt.y);
        dragStep = step;
        repaint();
    }
    else if (dragLane == Rate)
    {
        const int delta = (int) std::round ((rateDragStartY - pt.y) / 10.f);
        setVal (pRate[(size_t) dragStep], (float) jlimit (0, sq::kNumStepRates - 1, rateDragStartVal + delta));
        repaint();
    }
}

void StepGrid::mouseUp (const MouseEvent&) { endAll(); dragLane = None; }

void StepGrid::mouseDoubleClick (const MouseEvent& e)
{
    const Lane lane = laneAt (e.position);
    const int step = stepAt (e.position.x);
    if (lane == Curve) setVal (pOff[(size_t) step], 0.f);
    else if (lane == Depth) setVal (pDepth[(size_t) step], 0.f);
    else if (lane == Rate) setVal (pRate[(size_t) step], 4.f);
    endAll();
    repaint();
}

void StepGrid::mouseMove (const MouseEvent& e)
{
    hoverLane = laneAt (e.position);
    hoverStep = hoverLane == None ? -1 : stepAt (e.position.x);
    setMouseCursor (hoverLane == Rate ? MouseCursor::UpDownResizeCursor
                    : hoverLane == Shape ? MouseCursor::PointingHandCursor
                    : hoverLane == None ? MouseCursor::NormalCursor : MouseCursor::CrosshairCursor);
}

void StepGrid::mouseExit (const MouseEvent&) { hoverStep = -1; hoverLane = None; }

void StepGrid::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const Lane lane = laneAt (e.position);
    const int step = stepAt (e.position.x);
    const int dir = w.deltaY > 0 ? 1 : -1;
    if (lane == Rate)
        setVal (pRate[(size_t) step], (float) jlimit (0, sq::kNumStepRates - 1, (int) val (pRate[(size_t) step]) + dir));
    else if (lane == Shape)
        setVal (pShape[(size_t) step], (float) ((((int) val (pShape[(size_t) step]) + dir) % sq::kNumShapes + sq::kNumShapes) % sq::kNumShapes));
    else if (lane == Curve)
        setVal (pOff[(size_t) step], jlimit (-1.f, 1.f, val (pOff[(size_t) step]) + dir * 0.05f));
    else if (lane == Depth)
        setVal (pDepth[(size_t) step], jlimit (-1.f, 1.f, val (pDepth[(size_t) step]) + dir * 0.05f));
    endAll();
    repaint();
}

// ========================================================== FilterDisplay ===
FilterDisplay::FilterDisplay (Squalch32Processor& p) : proc (p)
{
    res   = proc.apvts.getRawParameterValue ("res");
    slope = proc.apvts.getRawParameterValue ("slope");
    mode  = proc.apvts.getRawParameterValue ("fmode");
}

void FilterDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff07100c));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (col::border);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);

    // grid
    g.setColour (Colours::white.withAlpha (0.05f));
    for (float f : { 100.f, 1000.f, 10000.f })
    {
        const float x = r.getX() + r.getWidth() * std::log10 (f / 20.f) / 3.f;
        g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    }

    const float fc = proc.engine.uiCutoffHz.load();
    const float k = jlimit (0.f, 3.9f, res->load() * 4.1f);
    const float sl = slope->load();
    const int md = (int) mode->load();

    Path p;
    const int N = 140;
    const float dbTop = 24.f, dbBot = -48.f;
    for (int i = 0; i <= N; ++i)
    {
        const float t = (float) i / N;
        const float f = 20.f * std::pow (1000.f, t);
        const std::complex<float> h1 = 1.f / std::complex<float> (1.f, f / fc);
        const std::complex<float> h2 = h1 * h1, h3 = h2 * h1, h4 = h3 * h1;
        const std::complex<float> u = 1.f / (1.f + k * h4);
        std::complex<float> y;
        if (md == 0)
        {
            const std::complex<float> taps[4] = { h1, h2, h3, h4 };
            const int a = jmin (2, (int) sl); const float fr = sl - (float) a;
            y = u * (taps[a] + (taps[a + 1] - taps[a]) * fr);
        }
        else if (md == 1)
        {
            const auto bp2 = 2.f * (h1 - h2), bp4 = 4.f * (h2 - 2.f * h3 + h4);
            y = u * (bp2 + (bp4 - bp2) * (sl / 3.f));
        }
        else
        {
            const auto hp2 = 1.f - 2.f * h1 + h2, hp4 = 1.f - 4.f * h1 + 6.f * h2 - 4.f * h3 + h4;
            y = u * (hp2 + (hp4 - hp2) * (sl / 3.f));
        }
        const float mag = std::abs (y) * (1.f + k * 0.28f);
        const float db = jlimit (dbBot, dbTop, 20.f * std::log10 (mag + 1e-6f));
        const float x = r.getX() + t * r.getWidth();
        const float yy = r.getY() + 4.f + (dbTop - db) / (dbTop - dbBot) * (r.getHeight() - 8.f);
        if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
    }
    Path fill (p);
    fill.lineTo (r.getRight(), r.getBottom()); fill.lineTo (r.getX(), r.getBottom()); fill.closeSubPath();
    g.setGradientFill (ColourGradient (col::magenta.withAlpha (0.35f), r.getX(), r.getY(), col::toxic.withAlpha (0.02f), r.getX(), r.getBottom(), false));
    g.fillPath (fill);
    g.setColour (col::magenta.withAlpha (0.3f));
    g.strokePath (p, PathStrokeType (5.f));
    g.setColour (col::magenta.brighter (0.3f));
    g.strokePath (p, PathStrokeType (1.6f));

    g.setFont (font (10.f, true));
    g.setColour (col::text.withAlpha (0.8f));
    g.drawText (fc >= 1000.f ? String (fc / 1000.f, 2) + " kHz" : String ((int) fc) + " Hz",
                r.reduced (8.f, 4.f), Justification::topRight);
}

// ================================================================ ModScope ==
void ModScope::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff07100c));
    g.fillRoundedRectangle (r, 5.f);
    const int n = sq::Engine::kScopeSize;
    const int w = proc.engine.scopeWrite.load();
    Path p;
    for (int i = 0; i < n; ++i)
    {
        const float v = jlimit (-1.3f, 1.3f, proc.engine.scope[(w + i) % n]);
        const float x = r.getX() + r.getWidth() * (float) i / (n - 1);
        const float y = r.getCentreY() - v * (r.getHeight() * 0.36f);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (col::lime.withAlpha (0.25f));
    g.strokePath (p, PathStrokeType (4.f));
    g.setColour (col::lime);
    g.strokePath (p, PathStrokeType (1.4f));
    g.setFont (font (9.f, true));
    g.setColour (col::dim);
    g.drawText ("MOD OUT", r.reduced (6.f, 2.f), Justification::topLeft);
}
} // namespace ui

// ================================================================== EDITOR ==
Squalch32Editor::Squalch32Editor (Squalch32Processor& p)
    : AudioProcessorEditor (p), proc (p), grid (p), filterDisplay (p), scope (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setBounds (0, 0, W, H);

    using namespace ui;
    const int y1 = 62, h1 = 250, y2 = 320, h2 = 272, y3 = 600, h3 = 210;

    int x = 10;
    addPanel ("OSC 1", { x, y1, 150, h1 }, 2, col::lime,
              { { "o1wave", "WAVE" }, { "o1oct", "OCTAVE" }, { "o1semi", "SEMI" }, { "o1fine", "FINE" } });
    x += 158;
    addPanel ("OSC 2", { x, y1, 210, h1 }, 3, col::lime,
              { { "o2wave", "WAVE" }, { "o2oct", "OCTAVE" }, { "o2semi", "SEMI" }, { "o2fine", "FINE" }, { "fm", "FM 1>2" }, { "o2sync", "HARD SYNC" } });
    x += 218;
    addPanel ("MIXER", { x, y1, 210, h1 }, 3, col::toxic,
              { { "lv1", "OSC 1" }, { "lv2", "OSC 2" }, { "lvsub", "SUB" }, { "suboct", "SUB OCT" }, { "lvnoise", "NOISE" }, { "fdbk", "FEEDBACK" } });
    x += 218;
    {
        auto& fp = addPanel ("LADDER FILTER", { x, y1, 330, h1 }, 4, col::magenta,
              { { "cutoff", "CUTOFF" }, { "res", "RESONANCE" }, { "drive", "MULTIDRIVE" }, { "slope", "SLOPE 6-24" },
                { "fmode", "MODE" }, { "ktrk", "KEY TRACK" }, { "envamt", "ENV AMT" }, { "resenv", "RES ENV" } });
        fp.topExtra = 70;
    }
    x += 338;
    addPanel ("ENVELOPES", { x, y1, W - 10 - x, h1 }, 4, col::violet,
              { { "fa", "F.ATTACK" }, { "fd", "F.DECAY" }, { "fs", "F.SUSTAIN" }, { "fr", "F.RELEASE" },
                { "aa", "A.ATTACK" }, { "ad", "A.DECAY" }, { "as", "A.SUSTAIN" }, { "ar", "A.RELEASE" },
                { "floop", "F.LOOP" }, { "velf", "VEL>FILT" }, { "vela", "VEL>AMP" }, { "glide", "GLIDE" } });

    {
        auto& sp = addPanel ("STEP-LFO SEQUENCER  /  32 STEPS  /  EVERY STEP IS AN LFO", { 10, y2, W - 20, h2 }, 4, col::lime,
              { { "seqon", "SEQ ON" }, { "seqretrig", "RETRIG" }, { "steps", "STEPS" }, { "stepdiv", "STEP LEN" },
                { "seqamt", ">CUTOFF" }, { "seqsmooth", "SMOOTH" }, { "speed", "SPEED" }, { "wander", "WANDER" },
                { "swing", "SWING" }, { "chaos", "CHAOS" }, { "seqres", ">RESO" }, { "seqpitch", ">PITCH" },
                { "seqdrive", ">DRIVE" }, { "seqtalk", ">TALK" } });
        sp.leftWidth = 320;
    }

    x = 10;
    addPanel ("LFO 2", { x, y3, 210, h3 }, 3, col::violet,
              { { "l2rate", "RATE" }, { "l2shape", "SHAPE" }, { "l2pitch", ">PITCH" }, { "l2cut", ">CUTOFF" }, { "l2wave", ">WAVE" }, { "l2res", ">RESO" } });
    x += 218;
    addPanel ("BITE!", { x, y3, 200, h3 }, 3, col::magenta,
              { { "bitetype", "TYPE" }, { "bite", "AMOUNT" }, { "bitemix", "MIX" }, { "bitepost", "POST FLT" }, { "crush", "CRUSH" } });
    x += 208;
    addPanel ("TALK", { x, y3, 92, h3 }, 1, col::toxic, { { "talkmix", "MIX" }, { "vowel", "A-E-I-O-U" } });
    x += 100;
    addPanel ("PHASER", { x, y3, 150, h3 }, 2, col::violet,
              { { "phmix", "MIX" }, { "phrate", "RATE" }, { "phdepth", "DEPTH" }, { "phfb", "FEEDBACK" } });
    x += 158;
    addPanel ("DELAY", { x, y3, 222, h3 }, 3, col::toxic,
              { { "dlmix", "MIX" }, { "dldiv", "TIME" }, { "dlfb", "FEEDBACK" }, { "dltone", "TONE" }, { "dlmod", "WOBBLE" }, { "dlpp", "PING PONG" } });
    x += 230;
    addPanel ("REVERB", { x, y3, 150, h3 }, 2, col::violet,
              { { "rvmix", "MIX" }, { "rvsize", "SIZE" }, { "rvdamp", "DAMP" } });
    x += 158;
    addPanel ("MASTER", { x, y3, W - 10 - x, h3 }, 2, col::lime,
              { { "out", "OUTPUT" }, { "legato", "LEGATO" }, { "bend", "BEND" } });

    content.addAndMakeVisible (grid);
    content.addAndMakeVisible (filterDisplay);
    content.addAndMakeVisible (scope);

    for (int i = 0; i < (int) proc.presets.size(); ++i) presetBox.addItem (proc.presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.setColour (ComboBox::arrowColourId, col::lime);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) { proc.setCurrentProgram (idx); lastProgram = idx; proc.updateHostDisplay(); }
    };
    content.addAndMakeVisible (presetBox);

    randBtn.setTooltip ("Generate a new random 32-step LFO pattern");
    mutateBtn.setTooltip ("Nudge the current pattern a little - evolve it");
    clearBtn.setTooltip ("Reset all steps");
    randBtn.onClick   = [this] { proc.randomizeSteps(); };
    mutateBtn.onClick = [this] { proc.mutateSteps(); };
    clearBtn.onClick  = [this] { proc.clearSteps(); };
    for (auto* b : { &randBtn, &mutateBtn, &clearBtn }) content.addAndMakeVisible (*b);

    layoutPanels();
    buildBackground();

    setResizable (true, true);
    setResizeLimits (W * 6 / 10, H * 6 / 10, W * 2, H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / H);
    setSize (W, H);
    startTimerHz (30);
}

Squalch32Editor::~Squalch32Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

Squalch32Editor::Panel& Squalch32Editor::addPanel (const String& title, Rectangle<int> r, int cols, Colour accent,
                                                    std::initializer_list<std::pair<const char*, const char*>> controls)
{
    Panel p { title, r, cols, accent, {}, 0, 0 };
    for (auto& c : controls)
    {
        cells.push_back (std::make_unique<ui::ControlCell> (proc.apvts, c.first, c.second, accent, &content));
        content.addAndMakeVisible (*cells.back());
        p.items.push_back (cells.back().get());
    }
    panels.push_back (p);
    return panels.back();
}

void Squalch32Editor::layoutPanels()
{
    for (auto& p : panels)
    {
        auto area = p.r.reduced (8).withTrimmedTop (22);
        if (p.topExtra > 0)
        {
            filterDisplay.setBounds (area.removeFromTop (p.topExtra - 6));
            area.removeFromTop (6);
        }
        if (p.leftWidth > 0)
        {
            auto right = area.withTrimmedLeft (p.leftWidth + 10);
            area = area.withWidth (p.leftWidth);
            scope.setBounds (right.removeFromTop (34).withTrimmedLeft (46));
            right.removeFromTop (6);
            grid.setBounds (right);
        }
        const int n = (int) p.items.size();
        const int rows = (n + p.cols - 1) / p.cols;
        const int cw = area.getWidth() / p.cols, ch = area.getHeight() / jmax (1, rows);
        for (int i = 0; i < n; ++i)
            p.items[(size_t) i]->setBounds (area.getX() + (i % p.cols) * cw, area.getY() + (i / p.cols) * ch, cw, ch);
    }
    presetBox.setBounds (560, 16, 290, 28);
    randBtn.setBounds (W - 290, 16, 86, 28);
    mutateBtn.setBounds (W - 196, 16, 86, 28);
    clearBtn.setBounds (W - 102, 16, 86, 28);
}

void Squalch32Editor::buildBackground()
{
    background = Image (Image::ARGB, W, H, true);
    Graphics g (background);
    using namespace ui;

    ColourGradient bg (Colour (0xff0b1a13), W * 0.3f, H * 0.2f, col::bg0, W * 0.9f, H * 1.1f, true);
    bg.addColour (0.6, Colour (0xff09140f));
    g.setGradientFill (bg);
    g.fillAll();

    // swamp contour lines
    Random rnd (4242);
    for (int l = 0; l < 26; ++l)
    {
        Path p;
        const float baseY = (float) l * H / 24.f;
        const float ph = rnd.nextFloat() * 6.28f, amp = 6.f + rnd.nextFloat() * 18.f, fr = 0.004f + rnd.nextFloat() * 0.006f;
        for (int xx = 0; xx <= W; xx += 8)
        {
            const float yy = baseY + amp * std::sin (xx * fr + ph) + amp * 0.5f * std::sin (xx * fr * 2.7f + ph * 1.3f);
            if (xx == 0) p.startNewSubPath (0.f, yy); else p.lineTo ((float) xx, yy);
        }
        g.setColour ((l % 3 == 0 ? col::toxic : col::violet).withAlpha (0.035f));
        g.strokePath (p, PathStrokeType (1.f));
    }
    // spores
    for (int i = 0; i < 260; ++i)
    {
        const float sx = rnd.nextFloat() * W, sy = rnd.nextFloat() * H, sr = 0.6f + rnd.nextFloat() * rnd.nextFloat() * 3.5f;
        const auto c = (i % 5 == 0 ? col::magenta : (i % 3 == 0 ? col::lime : col::toxic));
        g.setColour (c.withAlpha (0.05f + rnd.nextFloat() * 0.12f));
        g.fillEllipse (sx - sr, sy - sr, sr * 2.f, sr * 2.f);
    }

    // panels
    for (auto& p : panels)
    {
        const auto r = p.r.toFloat();
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (r.translated (0.f, 3.f), 10.f);
        ColourGradient pg (Colour (0xf0122219), r.getX(), r.getY(), Colour (0xf00b1611), r.getX(), r.getBottom(), false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (r, 10.f);
        g.setColour (p.accent.withAlpha (0.22f));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.f);
        // title
        g.setColour (p.accent);
        g.fillEllipse (r.getX() + 11.f, r.getY() + 11.f, 6.f, 6.f);
        g.setColour (p.accent.withAlpha (0.3f));
        g.fillEllipse (r.getX() + 8.f, r.getY() + 8.f, 12.f, 12.f);
        g.setFont (font (11.5f, true));
        g.setColour (p.accent);
        g.drawText (p.title, Rectangle<float> (r.getX() + 24.f, r.getY() + 5.f, r.getWidth() - 30.f, 18.f), Justification::centredLeft);
        g.setColour (p.accent.withAlpha (0.12f));
        g.drawHorizontalLine ((int) r.getY() + 26, r.getX() + 8.f, r.getRight() - 8.f);
    }

    // logo
    {
        const String a ("SQUALCH"), b ("32");
        auto f = font (34.f, true);
        g.setFont (f);
        for (int i = 6; i > 0; --i)
        {
            g.setColour (col::lime.withAlpha (0.025f * i));
            g.drawText (a, 18 - i / 2, 8 - i / 2, 220 + i, 44 + i, Justification::centredLeft);
        }
        g.setGradientFill (ColourGradient (col::lime, 18.f, 10.f, col::toxic, 200.f, 50.f, false));
        g.drawText (a, 18, 8, 220, 44, Justification::centredLeft);
        const int aw = (int) GlyphArrangement::getStringWidth (f, a);
        g.setColour (col::magenta);
        g.drawText (b, 18 + aw + 4, 8, 60, 44, Justification::centredLeft);
        g.setFont (font (10.f, true));
        g.setColour (col::dim);
        g.drawText ("STEP-LFO LADDER SYNTH", 238, 13, 300, 16, Justification::centredLeft);
        g.setColour (col::magenta.withAlpha (0.7f));
        g.drawText ("PSYCHEDELIC SWAMP MACHINE", 238, 29, 300, 16, Justification::centredLeft);
    }
    // vignette
    g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.5f, Colours::black.withAlpha (0.45f), 0.f, 0.f, true));
    g.fillAll();
}

void Squalch32Editor::paint (Graphics& g) { g.fillAll (ui::col::bg0); }

void Squalch32Editor::paintContent (Graphics& g) { g.drawImageAt (background, 0, 0); }

void Squalch32Editor::resized()
{
    const float s = (float) getWidth() / (float) W;
    content.setTransform (AffineTransform::scale (s));
}

void Squalch32Editor::timerCallback()
{
    grid.repaint();
    filterDisplay.repaint();
    scope.repaint();
    const int prog = proc.getCurrentProgram();
    if (prog != lastProgram)
    {
        lastProgram = prog;
        presetBox.setSelectedId (prog + 1, dontSendNotification);
    }
}
