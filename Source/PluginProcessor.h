#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <array>
#include <functional>
#include <memory>
#include "Pattern.h"

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

// Réglages d'une platine
struct DeckSettings
{
    std::atomic<double> fileBpm { 90.0 };
    std::atomic<double> anchor  { 0.0 };   // position du 1er temps (échantillons)
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
    static double guessBpm (double lengthSeconds);

    // patterns : lecture libre depuis le thread message, modification via editPattern
    const bj::Pattern& getPattern (int i) const { return patterns[(size_t) i]; }
    void editPattern (int i, const std::function<void (bj::Pattern&)>& fn);
    void requestPattern (int i);    // clic : enchaîne à la fin du pattern en cours
    std::atomic<int> playingPattern { 0 };
    std::atomic<int> queuedPattern { -1 };

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- infos temps réel pour l'affichage ----
    std::atomic<bool>   uiPlaying { false };
    std::atomic<int>    uiActiveDeck { -1 };
    std::atomic<int>    uiBlock { -1 };
    std::atomic<double> uiPatternPos { 0.0 };   // en temps
    std::atomic<double> uiDeckPos[2];           // position lecture (échantillons)
    std::atomic<double> uiZoneStart[2];         // zone jouée (échantillons)
    std::atomic<double> uiZoneLen[2];
    std::atomic<bool>   internalPlay { false };
    std::atomic<double> uiHostBpm { 120.0 };

private:
    struct DeckBlock
    {
        DeckData* data = nullptr;
        double fileSpb = 1, spb = 1, gain = 1, speedScale = 1, anchor = 0;
    };
    int buildSteps (const bj::Pattern& p, const DeckBlock* db, double fadeBeats);
    void renderStep (const bj::RenderStep& s, double tl, const DeckBlock* db, double fadeBeats,
                     float& L, float& R, double& posOut) const;
    void applyLoadedDefaults (int deck);
    static void setDefaultPattern (bj::Pattern& p);

    juce::AudioFormatManager formats;
    juce::SpinLock lock;
    std::shared_ptr<DeckData> decks[2];
    std::array<bj::Pattern, bj::kNumPatterns> patterns;
    DeckSettings deckSettings[2];

    // état du thread audio
    std::array<bj::RenderStep, bj::kMaxBlocks + 2> steps;
    int numSteps = 0;
    double sr = 44100.0;
    double internalPpq = 0.0;
    bool wasPlaying = false;
    double patternStart = 0.0;     // en temps (ppq) : début du pattern en cours
    double expectedPpq = -1.0;
    int stepIdx = 0;
    double curStepAbs = -1.0;
    bj::RenderStep curStep, prevStep;
    bool hasPrev = false;
    juce::SmoothedValue<float> outGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatJugglingProcessor)
};
