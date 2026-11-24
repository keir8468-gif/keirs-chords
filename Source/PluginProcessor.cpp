#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace kc;

static juce::StringArray toSA(const std::vector<std::string>& v) { juce::StringArray a; for (auto& s : v) a.add(juce::String::fromUTF8(s.c_str())); return a; }

juce::AudioProcessorValueTreeState::ParameterLayout KeirsChordsProcessor::createLayout()
{
    using namespace juce;
    const auto& L = Library::get();
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pid = [](const char* id) { return ParameterID{ id, 1 }; };
    p.push_back(std::make_unique<AudioParameterInt>(pid("prog"), "Progression", 0, (int)L.progs.size() - 1, 0));
    p.push_back(std::make_unique<AudioParameterInt>(pid("rhythm"), "Rhythm", 0, (int)L.rhythms.size() - 1, 0));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("key"), "Key", StringArray{ "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" }, 9));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("voicing"), "Voicing", toSA(voicingNames()), 0));
    StringArray shapes{ "Auto" }; for (auto& s : parallelShapes()) shapes.add(s.sym);
    p.push_back(std::make_unique<AudioParameterChoice>(pid("shape"), "Memory Shape", shapes, 0));
    p.push_back(std::make_unique<AudioParameterInt>(pid("octave"), "Octave", -2, 2, 0));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("complexity"), "Complexity", NormalisableRange<float>(0, 100, 1), 50));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("variation"), "Variation", NormalisableRange<float>(0, 100, 1), 25));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("dynamics"), "Dynamics", NormalisableRange<float>(0, 100, 1), 55));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("gate"), "Gate", NormalisableRange<float>(10, 150, 1), 100));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("bars"), "Loop Bars", StringArray{ "Auto", "1", "2", "4", "8", "16" }, 0));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("rate"), "Chord Rate", StringArray{ "Auto", "2 / bar", "1 / bar", "1 / 2 bars", "1 / 4 bars" }, 0));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("bass"), "Bass", StringArray{ "Off", "Held roots", "Off-beat", "Organ bounce", "Walking", "Tonic pedal", "Follow chords" }, 0));
    p.push_back(std::make_unique<AudioParameterInt>(pid("seed"), "Seed", 1, 9999, 1));
    // groove & feel
    p.push_back(std::make_unique<AudioParameterChoice>(pid("swingMode"), "Swing Model", toSA(swingModeNames()), 2));
    p.push_back(std::make_unique<AudioParameterInt>(pid("shuffle909"), "909 Shuffle", 1, 7, 3));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("swingPct"), "Swing %", NormalisableRange<float>(50, 75, 1), 56));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("swingRes"), "Swing Grid", StringArray{ "Auto", "16ths", "8ths" }, 0));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("feel"), "Human Feel", NormalisableRange<float>(0, 100, 1), 30));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("roll"), "Roll", NormalisableRange<float>(0, 20, 0.5f), 3));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("pocket"), "Pocket", NormalisableRange<float>(-25, 25, 0.5f), 0));
    p.push_back(std::make_unique<AudioParameterBool>(pid("kickSpace"), "Kick Space", true));
    // play & output
    p.push_back(std::make_unique<AudioParameterChoice>(pid("playMode"), "Play Mode", StringArray{ "Free", "Hold" }, 0));
    p.push_back(std::make_unique<AudioParameterBool>(pid("rootFollow"), "Key Sets Root", true));
    p.push_back(std::make_unique<AudioParameterInt>(pid("chordCh"), "Chord Channel", 1, 16, 1));
    p.push_back(std::make_unique<AudioParameterInt>(pid("bassCh"), "Bass Channel", 1, 16, 2));
    p.push_back(std::make_unique<AudioParameterBool>(pid("preview"), "Preview Sound", true));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("previewSound"), "Preview Voice", StringArray{ "Keys", "Pad", "Organ", "Stab" }, 0));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("previewVol"), "Preview Level", NormalisableRange<float>(-36, 6, 0.5f), -6));
    p.push_back(std::make_unique<AudioParameterFloat>(pid("bpm"), "Tempo", NormalisableRange<float>(60, 200, 0.1f), 124));
    p.push_back(std::make_unique<AudioParameterChoice>(pid("clockSrc"), "Clock", StringArray{ "Internal", "MIDI Clock" }, 0));
    return { p.begin(), p.end() };
}

KeirsChordsProcessor::KeirsChordsProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 24; ++i) { auto* v = new PreviewVoice(); v->mode = &previewMode; synth.addVoice(v); }
    synth.addSound(new PreviewSound());
    pending.reserve(1024);
    // a sensible, groovy starting point: Jersey stabs on a 3-3-2 rhythm
    const auto& L = Library::get();
    for (size_t i = 0; i < L.progs.size(); ++i) if (L.progs[i].id == "0:m 8:!6") { setParamPlain("prog", (float)i); break; }   // i7(11) - bVI6, a Jersey two-chord loop
    for (size_t i = 0; i < L.rhythms.size(); ++i) if (L.rhythms[i].name == "Jersey Push") { setParamPlain("rhythm", (float)i); break; }
    timerCallback();
    startTimerHz(30);
}

KeirsChordsProcessor::~KeirsChordsProcessor()
{
    stopTimer();
    delete livePattern.exchange(nullptr);
}

void KeirsChordsProcessor::setParamPlain(const juce::String& id, float plain)
{
    if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(plain));
}

Settings KeirsChordsProcessor::readSettings() const
{
    Settings s;
    s.prog = (int)pv("prog"); s.rhythm = (int)pv("rhythm"); s.key = (int)pv("key"); s.voicing = (int)pv("voicing"); s.shape = (int)pv("shape");
    s.octave = (int)pv("octave"); s.complexity = pv("complexity"); s.variation = pv("variation"); s.dynamics = pv("dynamics"); s.gate = pv("gate");
    static const int BARS[] = { 0, 1, 2, 4, 8, 16 };
    s.barsMode = BARS[juce::jlimit(0, 5, (int)pv("bars"))]; s.rateMode = (int)pv("rate"); s.bass = (int)pv("bass"); s.seed = (uint32_t)pv("seed");
    return s;
}
GrooveSettings KeirsChordsProcessor::readGroove() const
{
    GrooveSettings g; g.mode = (SwingMode)(int)pv("swingMode"); g.shuffle909 = (int)pv("shuffle909"); g.swingPct = pv("swingPct");
    g.resolution = (int)pv("swingRes"); g.feel = pv("feel");
    return g;
}

// message thread: rebuild the pattern whenever a pattern-shaping parameter changes
void KeirsChordsProcessor::timerCallback()
{
    auto s = readSettings();
    juce::String h; h << s.prog << '|' << s.rhythm << '|' << s.key << '|' << s.voicing << '|' << s.shape << '|' << s.octave << '|' << s.complexity << '|' << s.variation << '|'
                      << s.dynamics << '|' << s.gate << '|' << s.barsMode << '|' << s.rateMode << '|' << s.bass << '|' << (int)s.seed;
    previewMode.store((int)pv("previewSound"));
    const auto now = juce::Time::getMillisecondCounter();
    retired.erase(std::remove_if(retired.begin(), retired.end(), [&](auto& r) { return now - r.second > 2000; }), retired.end());
    if (h == lastHash) return;
    lastHash = h;
    auto* np = new Pattern(generate(s));
    { std::lock_guard<std::mutex> l(patternMutex); uiPattern = std::make_shared<const Pattern>(*np); }
    if (auto* old = livePattern.exchange(np)) retired.push_back({ std::unique_ptr<Pattern>(old), now });
}

void KeirsChordsProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate; synth.setCurrentPlaybackSampleRate(sampleRate);
    pending.clear(); expectedPpq = -1; wasActive = false;
#if JUCE_MAC || JUCE_LINUX
    if (isStandalone() && !virtualOut) virtualOut = juce::MidiOutput::createNewDevice("Keir's Chords");
    if (virtualOut) virtualOut->startBackgroundThread();
#endif
}

bool KeirsChordsProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& o = layouts.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

void KeirsChordsProcessor::markSounding(int pitch, bool on)
{
    auto& w = sounding[(size_t)(pitch >> 6)]; const juce::uint64 bit = 1ull << (pitch & 63);
    if (on) w.fetch_or(bit); else w.fetch_and(~bit);
}

void KeirsChordsProcessor::allNotesOff(juce::MidiBuffer& out, int offset)
{
    for (auto& p : pending) { out.addEvent(juce::MidiMessage::noteOff(p.ch, p.pitch), offset); }
    pending.clear();
    noteCount.fill(0);
    sounding[0] = 0; sounding[1] = 0;
}

void KeirsChordsProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    const bool standalone = isStandalone();
    const bool holdMode = (int)pv("playMode") == 1;
    const bool useClock = standalone && (int)pv("clockSrc") == 1;

    // ---- input: held notes + MIDI clock
    juce::MidiBuffer out;
    bool newPress = false;
    for (const auto m : midi) {
        const auto msg = m.getMessage();
        if (msg.isNoteOn()) { if (!held[(size_t)msg.getNoteNumber()]) { held[(size_t)msg.getNoteNumber()] = true; ++heldCount; } lastHeldNote = msg.getNoteNumber(); newPress = heldCount == 1; }
        else if (msg.isNoteOff()) { if (held[(size_t)msg.getNoteNumber()]) { held[(size_t)msg.getNoteNumber()] = false; heldCount = std::max(0, heldCount - 1); } }
        else if (msg.isMidiClock()) { clockTicks += 1; if (lastTickSample >= 0) { double dt = (double)(sampleCounter + m.samplePosition) - lastTickSample; if (dt > 0) clockBpm = clockBpm * 0.9 + 0.1 * (60.0 * sr / (dt * 24.0)); } lastTickSample = (double)(sampleCounter + m.samplePosition); }
        else if (msg.isMidiStart()) { clockTicks = 0; clockRunning = true; }
        else if (msg.isMidiContinue()) { clockRunning = true; }
        else if (msg.isMidiStop()) { clockRunning = false; }
        else if (msg.isSongPositionPointer()) { clockTicks = msg.getSongPositionPointerMidiBeat() * 6.0; }
        if (!holdMode && (msg.isNoteOnOrOff())) out.addEvent(msg, m.samplePosition);      // free mode: play along through
        else if (!msg.isNoteOnOrOff() && !msg.isMidiClock() && !msg.isMidiStart() && !msg.isMidiStop() && !msg.isMidiContinue()) out.addEvent(msg, m.samplePosition);
    }

    // ---- transport
    bool hostPlaying = false; double ppq = 0, bpm = pv("bpm");
    if (!standalone) {
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) {
            hostPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    } else if (useClock) {
        hostPlaying = clockRunning; bpm = juce::jlimit(40.0, 240.0, clockBpm);
        ppq = clockTicks / 24.0;
    } else {
        hostPlaying = transportRunning.load(); ppq = internalPpq;
    }
    const double beatsPerSample = bpm / (60.0 * sr);
    bool active; double p0;
    if (holdMode) {
        active = heldCount > 0;
        if (hostPlaying) p0 = ppq;
        else { if (newPress) holdPpq = 0; p0 = holdPpq; }
    } else { active = hostPlaying; p0 = ppq; }
    const double p1 = p0 + n * beatsPerSample;
    if (standalone && !useClock && transportRunning.load()) internalPpq += n * beatsPerSample;
    if (standalone && !useClock && !transportRunning.load()) internalPpq = 0;
    if (holdMode && !hostPlaying && active) holdPpq += n * beatsPerSample;

    if (!active) {
        if (wasActive) allNotesOff(out, 0);
        wasActive = false; expectedPpq = -1; loopPos = -1;
    } else {
        if (!wasActive || (expectedPpq >= 0 && std::abs(p0 - expectedPpq) > 0.02)) { allNotesOff(out, 0); }
        wasActive = true; expectedPpq = p1;

        const Pattern* P = livePattern.load();
        if (P && P->lengthSteps > 0) {
            const GrooveSettings g = readGroove();
            const double Lb = P->lengthSteps / 4.0;
            const int chordCh = (int)pv("chordCh"), bassCh = (int)pv("bassCh");
            const bool rootFollow = holdMode && (bool)pv("rootFollow") && lastHeldNote >= 0;
            int transpose = 0;
            if (rootFollow) { transpose = mod12(lastHeldNote - (int)pv("key")); if (transpose > 6) transpose -= 12; }
            const double msToBeats = bpm / 60000.0;
            const double roll = pv("roll") * msToBeats, pocket = pv("pocket") * msToBeats;
            const bool kickSpace = pv("kickSpace") > 0.5f;
            loopPos = std::fmod(p0, Lb) * 4.0;

            auto toSample = [&](double t) { return juce::jlimit(0, n - 1, (int)std::floor((t - p0) / beatsPerSample)); };
            // note-offs due in this block
            for (size_t i = 0; i < pending.size();) {
                if (pending[i].offPpq < p1) {
                    int idx = (pending[i].ch - 1) * 128 + pending[i].pitch;
                    if (--noteCount[(size_t)idx] <= 0) { noteCount[(size_t)idx] = 0; out.addEvent(juce::MidiMessage::noteOff(pending[i].ch, pending[i].pitch), toSample(std::max(p0, pending[i].offPpq))); markSounding(pending[i].pitch, false); }
                    pending[i] = pending.back(); pending.pop_back();
                } else ++i;
            }
            // note-ons
            const long kFirst = (long)std::floor((p0 - 1.0) / Lb), kLast = (long)std::floor((p1 + 1.0) / Lb);
            for (long k = kFirst; k <= kLast; ++k) {
                for (size_t ei = 0; ei < P->events.size(); ++ei) {
                    const auto& e = P->events[ei];
                    const uint32_t occ = (uint32_t)(k * 131 + (long)(e.step * 4));
                    double t = k * Lb + e.step / 4.0 + grooveDelay(g, e.step, e.eighthFeel, bpm, occ);
                    if (!e.bass) t += pocket + e.voice * roll;                      // roll: low notes first, like a hand
                    else t += std::min(0.0, pocket) * 0.25;                       // bass stays near the grid, the chords breathe around it
                    if (t < p0 || t >= p1) continue;
                    int pitch = juce::jlimit(0, 127, e.pitch + transpose);
                    int vel = grooveVelocity(g, e.vel, e.step, occ);
                    // kick space: hits landing on the 4/4 kick are softened and shortened so the kick punches through
                    const int inBar = (int)std::lround(std::fmod(e.step, 16.0));
                    double len = e.len / 4.0;
                    if (kickSpace && !e.bass && inBar % 4 == 0 && std::abs(e.step - std::round(e.step)) < 0.01 && len < 2.0) { vel = (int)(vel * 0.82); }
                    const int ch = e.bass ? bassCh : chordCh;
                    const int idx = (ch - 1) * 128 + pitch;
                    const int s = toSample(t);
                    if (noteCount[(size_t)idx] > 0) {   // retrigger: close the old one first
                        for (size_t i = 0; i < pending.size(); ++i) if (pending[i].ch == ch && pending[i].pitch == pitch) { pending[i] = pending.back(); pending.pop_back(); break; }
                        out.addEvent(juce::MidiMessage::noteOff(ch, pitch), s); noteCount[(size_t)idx] = 0;
                    }
                    out.addEvent(juce::MidiMessage::noteOn(ch, pitch, (juce::uint8)juce::jlimit(1, 127, vel)), s);
                    noteCount[(size_t)idx] = 1; markSounding(pitch, true);
                    if (pending.size() < 1000) pending.push_back({ t + std::max(0.02, len), pitch, ch });
                }
            }
        }
    }

    // ---- preview synth
    if (pv("preview") > 0.5f) {
        juce::MidiBuffer synthMidi;
        for (const auto m : out) { auto msg = m.getMessage(); if (msg.isNoteOnOrOff()) synthMidi.addEvent(msg, m.samplePosition); }
        synth.renderNextBlock(buffer, synthMidi, 0, n);
        buffer.applyGain(juce::Decibels::decibelsToGain(pv("previewVol")));
    } else synth.allNotesOff(0, true);

    if (virtualOut) virtualOut->sendBlockOfMessages(out, juce::Time::getMillisecondCounterHiRes() + 2.0, sr);
    midi.swapWith(out);
    sampleCounter += n;
}

// ---------------------------------------------------------------- presets / state
void KeirsChordsProcessor::applyPreset(const PresetData& p)
{
    const auto& L = Library::get();
    for (auto& kv : p.values) {
        float v = kv.second;
        if (kv.first == "prog" && p.progId.isNotEmpty()) { int i = L.findProg(p.progId.toStdString()); if (i >= 0) v = (float)i; }
        if (kv.first == "rhythm" && p.rhythmId.isNotEmpty()) { int i = L.findRhythm(p.rhythmId.toStdString()); if (i >= 0) v = (float)i; }
        setParamPlain(kv.first, v);
    }
    currentPreset = p.name;
    if (onPresetLoaded) onPresetLoaded();
}

PresetData KeirsChordsProcessor::captureState(const juce::String& name) const
{
    PresetData p; p.name = name; p.user = true;
    const auto& L = Library::get();
    int pi = (int)pv("prog"), ri = (int)pv("rhythm");
    p.genre = L.progs[(size_t)juce::jlimit(0, (int)L.progs.size() - 1, pi)].genre;
    p.progId = L.progs[(size_t)juce::jlimit(0, (int)L.progs.size() - 1, pi)].id;
    p.rhythmId = L.rhythms[(size_t)juce::jlimit(0, (int)L.rhythms.size() - 1, ri)].id;
    for (auto id : { "prog", "rhythm", "voicing", "shape", "octave", "complexity", "variation", "dynamics", "gate", "bars", "rate", "bass", "seed",
                     "swingMode", "shuffle909", "swingPct", "swingRes", "feel", "roll", "pocket", "kickSpace" })
        p.values[id] = pv(id);
    return p;
}

void KeirsChordsProcessor::setProgByIndex(int i) { setParamPlain("prog", (float)i); }
void KeirsChordsProcessor::setRhythmByIndex(int i) { setParamPlain("rhythm", (float)i); }

void KeirsChordsProcessor::randomise(int genre, bool progression, bool rhythm)
{
    const auto& L = Library::get();
    juce::Random r;
    if (progression) {
        std::vector<int> c; for (size_t i = 0; i < L.progs.size(); ++i) if (genre < 0 || L.progs[i].genre == genre) c.push_back((int)i);
        if (!c.empty()) setProgByIndex(c[(size_t)r.nextInt((int)c.size())]);
    }
    if (rhythm) {
        int g = genre >= 0 ? genre : L.progs[(size_t)(int)pv("prog")].genre;
        // weighted towards the groove sweet spot
        std::vector<std::pair<int, double>> c; for (size_t i = 0; i < L.rhythms.size(); ++i) if (L.rhythms[i].genre == g) c.push_back({ (int)i, std::pow(L.rhythms[i].groove + 0.25, 2.0) });
        if (!c.empty()) { Rng rr((uint32_t)r.nextInt()); setRhythmByIndex(pickWeighted(rr, c)); }
    }
    setParamPlain("seed", (float)(1 + r.nextInt(9999)));
}

bool KeirsChordsProcessor::exportMidi(const juce::File& f) const
{
    auto P = getPattern(); if (!P) return false;
    const auto g = readGroove();
    const double bpm = pv("bpm");
    juce::MidiMessageSequence seq; const int tpq = 960;
    seq.addEvent(juce::MidiMessage::tempoMetaEvent((int)(60000000.0 / bpm)), 0);
    const int chordCh = (int)pv("chordCh"), bassCh = (int)pv("bassCh");
    const double roll = pv("roll") * bpm / 60000.0, pocket = std::max(0.0, (double)pv("pocket")) * bpm / 60000.0;
    for (auto& e : P->events) {
        double t = e.step / 4.0 + grooveDelay(g, e.step, e.eighthFeel, bpm, (uint32_t)(e.step * 4)) + (e.bass ? 0.0 : pocket + e.voice * roll);
        const int ch = e.bass ? bassCh : chordCh;
        seq.addEvent(juce::MidiMessage::noteOn(ch, e.pitch, (juce::uint8)grooveVelocity(g, e.vel, e.step, 0)), t * tpq);
        seq.addEvent(juce::MidiMessage::noteOff(ch, e.pitch), (t + e.len / 4.0) * tpq);
    }
    seq.updateMatchedPairs(); seq.sort();
    juce::MidiFile mf; mf.setTicksPerQuarterNote(tpq); mf.addTrack(seq);
    f.deleteFile(); juce::FileOutputStream os(f); if (!os.openedOk()) return false;
    return mf.writeTo(os);
}

void KeirsChordsProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    const auto& L = Library::get();
    state.setProperty("progId", juce::String(L.progs[(size_t)juce::jlimit(0, (int)L.progs.size() - 1, (int)pv("prog"))].id), nullptr);
    state.setProperty("rhythmId", juce::String(L.rhythms[(size_t)juce::jlimit(0, (int)L.rhythms.size() - 1, (int)pv("rhythm"))].id), nullptr);
    state.setProperty("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void KeirsChordsProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        auto t = juce::ValueTree::fromXml(*xml);
        if (!t.isValid()) return;
        apvts.replaceState(t);
        const auto& L = Library::get();
        int pi = L.findProg(t.getProperty("progId").toString().toStdString()); if (pi >= 0) setProgByIndex(pi);
        int ri = L.findRhythm(t.getProperty("rhythmId").toString().toStdString()); if (ri >= 0) setRhythmByIndex(ri);
        currentPreset = t.getProperty("preset", "Init").toString();
    }
}

juce::AudioProcessorEditor* KeirsChordsProcessor::createEditor() { return new KeirsChordsEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeirsChordsProcessor(); }
