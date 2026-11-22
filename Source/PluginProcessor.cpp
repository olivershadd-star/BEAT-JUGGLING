#include "PluginProcessor.h"
#include "PluginEditor.h"

static const char* kDefaultProgram = "1 x4 | 1/2 x4 | 1/4 x4 1/2 1/2";

// ============================================================================
void DeckData::computePeaks()
{
    const int n = buffer.getNumSamples();
    const int blocks = (n + kPeakBlock - 1) / kPeakBlock;
    peakMin.assign ((size_t) blocks, 0.0f);
    peakMax.assign ((size_t) blocks, 0.0f);
    auto* l = buffer.getReadPointer (0);
    auto* r = buffer.getReadPointer (1);
    for (int b = 0; b < blocks; ++b)
    {
        float mn = 0.0f, mx = 0.0f;
        const int s0 = b * kPeakBlock, s1 = juce::jmin (n, s0 + kPeakBlock);
        for (int i = s0; i < s1; ++i)
        {
            const float v = 0.5f * (l[i] + r[i]);
            mn = juce::jmin (mn, v);
            mx = juce::jmax (mx, v);
        }
        peakMin[(size_t) b] = mn;
        peakMax[(size_t) b] = mx;
    }
}

// ============================================================================
BeatJugglingProcessor::BeatJugglingProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formats.registerBasicFormats();
    for (auto& p : uiDeckPos)   p = 0.0;
    for (auto& v : uiDeckSpeed) v = 0.0;
    setProgramText (kDefaultProgram);
}

juce::AudioProcessorValueTreeState::ParameterLayout BeatJugglingProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
                                                  NormalisableRange<float> (-30.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "xfade", 1 }, "Crossfade",
                                                  NormalisableRange<float> (0.5f, 30.0f, 0.1f, 0.5f), 4.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "sync", 1 }, "Sync tempo", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "cueA", 1 }, "Depart A",
                                                  NormalisableRange<float> (0.0f, 63.0f, 1.0f), 0.0f, AudioParameterFloatAttributes().withLabel ("/16")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "cueB", 1 }, "Depart B",
                                                  NormalisableRange<float> (0.0f, 63.0f, 1.0f), 0.0f, AudioParameterFloatAttributes().withLabel ("/16")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "gainA", 1 }, "Volume A",
                                                  NormalisableRange<float> (-30.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "gainB", 1 }, "Volume B",
                                                  NormalisableRange<float> (-30.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "Tempo interne",
                                                  NormalisableRange<float> (50.0f, 200.0f, 0.1f), 90.0f, AudioParameterFloatAttributes().withLabel ("BPM")));
    return l;
}

bool BeatJugglingProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void BeatJugglingProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    outGain.reset (sampleRate, 0.02);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("volume")->load()));
}

// ============================================================================
double BeatJugglingProcessor::guessBpm (double len)
{
    if (len <= 0.0) return 90.0;
    for (double beats : { 4.0, 8.0, 16.0, 2.0, 32.0, 1.0, 64.0, 3.0, 6.0, 12.0 })
    {
        const double bpm = beats * 60.0 / len;
        if (bpm >= 70.0 && bpm <= 150.0) return bpm;
    }
    return 90.0;
}

bool BeatJugglingProcessor::loadFile (int deck, const juce::File& f, bool alsoOtherIfEmpty)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));
    if (reader == nullptr) return false;

    const auto maxLen = (juce::int64) (reader->sampleRate * 60.0 * 15.0); // 15 minutes max
    const int len = (int) juce::jmin (reader->lengthInSamples, maxLen);
    if (len < 64) return false;

    auto d = std::make_shared<DeckData>();
    d->buffer.setSize (2, len);
    d->buffer.clear();
    reader->read (&d->buffer, 0, len, 0, true, reader->numChannels > 1);
    if (reader->numChannels == 1) d->buffer.copyFrom (1, 0, d->buffer, 0, 0, len);
    d->sampleRate = reader->sampleRate;
    d->file = f;
    d->computePeaks();

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        decks[deck] = d;
        if (alsoOtherIfEmpty && decks[1 - deck] == nullptr) decks[1 - deck] = d;
    }
    applyLoadedDefaults (deck);
    if (alsoOtherIfEmpty && decks[1 - deck] == d) copyDeck (deck, 1 - deck);
    return true;
}

void BeatJugglingProcessor::applyLoadedDefaults (int deck)
{
    auto d = getDeck (deck);
    if (d == nullptr) return;
    auto& s = deckSettings[deck];
    s.selStart = 0.0;
    s.selEnd   = (double) d->numSamples();
    s.anchor   = 0.0;
    s.fileBpm  = guessBpm (d->numSamples() / d->sampleRate);
}

void BeatJugglingProcessor::copyDeck (int from, int to)
{
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        decks[to] = decks[from];
    }
    auto& a = deckSettings[from];
    auto& b = deckSettings[to];
    b.selStart = a.selStart.load();
    b.selEnd   = a.selEnd.load();
    b.fileBpm  = a.fileBpm.load();
    b.anchor   = a.anchor.load();
}

std::shared_ptr<DeckData> BeatJugglingProcessor::getDeck (int deck)
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return decks[deck];
}

void BeatJugglingProcessor::setProgramText (const juce::String& text)
{
    programText = text;
    auto p = std::make_shared<bj::Program> (bj::parseProgram (text));
    if (p->error.isNotEmpty())
    {
        // on garde l'ancien programme valide, mais on mémorise l'erreur pour l'affichage
        auto keep = getProgram();
        auto err = std::make_shared<bj::Program> (keep ? *keep : bj::Program());
        err->error = p->error;
        const juce::SpinLock::ScopedLockType sl (lock);
        program = err;
        return;
    }
    const juce::SpinLock::ScopedLockType sl (lock);
    program = p;
}

std::shared_ptr<const bj::Program> BeatJugglingProcessor::getProgram()
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return program;
}

// ============================================================================
void BeatJugglingProcessor::renderStep (const bj::Step& s, double tl, const DeckBlock* db,
                                        float& L, float& R, double& posOut, double& speedOut) const
{
    L = R = 0.0f;
    const auto& d = db[s.deck];
    if (s.silent || d.data == nullptr) { speedOut = 0.0; return; }

    const auto ms = bj::evalMove (s, tl);
    const double cue16 = s.cue16 >= 0.0 ? s.cue16 : d.cue16;
    const double cuePos = d.selS + cue16 * d.fileSpb * 0.25;
    double pos = cuePos + (s.offset + ms.u) * d.spb;

    const int len = d.b - d.a;
    double rel = std::fmod (pos - d.a, (double) len);
    if (rel < 0) rel += len;
    pos = d.a + rel;
    posOut = pos;
    speedOut = ms.v * d.speedScale;

    const int i1 = (int) rel;
    const float t = (float) (rel - i1);
    auto wrap = [len] (int k) { k %= len; return k < 0 ? k + len : k; };
    const int i0 = d.a + wrap (i1 - 1), j1 = d.a + wrap (i1), i2 = d.a + wrap (i1 + 1), i3 = d.a + wrap (i1 + 2);

    const float g = (float) (ms.gain * d.gain);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* x = d.data->buffer.getReadPointer (ch);
        const float y0 = x[i0], y1 = x[j1], y2 = x[i2], y3 = x[i3];
        // interpolation Hermite
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        const float v = ((c3 * t + c2) * t + c1) * t + y1;
        (ch == 0 ? L : R) = v * g;
    }
}

void BeatJugglingProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    buffer.clear();

    // ---- transport ----
    bool playing = false;
    double ppq = 0.0;
    double bpm = apvts.getRawParameterValue ("bpm")->load();
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    bpm = juce::jlimit (20.0, 400.0, bpm);
    uiHostBpm = bpm;

    if (! playing && internalPlay.load())
    {
        if (! wasPlaying) internalPpq = 0.0;
        ppq = internalPpq;
        internalPpq += n * bpm / 60.0 / sr;
        playing = true;
    }
    wasPlaying = playing;
    uiPlaying = playing;
    if (! playing) { uiDeckSpeed[0] = 0.0; uiDeckSpeed[1] = 0.0; uiStep = -1; return; }

    const juce::SpinLock::ScopedTryLockType sl (lock);
    if (! sl.isLocked()) return;

    DeckData* dd[2] = { decks[0].get(), decks[1].get() };
    if (dd[0] == nullptr) dd[0] = dd[1];
    if (dd[1] == nullptr) dd[1] = dd[0];
    const bj::Program* prog = program.get();
    if (dd[0] == nullptr || prog == nullptr || prog->steps.empty() || prog->total <= 0.0) return;

    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    DeckBlock db[2];
    for (int k = 0; k < 2; ++k)
    {
        auto& b = db[k];
        b.data = dd[k];
        auto& st = deckSettings[(decks[k] != nullptr) ? k : 1 - k];
        const int ns = b.data->numSamples();
        b.selS = juce::jlimit (0.0, (double) ns - 1, st.selStart.load());
        b.selE = juce::jlimit (b.selS + 1, (double) ns, st.selEnd.load());
        if (b.selE - b.selS < 32) { b.selS = 0; b.selE = ns; }
        b.a = (int) b.selS;
        b.b = juce::jmax (b.a + 4, juce::jmin (ns, (int) std::ceil (b.selE)));
        const double fbpm = juce::jlimit (20.0, 400.0, st.fileBpm.load());
        b.fileSpb = 60.0 / fbpm * b.data->sampleRate;
        b.spb = sync ? b.fileSpb : 60.0 / bpm * b.data->sampleRate;
        b.speedScale = sync ? bpm / fbpm : 1.0;
        b.cue16 = apvts.getRawParameterValue (k == 0 ? "cueA" : "cueB")->load();
        b.gain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (k == 0 ? "gainA" : "gainB")->load());
    }

    const double bps = bpm / 60.0 / sr;          // temps par échantillon
    const double xfBeats = juce::jmax (1.0e-6, apvts.getRawParameterValue ("xfade")->load() * bpm / 60000.0);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("volume")->load()));

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const auto& steps = prog->steps;
    const int nSteps = (int) steps.size();
    int idx = -1;
    double lastPos = 0, lastSpeed = 0;
    int lastDeck = 0;

    for (int i = 0; i < n; ++i)
    {
        double p = std::fmod (ppq + i * bps, prog->total);
        if (p < 0) p += prog->total;
        if (idx < 0 || p < steps[(size_t) idx].start || p >= steps[(size_t) idx].start + steps[(size_t) idx].dur)
            idx = prog->find (p);

        const auto& s = steps[(size_t) idx];
        const double tl = p - s.start;
        float L, R;
        double pos = 0, speed = 0;
        renderStep (s, tl, db, L, R, pos, speed);

        if (tl < xfBeats)
        {
            const auto& ps = steps[(size_t) ((idx - 1 + nSteps) % nSteps)];
            float pl, pr; double pp, pv;
            renderStep (ps, ps.dur + tl, db, pl, pr, pp, pv);
            const float x = (float) (tl / xfBeats);
            const float fin  = std::sin (x * juce::MathConstants<float>::halfPi);
            const float fout = std::cos (x * juce::MathConstants<float>::halfPi);
            L = L * fin + pl * fout;
            R = R * fin + pr * fout;
        }

        const float g = outGain.getNextValue();
        if (outR != nullptr) { outL[i] = L * g; outR[i] = R * g; }
        else outL[i] = 0.5f * (L + R) * g;

        lastPos = pos; lastSpeed = speed; lastDeck = s.deck;
        if (i == n - 1) { uiStep = idx; uiProgramPos = p; uiActiveDeck = s.silent ? -1 : s.deck; }
    }

    // affichage : la platine active tourne, l'autre est "ramenée" à son départ
    uiDeckPos[lastDeck] = lastPos;
    uiDeckSpeed[lastDeck] = lastSpeed;
    const int other = 1 - lastDeck;
    uiDeckPos[other] = db[other].selS + db[other].cue16 * db[other].fileSpb * 0.25;
    uiDeckSpeed[other] = 0.0;
}

// ============================================================================
void BeatJugglingProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("BeatJuggling");
    if (auto params = apvts.copyState().createXml()) root.addChildElement (params.release());
    root.setAttribute ("program", programText);
    for (int k = 0; k < 2; ++k)
    {
        auto* e = root.createNewChildElement ("Deck");
        e->setAttribute ("index", k);
        auto d = getDeck (k);
        e->setAttribute ("file", d ? d->file.getFullPathName() : juce::String());
        auto& s = deckSettings[k];
        e->setAttribute ("selStart", s.selStart.load());
        e->setAttribute ("selEnd", s.selEnd.load());
        e->setAttribute ("bpm", s.fileBpm.load());
        e->setAttribute ("anchor", s.anchor.load());
    }
    copyXmlToBinary (root, destData);
}

void BeatJugglingProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("BeatJuggling")) return;
    if (auto* params = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));
    setProgramText (xml->getStringAttribute ("program", kDefaultProgram));

    for (auto* e : xml->getChildWithTagNameIterator ("Deck"))
    {
        const int k = juce::jlimit (0, 1, e->getIntAttribute ("index"));
        const juce::File f (e->getStringAttribute ("file"));
        if (f.existsAsFile())
        {
            auto other = getDeck (1 - k);
            if (k == 1 && other != nullptr && other->file == f) copyDeck (0, 1); // même fichier : on partage
            else loadFile (k, f, false);
        }
        auto& s = deckSettings[k];
        s.selStart = e->getDoubleAttribute ("selStart", s.selStart.load());
        s.selEnd   = e->getDoubleAttribute ("selEnd", s.selEnd.load());
        s.fileBpm  = e->getDoubleAttribute ("bpm", s.fileBpm.load());
        s.anchor   = e->getDoubleAttribute ("anchor", s.anchor.load());
    }
}

juce::AudioProcessorEditor* BeatJugglingProcessor::createEditor() { return new BeatJugglingEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BeatJugglingProcessor(); }
