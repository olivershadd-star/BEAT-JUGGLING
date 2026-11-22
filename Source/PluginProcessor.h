#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include "Program.h"

// Données audio d'une platine (immuables une fois chargées)
struct DeckData
{
    juce::AudioBuffer<float> buffer;    // toujours 2 canaux
    double sampleRate = 44100.0;
    juce::File file;
    std::vector<float> peakMin, peakMax; // un couple par bloc de kPeakBlock échantillons
    static constexpr int kPeakBlock = 64;

    int numSamples() const { return buffer.getNumSamples(); }
    void computePeaks();
};

// Réglages d'une platine modifiables depuis l'interface
struct DeckSettings
{
    std::atomic<double> selStart { 0.0 };   // en échantillons du fichier
    std::atomic<double> selEnd   { 0.0 };
    std::atomic<double> fileBpm  { 90.0 };
    std::atomic<double> anchor   { 0.0 };   // position du premier temps (échantillons)
};

class BeatJugglingProcessor : public juce::AudioProcessor
{
public:
    BeatJugglingProcessor();
    ~BeatJugglingProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Beat Juggling"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- API pour l'interface (thread message) ----
    bool loadFile (int deck, const juce::File& f, bool alsoOtherIfEmpty);
    void copyDeck (int from, int to);
    std::shared_ptr<DeckData> getDeck (int deck);
    DeckSettings& settings (int deck) { return deckSettings[deck]; }

    void setProgramText (const juce::String& text);
    juce::String getProgramText() const { return programText; }
    std::shared_ptr<const bj::Program> getProgram();

    static double guessBpm (double lengthSeconds);

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- infos temps réel pour l'affichage ----
    std::atomic<bool>   uiPlaying { false };
    std::atomic<int>    uiActiveDeck { 0 };
    std::atomic<int>    uiStep { -1 };
    std::atomic<double> uiProgramPos { 0.0 };
    std::atomic<double> uiDeckPos[2];    // position lecture en échantillons du fichier
    std::atomic<double> uiDeckSpeed[2];  // vitesse (1 = normal)
    std::atomic<bool>   internalPlay { false };
    std::atomic<double> uiHostBpm { 120.0 };

private:
    struct DeckBlock
    {
        DeckData* data = nullptr;
        double selS = 0, selE = 0, fileSpb = 1, spb = 1, cue16 = 0, gain = 1, speedScale = 1;
        int a = 0, b = 0;
    };
    void renderStep (const bj::Step& s, double tl, const DeckBlock* db,
                     float& L, float& R, double& posOut, double& speedOut) const;
    void applyLoadedDefaults (int deck);

    juce::AudioFormatManager formats;
    juce::SpinLock lock;
    std::shared_ptr<DeckData> decks[2];
    std::shared_ptr<const bj::Program> program;
    juce::String programText;
    DeckSettings deckSettings[2];

    double sr = 44100.0;
    double internalPpq = 0.0;
    bool wasPlaying = false;
    juce::SmoothedValue<float> outGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatJugglingProcessor)
};
