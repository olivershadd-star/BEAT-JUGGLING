#include "PluginEditor.h"

using namespace juce;

// ============================================================================
//  Look & feel
// ============================================================================
BJLookAndFeel::BJLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, bjc::bg);
    setColour (Label::textColourId, bjc::text);
    setColour (TextButton::buttonColourId, bjc::panel2);
    setColour (TextButton::buttonOnColourId, bjc::deckA.withAlpha (0.35f));
    setColour (TextButton::textColourOffId, bjc::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (ToggleButton::textColourId, bjc::text);
    setColour (ToggleButton::tickColourId, bjc::deckA);
    setColour (ToggleButton::tickDisabledColourId, bjc::dim);
    setColour (ComboBox::backgroundColourId, bjc::panel2);
    setColour (ComboBox::textColourId, bjc::text);
    setColour (ComboBox::outlineColourId, bjc::panel2.brighter (0.2f));
    setColour (ComboBox::arrowColourId, bjc::deckA);
    setColour (PopupMenu::backgroundColourId, bjc::panel);
    setColour (PopupMenu::textColourId, bjc::text);
    setColour (PopupMenu::highlightedBackgroundColourId, bjc::deckA.withAlpha (0.3f));
    setColour (TextEditor::backgroundColourId, Colour (0xff07090d));
    setColour (TextEditor::textColourId, Colour (0xffb8ffcf));
    setColour (TextEditor::outlineColourId, bjc::panel2.brighter (0.3f));
    setColour (TextEditor::focusedOutlineColourId, bjc::deckA);
    setColour (CaretComponent::caretColourId, bjc::deckA);
    setColour (Slider::textBoxTextColourId, bjc::text);
    setColour (Slider::textBoxBackgroundColourId, Colour (0xff07090d));
    setColour (Slider::textBoxOutlineColourId, bjc::panel2);
    setColour (Slider::trackColourId, bjc::deckA.withAlpha (0.6f));
    setColour (Slider::backgroundColourId, bjc::panel2);
    setColour (Slider::thumbColourId, bjc::deckA);
}

void BJLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                      float start, float end, Slider& s)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float ang = start + pos * (end - start);
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);

    g.setColour (bjc::panel2);
    g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));

    Path track;
    track.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, end, true);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    Path val;
    val.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, ang, true);
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (val, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (val, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    Path needle;
    needle.addRoundedRectangle (-1.5f, -(r - 7), 3.0f, r * 0.55f, 1.5f);
    g.setColour (bjc::text);
    g.fillPath (needle, AffineTransform::rotation (ang).translated (c));
}

void BJLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto col = b.getToggleState() ? b.findColour (TextButton::buttonOnColourId) : base;
    if (over) col = col.brighter (0.12f);
    if (down) col = col.brighter (0.25f);
    g.setColour (col);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (b.getToggleState() ? b.findColour (TextButton::buttonOnColourId).withAlpha (1.0f).brighter (0.4f)
                                    : Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

// ============================================================================
//  Platine
// ============================================================================
void TurntableView::tick()
{
    auto d = proc.getDeck (deck);
    if (d == nullptr) { repaint(); return; }
    auto& st = proc.settings (deck);
    const double beatLen = 60.0 / jmax (20.0, st.fileBpm.load()) * d->sampleRate;
    const double beats = (proc.uiDeckPos[deck].load() - st.selStart.load()) / beatLen;
    const double target = beats / 16.0 * MathConstants<double>::twoPi;   // 4 mesures = 1 tour

    double diff = target - angle;
    while (diff >  MathConstants<double>::pi) diff -= MathConstants<double>::twoPi;
    while (diff < -MathConstants<double>::pi) diff += MathConstants<double>::twoPi;
    const bool active = proc.uiPlaying.load() && proc.uiActiveDeck.load() == deck;
    angle += diff * (active ? 0.9 : 0.25);

    glow += ((active ? 1.0f : 0.0f) - glow) * 0.3f;
    repaint();
}

void TurntableView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (4.0f);
    const auto col = bjc::deck (deck);

    // socle
    g.setColour (bjc::panel);
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (col.withAlpha (0.15f + 0.5f * glow));
    g.drawRoundedRectangle (b, 10.0f, 1.5f + glow * 1.5f);

    const float size = jmin (b.getWidth() - 30.0f, b.getHeight() - 16.0f);
    const auto c = Point<float> (b.getX() + 10.0f + size * 0.5f, b.getCentreY());
    const float R = size * 0.5f;

    // plateau
    g.setColour (Colour (0xff2a2f3a));
    g.fillEllipse (Rectangle<float> (R * 2 + 8, R * 2 + 8).withCentre (c));
    for (int i = 0; i < 48; ++i)   // points stroboscopiques
    {
        const float a = (float) (i * MathConstants<double>::twoPi / 48.0 + angle);
        g.setColour (Colours::white.withAlpha (0.18f));
        g.fillEllipse (Rectangle<float> (2.0f, 2.0f).withCentre (c + Point<float> (std::sin (a), -std::cos (a)) * (R + 2.0f)));
    }

    // vinyle
    g.setColour (Colour (0xff0a0a0c));
    g.fillEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c));
    for (float rr = R * 0.38f; rr < R * 0.97f; rr += 2.6f)
    {
        g.setColour (Colours::white.withAlpha (0.035f));
        g.drawEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c), 0.8f);
    }

    // secteur rouge = longueur de la sélection (1 mesure = 1/4 de disque, 4 mesures = tout)
    auto d = proc.getDeck (deck);
    if (d != nullptr)
    {
        auto& st = proc.settings (deck);
        const double beatLen = 60.0 / jmax (20.0, st.fileBpm.load()) * d->sampleRate;
        const double selBeats = (st.selEnd.load() - st.selStart.load()) / beatLen;
        const float frac = (float) jlimit (0.01, 1.0, selBeats / 16.0);
        const float a0 = (float) -angle;
        const float a1 = a0 + frac * MathConstants<float>::twoPi;

        Path sector;
        sector.addPieSegment (Rectangle<float> (R * 1.9f, R * 1.9f).withCentre (c), a0, a1, 0.42f);
        g.setColour (bjc::red.withAlpha (0.18f + 0.25f * glow));
        g.fillPath (sector);
        Path rim;
        rim.addCentredArc (c.x, c.y, R * 0.93f, R * 0.93f, 0, a0, a1, true);
        g.setColour (bjc::red.withAlpha (0.35f + 0.4f * glow));
        g.strokePath (rim, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::butt));
        g.setColour (bjc::red.brighter (0.4f).withAlpha (0.6f + 0.4f * glow));
        g.strokePath (rim, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::butt));

        // repère du départ
        const float ac = a0 + (float) (proc.apvts.getRawParameterValue (deck == 0 ? "cueA" : "cueB")->load() / 64.0) * MathConstants<float>::twoPi;
        g.setColour (bjc::cue);
        g.drawLine (Line<float> (c + Point<float> (std::sin (ac), -std::cos (ac)) * (R * 0.42f),
                                 c + Point<float> (std::sin (ac), -std::cos (ac)) * (R * 0.99f)), 2.0f);
    }

    // macaron
    g.setColour (col.withAlpha (0.85f));
    g.fillEllipse (Rectangle<float> (R * 0.7f, R * 0.7f).withCentre (c));
    {
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation ((float) angle, c.x, c.y));
        g.setColour (Colours::black.withAlpha (0.85f));
        g.setFont (FontOptions (R * 0.22f, Font::bold));
        g.drawText (deck == 0 ? "A" : "B", Rectangle<float> (R * 0.7f, R * 0.4f).withCentre (c.translated (0, -R * 0.12f)), Justification::centred);
        g.setFont (FontOptions (R * 0.08f));
        g.drawText ("BEAT JUGGLING", Rectangle<float> (R * 0.7f, R * 0.2f).withCentre (c.translated (0, R * 0.14f)), Justification::centred);
    }
    g.setColour (Colours::silver);
    g.fillEllipse (Rectangle<float> (6.0f, 6.0f).withCentre (c));

    // bras + cellule fixés en haut (le diamant est à 12h)
    const auto pivot = Point<float> (b.getRight() - 14.0f, b.getY() + 22.0f);
    const auto head  = Point<float> (c.x + 4.0f, c.y - R * 0.84f);
    g.setColour (Colour (0xff9aa3b5));
    g.drawLine (Line<float> (pivot, head), 3.0f);
    g.fillEllipse (Rectangle<float> (14.0f, 14.0f).withCentre (pivot));
    g.setColour (col);
    g.fillRoundedRectangle (Rectangle<float> (12.0f, 7.0f).withCentre (head), 2.0f);

    if (d == nullptr)
    {
        g.setColour (bjc::dim);
        g.setFont (FontOptions (13.0f));
        g.drawText ("Glisse un son ici", Rectangle<float> (R * 2, 20).withCentre (c.translated (0, R * 0.55f)), Justification::centred);
    }
}

// ============================================================================
//  Forme d'onde
// ============================================================================
void WaveformView::setDeck (int d)
{
    deck = d;
    data = nullptr;
    refresh();
}

void WaveformView::refresh()
{
    auto d = proc.getDeck (deck);
    if (d != data)
    {
        data = d;
        zoomAll();
        if (data != nullptr && proc.settings (deck).selEnd.load() > proc.settings (deck).selStart.load())
            zoomToSelection();
    }
    repaint();
}

double WaveformView::beatLen() const
{
    if (data == nullptr) return 1.0;
    return 60.0 / jmax (20.0, proc.settings (deck).fileBpm.load()) * data->sampleRate;
}

double WaveformView::xToSample (float x) const
{
    auto a = mainArea();
    return viewStart + (x - a.getX()) / (double) jmax (1, a.getWidth()) * viewLen;
}

float WaveformView::sampleToX (double s) const
{
    auto a = mainArea();
    return (float) (a.getX() + (s - viewStart) / viewLen * a.getWidth());
}

double WaveformView::snapSample (double s) const
{
    if (! snap || data == nullptr) return s;
    const double step = beatLen() / 4.0;
    const double anc = proc.settings (deck).anchor.load();
    return anc + std::round ((s - anc) / step) * step;
}

void WaveformView::clampView()
{
    if (data == nullptr) { viewStart = 0; viewLen = 1; return; }
    const double total = data->numSamples();
    viewLen = jlimit (32.0, total, viewLen);
    viewStart = jlimit (0.0, jmax (0.0, total - viewLen), viewStart);
}

void WaveformView::zoom (double factor, double centre)
{
    if (data == nullptr) return;
    if (centre < 0.0) centre = viewStart + viewLen * 0.5;
    const double rel = (centre - viewStart) / viewLen;
    viewLen *= factor;
    clampView();
    viewStart = centre - rel * viewLen;
    clampView();
    repaint();
}

void WaveformView::zoomAll()
{
    viewStart = 0;
    viewLen = data != nullptr ? (double) data->numSamples() : 1.0;
    repaint();
}

void WaveformView::zoomToSelection()
{
    if (data == nullptr) return;
    auto& st = proc.settings (deck);
    const double len = st.selEnd.load() - st.selStart.load();
    if (len <= 0) return;
    viewLen = len * 1.25;
    viewStart = st.selStart.load() - len * 0.125;
    clampView();
    repaint();
}

void WaveformView::drawWave (Graphics& g, Rectangle<int> area, double start, double len, Colour c)
{
    if (data == nullptr || area.getWidth() <= 0) return;
    const int w = area.getWidth();
    const float mid = (float) area.getCentreY();
    const float amp = area.getHeight() * 0.48f;
    const double spp = len / w;
    const int total = data->numSamples();
    auto* l = data->buffer.getReadPointer (0);
    auto* r = data->buffer.getReadPointer (1);

    g.setColour (c);
    if (spp < 1.5)
    {
        Path p;
        bool started = false;
        for (int px = 0; px <= w; ++px)
        {
            const double s = start + px * spp;
            const int i = (int) s;
            if (i < 0 || i + 1 >= total) continue;
            const float t = (float) (s - i);
            const float v = 0.5f * ((l[i] + r[i]) * (1 - t) + (l[i + 1] + r[i + 1]) * t);
            const float y = mid - v * amp;
            if (! started) { p.startNewSubPath ((float) (area.getX() + px), y); started = true; }
            else p.lineTo ((float) (area.getX() + px), y);
        }
        g.strokePath (p, PathStrokeType (1.5f));
        return;
    }

    for (int px = 0; px < w; ++px)
    {
        const double s0 = start + px * spp, s1 = s0 + spp;
        if (s1 < 0 || s0 >= total) continue;
        const int i0 = jmax (0, (int) s0), i1 = jmin (total, (int) std::ceil (s1));
        float mn = 0, mx = 0;
        if (spp >= DeckData::kPeakBlock * 2)
        {
            const int b0 = i0 / DeckData::kPeakBlock, b1 = jmin ((int) data->peakMax.size(), i1 / DeckData::kPeakBlock + 1);
            for (int b = b0; b < b1; ++b) { mn = jmin (mn, data->peakMin[(size_t) b]); mx = jmax (mx, data->peakMax[(size_t) b]); }
        }
        else
        {
            for (int i = i0; i < i1; ++i) { const float v = 0.5f * (l[i] + r[i]); mn = jmin (mn, v); mx = jmax (mx, v); }
        }
        g.drawVerticalLine (area.getX() + px, mid - mx * amp, jmax (mid - mn * amp, mid - mx * amp + 1.0f));
    }
}

void WaveformView::paint (Graphics& g)
{
    auto a = mainArea();
    g.setColour (Colour (0xff07090d));
    g.fillRect (getLocalBounds());
    const auto col = bjc::deck (deck);

    if (data == nullptr)
    {
        g.setColour (bjc::dim);
        g.setFont (FontOptions (15.0f));
        g.drawText (String::fromUTF8 ("Charge ou glisse un fichier audio (WAV, AIFF, FLAC...) pour la platine ") + (deck == 0 ? "A" : "B"),
                    a, Justification::centred);
        return;
    }

    auto& st = proc.settings (deck);
    const double selS = st.selStart.load(), selE = st.selEnd.load();
    const double bl = beatLen();
    const double anc = st.anchor.load();
    const double spp = viewLen / jmax (1, a.getWidth());

    // sélection
    {
        const float x0 = jmax ((float) a.getX(), sampleToX (selS));
        const float x1 = jmin ((float) a.getRight(), sampleToX (selE));
        if (x1 > x0)
        {
            g.setColour (bjc::red.withAlpha (0.13f));
            g.fillRect (x0, (float) a.getY(), x1 - x0, (float) a.getHeight());
        }
    }

    // grille
    const double sixteenth = bl / 4.0;
    if (sixteenth / spp > 2.0)
    {
        const bool show16 = sixteenth / spp > 7.0;
        const bool showBeat = bl / spp > 5.0;
        const double step = show16 ? sixteenth : (showBeat ? bl : bl * 4.0);
        long long k0 = (long long) std::floor ((viewStart - anc) / step);
        const long long k1 = (long long) std::ceil ((viewStart + viewLen - anc) / step);
        const int per = show16 ? 1 : (showBeat ? 4 : 16);
        for (long long k = k0; k <= k1; ++k)
        {
            const long long idx16 = k * per;
            const float x = sampleToX (anc + k * step);
            float alpha = 0.06f;
            if (idx16 % 4 == 0)  alpha = 0.14f;
            if (idx16 % 16 == 0) alpha = 0.32f;
            g.setColour (Colours::white.withAlpha (alpha));
            g.drawVerticalLine ((int) x, (float) a.getY(), (float) a.getBottom());
            if (idx16 % 16 == 0 && bl * 4 / spp > 40)
            {
                g.setColour (bjc::dim);
                g.setFont (FontOptions (10.0f));
                g.drawText (String (idx16 / 16 + 1), (int) x + 3, a.getY() + 2, 30, 12, Justification::left);
            }
        }
    }
    else
    {
        // trop dézoomé pour la grille : on montre au moins les mesures si possible
        const double bar = bl * 4.0;
        if (bar / spp > 3.0)
        {
            long long k0 = (long long) std::floor ((viewStart - anc) / bar);
            const long long k1 = (long long) std::ceil ((viewStart + viewLen - anc) / bar);
            g.setColour (Colours::white.withAlpha (0.12f));
            for (long long k = k0; k <= k1; ++k) g.drawVerticalLine ((int) sampleToX (anc + k * bar), (float) a.getY(), (float) a.getBottom());
        }
    }

    // onde
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (a.getCentreY(), (float) a.getX(), (float) a.getRight());
    drawWave (g, a, viewStart, viewLen, col.withAlpha (0.9f));

    // bords de sélection
    g.setColour (bjc::red);
    for (double s : { selS, selE })
    {
        const float x = sampleToX (s);
        if (x >= a.getX() - 1 && x <= a.getRight() + 1)
        {
            g.fillRect (x - 1.0f, (float) a.getY(), 2.0f, (float) a.getHeight());
            Path tri;
            const float dir = (s == selS) ? 1.0f : -1.0f;
            tri.addTriangle (x, (float) a.getY(), x + 9 * dir, (float) a.getY(), x, (float) a.getY() + 9);
            g.fillPath (tri);
        }
    }

    // départ (cue)
    const double cue16 = proc.apvts.getRawParameterValue (deck == 0 ? "cueA" : "cueB")->load();
    const double cuePos = selS + cue16 * sixteenth;
    {
        const float x = sampleToX (cuePos);
        if (x >= a.getX() && x <= a.getRight())
        {
            g.setColour (bjc::cue);
            g.drawVerticalLine ((int) x, (float) a.getY(), (float) a.getBottom());
            g.setFont (FontOptions (10.0f, Font::bold));
            g.drawText (String::fromUTF8 ("DÉPART"), (int) x + 3, a.getBottom() - 14, 60, 12, Justification::left);
        }
    }

    // premier temps (ancre)
    {
        const float x = sampleToX (anc);
        if (x >= a.getX() && x <= a.getRight())
        {
            g.setColour (Colours::limegreen.withAlpha (0.8f));
            Path tri;
            tri.addTriangle (x - 5, (float) a.getBottom(), x + 5, (float) a.getBottom(), x, (float) a.getBottom() - 8);
            g.fillPath (tri);
        }
    }

    // tête de lecture
    if (proc.uiPlaying.load())
    {
        const float x = sampleToX (proc.uiDeckPos[deck].load());
        if (x >= a.getX() && x <= a.getRight())
        {
            g.setColour (Colours::white);
            g.fillRect (x - 1.0f, (float) a.getY(), 2.0f, (float) a.getHeight());
        }
    }

    // vue d'ensemble
    auto o = overviewArea();
    g.setColour (bjc::panel);
    g.fillRect (o);
    drawWave (g, o.reduced (0, 2), 0.0, data->numSamples(), col.withAlpha (0.45f));
    const double total = data->numSamples();
    auto ox = [&] (double s) { return (float) (o.getX() + s / total * o.getWidth()); };
    g.setColour (bjc::red.withAlpha (0.35f));
    g.fillRect (Rectangle<float> (ox (selS), (float) o.getY(), jmax (2.0f, ox (selE) - ox (selS)), (float) o.getHeight()));
    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawRect (Rectangle<float> (ox (viewStart), (float) o.getY(), jmax (3.0f, ox (viewStart + viewLen) - ox (viewStart)), (float) o.getHeight()), 1.5f);
}

void WaveformView::mouseMove (const MouseEvent& e)
{
    if (data == nullptr || overviewArea().contains (e.getPosition())) { setMouseCursor (MouseCursor::NormalCursor); return; }
    auto& st = proc.settings (deck);
    const float xs = sampleToX (st.selStart.load()), xe = sampleToX (st.selEnd.load());
    const bool edge = std::abs (e.position.x - xs) < 6 || std::abs (e.position.x - xe) < 6;
    setMouseCursor (edge ? MouseCursor::LeftRightResizeCursor : MouseCursor::IBeamCursor);
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    if (data == nullptr) return;
    auto& st = proc.settings (deck);
    dragOrigin = e.position.x;
    dragViewStart = viewStart;

    if (overviewArea().contains (e.getPosition()))
    {
        drag = Drag::overview;
        mouseDrag (e);
        return;
    }
    if (e.mods.isPopupMenu() || e.mods.isMiddleButtonDown()) { drag = Drag::pan; return; }

    const double s = xToSample (e.position.x);
    if (e.mods.isAltDown())
    {
        st.anchor = s;
        drag = Drag::none;
        repaint();
        return;
    }
    if (e.mods.isCtrlDown() || e.mods.isCommandDown())
    {
        // Ctrl + clic : place le départ de la platine
        const double cue = jlimit (0.0, 63.0, std::round ((snapSample (s) - st.selStart.load()) / (beatLen() / 4.0)));
        if (auto* p = proc.apvts.getParameter (deck == 0 ? "cueA" : "cueB"))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) cue));
        drag = Drag::none;
        repaint();
        return;
    }

    const float xs = sampleToX (st.selStart.load()), xe = sampleToX (st.selEnd.load());
    if (std::abs (e.position.x - xs) < 6)      drag = Drag::edgeStart;
    else if (std::abs (e.position.x - xe) < 6) drag = Drag::edgeEnd;
    else
    {
        drag = Drag::newSel;
        dragOrigin = snapSample (s);
        st.selStart = dragOrigin;
        st.selEnd = dragOrigin;
    }
}

void WaveformView::mouseDrag (const MouseEvent& e)
{
    if (data == nullptr) return;
    auto& st = proc.settings (deck);
    const double total = data->numSamples();

    switch (drag)
    {
        case Drag::overview:
        {
            auto o = overviewArea();
            const double centre = (e.position.x - o.getX()) / (double) jmax (1, o.getWidth()) * total;
            viewStart = centre - viewLen * 0.5;
            clampView();
            break;
        }
        case Drag::pan:
            viewStart = dragViewStart - (e.position.x - dragOrigin) * viewLen / jmax (1, mainArea().getWidth());
            clampView();
            break;
        case Drag::newSel:
        {
            const double s = jlimit (0.0, total, snapSample (xToSample (e.position.x)));
            st.selStart = jmin (dragOrigin, s);
            st.selEnd   = jmax (dragOrigin, s);
            break;
        }
        case Drag::edgeStart:
        case Drag::edgeEnd:
        {
            const double s = jlimit (0.0, total, snapSample (xToSample (e.position.x)));
            if (drag == Drag::edgeStart) st.selStart = s; else st.selEnd = s;
            if (st.selStart.load() > st.selEnd.load())
            {
                const double t = st.selStart.load(); st.selStart = st.selEnd.load(); st.selEnd = t;
                drag = (drag == Drag::edgeStart) ? Drag::edgeEnd : Drag::edgeStart;
            }
            break;
        }
        case Drag::none: break;
    }

    // défilement automatique quand on sélectionne près des bords
    if (drag == Drag::newSel || drag == Drag::edgeStart || drag == Drag::edgeEnd)
    {
        auto a = mainArea();
        if (e.position.x > a.getRight() - 10) { viewStart += viewLen * 0.02; clampView(); }
        if (e.position.x < a.getX() + 10)     { viewStart -= viewLen * 0.02; clampView(); }
        if (onSelectionChanged) onSelectionChanged();
    }
    repaint();
}

void WaveformView::mouseUp (const MouseEvent&)
{
    if (data != nullptr && (drag == Drag::newSel || drag == Drag::edgeStart || drag == Drag::edgeEnd))
    {
        auto& st = proc.settings (deck);
        if (st.selEnd.load() - st.selStart.load() < 32.0)   // simple clic : on ne casse pas la sélection
        {
            st.selStart = 0.0;
            st.selEnd = (double) data->numSamples();
        }
        if (onSelectionChanged) onSelectionChanged();
    }
    drag = Drag::none;
    repaint();
}

void WaveformView::mouseDoubleClick (const MouseEvent& e)
{
    if (data == nullptr || overviewArea().contains (e.getPosition())) return;
    auto& st = proc.settings (deck);
    const double unit = e.mods.isShiftDown() ? beatLen() : beatLen() * 4.0;   // Maj = un temps, sinon une mesure
    const double anc = st.anchor.load();
    const double s = xToSample (e.position.x);
    const double k = std::floor ((s - anc) / unit);
    st.selStart = jmax (0.0, anc + k * unit);
    st.selEnd = jmin ((double) data->numSamples(), anc + (k + 1) * unit);
    if (onSelectionChanged) onSelectionChanged();
    repaint();
}

void WaveformView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (data == nullptr) return;
    const bool horizontal = std::abs (w.deltaX) > std::abs (w.deltaY);
    if (e.mods.isShiftDown() || horizontal)
    {
        const float d = horizontal ? w.deltaX : w.deltaY;
        viewStart -= d * viewLen * 0.5;
        clampView();
        repaint();
        return;
    }
    zoom (std::pow (0.5, w.deltaY * 2.0), xToSample (e.position.x));
}

void WaveformView::mouseMagnify (const MouseEvent& e, float scale)
{
    if (scale > 0.0f) zoom (1.0 / scale, xToSample (e.position.x));
}

// ============================================================================
//  Timeline du programme
// ============================================================================
void ProgramView::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff07090d));
    g.fillRoundedRectangle (b, 4.0f);
    auto prog = proc.getProgram();
    if (prog == nullptr || prog->steps.empty() || prog->total <= 0) return;

    const float w = b.getWidth() - 4.0f;
    auto xOf = [&] (double beat) { return b.getX() + 2.0f + (float) (beat / prog->total) * w; };
    const int cur = proc.uiPlaying.load() ? proc.uiStep.load() : -1;

    for (int i = 0; i < (int) prog->steps.size(); ++i)
    {
        const auto& s = prog->steps[(size_t) i];
        auto r = Rectangle<float> (xOf (s.start), b.getY() + 4, jmax (1.0f, xOf (s.start + s.dur) - xOf (s.start) - 1.5f), b.getHeight() - 16);
        auto c = s.silent ? Colour (0xff333844) : bjc::deck (s.deck);
        g.setColour (c.withAlpha (i == cur ? 1.0f : 0.45f));
        g.fillRoundedRectangle (r, 2.0f);
        if (r.getWidth() > 12)
        {
            String lbl = s.silent ? "_" : (s.deck == 0 ? "A" : "B");
            lbl << bj::moveLetter (s.move);
            if (s.roll > 1) lbl << "*" << s.roll;
            if (s.cont) lbl << "c";
            g.setColour (Colours::black.withAlpha (0.85f));
            g.setFont (FontOptions (11.0f, Font::bold));
            g.drawText (lbl, r, Justification::centred);
        }
    }

    // repères de temps et de mesure
    for (int k = 0; k <= (int) std::ceil (prog->total); ++k)
    {
        const float x = xOf (k);
        g.setColour (Colours::white.withAlpha (k % 4 == 0 ? 0.5f : 0.15f));
        g.drawVerticalLine ((int) x, b.getBottom() - (k % 4 == 0 ? 10.0f : 6.0f), b.getBottom());
    }

    if (proc.uiPlaying.load())
    {
        const float x = xOf (proc.uiProgramPos.load());
        g.setColour (Colours::white);
        g.fillRect (x - 1.0f, b.getY(), 2.0f, b.getHeight());
    }

    g.setColour (bjc::dim);
    g.setFont (FontOptions (10.0f));
    g.drawText (String (prog->total, 2) + " temps", b.reduced (4, 0), Justification::bottomRight);
}

// ============================================================================
//  Éditeur
// ============================================================================
static void setupKnob (Slider& s, Label& l, const String& name, Colour c, Component& parent)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::TextBoxBelow, false, 64, 16);
    s.setColour (Slider::rotarySliderFillColourId, c);
    parent.addAndMakeVisible (s);
    l.setText (name, dontSendNotification);
    l.setJustificationType (Justification::centred);
    l.setFont (FontOptions (12.0f));
    l.setColour (Label::textColourId, bjc::dim);
    parent.addAndMakeVisible (l);
}

BeatJugglingEditor::BeatJugglingEditor (BeatJugglingProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto& ap = proc.apvts;

    addAndMakeVisible (tableA);
    addAndMakeVisible (tableB);
    tableA.onClick = [this] { setEditDeck (0); };
    tableB.onClick = [this] { setEditDeck (1); };

    setupKnob (cueA,  cueALbl,  String::fromUTF8 ("Départ A"), bjc::cue,   *this);
    setupKnob (gainA, gainALbl, "Volume A",                    bjc::deckA, *this);
    setupKnob (cueB,  cueBLbl,  String::fromUTF8 ("Départ B"), bjc::cue,   *this);
    setupKnob (gainB, gainBLbl, "Volume B",                    bjc::deckB, *this);
    setupKnob (volume, volumeLbl, "Volume",                    Colours::white, *this);
    setupKnob (xfade,  xfadeLbl,  "Crossfade",                 bjc::red,   *this);
    setupKnob (tempo,  tempoLbl,  "Tempo interne",             bjc::dim,   *this);
    cueAAtt   = std::make_unique<SliderAtt> (ap, "cueA", cueA);
    cueBAtt   = std::make_unique<SliderAtt> (ap, "cueB", cueB);
    gainAAtt  = std::make_unique<SliderAtt> (ap, "gainA", gainA);
    gainBAtt  = std::make_unique<SliderAtt> (ap, "gainB", gainB);
    volumeAtt = std::make_unique<SliderAtt> (ap, "volume", volume);
    xfadeAtt  = std::make_unique<SliderAtt> (ap, "xfade", xfade);
    tempoAtt  = std::make_unique<SliderAtt> (ap, "bpm", tempo);
    cueA.setTooltip (String::fromUTF8 ("Point de départ de la platine A dans la sélection, en 1/16 (4 = 2e temps)"));
    cueB.setTooltip (String::fromUTF8 ("Point de départ de la platine B dans la sélection, en 1/16 (4 = 2e temps)"));
    tempo.setTooltip (String::fromUTF8 ("Utilisé quand le plugin tourne sans Ableton (bouton PLAY)"));

    addAndMakeVisible (syncBtn);
    syncAtt = std::make_unique<ButtonAtt> (ap, "sync", syncBtn);
    syncBtn.setTooltip (String::fromUTF8 ("Activé : la boucle est calée sur le tempo d'Ableton (la hauteur suit, comme un vinyle)"));

    addAndMakeVisible (playBtn);
    playBtn.setClickingTogglesState (true);
    playBtn.setColour (TextButton::buttonOnColourId, Colours::limegreen.withAlpha (0.45f));
    playBtn.setTooltip (String::fromUTF8 ("Lecture sans le transport d'Ableton (pour tester). Dans Ableton, lance simplement la lecture."));
    playBtn.onClick = [this] { proc.internalPlay = playBtn.getToggleState(); };
    playBtn.setToggleState (proc.internalPlay.load(), dontSendNotification);

    for (auto* b : { &editA, &editB })
    {
        addAndMakeVisible (*b);
        b->setRadioGroupId (101);
        b->setClickingTogglesState (true);
    }
    editA.setColour (TextButton::buttonOnColourId, bjc::deckA.withAlpha (0.45f));
    editB.setColour (TextButton::buttonOnColourId, bjc::deckB.withAlpha (0.45f));
    editA.onClick = [this] { if (editA.getToggleState()) setEditDeck (0); };
    editB.onClick = [this] { if (editB.getToggleState()) setEditDeck (1); };

    addAndMakeVisible (loadBtn);
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> ("Choisir un son", File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this] (const FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f.existsAsFile()) loadInto (editDeck, f);
                              });
    };
    addAndMakeVisible (copyBtn);
    copyBtn.setTooltip (String::fromUTF8 ("Met le même son, la même sélection et le même BPM sur l'autre platine"));
    copyBtn.onClick = [this] { proc.copyDeck (editDeck, 1 - editDeck); updateDeckInfo(); };

    fileLbl.setFont (FontOptions (12.0f));
    fileLbl.setColour (Label::textColourId, bjc::dim);
    addAndMakeVisible (fileLbl);

    bpmLbl.setFont (FontOptions (12.0f));
    addAndMakeVisible (bpmLbl);
    bpmSlider.setSliderStyle (Slider::LinearBar);
    bpmSlider.setRange (40.0, 250.0, 0.01);
    bpmSlider.setColour (Slider::trackColourId, bjc::panel2.brighter (0.15f));
    bpmSlider.setTooltip (String::fromUTF8 ("Tempo d'origine du son : sert à dessiner la grille et à caler la boucle sur Ableton"));
    bpmSlider.onValueChange = [this] { proc.settings (editDeck).fileBpm = bpmSlider.getValue(); };
    addAndMakeVisible (bpmSlider);
    for (auto* b : { &halfBtn, &doubleBtn }) addAndMakeVisible (*b);
    halfBtn.onClick   = [this] { bpmSlider.setValue (bpmSlider.getValue() * 0.5); };
    doubleBtn.onClick = [this] { bpmSlider.setValue (bpmSlider.getValue() * 2.0); };

    addAndMakeVisible (snapBtn);
    snapBtn.setToggleState (true, dontSendNotification);
    snapBtn.onClick = [this] { wave.snap = snapBtn.getToggleState(); };

    calLbl.setFont (FontOptions (12.0f));
    addAndMakeVisible (calLbl);
    calCombo.addItem ("1 temps", 1);
    calCombo.addItem ("2 temps", 2);
    calCombo.addItem ("1 mesure", 4);
    calCombo.addItem ("2 mesures", 8);
    calCombo.addItem ("4 mesures", 16);
    calCombo.setSelectedId (4, dontSendNotification);
    addAndMakeVisible (calCombo);
    addAndMakeVisible (calBtn);
    calBtn.setTooltip (String::fromUTF8 ("Calcule le BPM du son à partir de ta sélection, et place le 1er temps au début de la sélection"));
    calBtn.onClick = [this]
    {
        auto d = proc.getDeck (editDeck);
        if (d == nullptr) return;
        auto& st = proc.settings (editDeck);
        const double len = (st.selEnd.load() - st.selStart.load()) / d->sampleRate;
        if (len <= 0.01) return;
        double bpm = calCombo.getSelectedId() * 60.0 / len;
        while (bpm > 250.0) bpm *= 0.5;
        while (bpm < 40.0)  bpm *= 2.0;
        st.fileBpm = bpm;
        st.anchor = st.selStart.load();
        updateDeckInfo();
    };

    addAndMakeVisible (wave);
    wave.onSelectionChanged = [this] { updateDeckInfo(); };
    for (auto* b : { &zoomIn, &zoomOut, &zoomAllBtn, &zoomSelBtn }) addAndMakeVisible (*b);
    zoomIn.onClick     = [this] { wave.zoom (0.5, -1.0); };
    zoomOut.onClick    = [this] { wave.zoom (2.0, -1.0); };
    zoomAllBtn.onClick = [this] { wave.zoomAll(); };
    zoomSelBtn.onClick = [this] { wave.zoomToSelection(); };
    waveInfo.setFont (FontOptions (12.0f));
    waveInfo.setColour (Label::textColourId, bjc::dim);
    addAndMakeVisible (waveInfo);

    // programme
    presets.addItem (String::fromUTF8 ("Exemples de passe-passe..."), 1);
    presets.addItem (String::fromUTF8 ("Classique : 1 temps / 1 temps"), 2);
    presets.addItem (String::fromUTF8 ("Accélération : 1 → 1/2 → 1/4"), 3);
    presets.addItem (String::fromUTF8 ("Trois quarts de temps"), 4);
    presets.addItem (String::fromUTF8 ("Ton phrasé : 1 x3, 1/2 x5, 3/2 x3"), 5);
    presets.addItem (String::fromUTF8 ("Contretemps : A sur la grosse caisse, B sur la caisse claire"), 6);
    presets.addItem (String::fromUTF8 ("3 + 2"), 7);
    presets.addItem (String::fromUTF8 ("Mouvements : reverse, stop, backspin"), 8);
    presets.addItem (String::fromUTF8 ("Scratch et roll"), 9);
    presets.setSelectedId (1, dontSendNotification);
    presets.onChange = [this]
    {
        static const char* texts[] = {
            "", "",
            "1 x8",
            "1 x4 | 1/2 x4 | 1/4 x4 1/2 1/2",
            "3/4 x8 | 1 x2",
            "1 x3 | 1/2 x5 | 3/2 x3",
            "A1 B1@4 A1 B1@4 | A1/2 B1/2@4 A1/2 B1/2@4 | A1 B1@4",
            "3+2 x3 1",
            "1 x2 1r 1 | 1/2 x4 1s 1k",
            "1 1*4 1 1z | 1/2 x2 1*8 2s" };
        const int id = presets.getSelectedId();
        if (id >= 2 && id <= 9)
        {
            progEditor.setText (texts[id], false);
            programEdited();
        }
        presets.setSelectedId (1, dontSendNotification);
    };
    addAndMakeVisible (presets);

    const char* moves[][2] = {
        { "Reverse", "r" }, { "Stop", "s" }, { "Démarrage", "w" }, { "Backspin", "k" },
        { "Scratch", "z" }, { "Roll *4", "*4" }, { "Continue", "c" }, { "Silence", "_1" } };
    for (auto& m : moves)
    {
        auto* b = moveBtns.add (new TextButton (String::fromUTF8 (m[0])));
        const String code (m[1]);
        b->setTooltip (String::fromUTF8 ("Ajoute « ") + code + String::fromUTF8 (" » au curseur du programme"));
        b->onClick = [this, code] { insertMove (code); };
        addAndMakeVisible (b);
    }

    progEditor.setMultiLine (false);
    progEditor.setFont (FontOptions (Font::getDefaultMonospacedFontName(), 18.0f, Font::plain));
    progEditor.setText (proc.getProgramText(), false);
    progEditor.onTextChange = [this] { programEdited(); };
    addAndMakeVisible (progEditor);

    progError.setFont (FontOptions (12.0f));
    progError.setColour (Label::textColourId, bjc::red.brighter (0.3f));
    addAndMakeVisible (progError);

    helpLbl.setFont (FontOptions (12.0f));
    helpLbl.setColour (Label::textColourId, bjc::dim);
    helpLbl.setJustificationType (Justification::topLeft);
    helpLbl.setText (String::fromUTF8 (
        "Durées en temps : 1   1/2   3/4   1.5     Répéter : 1 x3     Groupe : 3+2     Platine forcée : A1  B1/2     Silence : _1     Départ forcé (en 1/16) : B1@4\n"
        "Mouvements après la durée : r reverse · s stop platine · w démarrage · k backspin · z scratch · *4 roll · c continue (ne revient pas au départ)      Ex : 1 x3 | 1/2 x5 | 3/2 x3 1s\n"
        "Onde : molette = zoom · Maj+molette = défiler · glisser = sélection · double-clic = mesure (Maj : temps) · Ctrl+clic = départ · Alt+clic = 1er temps · clic droit glisser = déplacer"),
        dontSendNotification);
    addAndMakeVisible (helpLbl);

    addAndMakeVisible (progView);

    setEditDeck (0);
    programEdited();
    setSize (1040, 760);
    startTimerHz (40);
}

BeatJugglingEditor::~BeatJugglingEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BeatJugglingEditor::setEditDeck (int d)
{
    editDeck = d;
    (d == 0 ? editA : editB).setToggleState (true, dontSendNotification);
    copyBtn.setButtonText (d == 0 ? "Copier vers B" : "Copier vers A");
    wave.setDeck (d);
    updateDeckInfo();
    repaint();
}

void BeatJugglingEditor::loadInto (int d, const File& f)
{
    if (! proc.loadFile (d, f, true))
    {
        fileLbl.setText (String::fromUTF8 ("Impossible de lire ce fichier : ") + f.getFileName(), dontSendNotification);
        return;
    }
    setEditDeck (d);
}

void BeatJugglingEditor::updateDeckInfo()
{
    auto d = proc.getDeck (editDeck);
    auto& st = proc.settings (editDeck);
    bpmSlider.setValue (st.fileBpm.load(), dontSendNotification);

    if (d == nullptr)
    {
        fileLbl.setText (String::fromUTF8 ("Aucun son chargé"), dontSendNotification);
        waveInfo.setText ({}, dontSendNotification);
        return;
    }
    fileLbl.setText (d->file.getFileName() + "   (" + String (d->numSamples() / d->sampleRate, 2) + " s)", dontSendNotification);

    const double beatLen = 60.0 / jmax (20.0, st.fileBpm.load()) * d->sampleRate;
    const double beats = (st.selEnd.load() - st.selStart.load()) / beatLen;
    String info = String::fromUTF8 ("Sélection ") + String (editDeck == 0 ? "A" : "B") + " : "
                + String (beats, 2) + " temps";
    const double bars = beats / 4.0;
    if (std::abs (bars - std::round (bars)) < 0.02 && bars >= 1.0)
        info << "  =  " << (int) std::round (bars) << (std::round (bars) > 1 ? " mesures" : " mesure");
    info << bj::U ("   ·   ") << String ((st.selEnd.load() - st.selStart.load()) / d->sampleRate, 3) << " s";
    waveInfo.setText (info, dontSendNotification);
}

void BeatJugglingEditor::programEdited()
{
    proc.setProgramText (progEditor.getText());
    auto p = proc.getProgram();
    if (p != nullptr && p->error.isNotEmpty())
    {
        progError.setText (String::fromUTF8 ("Erreur : ") + p->error + String::fromUTF8 ("  (le dernier programme valide continue de jouer)"), dontSendNotification);
        progEditor.setColour (TextEditor::outlineColourId, bjc::red);
    }
    else
    {
        progError.setText (p != nullptr ? (String (p->steps.size()) + bj::U (" segments  ·  ") + String (p->total, 2) + " temps") : String(), dontSendNotification);
        progError.setColour (Label::textColourId, bjc::dim);
        progEditor.setColour (TextEditor::outlineColourId, bjc::panel2.brighter (0.3f));
        progError.setColour (Label::textColourId, p != nullptr && p->error.isNotEmpty() ? bjc::red : bjc::dim);
    }
    if (p != nullptr && p->error.isNotEmpty()) progError.setColour (Label::textColourId, bjc::red.brighter (0.3f));
    progEditor.repaint();
    progView.repaint();
}

void BeatJugglingEditor::insertMove (const String& s)
{
    progEditor.grabKeyboardFocus();
    String ins = s;
    if (s.startsWithChar ('_'))
    {
        const auto before = progEditor.getText().substring (0, progEditor.getCaretPosition());
        if (before.isNotEmpty() && ! before.endsWithChar (' ')) ins = " " + ins;
        ins += " ";
    }
    progEditor.insertTextAtCaret (ins);
    programEdited();
}

bool BeatJugglingEditor::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) return true;
    return false;
}

void BeatJugglingEditor::filesDropped (const StringArray& files, int x, int y)
{
    int d = editDeck;
    if (tableA.getBounds().contains (x, y)) d = 0;
    else if (tableB.getBounds().contains (x, y)) d = 1;
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) { loadInto (d, File (f)); break; }
}

void BeatJugglingEditor::timerCallback()
{
    tableA.tick();
    tableB.tick();
    wave.refresh();
    progView.repaint();
    if (playBtn.getToggleState() != proc.internalPlay.load())
        playBtn.setToggleState (proc.internalPlay.load(), dontSendNotification);

    for (int k = 0; k < 2; ++k)
    {
        auto d = proc.getDeck (k);
        if (d != shownData[k]) { shownData[k] = d; if (k == editDeck) updateDeckInfo(); }
    }
    repaint (getLocalBounds().withHeight (44));
    repaint (296, 182, 450, 30);
}

void BeatJugglingEditor::paint (Graphics& g)
{
    g.fillAll (bjc::bg);

    // titre
    auto top = getLocalBounds().removeFromTop (44);
    g.setGradientFill (ColourGradient (bjc::deckA.withAlpha (0.18f), 0, 0, bjc::deckB.withAlpha (0.18f), (float) getWidth(), 0, false));
    g.fillRect (top);
    g.setFont (FontOptions (24.0f, Font::bold));
    g.setColour (Colours::white);
    g.drawText ("BEAT JUGGLING", top.reduced (16, 0), Justification::centredLeft);
    g.setFont (FontOptions (12.0f));
    g.setColour (bjc::dim);
    g.drawText ("passe-passe automatique", top.reduced (16, 0).withTrimmedLeft (215), Justification::centredLeft);

    // statut
    const bool playing = proc.uiPlaying.load();
    String status = playing ? String (bj::U ("LECTURE  ·  ")) : String (bj::U ("STOP  ·  "));
    status << String (proc.uiHostBpm.load(), 1) << " BPM";
    g.setColour (playing ? Colours::limegreen : bjc::dim);
    g.drawText (status, top.reduced (16, 0), Justification::centredRight);

    // panneau central
    auto centre = Rectangle<int> (282, 50, 476, 268);
    g.setColour (bjc::panel);
    g.fillRoundedRectangle (centre.toFloat(), 8.0f);

    // crossfader décoratif
    auto xf = Rectangle<float> (300.0f, 186.0f, 440.0f, 22.0f);
    g.setColour (Colour (0xff07090d));
    g.fillRoundedRectangle (xf, 11.0f);
    const int act = proc.uiActiveDeck.load();
    const float pos = ! playing ? 0.5f : (act == 0 ? 0.08f : (act == 1 ? 0.92f : 0.5f));
    static float smooth = 0.5f;
    smooth += (pos - smooth) * 0.6f;
    const float cx = xf.getX() + 20.0f + smooth * (xf.getWidth() - 40.0f);
    g.setColour (bjc::deckA.withAlpha (0.5f));
    g.drawText ("A", xf.withWidth (20.0f), Justification::centred);
    g.setColour (bjc::deckB.withAlpha (0.5f));
    g.drawText ("B", xf.withTrimmedLeft (xf.getWidth() - 20.0f), Justification::centred);
    g.setColour (playing && act >= 0 ? bjc::deck (act) : bjc::text);
    g.fillRoundedRectangle (Rectangle<float> (28.0f, 18.0f).withCentre ({ cx, xf.getCentreY() }), 3.0f);

    // fond onde + programme
    g.setColour (bjc::panel);
    g.fillRoundedRectangle (Rectangle<float> (8.0f, 324.0f, (float) getWidth() - 16.0f, 210.0f), 8.0f);
    g.fillRoundedRectangle (Rectangle<float> (8.0f, 540.0f, (float) getWidth() - 16.0f, 212.0f), 8.0f);
    g.setColour (bjc::deck (editDeck));
    g.setFont (FontOptions (13.0f, Font::bold));
    g.drawText (String::fromUTF8 ("ONDE — PLATINE ") + (editDeck == 0 ? "A" : "B"), 18, 328, 200, 22, Justification::centredLeft);
    g.setColour (bjc::text);
    g.drawText ("PROGRAMME DE PASSE-PASSE", 18, 544, 260, 22, Justification::centredLeft);
}

void BeatJugglingEditor::resized()
{
    // platines
    tableA.setBounds (8, 50, 268, 205);
    tableB.setBounds (getWidth() - 276, 50, 268, 205);
    const int kw = 70, kh = 62;
    cueA.setBounds (40, 256, kw, kh - 14);   cueALbl.setBounds (30, 300, kw + 20, 16);
    gainA.setBounds (150, 256, kw, kh - 14); gainALbl.setBounds (140, 300, kw + 20, 16);
    cueB.setBounds (getWidth() - 236, 256, kw, kh - 14);  cueBLbl.setBounds (getWidth() - 246, 300, kw + 20, 16);
    gainB.setBounds (getWidth() - 126, 256, kw, kh - 14); gainBLbl.setBounds (getWidth() - 136, 300, kw + 20, 16);

    // centre
    const int cx = 292;
    editA.setBounds (cx, 58, 90, 26);
    editB.setBounds (cx + 94, 58, 90, 26);
    loadBtn.setBounds (cx + 192, 58, 140, 26);
    copyBtn.setBounds (cx + 336, 58, 120, 26);
    fileLbl.setBounds (cx, 88, 456, 20);
    bpmLbl.setBounds (cx, 114, 80, 24);
    bpmSlider.setBounds (cx + 80, 114, 130, 24);
    halfBtn.setBounds (cx + 214, 114, 34, 24);
    doubleBtn.setBounds (cx + 250, 114, 34, 24);
    snapBtn.setBounds (cx + 300, 114, 150, 24);
    calLbl.setBounds (cx, 146, 110, 24);
    calCombo.setBounds (cx + 110, 146, 120, 24);
    calBtn.setBounds (cx + 236, 146, 130, 24);

    volume.setBounds (cx + 4, 216, 70, 70);    volumeLbl.setBounds (cx - 6, 288, 90, 16);
    xfade.setBounds (cx + 94, 216, 70, 70);    xfadeLbl.setBounds (cx + 84, 288, 90, 16);
    tempo.setBounds (cx + 184, 216, 70, 70);   tempoLbl.setBounds (cx + 174, 288, 90, 16);
    syncBtn.setBounds (cx + 276, 228, 120, 24);
    playBtn.setBounds (cx + 276, 260, 170, 34);

    // onde
    zoomSelBtn.setBounds (getWidth() - 100, 328, 82, 22);
    zoomAllBtn.setBounds (getWidth() - 154, 328, 50, 22);
    zoomOut.setBounds (getWidth() - 184, 328, 26, 22);
    zoomIn.setBounds (getWidth() - 214, 328, 26, 22);
    waveInfo.setBounds (220, 328, getWidth() - 450, 22);
    wave.setBounds (16, 354, getWidth() - 32, 172);

    // programme
    presets.setBounds (280, 544, 300, 22);
    int bx = 590;
    for (auto* b : moveBtns) { b->setBounds (bx, 544, 50, 22); bx += 52; }
    progEditor.setBounds (16, 572, getWidth() - 32, 34);
    progError.setBounds (16, 608, getWidth() - 32, 18);
    progView.setBounds (16, 628, getWidth() - 32, 50);
    helpLbl.setBounds (16, 682, getWidth() - 32, 66);
}
