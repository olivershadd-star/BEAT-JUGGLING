#include "PluginProcessor.h"
#include "PluginEditor.h"

using bj::Block;
using bj::BlockType;
using bj::Move;

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
    for (int k = 0; k < 2; ++k) { uiDeckPos[k] = 0.0; uiZoneStart[k] = 0.0; uiZoneLen[k] = 0.0; }
    setDefaultPattern (patterns[0]);
    for (int i = 1; i < bj::kNumPatterns; ++i) patterns[(size_t) i].bars = 4;
}

void BeatJugglingProcessor::setDefaultPattern (bj::Pattern& p)
{
    // 4 mesures : 1 mesure / 1 mesure / 2 temps / 2 temps / 1 temps / 1 temps / stop / scratch
    p.bars = 4;
    p.blocks.clear();
    auto play = [&p] (int deck, int start16, int len16) { Block b; b.type = BlockType::play; b.deck = deck; b.start16 = start16; b.len16 = len16; p.blocks.push_back (b); };
    auto move = [&p] (Move m, int len16, float a) { Block b; b.type = BlockType::move; b.move = m; b.len16 = len16; b.amount = a; b.fromPrev = true; p.blocks.push_back (b); };
    play (0, 0, 16);    // Gauche : mesure 1
    play (1, 16, 16);   // Droite : mesure 2
    play (0, 32, 8);    // Gauche : 2 temps à partir de la mesure 3
    play (1, 36, 8);    // Droite : 2 temps à partir du 2e temps de la mesure 3
    play (0, 48, 4);    // Gauche : 1 temps (mesure 4)
    play (1, 52, 4);    // Droite : 1 temps (2e temps de la mesure 4)
    move (Move::stop, 4, 0.4f);
    move (Move::scratch, 4, 0.4f);
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
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "gainA", 1 }, "Volume Gauche",
                                                  NormalisableRange<float> (-30.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "gainB", 1 }, "Volume Droite",
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
    for (double beats : { 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 2.0, 1.0, 3.0, 6.0, 12.0 })
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

    bool shareOther = false;
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        decks[deck] = d;
        if (alsoOtherIfEmpty && decks[1 - deck] == nullptr) { decks[1 - deck] = d; shareOther = true; }
    }
    applyLoadedDefaults (deck);
    if (shareOther) copyDeck (deck, 1 - deck);
    return true;
}

void BeatJugglingProcessor::applyLoadedDefaults (int deck)
{
    auto d = getDeck (deck);
    if (d == nullptr) return;
    auto& s = deckSettings[deck];
    s.anchor  = 0.0;
    s.fileBpm = guessBpm (d->numSamples() / d->sampleRate);
}

void BeatJugglingProcessor::copyDeck (int from, int to)
{
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        decks[to] = decks[from];
    }
    deckSettings[to].fileBpm = deckSettings[from].fileBpm.load();
    deckSettings[to].anchor  = deckSettings[from].anchor.load();
}

std::shared_ptr<DeckData> BeatJugglingProcessor::getDeck (int deck)
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return decks[deck];
}

void BeatJugglingProcessor::editPattern (int i, const std::function<void (bj::Pattern&)>& fn)
{
    auto copy = patterns[(size_t) i];
    fn (copy);
    if ((int) copy.blocks.size() > bj::kMaxBlocks) copy.blocks.resize ((size_t) bj::kMaxBlocks);
    copy.bars = juce::jlimit (1, 32, copy.bars);
    for (auto& b : copy.blocks)
    {
        b.len16 = juce::jlimit (1, 32 * 16, b.len16);
        b.start16 = juce::jmax (0, b.start16);
        b.deck = juce::jlimit (0, 1, b.deck);
        b.amount = juce::jlimit (0.0f, 1.0f, b.amount);
    }
    const juce::SpinLock::ScopedLockType sl (lock);
    std::swap (patterns[(size_t) i], copy);   // l'ancien est libéré hors du verrou
}

void BeatJugglingProcessor::requestPattern (int i)
{
    i = juce::jlimit (0, bj::kNumPatterns - 1, i);
    if (! uiPlaying.load())
    {
        playingPattern = i;
        queuedPattern = -1;
        return;
    }
    queuedPattern = (i == playingPattern.load()) ? -1 : i;
}

// ============================================================================
int BeatJugglingProcessor::buildSteps (const bj::Pattern& p, const DeckBlock* db, double fadeBeats)
{
    const double total = p.totalBeats();
    double t = 0;
    int n = 0;
    bool hasLast = false;
    int lastDeck = 0;
    double lastEnd = 0;

    for (int i = 0; i < (int) p.blocks.size() && n < bj::kMaxBlocks; ++i)
    {
        const auto& b = p.blocks[(size_t) i];
        if (t >= total - 1.0e-9) break;
        double dur = b.len16 / 4.0;
        if (t + dur > total) dur = total - t;          // déborde : raccourci

        bj::RenderStep s;
        s.start = t; s.dur = dur; s.silent = false; s.block = i; s.amount = b.amount;

        if (b.type == BlockType::play)
        {
            s.deck = b.deck;
            const auto& d = db[s.deck];
            const double zone = d.anchor + b.start16 * d.fileSpb * 0.25;
            s.isMove = false;
            s.dir = b.reverse ? -1.0 : 1.0;
            s.base = b.reverse ? zone + dur * d.spb : zone;
            s.zoneStart = zone;
            s.zoneLen = dur * d.spb;
            lastEnd = s.base + s.dir * dur * d.spb;
        }
        else
        {
            s.isMove = true;
            s.move = b.move;
            if (b.fromPrev && hasLast) { s.deck = lastDeck; s.base = lastEnd; }
            else
            {
                s.deck = b.deck;
                s.base = db[s.deck].anchor + b.start16 * db[s.deck].fileSpb * 0.25;
            }
            const auto ms = bj::evalMove (s.move, s.amount, dur, dur, fadeBeats);
            s.zoneStart = juce::jmin (s.base, s.base + ms.u * db[s.deck].spb);
            s.zoneLen = std::abs (ms.u * db[s.deck].spb);
            lastEnd = s.base + ms.u * db[s.deck].spb;
        }
        hasLast = true;
        lastDeck = s.deck;
        t += dur;
        steps[(size_t) n++] = s;
    }

    if (t < total - 1.0e-9 || n == 0)
    {
        bj::RenderStep s;
        s.start = t; s.dur = total - t; s.silent = true; s.block = -1;
        steps[(size_t) n++] = s;
    }
    return n;
}

void BeatJugglingProcessor::renderStep (const bj::RenderStep& s, double tl, const DeckBlock* db, double fadeBeats,
                                        float& L, float& R, double& posOut) const
{
    L = R = 0.0f;
    if (s.silent) return;
    const auto& d = db[s.deck];
    if (d.data == nullptr) return;

    double u, g;
    if (s.isMove)
    {
        const auto ms = bj::evalMove (s.move, s.amount, s.dur, tl, fadeBeats);
        u = ms.u; g = ms.gain;
    }
    else { u = s.dir * tl; g = 1.0; }

    const int len = d.data->numSamples();
    double pos = std::fmod (s.base + u * d.spb, (double) len);
    if (pos < 0) pos += len;
    posOut = pos;

    const int i1 = (int) pos;
    const float t = (float) (pos - i1);
    auto wrap = [len] (int k) { k %= len; return k < 0 ? k + len : k; };
    const int i0 = wrap (i1 - 1), j1 = wrap (i1), i2 = wrap (i1 + 1), i3 = wrap (i1 + 2);

    const float gg = (float) (g * d.gain);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* x = d.data->buffer.getReadPointer (ch);
        const float y0 = x[i0], y1 = x[j1], y2 = x[i2], y3 = x[i3];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        const float v = ((c3 * t + c2) * t + c1) * t + y1;
        (ch == 0 ? L : R) = v * gg;
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
    const bool justStarted = playing && ! wasPlaying;
    wasPlaying = playing;
    uiPlaying = playing;
    if (! playing) { uiBlock = -1; uiActiveDeck = -1; hasPrev = false; expectedPpq = -1.0; return; }

    const juce::SpinLock::ScopedTryLockType sl (lock);
    if (! sl.isLocked()) return;

    DeckData* dd[2] = { decks[0].get(), decks[1].get() };
    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    DeckBlock db[2];
    for (int k = 0; k < 2; ++k)
    {
        auto& b = db[k];
        b.data = dd[k];
        if (b.data == nullptr) continue;
        auto& st = deckSettings[k];
        const double fbpm = juce::jlimit (20.0, 400.0, st.fileBpm.load());
        b.anchor = st.anchor.load();
        b.fileSpb = 60.0 / fbpm * b.data->sampleRate;
        b.spb = sync ? b.fileSpb : 60.0 / bpm * b.data->sampleRate;
        b.speedScale = sync ? bpm / fbpm : 1.0;
        b.gain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue (k == 0 ? "gainA" : "gainB")->load());
    }

    const double bps = bpm / 60.0 / sr;
    const double xfBeats = juce::jmax (1.0e-6, apvts.getRawParameterValue ("xfade")->load() * bpm / 60000.0);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("volume")->load()));

    // ---- position dans le pattern ----
    int cur = juce::jlimit (0, bj::kNumPatterns - 1, playingPattern.load());
    double len = patterns[(size_t) cur].totalBeats();
    const bool jumped = expectedPpq >= 0.0 && std::abs (ppq - expectedPpq) > 0.01;
    if (justStarted || jumped || expectedPpq < 0.0 || ppq < patternStart)
    {
        const int q = queuedPattern.exchange (-1);
        if (q >= 0) { cur = q; playingPattern = q; len = patterns[(size_t) cur].totalBeats(); }
        patternStart = std::floor (ppq / len) * len;
        hasPrev = false;
        curStepAbs = -1.0;
    }
    numSteps = buildSteps (patterns[(size_t) cur], db, xfBeats);

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    double lastPos = 0;

    for (int i = 0; i < n; ++i)
    {
        const double beat = ppq + i * bps;
        while (beat >= patternStart + len)
        {
            patternStart += len;
            const int q = queuedPattern.exchange (-1);
            if (q >= 0 && q != cur)
            {
                cur = q;
                playingPattern = q;
                len = patterns[(size_t) cur].totalBeats();
                numSteps = buildSteps (patterns[(size_t) cur], db, xfBeats);
            }
            stepIdx = 0;
        }
        const double p = juce::jmax (0.0, beat - patternStart);

        // segment courant
        if (stepIdx >= numSteps || p < steps[(size_t) stepIdx].start) stepIdx = 0;
        while (stepIdx + 1 < numSteps && p >= steps[(size_t) stepIdx].start + steps[(size_t) stepIdx].dur) ++stepIdx;
        const auto& s = steps[(size_t) stepIdx];
        const double abs = patternStart + s.start;
        if (std::abs (abs - curStepAbs) > 1.0e-9)
        {
            if (curStepAbs >= 0.0) { prevStep = curStep; hasPrev = true; }
            curStepAbs = abs;
        }
        curStep = s;

        const double tl = p - s.start;
        float L, R;
        double pos = 0;
        renderStep (s, tl, db, xfBeats, L, R, pos);

        if (tl < xfBeats && hasPrev)
        {
            float pl, pr; double pp;
            renderStep (prevStep, prevStep.dur + tl, db, xfBeats, pl, pr, pp);
            const float x = (float) (tl / xfBeats);
            const float fin  = x;          // fondu linéaire : pas de bosse de volume
            const float fout = 1.0f - x;   // quand les deux platines jouent le même son
            L = L * fin + pl * fout;
            R = R * fin + pr * fout;
        }

        const float g = outGain.getNextValue();
        if (outR != nullptr) { outL[i] = L * g; outR[i] = R * g; }
        else outL[i] = 0.5f * (L + R) * g;
        lastPos = pos;
    }
    expectedPpq = ppq + n * bps;

    // ---- affichage ----
    const auto& s = steps[(size_t) juce::jlimit (0, numSteps - 1, stepIdx)];
    uiBlock = s.block;
    uiPatternPos = juce::jmax (0.0, ppq + n * bps - patternStart);
    uiActiveDeck = s.silent ? -1 : s.deck;
    if (! s.silent)
    {
        uiDeckPos[s.deck] = lastPos;
        if (! s.isMove) { uiZoneStart[s.deck] = s.zoneStart; uiZoneLen[s.deck] = s.zoneLen; }
        const int other = 1 - s.deck;
        uiDeckPos[other] = uiZoneStart[other].load();   // l'autre platine est ramenée au départ
    }
}

// ============================================================================
void BeatJugglingProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("BeatJuggling");
    root.setAttribute ("version", 2);
    if (auto params = apvts.copyState().createXml()) root.addChildElement (params.release());
    root.setAttribute ("playingPattern", playingPattern.load());

    for (int k = 0; k < 2; ++k)
    {
        auto* e = root.createNewChildElement ("Deck");
        e->setAttribute ("index", k);
        auto d = getDeck (k);
        e->setAttribute ("file", d ? d->file.getFullPathName() : juce::String());
        e->setAttribute ("bpm", deckSettings[k].fileBpm.load());
        e->setAttribute ("anchor", deckSettings[k].anchor.load());
    }
    for (int i = 0; i < bj::kNumPatterns; ++i)
    {
        const auto& p = patterns[(size_t) i];
        auto* pe = root.createNewChildElement ("Pattern");
        pe->setAttribute ("index", i);
        pe->setAttribute ("bars", p.bars);
        for (auto& b : p.blocks)
        {
            auto* be = pe->createNewChildElement ("Block");
            be->setAttribute ("type", (int) b.type);
            be->setAttribute ("deck", b.deck);
            be->setAttribute ("start", b.start16);
            be->setAttribute ("len", b.len16);
            be->setAttribute ("reverse", b.reverse);
            be->setAttribute ("move", (int) b.move);
            be->setAttribute ("amount", (double) b.amount);
            be->setAttribute ("fromPrev", b.fromPrev);
        }
    }
    copyXmlToBinary (root, destData);
}

void BeatJugglingProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("BeatJuggling")) return;
    if (auto* params = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));
    playingPattern = juce::jlimit (0, bj::kNumPatterns - 1, xml->getIntAttribute ("playingPattern", 0));
    queuedPattern = -1;

    for (auto* e : xml->getChildWithTagNameIterator ("Deck"))
    {
        const int k = juce::jlimit (0, 1, e->getIntAttribute ("index"));
        const juce::File f (e->getStringAttribute ("file"));
        if (f.existsAsFile())
        {
            auto other = getDeck (1 - k);
            if (k == 1 && other != nullptr && other->file == f) copyDeck (0, 1);
            else loadFile (k, f, false);
        }
        deckSettings[k].fileBpm = e->getDoubleAttribute ("bpm", deckSettings[k].fileBpm.load());
        deckSettings[k].anchor  = e->getDoubleAttribute ("anchor", deckSettings[k].anchor.load());
    }

    if (xml->getIntAttribute ("version", 1) >= 2)
    {
        for (auto* pe : xml->getChildWithTagNameIterator ("Pattern"))
        {
            const int i = juce::jlimit (0, bj::kNumPatterns - 1, pe->getIntAttribute ("index"));
            editPattern (i, [pe] (bj::Pattern& p)
            {
                p.bars = pe->getIntAttribute ("bars", 4);
                p.blocks.clear();
                for (auto* be : pe->getChildWithTagNameIterator ("Block"))
                {
                    Block b;
                    b.type = be->getIntAttribute ("type") == 1 ? BlockType::move : BlockType::play;
                    b.deck = be->getIntAttribute ("deck");
                    b.start16 = be->getIntAttribute ("start");
                    b.len16 = be->getIntAttribute ("len", 16);
                    b.reverse = be->getBoolAttribute ("reverse");
                    b.move = (Move) juce::jlimit (0, (int) Move::count - 1, be->getIntAttribute ("move"));
                    b.amount = (float) be->getDoubleAttribute ("amount", 0.4);
                    b.fromPrev = be->getBoolAttribute ("fromPrev", true);
                    p.blocks.push_back (b);
                }
            });
        }
    }
}

juce::AudioProcessorEditor* BeatJugglingProcessor::createEditor() { return new BeatJugglingEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BeatJugglingProcessor(); }
