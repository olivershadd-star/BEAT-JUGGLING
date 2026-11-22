#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace bjc
{
    const juce::Colour bg       { 0xff0b0d12 };
    const juce::Colour panel    { 0xff151922 };
    const juce::Colour panel2   { 0xff1d2230 };
    const juce::Colour text     { 0xffd8e0ea };
    const juce::Colour dim      { 0xff7a8599 };
    const juce::Colour deckA    { 0xff29e0ff };
    const juce::Colour deckB    { 0xffff3d7f };
    const juce::Colour red      { 0xffff2a2a };
    const juce::Colour cue      { 0xffffd23f };
    inline juce::Colour deck (int d) { return d == 0 ? deckA : deckB; }
}

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
// Platine décorative : vinyle qui tourne, secteur rouge = longueur de la boucle
class TurntableView : public juce::Component
{
public:
    TurntableView (BeatJugglingProcessor& p, int deckIndex) : proc (p), deck (deckIndex) {}
    void paint (juce::Graphics&) override;
    void tick();
    std::function<void()> onClick;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    BeatJugglingProcessor& proc;
    int deck;
    double angle = 0.0;     // angle affiché (radians)
    float glow = 0.0f;
};

// ----------------------------------------------------------------------------
// Forme d'onde zoomable : sélection de la phrase, départ, grille
class WaveformView : public juce::Component
{
public:
    explicit WaveformView (BeatJugglingProcessor& p) : proc (p) {}

    void setDeck (int d);
    int getDeck() const { return deck; }
    void refresh();                 // appelé par le timer
    void zoom (double factor, double centreSample);
    void zoomAll();
    void zoomToSelection();
    bool snap = true;
    std::function<void()> onSelectionChanged;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scale) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    enum class Drag { none, pan, newSel, edgeStart, edgeEnd, overview };
    juce::Rectangle<int> mainArea() const { return getLocalBounds().withTrimmedBottom (overviewH); }
    juce::Rectangle<int> overviewArea() const { return getLocalBounds().removeFromBottom (overviewH); }
    double xToSample (float x) const;
    float sampleToX (double s) const;
    double beatLen() const;
    double snapSample (double s) const;
    void clampView();
    void drawWave (juce::Graphics&, juce::Rectangle<int> area, double start, double len, juce::Colour c);

    BeatJugglingProcessor& proc;
    int deck = 0;
    std::shared_ptr<DeckData> data;
    double viewStart = 0.0, viewLen = 1.0;
    Drag drag = Drag::none;
    double dragOrigin = 0.0, dragViewStart = 0.0;
    double lastPlayPos = -1.0;
    static constexpr int overviewH = 22;
};

// ----------------------------------------------------------------------------
// Timeline du programme (blocs A/B)
class ProgramView : public juce::Component
{
public:
    explicit ProgramView (BeatJugglingProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    BeatJugglingProcessor& proc;
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

private:
    void timerCallback() override;
    void setEditDeck (int d);
    void loadInto (int d, const juce::File& f);
    void updateDeckInfo();
    void programEdited();
    void insertMove (const juce::String& s);

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BeatJugglingProcessor& proc;
    BJLookAndFeel lnf;

    TurntableView tableA { proc, 0 }, tableB { proc, 1 };
    WaveformView wave { proc };
    ProgramView progView { proc };

    juce::Slider cueA, cueB, gainA, gainB, volume, xfade, tempo;
    juce::Label  cueALbl, cueBLbl, gainALbl, gainBLbl, volumeLbl, xfadeLbl, tempoLbl;
    std::unique_ptr<SliderAtt> cueAAtt, cueBAtt, gainAAtt, gainBAtt, volumeAtt, xfadeAtt, tempoAtt;

    juce::ToggleButton syncBtn { "Sync tempo" };
    std::unique_ptr<ButtonAtt> syncAtt;
    juce::TextButton playBtn { "PLAY" };

    juce::TextButton editA { "Platine A" }, editB { "Platine B" };
    juce::TextButton loadBtn { "Charger un son..." }, copyBtn { "Copier vers B" };
    juce::Label fileLbl;
    juce::Label bpmLbl { {}, "BPM du son" };
    juce::Slider bpmSlider;
    juce::TextButton halfBtn { "/2" }, doubleBtn { "x2" };
    juce::ToggleButton snapBtn { "Grille 1/16" };
    juce::Label calLbl { {}, "La selection fait" };
    juce::ComboBox calCombo;
    juce::TextButton calBtn { "Caler le BPM" };

    juce::TextButton zoomIn { "+" }, zoomOut { "-" }, zoomAllBtn { "Tout" }, zoomSelBtn { "Selection" };
    juce::Label waveInfo;

    juce::ComboBox presets;
    juce::TextEditor progEditor;
    juce::Label progError, helpLbl;
    juce::OwnedArray<juce::TextButton> moveBtns;

    std::unique_ptr<juce::FileChooser> chooser;
    int editDeck = 0;
    std::shared_ptr<DeckData> shownData[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatJugglingEditor)
};
