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
//  Utilitaires
// ============================================================================
static String U8 (const char* s) { return String::fromUTF8 (s); }

static String friendlyDur (int len16)
{
    if (len16 % 16 == 0) return String (len16 / 16) + " mes";
    if (len16 > 16 && len16 % 8 == 0) return String (len16 / 16.0, 1).replaceCharacter ('.', ',') + " mes";
    if (len16 % 4 == 0)  return String (len16 / 4) + " t";
    return bj::fmtDur16 (len16);
}

static void makeSeg (OwnedArray<TextButton>& arr, const StringArray& names, int group, Colour col,
                     Component& parent, std::function<void (int)> onPick)
{
    for (int i = 0; i < names.size(); ++i)
    {
        auto* b = arr.add (new TextButton (names[i]));
        b->setRadioGroupId (group);
        b->setClickingTogglesState (true);
        b->setColour (TextButton::buttonOnColourId, col.withAlpha (0.55f));
        b->onClick = [b, i, onPick] { if (b->getToggleState()) onPick (i); };
        parent.addAndMakeVisible (b);
    }
}

static void setSeg (OwnedArray<TextButton>& arr, int index)
{
    for (int i = 0; i < arr.size(); ++i) arr[i]->setToggleState (i == index, dontSendNotification);
}

static void setupKnob (Slider& s, Label& l, const String& name, Colour c, Component& parent, int textW = 64)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::TextBoxBelow, false, textW, 18);
    s.setColour (Slider::rotarySliderFillColourId, c);
    parent.addAndMakeVisible (s);
    l.setText (name, dontSendNotification);
    l.setJustificationType (Justification::centred);
    l.setFont (FontOptions (12.0f));
    l.setColour (Label::textColourId, bjc::dim);
    parent.addAndMakeVisible (l);
}

static void smallLabel (Label& l, const String& t, Component& parent)
{
    l.setText (t, dontSendNotification);
    l.setFont (FontOptions (11.0f, Font::bold));
    l.setColour (Label::textColourId, bjc::dim);
    parent.addAndMakeVisible (l);
}

// ============================================================================
//  Platine
// ============================================================================
void TurntableView::tick()
{
    auto d = proc.getDeck (deck);
    if (d == nullptr) { repaint(); return; }
    const double beatLen = 60.0 / jmax (20.0, proc.settings (deck).fileBpm.load()) * d->sampleRate;
    double zs = 0, zl = 0;
    if (getZone) getZone (zs, zl);
    const double beats = (proc.uiDeckPos[deck].load() - zs) / beatLen;
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
    auto b = getLocalBounds().toFloat();
    const auto col = bjc::deck (deck);
    const float size = jmin (b.getWidth() - 40.0f, b.getHeight() - 6.0f);
    const auto c = Point<float> (b.getCentreX() - 10.0f, b.getCentreY());
    const float R = size * 0.5f;

    g.setColour (Colour (0xff2a2f3a));
    g.fillEllipse (Rectangle<float> (R * 2 + 10, R * 2 + 10).withCentre (c));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) (i * MathConstants<double>::twoPi / 48.0 + angle);
        g.setColour (Colours::white.withAlpha (0.2f));
        g.fillEllipse (Rectangle<float> (2.0f, 2.0f).withCentre (c + Point<float> (std::sin (a), -std::cos (a)) * (R + 3.0f)));
    }
    g.setColour (Colour (0xff0a0a0c));
    g.fillEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c));
    for (float rr = R * 0.38f; rr < R * 0.97f; rr += 2.6f)
    {
        g.setColour (Colours::white.withAlpha (0.035f));
        g.drawEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c), 0.8f);
    }

    auto d = proc.getDeck (deck);
    double zs = 0, zl = 0;
    if (d != nullptr && getZone && getZone (zs, zl) && zl > 0)
    {
        const double beatLen = 60.0 / jmax (20.0, proc.settings (deck).fileBpm.load()) * d->sampleRate;
        const float frac = (float) jlimit (0.01, 1.0, (zl / beatLen) / 16.0);   // 1 mesure = 1/4 de disque
        const float a0 = (float) -angle;
        const float a1 = a0 + frac * MathConstants<float>::twoPi;
        Path sector;
        sector.addPieSegment (Rectangle<float> (R * 1.9f, R * 1.9f).withCentre (c), a0, a1, 0.42f);
        g.setColour (bjc::red.withAlpha (0.16f + 0.25f * glow));
        g.fillPath (sector);
        Path rim;
        rim.addCentredArc (c.x, c.y, R * 0.93f, R * 0.93f, 0, a0, a1, true);
        g.setColour (bjc::red.withAlpha (0.35f + 0.45f * glow));
        g.strokePath (rim, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::butt));
        g.setColour (bjc::red.brighter (0.4f).withAlpha (0.6f + 0.4f * glow));
        g.strokePath (rim, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::butt));
    }

    g.setColour (col.withAlpha (0.9f));
    g.fillEllipse (Rectangle<float> (R * 0.7f, R * 0.7f).withCentre (c));
    {
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation ((float) angle, c.x, c.y));
        g.setColour (Colours::black.withAlpha (0.85f));
        g.setFont (FontOptions (R * 0.24f, Font::bold));
        g.drawText (deck == 0 ? "G" : "D", Rectangle<float> (R * 0.7f, R * 0.4f).withCentre (c.translated (0, -R * 0.1f)), Justification::centred);
        g.setFont (FontOptions (R * 0.075f));
        g.drawText ("ANKS LABS", Rectangle<float> (R * 0.7f, R * 0.2f).withCentre (c.translated (0, R * 0.15f)), Justification::centred);
    }
    g.setColour (Colours::silver);
    g.fillEllipse (Rectangle<float> (6.0f, 6.0f).withCentre (c));

    const auto pivot = Point<float> (b.getRight() - 14.0f, b.getY() + 14.0f);
    const auto head  = Point<float> (c.x + 4.0f, c.y - R * 0.84f);
    g.setColour (Colour (0xff9aa3b5));
    g.drawLine (Line<float> (pivot, head), 3.0f);
    g.fillEllipse (Rectangle<float> (14.0f, 14.0f).withCentre (pivot));
    g.setColour (col);
    g.fillRoundedRectangle (Rectangle<float> (12.0f, 7.0f).withCentre (head), 2.0f);

    if (d == nullptr)
    {
        g.setColour (bjc::text);
        g.setFont (FontOptions (12.0f));
        g.drawText ("Glisse un son ici", Rectangle<float> (R * 2, 18).withCentre (c.translated (0, R * 0.58f)), Justification::centred);
    }
}

// ============================================================================
//  Bouton de pattern
// ============================================================================
void PatternButton::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const auto& p = proc.getPattern (index);
    const bool playing = proc.uiPlaying.load() && proc.playingPattern.load() == index;
    const bool current = proc.playingPattern.load() == index;
    const bool queued = proc.queuedPattern.load() == index;

    Colour border = bjc::line, fill = bjc::panel2, subCol = bjc::dim;
    String sub = String (p.bars) + " mes";
    if (p.blocks.empty()) sub << U8 (" · vide");
    if (playing)      { border = bjc::green; fill = bjc::green.withAlpha (0.16f); subCol = bjc::green; sub << U8 (" · en cours"); }
    else if (current) { border = bjc::green.withAlpha (0.6f); subCol = bjc::green; sub << U8 (" · prêt"); }
    if (queued)       { border = bjc::cue; fill = bjc::cue.withAlpha (0.12f); subCol = bjc::cue; sub = String (p.bars) + U8 (" mes · enchaîne"); }

    g.setColour (fill);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (border);
    g.drawRoundedRectangle (r, 6.0f, 2.0f);
    if (editing)
    {
        g.setColour (Colours::white);
        g.fillRoundedRectangle (Rectangle<float> (r.getX() + 8, r.getBottom() - 5, r.getWidth() - 16, 3), 1.5f);
    }
    g.setColour (bjc::text);
    g.setFont (FontOptions (14.0f, Font::bold));
    g.drawText ("Pattern " + String (index + 1), Rectangle<float> (r.getX() + 10, r.getY() + 5, r.getWidth() - 16, 20), Justification::centredLeft);
    g.setColour (subCol);
    g.setFont (FontOptions (11.5f));
    g.drawText (sub, Rectangle<float> (r.getX() + 10, r.getY() + 25, r.getWidth() - 16, 15), Justification::centredLeft);
}

// ============================================================================
//  Vinyle (forme d'onde)
// ============================================================================
WaveformView::WaveformView (BeatJugglingEditor& e) : ed (e), proc (e.proc) {}

void WaveformView::setDeck (int d)
{
    if (d == deck && data != nullptr) { repaint(); return; }
    deck = d;
    data = nullptr;
    measureStart = measureEnd = -1;
    refresh();
}

void WaveformView::refresh()
{
    auto d = proc.getDeck (deck);
    if (d != data)
    {
        data = d;
        zoomAll();
    }
    repaint();
}

double WaveformView::sixteenth() const
{
    if (data == nullptr) return 1.0;
    return 60.0 / jmax (20.0, proc.settings (deck).fileBpm.load()) * data->sampleRate / 4.0;
}

double WaveformView::anchor() const { return proc.settings (deck).anchor.load(); }

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

void WaveformView::zoomToBlock()
{
    auto* b = ed.selected();
    if (data == nullptr || b == nullptr || ed.resolvedDeck (ed.selBlock) != deck) return;
    const double six = sixteenth();
    const double s0 = anchor() + b->start16 * six, len = b->len16 * six;
    viewLen = len * 1.3;
    viewStart = s0 - len * 0.15;
    clampView();
    repaint();
}

int WaveformView::blockAt (float x) const
{
    const auto& p = ed.pattern();
    const double six = sixteenth(), anc = anchor();
    const double s = xToSample (x);
    // le bloc sélectionné a la priorité
    auto inside = [&] (int i)
    {
        const auto& b = p.blocks[(size_t) i];
        if (b.type != bj::BlockType::play || b.deck != deck) return false;
        const double a = anc + b.start16 * six;
        return s >= a && s <= a + b.len16 * six;
    };
    if (ed.selBlock >= 0 && ed.selBlock < (int) p.blocks.size() && inside (ed.selBlock)) return ed.selBlock;
    for (int i = 0; i < (int) p.blocks.size(); ++i) if (inside (i)) return i;
    return -1;
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
            for (int i = i0; i < i1; ++i) { const float v = 0.5f * (l[i] + r[i]); mn = jmin (mn, v); mx = jmax (mx, v); }
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
        g.setColour (bjc::text);
        g.setFont (FontOptions (15.0f));
        g.drawText (U8 ("Charge ou glisse un son (WAV, AIFF, FLAC, MP3) sur la platine ") + (deck == 0 ? "gauche" : "droite"),
                    a, Justification::centred);
        return;
    }

    const double six = sixteenth(), anc = anchor();
    const double spp = viewLen / jmax (1, a.getWidth());
    const auto& p = ed.pattern();

    // zones des blocs de cette platine
    for (int i = 0; i < (int) p.blocks.size(); ++i)
    {
        const auto& b = p.blocks[(size_t) i];
        if (b.deck != deck) continue;
        if (b.type == bj::BlockType::move && b.fromPrev) continue;
        const bool sel = (i == ed.selBlock);
        const float x0 = sampleToX (anc + b.start16 * six);
        if (b.type == bj::BlockType::move)
        {
            g.setColour (sel ? bjc::red : col.withAlpha (0.6f));
            g.fillRect (x0 - 1.0f, (float) a.getY(), 2.0f, (float) a.getHeight());
            continue;
        }
        const float x1 = sampleToX (anc + (b.start16 + b.len16) * six);
        if (x1 < a.getX() || x0 > a.getRight()) continue;
        const auto zr = Rectangle<float> (x0, (float) a.getY(), jmax (1.0f, x1 - x0), (float) a.getHeight());
        if (sel)
        {
            g.setColour (bjc::red.withAlpha (0.22f));
            g.fillRect (zr);
        }
        else
        {
            g.setColour (col.withAlpha (0.10f));
            g.fillRect (zr);
            g.setColour (col.withAlpha (0.55f));
            g.fillRect (zr.withTop (zr.getBottom() - 3));
        }
    }

    // grille
    if (six / spp > 2.0)
    {
        const bool show16 = six / spp > 7.0;
        const bool showBeat = six * 4 / spp > 5.0;
        const double step = show16 ? six : (showBeat ? six * 4 : six * 16);
        const int per = show16 ? 1 : (showBeat ? 4 : 16);
        long long k0 = (long long) std::floor ((viewStart - anc) / step);
        const long long k1 = (long long) std::ceil ((viewStart + viewLen - anc) / step);
        for (long long k = k0; k <= k1; ++k)
        {
            const long long idx16 = k * per;
            const float x = sampleToX (anc + k * step);
            float alpha = 0.06f;
            if (idx16 % 4 == 0)  alpha = 0.13f;
            if (idx16 % 16 == 0) alpha = 0.3f;
            g.setColour (Colours::white.withAlpha (alpha));
            g.drawVerticalLine ((int) x, (float) a.getY(), (float) a.getBottom());
            if (idx16 % 16 == 0 && six * 16 / spp > 24 && idx16 >= 0)
            {
                g.setColour (bjc::dim);
                g.setFont (FontOptions (10.0f));
                g.drawText (String (idx16 / 16 + 1), (int) x + 3, a.getY() + 2, 30, 12, Justification::left);
            }
        }
    }
    else
    {
        const double bar = six * 16;
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
    drawWave (g, a, viewStart, viewLen, col.withAlpha (0.88f));

    // bords + étiquettes
    for (int i = 0; i < (int) p.blocks.size(); ++i)
    {
        const auto& b = p.blocks[(size_t) i];
        if (b.deck != deck || b.type != bj::BlockType::play) continue;
        const bool sel = (i == ed.selBlock);
        const float x0 = sampleToX (anc + b.start16 * six);
        const float x1 = sampleToX (anc + (b.start16 + b.len16) * six);
        if (x1 < a.getX() || x0 > a.getRight()) continue;
        if (sel)
        {
            g.setColour (bjc::red);
            for (float x : { x0, x1 }) g.fillRect (x - 1.0f, (float) a.getY(), 2.0f, (float) a.getHeight());
            Path t1, t2;
            t1.addTriangle (x0, (float) a.getY(), x0 + 10, (float) a.getY(), x0, (float) a.getY() + 10);
            t2.addTriangle (x1, (float) a.getY(), x1 - 10, (float) a.getY(), x1, (float) a.getY() + 10);
            g.fillPath (t1); g.fillPath (t2);
            if (b.reverse)
            {
                g.setFont (FontOptions (11.0f, Font::bold));
                g.drawText (U8 ("◀ arrière"), (int) x0 + 6, a.getY() + 14, 90, 14, Justification::left);
            }
        }
        g.setFont (FontOptions (11.0f, Font::bold));
        const String lbl = "BLOC " + String (i + 1);
        const int tw = 56;
        int row = 0;   // étiquettes empilées si plusieurs zones commencent au même endroit
        for (int k = 0; k < i; ++k)
            if (p.blocks[(size_t) k].type == bj::BlockType::play && p.blocks[(size_t) k].deck == deck
                && std::abs (sampleToX (anc + p.blocks[(size_t) k].start16 * six) - x0) < tw) ++row;
        auto lr = Rectangle<int> ((int) x0 + 5, a.getBottom() - 20 - (row % 6) * 17, tw, 15);
        if (sel)
        {
            g.setColour (bjc::red);
            g.fillRoundedRectangle (lr.toFloat(), 3.0f);
            g.setColour (Colours::white);
        }
        else g.setColour (col);
        g.drawText (lbl, lr, Justification::centred);
    }

    // zone verte de mesure (Alt + glisser)
    if (measureEnd > measureStart)
    {
        const float x0 = sampleToX (measureStart), x1 = sampleToX (measureEnd);
        g.setColour (bjc::green.withAlpha (0.18f));
        g.fillRect (x0, (float) a.getY(), x1 - x0, (float) a.getHeight());
        g.setColour (bjc::green);
        g.drawRect (x0, (float) a.getY(), x1 - x0, (float) a.getHeight(), 1.5f);
    }

    // 1er temps
    {
        const float x = sampleToX (anc);
        if (x >= a.getX() && x <= a.getRight())
        {
            g.setColour (bjc::green);
            Path tri;
            tri.addTriangle (x - 5, (float) a.getBottom(), x + 5, (float) a.getBottom(), x, (float) a.getBottom() - 8);
            g.fillPath (tri);
        }
    }

    // tête de lecture
    if (proc.uiPlaying.load() && proc.uiActiveDeck.load() == deck)
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
    g.setColour (bjc::header);
    g.fillRect (o);
    drawWave (g, o.reduced (0, 2), 0.0, data->numSamples(), col.withAlpha (0.45f));
    const double total = data->numSamples();
    auto ox = [&] (double s) { return (float) (o.getX() + s / total * o.getWidth()); };
    if (auto* b = ed.selected())
        if (b->type == bj::BlockType::play && b->deck == deck)
        {
            const double s0 = anc + b->start16 * six;
            g.setColour (bjc::red.withAlpha (0.4f));
            g.fillRect (Rectangle<float> (ox (s0), (float) o.getY(), jmax (2.0f, ox (s0 + b->len16 * six) - ox (s0)), (float) o.getHeight()));
        }
    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawRect (Rectangle<float> (ox (viewStart), (float) o.getY(), jmax (3.0f, ox (viewStart + viewLen) - ox (viewStart)), (float) o.getHeight()), 1.5f);
}

void WaveformView::mouseMove (const MouseEvent& e)
{
    if (data == nullptr || overviewArea().contains (e.getPosition())) { setMouseCursor (MouseCursor::NormalCursor); return; }
    auto* b = ed.selected();
    if (b != nullptr && b->type == bj::BlockType::play && b->deck == deck)
    {
        const double six = sixteenth(), anc = anchor();
        const float x0 = sampleToX (anc + b->start16 * six), x1 = sampleToX (anc + (b->start16 + b->len16) * six);
        if (std::abs (e.position.x - x0) < 6 || std::abs (e.position.x - x1) < 6) { setMouseCursor (MouseCursor::LeftRightResizeCursor); return; }
    }
    setMouseCursor (blockAt (e.position.x) >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    drag = Drag::none;
    if (data == nullptr) return;
    dragX = e.position.x;
    dragViewStart = viewStart;
    if (overviewArea().contains (e.getPosition())) { drag = Drag::overview; mouseDrag (e); return; }
    if (e.mods.isPopupMenu() || e.mods.isMiddleButtonDown()) { drag = Drag::pan; return; }

    const double s = xToSample (e.position.x);
    const double six = sixteenth(), anc = anchor();

    if (e.mods.isAltDown())
    {
        drag = Drag::measure;
        dragSample = s;
        measureStart = measureEnd = s;
        repaint();
        return;
    }

    auto* b = ed.selected();
    const bool selHere = b != nullptr && b->deck == deck && ! (b->type == bj::BlockType::move && b->fromPrev);

    if ((e.mods.isCtrlDown() || e.mods.isCommandDown()) && selHere)
    {
        const int st = jmax (0, (int) std::round ((s - anc) / six));
        ed.modifyBlock (ed.selBlock, [st] (bj::Block& bb) { bb.start16 = st; });
        return;
    }

    if (selHere && b->type == bj::BlockType::play)
    {
        const float x0 = sampleToX (anc + b->start16 * six), x1 = sampleToX (anc + (b->start16 + b->len16) * six);
        dragStart16 = b->start16; dragLen16 = b->len16; dragSample = s;
        if (std::abs (e.position.x - x0) < 6) { drag = Drag::edgeStart; return; }
        if (std::abs (e.position.x - x1) < 6) { drag = Drag::edgeEnd; return; }
    }

    const int hit = blockAt (e.position.x);
    if (hit >= 0)
    {
        if (hit != ed.selBlock) ed.selectBlock (hit);
        if (auto* hb = ed.selected())
        {
            dragStart16 = hb->start16; dragLen16 = hb->len16; dragSample = s;
            drag = Drag::move;
        }
    }
}

void WaveformView::mouseDrag (const MouseEvent& e)
{
    if (data == nullptr) return;
    const double total = data->numSamples();
    const double six = sixteenth(), anc = anchor();
    const double s = jlimit (0.0, total, xToSample (e.position.x));

    switch (drag)
    {
        case Drag::overview:
        {
            auto o = overviewArea();
            viewStart = (e.position.x - o.getX()) / (double) jmax (1, o.getWidth()) * total - viewLen * 0.5;
            clampView();
            break;
        }
        case Drag::pan:
            viewStart = dragViewStart - (e.position.x - dragX) * viewLen / jmax (1, mainArea().getWidth());
            clampView();
            break;
        case Drag::move:
        {
            const int st = jmax (0, dragStart16 + (int) std::round ((s - dragSample) / six));
            ed.modifyBlock (ed.selBlock, [st] (bj::Block& b) { b.start16 = st; });
            break;
        }
        case Drag::edgeStart:
        {
            const int end = dragStart16 + dragLen16;
            const int st = jlimit (0, end - 1, (int) std::round ((s - anc) / six));
            ed.modifyBlock (ed.selBlock, [st, end] (bj::Block& b) { b.start16 = st; b.len16 = end - st; });
            break;
        }
        case Drag::edgeEnd:
        {
            const int en = (int) std::round ((s - anc) / six);
            const int ln = jmax (1, en - dragStart16);
            ed.modifyBlock (ed.selBlock, [ln] (bj::Block& b) { b.len16 = ln; });
            break;
        }
        case Drag::measure:
            measureStart = jmin (dragSample, s);
            measureEnd = jmax (dragSample, s);
            break;
        case Drag::none: break;
    }

    if (drag == Drag::move || drag == Drag::edgeStart || drag == Drag::edgeEnd || drag == Drag::measure)
    {
        auto a = mainArea();
        if (e.position.x > a.getRight() - 10) { viewStart += viewLen * 0.02; clampView(); }
        if (e.position.x < a.getX() + 10)     { viewStart -= viewLen * 0.02; clampView(); }
    }
    repaint();
}

void WaveformView::mouseUp (const MouseEvent& e)
{
    if (drag == Drag::measure && measureEnd - measureStart < 32.0)
    {
        // simple Alt + clic : place le 1er temps
        proc.settings (deck).anchor = xToSample (e.position.x);
        measureStart = measureEnd = -1;
        ed.blocksChanged();
    }
    drag = Drag::none;
    repaint();
}

void WaveformView::mouseDoubleClick (const MouseEvent& e)
{
    if (data == nullptr || overviewArea().contains (e.getPosition())) return;
    auto* b = ed.selected();
    if (b == nullptr || b->deck != deck || (b->type == bj::BlockType::move && b->fromPrev)) return;
    const double six = sixteenth(), anc = anchor();
    const int unit = e.mods.isShiftDown() ? 4 : 16;
    const int st = jmax (0, (int) std::floor ((xToSample (e.position.x) - anc) / six / unit) * unit);
    ed.modifyBlock (ed.selBlock, [st] (bj::Block& bb) { bb.start16 = st; });
}

void WaveformView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (data == nullptr) return;
    const bool horizontal = std::abs (w.deltaX) > std::abs (w.deltaY);
    if (e.mods.isShiftDown() || horizontal)
    {
        viewStart -= (horizontal ? w.deltaX : w.deltaY) * viewLen * 0.5;
        clampView();
        repaint();
        return;
    }
    zoom (std::pow (0.5, w.deltaY * 2.0), xToSample (e.position.x));
}

void WaveformView::mouseMagnify (const MouseEvent& e, float s)
{
    if (s > 0.0f) zoom (1.0 / s, xToSample (e.position.x));
}

// ============================================================================
//  Séquence
// ============================================================================
double SequenceView::total() const { return ed.pattern().totalBeats(); }

float SequenceView::beatToX (double b) const
{
    return (float) ((b - viewStart) / shownLen() * getWidth());
}

double SequenceView::xToBeat (float x) const
{
    return viewStart + x / (double) jmax (1, getWidth()) * shownLen();
}

void SequenceView::clampView()
{
    const double t = total();
    if (viewLen > 0) viewLen = jlimit (0.5, t, viewLen);
    if (viewLen >= t - 1.0e-9) viewLen = -1;
    viewStart = jlimit (0.0, jmax (0.0, t - shownLen()), viewStart);
}

void SequenceView::paint (Graphics& g)
{
    clampView();
    const auto& p = ed.pattern();
    const double t = total();
    const float blockH = 44.0f;
    auto area = getLocalBounds().toFloat();

    g.setColour (Colour (0xff07090d));
    g.fillRoundedRectangle (area.withHeight (blockH + 2), 5.0f);

    double pos = 0;
    for (int i = 0; i < (int) p.blocks.size(); ++i)
    {
        const auto& b = p.blocks[(size_t) i];
        const double d = b.len16 / 4.0;
        if (pos >= t) break;
        const double shown = jmin (d, t - pos);
        const bool cut = shown < d - 1.0e-9;
        const float x0 = beatToX (pos), x1 = beatToX (pos + shown);
        pos += d;
        if (x1 < 0 || x0 > getWidth()) continue;

        const int dk = ed.resolvedDeck (i);
        const auto col = bjc::deck (dk);
        const bool sel = (i == ed.selBlock);
        const bool now = ed.proc.uiPlaying.load() && ed.proc.playingPattern.load() == ed.editPat && ed.proc.uiBlock.load() == i;
        auto r = Rectangle<float> (x0 + 1, 1, jmax (2.0f, x1 - x0 - 3), blockH);
        const bool isMove = b.type == bj::BlockType::move;

        g.setColour (isMove ? bjc::panel2 : col.withAlpha (now ? 1.0f : 0.8f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (sel ? Colours::white : col);
        g.drawRoundedRectangle (r, 4.0f, sel ? 2.5f : 1.5f);
        if (now)
        {
            g.setColour (Colours::white.withAlpha (0.25f));
            g.fillRoundedRectangle (r, 4.0f);
        }
        if (cut)
        {
            g.setColour (bjc::red);
            g.fillRect (r.getRight() - 4, r.getY(), 4.0f, r.getHeight());
        }

        g.setColour (isMove ? col : Colour (0xff07090d));
        auto tr = r.reduced (6, 3);
        if (r.getWidth() > 70)
        {
            g.setFont (FontOptions (12.0f, Font::bold));
            g.drawText (String (i + 1) + U8 (" · ") + (dk == 0 ? "Gauche" : "Droite"), tr.removeFromTop (tr.getHeight() * 0.5f), Justification::bottomLeft);
            g.setFont (FontOptions (11.0f));
            String sub = isMove ? bj::moveName (b.move) : String (b.reverse ? U8 ("Arrière") : String ("Lecture"));
            sub << U8 (" · ") << friendlyDur (b.len16);
            if (cut) sub << " (raccourci)";
            g.drawText (sub, tr, Justification::topLeft);
        }
        else if (r.getWidth() > 16)
        {
            g.setFont (FontOptions (11.0f, Font::bold));
            g.drawText (String (i + 1), tr.removeFromTop (tr.getHeight() * 0.5f), Justification::centredBottom);
            g.setFont (FontOptions (9.0f, Font::bold));
            g.drawText (isMove ? bj::moveName (b.move).substring (0, 2).toUpperCase() : String (b.reverse ? "ARR" : "LEC"), tr, Justification::centredTop);
        }
    }

    // fin vide = silence
    if (pos < t)
    {
        const float x0 = beatToX (pos), x1 = beatToX (t);
        auto r = Rectangle<float> (x0 + 1, 1, jmax (2.0f, x1 - x0 - 2), blockH);
        g.setColour (Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (bjc::dim.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        if (r.getWidth() > 50)
        {
            g.setFont (FontOptions (11.0f));
            g.setColour (bjc::dim);
            g.drawText ("silence", r, Justification::centred);
        }
    }

    // règle
    const float ry = blockH + 4;
    const double sl = shownLen();
    const double pxPerBeat = getWidth() / sl;
    const int stepBeats = pxPerBeat * 4 < 22 ? 16 : 4;
    for (int k = 0; k <= (int) std::ceil (t); ++k)
    {
        const float x = beatToX (k);
        if (x < -1 || x > getWidth() + 1) continue;
        const bool bar = k % 4 == 0;
        if (! bar && pxPerBeat < 6) continue;
        g.setColour (Colours::white.withAlpha (bar ? 0.5f : 0.18f));
        g.drawVerticalLine ((int) x, ry, ry + (bar ? 8.0f : 4.0f));
        if (bar && (k % stepBeats == 0) && k < t)
        {
            g.setColour (bjc::dim);
            g.setFont (FontOptions (10.0f));
            g.drawText (String (k / 4 + 1), (int) x + 3, (int) ry + 2, 30, 12, Justification::left);
        }
    }

    // tête de lecture
    if (ed.proc.uiPlaying.load() && ed.proc.playingPattern.load() == ed.editPat)
    {
        const float x = beatToX (ed.proc.uiPatternPos.load());
        if (x >= 0 && x <= getWidth())
        {
            g.setColour (Colours::white);
            g.fillRect (x - 1.0f, 0.0f, 2.0f, blockH + 4);
        }
    }
}

void SequenceView::mouseDown (const MouseEvent& e)
{
    panning = e.mods.isPopupMenu() || e.mods.isMiddleButtonDown();
    panX = e.position.x;
    panStart = viewStart;
    if (panning) return;
    const double beat = xToBeat (e.position.x);
    const auto& p = ed.pattern();
    double pos = 0;
    for (int i = 0; i < (int) p.blocks.size(); ++i)
    {
        const double d = p.blocks[(size_t) i].len16 / 4.0;
        if (beat >= pos && beat < pos + d) { ed.selectBlock (i); return; }
        pos += d;
    }
}

void SequenceView::mouseDrag (const MouseEvent& e)
{
    if (! panning) return;
    viewStart = panStart - (e.position.x - panX) / (double) jmax (1, getWidth()) * shownLen();
    clampView();
    repaint();
}

void SequenceView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const bool horizontal = std::abs (w.deltaX) > std::abs (w.deltaY);
    if (e.mods.isShiftDown() || horizontal)
    {
        viewStart -= (horizontal ? w.deltaX : w.deltaY) * shownLen() * 0.5;
        clampView();
        repaint();
        return;
    }
    const double centre = xToBeat (e.position.x);
    const double rel = (centre - viewStart) / shownLen();
    viewLen = shownLen() * std::pow (0.5, w.deltaY * 2.0);
    clampView();
    viewStart = centre - rel * shownLen();
    clampView();
    repaint();
}

// ============================================================================
//  Éditeur
// ============================================================================
BeatJugglingEditor::BeatJugglingEditor (BeatJugglingProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    auto& ap = proc.apvts;
    auto& c = content;

    // ---- patterns ----
    for (int i = 0; i < bj::kNumPatterns; ++i)
    {
        auto* b = patBtns.add (new PatternButton (proc, i));
        b->onClick = [this] (int idx)
        {
            editPat = idx;
            selBlock = 0;
            seq.resetZoom();
            proc.requestPattern (idx);
            blocksChanged();
        };
        c.addAndMakeVisible (b);
    }
    patHint.setText (U8 ("Clic = édite ce pattern et l'enchaîne à la fin de celui en cours"), dontSendNotification);
    patHint.setFont (FontOptions (11.0f));
    patHint.setColour (Label::textColourId, bjc::dim);
    patHint.setJustificationType (Justification::centredLeft);
    c.addAndMakeVisible (patHint);

    for (int j = 0; j < bj::kNumBarOptions; ++j)
    {
        auto* b = barBtns.add (new TextButton (String (bj::kBarOptions[j])));
        b->setRadioGroupId (201);
        b->setClickingTogglesState (true);
        b->setColour (TextButton::buttonOnColourId, bjc::red.withAlpha (0.6f));
        const int bars = bj::kBarOptions[j];
        b->onClick = [this, b, bars] { if (b->getToggleState()) { proc.editPattern (editPat, [bars] (bj::Pattern& pp) { pp.bars = bars; }); seq.resetZoom(); blocksChanged(); } };
        c.addAndMakeVisible (b);
    }
    remainLbl.setFont (FontOptions (12.0f, Font::bold));
    remainLbl.setJustificationType (Justification::centredLeft);
    c.addAndMakeVisible (remainLbl);

    // ---- platines ----
    for (int k = 0; k < 2; ++k)
    {
        auto& table = k == 0 ? tableL : tableR;
        c.addAndMakeVisible (table);
        table.onClick = [this, k] { wave.setDeck (k); updateDeckInfo(); repaint(); };
        table.getZone = [this, k] (double& zs, double& zl)
        {
            if (proc.uiPlaying.load())
            {
                zs = proc.uiZoneStart[k].load(); zl = proc.uiZoneLen[k].load();
                if (zl > 0) return true;
            }
            // sinon : bloc sélectionné s'il est sur cette platine, ou le premier bloc de lecture de la platine
            auto d = proc.getDeck (k);
            if (d == nullptr) return false;
            const double six = 60.0 / jmax (20.0, proc.settings (k).fileBpm.load()) * d->sampleRate / 4.0;
            const double anc = proc.settings (k).anchor.load();
            const auto& pp = pattern();
            int pick = -1;
            if (auto* sb = selected()) if (sb->type == bj::BlockType::play && sb->deck == k) pick = selBlock;
            for (int i = 0; pick < 0 && i < (int) pp.blocks.size(); ++i)
                if (pp.blocks[(size_t) i].type == bj::BlockType::play && pp.blocks[(size_t) i].deck == k) pick = i;
            if (pick < 0) return false;
            const auto& b = pp.blocks[(size_t) pick];
            zs = anc + b.start16 * six; zl = b.len16 * six;
            proc.uiDeckPos[k] = proc.uiPlaying.load() ? proc.uiDeckPos[k].load() : zs;
            return true;
        };

        auto& load = k == 0 ? loadL : loadR;
        c.addAndMakeVisible (load);
        load.onClick = [this, k]
        {
            chooser = std::make_unique<FileChooser> ("Choisir un son", File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                  [this, k] (const FileChooser& fc)
                                  {
                                      auto f = fc.getResult();
                                      if (f.existsAsFile()) loadInto (k, f);
                                  });
        };
        auto& copy = k == 0 ? copyL : copyR;
        copy.setButtonText (k == 0 ? U8 ("Copier → Droite") : U8 ("Copier → Gauche"));
        copy.setTooltip (U8 ("Met le même son et le même BPM sur l'autre platine"));
        c.addAndMakeVisible (copy);
        copy.onClick = [this, k] { proc.copyDeck (k, 1 - k); updateDeckInfo(); blocksChanged(); };

        auto& fl = k == 0 ? fileL : fileR;
        fl.setFont (FontOptions (11.0f));
        fl.setColour (Label::textColourId, bjc::dim);
        fl.setJustificationType (Justification::centredRight);
        c.addAndMakeVisible (fl);

        auto& bl = k == 0 ? bpmLblL : bpmLblR;
        bl.setText ("BPM du son", dontSendNotification);
        bl.setFont (FontOptions (11.0f));
        bl.setColour (Label::textColourId, bjc::dim);
        c.addAndMakeVisible (bl);
        auto& bs = k == 0 ? bpmL : bpmR;
        bs.setSliderStyle (Slider::LinearBar);
        bs.setRange (40.0, 250.0, 0.01);
        bs.setColour (Slider::trackColourId, bjc::panel2.brighter (0.15f));
        bs.setTooltip (U8 ("Tempo d'origine du son : sert à la grille et au calage sur Ableton"));
        bs.onValueChange = [this, k, &bs] { proc.settings (k).fileBpm = bs.getValue(); blocksChanged(); };
        c.addAndMakeVisible (bs);
        auto& h = k == 0 ? halfL : halfR;
        auto& dbl = k == 0 ? dblL : dblR;
        c.addAndMakeVisible (h);
        c.addAndMakeVisible (dbl);
        h.onClick   = [&bs] { bs.setValue (bs.getValue() * 0.5); };
        dbl.onClick = [&bs] { bs.setValue (bs.getValue() * 2.0); };
    }

    // ---- mix ----
    setupKnob (gainA,  gainALbl,  "Volume G",  bjc::deckA,     c);
    setupKnob (gainB,  gainBLbl,  "Volume D",  bjc::deckB,     c);
    setupKnob (volume, volumeLbl, "Volume",    Colours::white, c);
    setupKnob (xfade,  xfadeLbl,  "Crossfade", bjc::red,       c);
    gainAAtt  = std::make_unique<SliderAtt> (ap, "gainA", gainA);
    gainBAtt  = std::make_unique<SliderAtt> (ap, "gainB", gainB);
    volumeAtt = std::make_unique<SliderAtt> (ap, "volume", volume);
    xfadeAtt  = std::make_unique<SliderAtt> (ap, "xfade", xfade);
    xfade.setTooltip (U8 ("Durée du fondu à chaque changement de bloc (anti-clic)"));

    tempo.setSliderStyle (Slider::LinearBar);
    tempo.setColour (Slider::trackColourId, bjc::panel2.brighter (0.15f));
    tempoAtt = std::make_unique<SliderAtt> (ap, "bpm", tempo);
    tempo.setTooltip (U8 ("Tempo utilisé par le Play test (sans Ableton)"));
    c.addAndMakeVisible (tempo);
    tempoLbl.setText ("Tempo test", dontSendNotification);
    tempoLbl.setFont (FontOptions (11.0f));
    tempoLbl.setColour (Label::textColourId, bjc::dim);
    c.addAndMakeVisible (tempoLbl);

    c.addAndMakeVisible (syncBtn);
    syncAtt = std::make_unique<ButtonAtt> (ap, "sync", syncBtn);
    syncBtn.setTooltip (U8 ("Activé : les sons sont calés sur le tempo d'Ableton (la hauteur suit, comme un vinyle)"));

    playBtn.setButtonText (U8 ("▶ Play test"));
    playBtn.setClickingTogglesState (true);
    playBtn.setColour (TextButton::buttonOnColourId, bjc::green.withAlpha (0.45f));
    playBtn.setTooltip (U8 ("Lecture sans Ableton. Dans Ableton, lance simplement la lecture."));
    playBtn.onClick = [this] { proc.internalPlay = playBtn.getToggleState(); };
    playBtn.setToggleState (proc.internalPlay.load(), dontSendNotification);
    c.addAndMakeVisible (playBtn);

    // ---- vinyle ----
    c.addAndMakeVisible (wave);
    for (auto* b : { &waveDeckL, &waveDeckR })
    {
        b->setRadioGroupId (301);
        b->setClickingTogglesState (true);
        c.addAndMakeVisible (*b);
    }
    waveDeckL.setColour (TextButton::buttonOnColourId, bjc::deckA.withAlpha (0.5f));
    waveDeckR.setColour (TextButton::buttonOnColourId, bjc::deckB.withAlpha (0.5f));
    waveDeckL.onClick = [this] { if (waveDeckL.getToggleState()) { wave.setDeck (0); updateDeckInfo(); repaint(); } };
    waveDeckR.onClick = [this] { if (waveDeckR.getToggleState()) { wave.setDeck (1); updateDeckInfo(); repaint(); } };

    for (auto* b : { &zoomIn, &zoomOut, &zoomAllBtn, &zoomBlockBtn }) c.addAndMakeVisible (*b);
    zoomIn.onClick       = [this] { wave.zoom (0.5, -1.0); };
    zoomOut.onClick      = [this] { wave.zoom (2.0, -1.0); };
    zoomAllBtn.onClick   = [this] { wave.zoomAll(); };
    zoomBlockBtn.onClick = [this] { wave.zoomToBlock(); };
    zoomBlockBtn.setTooltip (U8 ("Zoome sur le bloc sélectionné"));

    waveInfo.setFont (FontOptions (11.0f));
    waveInfo.setColour (Label::textColourId, bjc::dim);
    c.addAndMakeVisible (waveInfo);

    calCombo.addItem (U8 ("Vert = 1 temps"), 1);
    calCombo.addItem (U8 ("Vert = 2 temps"), 2);
    calCombo.addItem (U8 ("Vert = 1 mesure"), 4);
    calCombo.addItem (U8 ("Vert = 2 mesures"), 8);
    calCombo.addItem (U8 ("Vert = 4 mesures"), 16);
    calCombo.setSelectedId (4, dontSendNotification);
    calCombo.setTooltip (U8 ("Alt + glisser sur le vinyle dessine une zone verte : indique ce qu'elle dure, puis Caler le BPM"));
    c.addAndMakeVisible (calCombo);
    c.addAndMakeVisible (calBtn);
    calBtn.setTooltip (U8 ("Calcule le BPM du son à partir de la zone verte et place le 1er temps à son début"));
    calBtn.onClick = [this]
    {
        const int k = wave.getDeck();
        auto d = proc.getDeck (k);
        if (d == nullptr || wave.measureEnd - wave.measureStart < 64) return;
        const double len = (wave.measureEnd - wave.measureStart) / d->sampleRate;
        double bpm = calCombo.getSelectedId() * 60.0 / len;
        while (bpm > 250.0) bpm *= 0.5;
        while (bpm < 40.0)  bpm *= 2.0;
        proc.settings (k).fileBpm = bpm;
        proc.settings (k).anchor = wave.measureStart;
        wave.measureStart = wave.measureEnd = -1;
        updateDeckInfo();
        blocksChanged();
    };

    // ---- séquence ----
    c.addAndMakeVisible (seq);
    addBtn.setTooltip (U8 ("Ajoute un bloc après le bloc sélectionné"));
    addBtn.onClick = [this] { addBlock(); };
    c.addAndMakeVisible (addBtn);
    seqHint.setText (U8 ("Clic = régler un bloc · molette = zoom · Maj+molette = défiler · le bloc qui déborde est raccourci · fin vide = silence"), dontSendNotification);
    seqHint.setFont (FontOptions (11.0f));
    seqHint.setColour (Label::textColourId, bjc::dim);
    seqHint.setJustificationType (Justification::centredRight);
    c.addAndMakeVisible (seqHint);

    // ---- réglages du bloc ----
    blockTitle.setFont (FontOptions (18.0f, Font::bold));
    c.addAndMakeVisible (blockTitle);
    blockInfo.setFont (FontOptions (11.0f));
    blockInfo.setColour (Label::textColourId, bjc::dim);
    blockInfo.setJustificationType (Justification::topLeft);
    c.addAndMakeVisible (blockInfo);
    emptyLbl.setText (U8 ("Ce pattern est vide : clique sur « + » pour ajouter un bloc."), dontSendNotification);
    emptyLbl.setFont (FontOptions (14.0f));
    emptyLbl.setJustificationType (Justification::centred);
    c.addChildComponent (emptyLbl);

    smallLabel (typeLbl, "TYPE", c);
    smallLabel (deckLbl, "PLATINE", c);
    smallLabel (sensLbl, "SENS", c);
    smallLabel (moveLbl, "MOUVEMENT", c);
    smallLabel (fromLbl, "PART DE", c);

    makeSeg (typeBtns, StringArray ("Lecture", "Mouvement"), 401, bjc::deckA, c, [this] (int i)
    {
        modifyBlock (selBlock, [i] (bj::Block& b) { b.type = i == 0 ? bj::BlockType::play : bj::BlockType::move; });
    });
    makeSeg (deckBtns, StringArray ("Gauche", "Droite"), 402, bjc::deckA, c, [this] (int i)
    {
        modifyBlock (selBlock, [i] (bj::Block& b) { b.deck = i; });
    });
    deckBtns[1]->setColour (TextButton::buttonOnColourId, bjc::deckB.withAlpha (0.55f));
    makeSeg (sensBtns, StringArray ("Avant", U8 ("Arrière")), 403, bjc::deckA, c, [this] (int i)
    {
        modifyBlock (selBlock, [i] (bj::Block& b) { b.reverse = i == 1; });
    });
    StringArray mv;
    for (int m = 0; m < (int) bj::Move::count; ++m) mv.add (bj::moveName ((bj::Move) m));
    makeSeg (moveBtns, mv, 404, bjc::deckB, c, [this] (int i)
    {
        modifyBlock (selBlock, [i] (bj::Block& b) { b.move = (bj::Move) i; });
    });
    makeSeg (fromBtns, StringArray ("Suite du bloc", U8 ("Autre départ")), 405, bjc::deckB, c, [this] (int i)
    {
        modifyBlock (selBlock, [i] (bj::Block& b) { b.fromPrev = i == 0; });
    });

    setupKnob (startKnob, startLbl, U8 ("Départ"), bjc::cue, c, 72);
    setupKnob (durKnob, durLbl, U8 ("Durée"), bjc::red, c, 72);
    setupKnob (amountKnob, amountLbl, "Courbe", bjc::deckB, c, 72);
    startKnob.setTextBoxIsEditable (true);
    durKnob.setTextBoxIsEditable (true);
    startKnob.setRange (0, 512, 1);
    durKnob.setRange (1, 512, 1);
    amountKnob.setRange (0.0, 1.0, 0.01);
    startKnob.textFromValueFunction = [] (double v) { return bj::fmtPos16 ((int) std::round (v)); };
    startKnob.valueFromTextFunction = [this] (const String& t) { return (double) bj::parsePos16 (t, (int) startKnob.getValue()); };
    durKnob.textFromValueFunction = [] (double v) { return bj::fmtDur16 ((int) std::round (v)); };
    durKnob.valueFromTextFunction = [this] (const String& t) { return (double) bj::parseDur16 (t, (int) durKnob.getValue()); };
    amountKnob.textFromValueFunction = [this] (double v)
    {
        auto* b = selected();
        return bj::amountText (b ? b->move : bj::Move::stop, (float) v);
    };
    startKnob.setTooltip (U8 ("Départ dans le son : mesure.temps.double-croche (double-clic sur la valeur pour la taper)"));
    durKnob.setTooltip (U8 ("Durée du bloc : mesures.temps.doubles-croches (0.1.0 = un temps)"));
    startKnob.onValueChange  = [this] { const int v = (int) std::round (startKnob.getValue()); modifyBlock (selBlock, [v] (bj::Block& b) { b.start16 = v; }); };
    durKnob.onValueChange    = [this] { const int v = (int) std::round (durKnob.getValue());   modifyBlock (selBlock, [v] (bj::Block& b) { b.len16 = v; }); };
    amountKnob.onValueChange = [this] { const float v = (float) amountKnob.getValue();         modifyBlock (selBlock, [v] (bj::Block& b) { b.amount = v; }); };

    leftBtn.setButtonText (U8 ("◀"));
    rightBtn.setButtonText (U8 ("▶"));
    leftBtn.setTooltip (U8 ("Déplace le bloc vers la gauche"));
    rightBtn.setTooltip (U8 ("Déplace le bloc vers la droite"));
    for (auto* b : { &leftBtn, &rightBtn, &dupBtn, &delBtn }) c.addAndMakeVisible (*b);
    delBtn.setColour (TextButton::textColourOffId, Colour (0xffff8a8a));
    leftBtn.onClick  = [this] { moveSelected (-1); };
    rightBtn.onClick = [this] { moveSelected (1); };
    dupBtn.onClick   = [this] { duplicateSelected(); };
    delBtn.onClick   = [this] { deleteSelected(); };

    // fenêtre redimensionnable (proportions fixes)
    setResizable (true, true);
    if (auto* cons = getConstrainer()) cons->setFixedAspectRatio ((double) kW / kH);
    setResizeLimits (640, 440, 1920, 1320);

    selectBlock (0);
    setSize (1100, (int) std::round (1100.0 * kH / kW));
    startTimerHz (40);
}

BeatJugglingEditor::~BeatJugglingEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------------------
const bj::Block* BeatJugglingEditor::selected() const
{
    const auto& p = pattern();
    if (selBlock < 0 || selBlock >= (int) p.blocks.size()) return nullptr;
    return &p.blocks[(size_t) selBlock];
}

int BeatJugglingEditor::resolvedDeck (int i) const
{
    const auto& p = pattern();
    if (i < 0 || i >= (int) p.blocks.size()) return 0;
    int deck = p.blocks[0].deck;
    for (int k = 0; k <= i; ++k)
    {
        const auto& b = p.blocks[(size_t) k];
        if (b.type == bj::BlockType::play || ! b.fromPrev || k == 0) deck = b.deck;
    }
    return deck;
}

void BeatJugglingEditor::selectBlock (int i)
{
    const int n = (int) pattern().blocks.size();
    selBlock = n == 0 ? -1 : jlimit (0, n - 1, i);
    if (auto* b = selected())
    {
        juce::ignoreUnused (b);
        wave.setDeck (resolvedDeck (selBlock));
    }
    blocksChanged();
}

void BeatJugglingEditor::modifyBlock (int i, const std::function<void (bj::Block&)>& fn)
{
    if (updating) return;
    if (i < 0 || i >= (int) pattern().blocks.size()) return;
    proc.editPattern (editPat, [i, &fn] (bj::Pattern& p) { fn (p.blocks[(size_t) i]); });
    blocksChanged();
}

void BeatJugglingEditor::blocksChanged()
{
    updatePatternInfo();
    updateBlockEditor();
    updateDeckInfo();
    seq.repaint();
    wave.repaint();
    tableL.repaint();
    tableR.repaint();
    for (auto* b : patBtns) b->repaint();
}

void BeatJugglingEditor::addBlock()
{
    const auto& p = pattern();
    if ((int) p.blocks.size() >= bj::kMaxBlocks) return;
    bj::Block nb;
    nb.type = bj::BlockType::play;
    nb.deck = p.blocks.empty() ? 0 : 1 - resolvedDeck (selBlock);
    const int remain = p.total16() - p.used16();
    nb.len16 = remain > 0 ? jmin (16, remain) : 4;
    if (auto* s = selected()) if (s->type == bj::BlockType::play) nb.start16 = s->start16;
    const int at = p.blocks.empty() ? 0 : selBlock + 1;
    proc.editPattern (editPat, [nb, at] (bj::Pattern& pp) { pp.blocks.insert (pp.blocks.begin() + jlimit (0, (int) pp.blocks.size(), at), nb); });
    selectBlock (at);
}

void BeatJugglingEditor::moveSelected (int delta)
{
    const int n = (int) pattern().blocks.size();
    const int j = selBlock + delta;
    if (selBlock < 0 || j < 0 || j >= n) return;
    const int i = selBlock;
    proc.editPattern (editPat, [i, j] (bj::Pattern& pp) { std::swap (pp.blocks[(size_t) i], pp.blocks[(size_t) j]); });
    selectBlock (j);
}

void BeatJugglingEditor::duplicateSelected()
{
    auto* s = selected();
    if (s == nullptr || (int) pattern().blocks.size() >= bj::kMaxBlocks) return;
    const auto copy = *s;
    const int at = selBlock + 1;
    proc.editPattern (editPat, [copy, at] (bj::Pattern& pp) { pp.blocks.insert (pp.blocks.begin() + at, copy); });
    selectBlock (at);
}

void BeatJugglingEditor::deleteSelected()
{
    if (selected() == nullptr) return;
    const int i = selBlock;
    proc.editPattern (editPat, [i] (bj::Pattern& pp) { pp.blocks.erase (pp.blocks.begin() + i); });
    selectBlock (i > 0 ? i - 1 : 0);
}

// ---------------------------------------------------------------------------
void BeatJugglingEditor::loadInto (int d, const File& f)
{
    if (! proc.loadFile (d, f, true))
    {
        (d == 0 ? fileL : fileR).setText (U8 ("Fichier illisible : ") + f.getFileName(), dontSendNotification);
        return;
    }
    wave.setDeck (d);
    blocksChanged();
}

void BeatJugglingEditor::updateDeckInfo()
{
    for (int k = 0; k < 2; ++k)
    {
        auto d = proc.getDeck (k);
        auto& fl = k == 0 ? fileL : fileR;
        auto& bs = k == 0 ? bpmL : bpmR;
        bs.setValue (proc.settings (k).fileBpm.load(), dontSendNotification);
        if (d == nullptr) { fl.setText (U8 ("aucun son"), dontSendNotification); continue; }
        const double bars = d->numSamples() / d->sampleRate / (60.0 / jmax (20.0, proc.settings (k).fileBpm.load()) * 4.0);
        fl.setText (d->file.getFileNameWithoutExtension().substring (0, 26) + U8 (" · ") + String (bars, 1) + " mes", dontSendNotification);
    }

    const int k = wave.getDeck();
    waveDeckL.setToggleState (k == 0, dontSendNotification);
    waveDeckR.setToggleState (k == 1, dontSendNotification);
    String info;
    if (auto* b = selected())
    {
        if (resolvedDeck (selBlock) == k)
            info << "Bloc " << (selBlock + 1) << U8 (" : tire la zone rouge ou ses bords · double-clic = la mesure (Maj : le temps) · Ctrl+clic = départ");
        else
            info << "Bloc " << (selBlock + 1) << U8 (" est sur l'autre platine");
        juce::ignoreUnused (b);
    }
    info << U8 (" · Alt+glisser = zone verte · Alt+clic = 1er temps");
    waveInfo.setText (info, dontSendNotification);
}

void BeatJugglingEditor::updatePatternInfo()
{
    const auto& p = pattern();
    for (int i = 0; i < patBtns.size(); ++i) patBtns[i]->editing = (i == editPat);
    for (int j = 0; j < bj::kNumBarOptions; ++j) barBtns[j]->setToggleState (bj::kBarOptions[j] == p.bars, dontSendNotification);

    const int rem = p.total16() - p.used16();
    if (rem > 0)       { remainLbl.setText ("reste " + bj::fmtDur16 (rem) + "\n(fin = silence)", dontSendNotification); remainLbl.setColour (Label::textColourId, bjc::cue); }
    else if (rem == 0) { remainLbl.setText ("Plein", dontSendNotification); remainLbl.setColour (Label::textColourId, bjc::green); }
    else               { remainLbl.setText (U8 ("déborde de ") + bj::fmtDur16 (-rem) + U8 ("\n(raccourci)"), dontSendNotification); remainLbl.setColour (Label::textColourId, Colour (0xffff7070)); }
}

void BeatJugglingEditor::updateBlockEditor()
{
    const ScopedValueSetter<bool> guard (updating, true);
    auto* b = selected();
    const bool has = b != nullptr;
    emptyLbl.setVisible (! has);
    for (Component* comp : std::initializer_list<Component*> { &blockTitle, &blockInfo, &typeLbl, &deckLbl, &startKnob, &startLbl, &durKnob, &durLbl,
                                                               &leftBtn, &rightBtn, &dupBtn, &delBtn })
        comp->setVisible (has);
    for (auto* arr : { &typeBtns, &deckBtns })
        for (auto* bt : *arr) bt->setVisible (has);
    if (! has)
    {
        for (auto* arr : { &sensBtns, &moveBtns, &fromBtns }) for (auto* bt : *arr) bt->setVisible (false);
        for (Component* comp : std::initializer_list<Component*> { &sensLbl, &moveLbl, &fromLbl, &amountKnob, &amountLbl }) comp->setVisible (false);
        return;
    }

    const bool isMove = b->type == bj::BlockType::move;
    const bool linked = isMove && b->fromPrev && selBlock > 0;

    blockTitle.setText ("Bloc " + String (selBlock + 1), dontSendNotification);
    int pos16 = 0;
    for (int i = 0; i < selBlock; ++i) pos16 += pattern().blocks[(size_t) i].len16;
    String info = "dans le pattern : " + bj::fmtPos16 (pos16) + U8 (" → ") + bj::fmtPos16 (pos16 + b->len16);
    if (pos16 + b->len16 > pattern().total16()) info << U8 ("\nraccourci à la fin du pattern");
    if (linked) info << U8 ("\nsuit le bloc ") << selBlock << " (" << (resolvedDeck (selBlock) == 0 ? "Gauche" : "Droite") << ")";
    blockInfo.setText (info, dontSendNotification);

    setSeg (typeBtns, isMove ? 1 : 0);
    setSeg (deckBtns, linked ? resolvedDeck (selBlock) : b->deck);
    for (auto* bt : deckBtns) bt->setEnabled (! linked);
    startKnob.setEnabled (! linked);
    deckLbl.setText (linked ? "PLATINE (suit)" : "PLATINE", dontSendNotification);

    for (auto* bt : sensBtns) bt->setVisible (! isMove);
    sensLbl.setVisible (! isMove);
    setSeg (sensBtns, b->reverse ? 1 : 0);
    for (auto* arr : { &moveBtns, &fromBtns }) for (auto* bt : *arr) bt->setVisible (isMove);
    moveLbl.setVisible (isMove);
    fromLbl.setVisible (isMove);
    amountKnob.setVisible (isMove);
    amountLbl.setVisible (isMove);
    setSeg (moveBtns, (int) b->move);
    setSeg (fromBtns, b->fromPrev ? 0 : 1);
    amountLbl.setText (bj::amountName (b->move), dontSendNotification);

    // plage du départ : longueur du son de la platine
    int maxStart = 512;
    if (auto d = proc.getDeck (b->deck))
    {
        const double six = 60.0 / jmax (20.0, proc.settings (b->deck).fileBpm.load()) * d->sampleRate / 4.0;
        maxStart = jmax (16, (int) ((d->numSamples() - proc.settings (b->deck).anchor.load()) / six) - 1);
    }
    startKnob.setRange (0, jmax (maxStart, b->start16), 1);
    startKnob.setMouseDragSensitivity (jlimit (250, 2400, maxStart * 3));
    durKnob.setMouseDragSensitivity (900);
    startKnob.setValue (b->start16, dontSendNotification);
    durKnob.setValue (b->len16, dontSendNotification);
    amountKnob.setValue (b->amount, dontSendNotification);
    amountKnob.updateText();

    leftBtn.setEnabled (selBlock > 0);
    rightBtn.setEnabled (selBlock < (int) pattern().blocks.size() - 1);
}

// ---------------------------------------------------------------------------
bool BeatJugglingEditor::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) return true;
    return false;
}

void BeatJugglingEditor::filesDropped (const StringArray& files, int x, int y)
{
    const auto pt = content.getLocalPoint (this, Point<int> (x, y));
    int d = wave.getDeck();
    if (pt.x < kW / 3) d = 0;
    else if (pt.x > kW * 2 / 3) d = 1;
    if (wave.getBounds().contains (pt)) d = wave.getDeck();
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) { loadInto (d, File (f)); break; }
}

void BeatJugglingEditor::timerCallback()
{
    tableL.tick();
    tableR.tick();
    wave.refresh();
    seq.repaint();
    if (playBtn.getToggleState() != proc.internalPlay.load())
        playBtn.setToggleState (proc.internalPlay.load(), dontSendNotification);

    for (int k = 0; k < 2; ++k)
    {
        auto d = proc.getDeck (k);
        if (d != shownData[k]) { shownData[k] = d; blocksChanged(); }
    }
    const int pl = proc.playingPattern.load(), q = proc.queuedPattern.load(), ub = proc.uiBlock.load();
    if (pl != lastPlaying || q != lastQueued || ub != lastUiBlock)
    {
        lastPlaying = pl; lastQueued = q; lastUiBlock = ub;
        for (auto* b : patBtns) b->repaint();
    }

    const bool playing = proc.uiPlaying.load();
    const int act = proc.uiActiveDeck.load();
    const float target = ! playing ? 0.5f : (act == 0 ? 0.08f : (act == 1 ? 0.92f : 0.5f));
    xfSmooth += (target - xfSmooth) * 0.6f;
    content.repaint (0, 0, kW, 48);
    content.repaint (340, 150, 600, 36);
}

// ---------------------------------------------------------------------------
void BeatJugglingEditor::paint (Graphics& g)
{
    g.fillAll (bjc::bg);
}

void BeatJugglingEditor::resized()
{
    const float s = getWidth() / (float) kW;
    content.setBounds (0, 0, kW, kH);
    content.setTransform (AffineTransform::scale (s));
    layoutContent();
}

void BeatJugglingEditor::Content::paint (Graphics& g)
{
    g.fillAll (bjc::bg);
    auto& p = ed.proc;

    auto panelAt = [&g] (int x, int y, int w, int h, Colour border = Colours::transparentBlack)
    {
        g.setColour (bjc::panel);
        g.fillRoundedRectangle ((float) x, (float) y, (float) w, (float) h, 10.0f);
        if (! border.isTransparent())
        {
            g.setColour (border);
            g.drawRoundedRectangle ((float) x + 1, (float) y + 1, (float) w - 2, (float) h - 2, 10.0f, 2.0f);
        }
    };
    auto caption = [&g] (const String& t, int x, int y, int w, Colour c = bjc::dim)
    {
        g.setColour (c);
        g.setFont (FontOptions (11.0f, Font::bold));
        g.drawText (t, x, y, w, 16, Justification::centredLeft);
    };

    // en-tête
    g.setColour (bjc::header);
    g.fillRect (0, 0, kW, 48);
    g.setColour (bjc::panel2);
    g.drawHorizontalLine (48, 0, (float) kW);
    g.setColour (Colours::white);
    g.setFont (FontOptions (24.0f, Font::bold));
    g.drawText ("BEAT JUGGLING", 24, 0, 260, 48, Justification::centredLeft);
    g.setColour (bjc::dim);
    g.setFont (FontOptions (12.0f));
    g.drawText (U8 ("ANKS Labs · passe-passe séquencé"), 252, 0, 300, 48, Justification::centredLeft);
    const bool playing = p.uiPlaying.load();
    String status = playing ? U8 ("LECTURE · ") : U8 ("STOP · ");
    status << String (p.uiHostBpm.load(), 1) << " BPM";
    status << (p.internalPlay.load() ? U8 (" · play test") : U8 (" · calé sur Ableton"));
    g.setColour (playing ? bjc::green : bjc::dim);
    g.setFont (FontOptions (13.0f, Font::bold));
    g.drawText (status, 0, 0, kW - 24, 48, Justification::centredRight);

    // patterns
    panelAt (16, 56, 1248, 64);
    caption ("PATTERNS", 28, 80, 80);
    caption ("LONGUEUR (MESURES)", 912, 60, 200);

    // platines
    const int act = playing ? p.uiActiveDeck.load() : -1;
    panelAt (16, 128, 300, 268, act == 0 ? bjc::deckA.withAlpha (0.8f) : bjc::panel2);
    panelAt (964, 128, 300, 268, act == 1 ? bjc::deckB.withAlpha (0.8f) : bjc::panel2);
    g.setFont (FontOptions (13.0f, Font::bold));
    g.setColour (bjc::deckA);
    g.drawText ("PLATINE GAUCHE", 28, 134, 150, 18, Justification::centredLeft);
    g.setColour (bjc::deckB);
    g.drawText ("PLATINE DROITE", 976, 134, 150, 18, Justification::centredLeft);

    // mix
    panelAt (328, 128, 624, 268);
    caption ("MIX", 344, 134, 100);
    auto xf = Rectangle<float> (344.0f, 154.0f, 592.0f, 26.0f);
    g.setColour (Colour (0xff07090d));
    g.fillRoundedRectangle (xf, 13.0f);
    g.setFont (FontOptions (12.0f, Font::bold));
    g.setColour (bjc::deckA);
    g.drawText ("G", xf.withWidth (26.0f), Justification::centred);
    g.setColour (bjc::deckB);
    g.drawText ("D", xf.withTrimmedLeft (xf.getWidth() - 26.0f), Justification::centred);
    const float cx = xf.getX() + 30.0f + ed.xfSmooth * (xf.getWidth() - 60.0f);
    g.setColour (act >= 0 ? bjc::deck (act) : bjc::text);
    g.fillRoundedRectangle (Rectangle<float> (34.0f, 20.0f).withCentre ({ cx, xf.getCentreY() }), 4.0f);

    // vinyle
    panelAt (16, 404, 1248, 226);
    caption ("VINYLE", 28, 415, 60);

    // séquence
    panelAt (16, 638, 1248, 92);
    const auto& pat = ed.pattern();
    caption (U8 ("SÉQUENCE · PATTERN ") + String (ed.editPat + 1) + U8 (" · ") + String (pat.bars) + (pat.bars > 1 ? " MESURES" : " MESURE"), 28, 642, 400, bjc::text);

    // réglages
    panelAt (16, 738, 1248, 134, ed.selected() != nullptr ? bjc::red.withAlpha (0.5f) : Colours::transparentBlack);
}

void BeatJugglingEditor::layoutContent()
{
    // patterns
    for (int i = 0; i < patBtns.size(); ++i) patBtns[i]->setBounds (100 + i * 148, 64, 140, 48);
    patHint.setBounds (700, 64, 200, 48);
    for (int j = 0; j < barBtns.size(); ++j) barBtns[j]->setBounds (912 + j * 36, 80, 32, 28);
    remainLbl.setBounds (1134, 66, 128, 48);

    // platines
    for (int k = 0; k < 2; ++k)
    {
        const int x = k == 0 ? 16 : 964;
        (k == 0 ? fileL : fileR).setBounds (x + 140, 134, 152, 18);
        (k == 0 ? tableL : tableR).setBounds (x + 8, 156, 284, 170);
        (k == 0 ? loadL : loadR).setBounds (x + 10, 332, 136, 26);
        (k == 0 ? copyL : copyR).setBounds (x + 152, 332, 138, 26);
        (k == 0 ? bpmLblL : bpmLblR).setBounds (x + 10, 364, 70, 24);
        (k == 0 ? bpmL : bpmR).setBounds (x + 80, 364, 120, 24);
        (k == 0 ? halfL : halfR).setBounds (x + 204, 364, 40, 24);
        (k == 0 ? dblL : dblR).setBounds (x + 248, 364, 42, 24);
    }

    // mix
    const int ky = 194;
    Slider* knobs[] = { &gainA, &gainB, &volume, &xfade };
    Label* lbls[]   = { &gainALbl, &gainBLbl, &volumeLbl, &xfadeLbl };
    for (int i = 0; i < 4; ++i)
    {
        const int x = 372 + i * 140;
        knobs[i]->setBounds (x, ky, 84, 86);
        lbls[i]->setBounds (x - 8, ky + 88, 100, 16);
    }
    syncBtn.setBounds (360, 318, 180, 28);
    playBtn.setBounds (552, 316, 140, 32);
    tempoLbl.setBounds (708, 320, 72, 24);
    tempo.setBounds (778, 320, 150, 24);

    // vinyle
    waveDeckL.setBounds (92, 410, 74, 26);
    waveDeckR.setBounds (170, 410, 74, 26);
    waveInfo.setBounds (250, 410, 520, 26);
    calCombo.setBounds (776, 410, 132, 26);
    calBtn.setBounds (912, 410, 108, 26);
    zoomOut.setBounds (1030, 410, 30, 26);
    zoomIn.setBounds (1064, 410, 30, 26);
    zoomAllBtn.setBounds (1098, 410, 56, 26);
    zoomBlockBtn.setBounds (1158, 410, 96, 26);
    wave.setBounds (28, 442, 1224, 180);

    // séquence
    seqHint.setBounds (430, 642, 822, 16);
    seq.setBounds (28, 662, 1160, 62);
    addBtn.setBounds (1196, 662, 56, 46);

    // réglages du bloc
    emptyLbl.setBounds (28, 746, 1224, 118);
    blockTitle.setBounds (28, 746, 240, 26);
    blockInfo.setBounds (28, 774, 260, 50);
    leftBtn.setBounds (28, 834, 36, 28);
    rightBtn.setBounds (68, 834, 36, 28);
    dupBtn.setBounds (108, 834, 82, 28);
    delBtn.setBounds (194, 834, 86, 28);

    typeLbl.setBounds (300, 750, 150, 16);
    for (int i = 0; i < typeBtns.size(); ++i) typeBtns[i]->setBounds (300 + i * 76, 770, 72, 30);
    deckLbl.setBounds (460, 750, 150, 16);
    for (int i = 0; i < deckBtns.size(); ++i) deckBtns[i]->setBounds (460 + i * 74, 770, 70, 30);

    startKnob.setBounds (618, 748, 86, 96);
    startLbl.setBounds (612, 846, 98, 16);
    durKnob.setBounds (712, 748, 86, 96);
    durLbl.setBounds (706, 846, 98, 16);

    sensLbl.setBounds (816, 750, 150, 16);
    for (int i = 0; i < sensBtns.size(); ++i) sensBtns[i]->setBounds (816 + i * 76, 770, 72, 30);

    moveLbl.setBounds (816, 750, 150, 16);
    for (int i = 0; i < moveBtns.size(); ++i) moveBtns[i]->setBounds (816 + i * 68, 770, 64, 30);
    fromLbl.setBounds (816, 808, 150, 16);
    for (int i = 0; i < fromBtns.size(); ++i) fromBtns[i]->setBounds (816 + i * 114, 826, 110, 30);
    amountKnob.setBounds (1164, 748, 86, 96);
    amountLbl.setBounds (1158, 846, 98, 16);
}
