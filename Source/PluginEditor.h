#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace bjc
{
    const juce::Colour bg       { 0xff0b0d12 };
    const juce::Colour header   { 0xff11151d };
    const juce::Colour panel    { 0xff151922 };
    const juce::Colour panel2   { 0xff1d2230 };
    const juce::Colour line     { 0xff2a3142 };
    const juce::Colour text     { 0xffd8e0ea };
    const juce::Colour dim      { 0xff8a95a8 };
    const juce::Colour deckA    { 0xff29e0ff };   // Gauche
    const juce::Colour deckB    { 0xffff3d7f };   // Droite
    const juce::Colour red      { 0xffff2a2a };
    const juce::Colour cue      { 0xffffd23f };
    const juce::Colour green    { 0xff3bdc7a };
    inline juce::Colour deck (int d) { return d == 0 ? deckA : deckB; }
}

class BeatJugglingEditor;

// ----------------------------------------------------------------------------
class BJLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BJLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
};

// ----------------------------------------------------------------------------
// Platine décorative : vinyle qui tourne, secteur rouge = zone jouée
class TurntableView : public juce::Component
{
public:
    TurntableView (BeatJugglingProcessor& p, int deckIndex) : proc (p), deck (deckIndex) {}
    void paint (juce::Graphics&) override;
    void tick();
    std::function<void()> onClick;
    std::function<bool (double& zoneStart, double& zoneLen)> getZone;   // zone à afficher (échantillons)
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
private:
    BeatJugglingProcessor& proc;
    int deck;
    double angle = 0.0;
    float glow = 0.0f;
};

// ----------------------------------------------------------------------------
// Bouton de pattern (deux lignes : nom + état)
class PatternButton : public juce::Component
{
public:
    PatternButton (BeatJugglingProcessor& p, int i) : proc (p), index (i) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override { if (e.mouseWasClicked() && onClick) onClick (index); }
    std::function<void (int)> onClick;
    bool editing = false;
private:
    BeatJugglingProcessor& proc;
    int index;
};

// ----------------------------------------------------------------------------
// Vinyle (forme d'onde) : les zones des blocs de lecture de cette platine
class WaveformView : public juce::Component
{
public:
    explicit WaveformView (BeatJugglingEditor& e);

    void setDeck (int d);
    int getDeck() const { return deck; }
    void refresh();
    void zoom (double factor, double centreSample);
    void zoomAll();
    void zoomToBlock();
    bool snap = true;
    double measureStart = -1, measureEnd = -1;   // zone verte (Alt + glisser) pour caler le BPM

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scale) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    enum class Drag { none, pan, move, edgeStart, edgeEnd, overview, measure };
    juce::Rectangle<int> mainArea() const { return getLocalBounds().withTrimmedBottom (overviewH); }
    juce::Rectangle<int> overviewArea() const { return getLocalBounds().removeFromBottom (overviewH); }
    double xToSample (float x) const;
    float sampleToX (double s) const;
    double sixteenth() const;
    double anchor() const;
    void clampView();
    void drawWave (juce::Graphics&, juce::Rectangle<int> area, double start, double len, juce::Colour c);
    int blockAt (float x) const;   // bloc de lecture de cette platine sous la souris

    BeatJugglingEditor& ed;
    BeatJugglingProcessor& proc;
    int deck = 0;
    std::shared_ptr<DeckData> data;
    double viewStart = 0.0, viewLen = 1.0;
    Drag drag = Drag::none;
    double dragX = 0.0, dragViewStart = 0.0, dragSample = 0.0;
    int dragStart16 = 0, dragLen16 = 0;
    static constexpr int overviewH = 22;
};

// ----------------------------------------------------------------------------
// Séquence du pattern : blocs proportionnels, zoomable
class SequenceView : public juce::Component
{
public:
    explicit SequenceView (BeatJugglingEditor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void resetZoom() { viewStart = 0; viewLen = -1; }
private:
    double total() const;
    double shownLen() const { return viewLen > 0 ? viewLen : total(); }
    float beatToX (double b) const;
    double xToBeat (float x) const;
    void clampView();
    BeatJugglingEditor& ed;
    double viewStart = 0, viewLen = -1;   // en temps ; -1 = tout le pattern
    float panX = 0; double panStart = 0;
    bool panning = false;
};

// ----------------------------------------------------------------------------
class BeatJugglingEditor : public juce::AudioProcessorEditor,
                           public juce::FileDragAndDropTarget,
                           private juce::Timer
{
public:
    explicit BeatJugglingEditor (BeatJugglingProcessor&);
    ~BeatJugglingEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // utilisé par les vues
    BeatJugglingProcessor& proc;
    int editPat = 0;
    int selBlock = 0;
    const bj::Pattern& pattern() const { return proc.getPattern (editPat); }
    const bj::Block* selected() const;
    void selectBlock (int i);
    void modifyBlock (int i, const std::function<void (bj::Block&)>& fn);
    void blocksChanged();
    int resolvedDeck (int blockIndex) const;   // platine réellement jouée par un bloc

    static constexpr int kW = 1280, kH = 880;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (BeatJugglingEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override;
    private:
        BeatJugglingEditor& ed;
    };

    void timerCallback() override;
    void loadInto (int d, const juce::File& f);
    void updateDeckInfo();
    void updateBlockEditor();
    void updatePatternInfo();
    void layoutContent();
    void addBlock();
    void moveSelected (int delta);
    void duplicateSelected();
    void deleteSelected();

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BJLookAndFeel lnf;
    Content content { *this };   // tout est dessiné à 1280 x 880 puis mis à l'échelle

    // patterns
    juce::OwnedArray<PatternButton> patBtns;
    juce::OwnedArray<juce::TextButton> barBtns;
    juce::Label patHint, remainLbl;

    // platines
    TurntableView tableL { proc, 0 }, tableR { proc, 1 };
    juce::TextButton loadL { "Charger un son..." }, loadR { "Charger un son..." };
    juce::TextButton copyL, copyR;
    juce::Label fileL, fileR, bpmLblL, bpmLblR;
    juce::Slider bpmL, bpmR;
    juce::TextButton halfL { "/2" }, dblL { "x2" }, halfR { "/2" }, dblR { "x2" };

    // mix
    juce::Slider gainA, gainB, volume, xfade, tempo;
    juce::Label gainALbl, gainBLbl, volumeLbl, xfadeLbl, tempoLbl;
    std::unique_ptr<SliderAtt> gainAAtt, gainBAtt, volumeAtt, xfadeAtt, tempoAtt;
    juce::ToggleButton syncBtn { "Sync tempo Ableton" };
    std::unique_ptr<ButtonAtt> syncAtt;
    juce::TextButton playBtn;

    // vinyle
    WaveformView wave { *this };
    juce::TextButton zoomIn { "+" }, zoomOut { "-" }, zoomAllBtn { "Tout" }, zoomBlockBtn { "Bloc" };
    juce::TextButton waveDeckL { "Gauche" }, waveDeckR { "Droite" };
    juce::ToggleButton snapBtn { "Grille 1/16" };
    juce::Label waveInfo;
    juce::ComboBox calCombo;
    juce::TextButton calBtn { "Caler le BPM" };

    // séquence
    SequenceView seq { *this };
    juce::TextButton addBtn { "+" };
    juce::Label seqHint;

    // réglages du bloc
    juce::Label blockTitle, blockInfo, typeLbl, deckLbl, sensLbl, moveLbl, fromLbl;
    juce::OwnedArray<juce::TextButton> typeBtns, deckBtns, sensBtns, moveBtns, fromBtns;
    juce::Slider startKnob, durKnob, amountKnob;
    juce::Label startLbl, durLbl, amountLbl, emptyLbl;
    juce::TextButton leftBtn, rightBtn, dupBtn { "Dupliquer" }, delBtn { "Supprimer" };

    std::unique_ptr<juce::FileChooser> chooser;
    std::shared_ptr<DeckData> shownData[2];
    int lastPlaying = -1, lastQueued = -2, lastUiBlock = -2;
public:
    float xfSmooth = 0.5f;
private:
    bool updating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatJugglingEditor)
};
