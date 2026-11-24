#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "engine/Engine.h"
#include "engine/Groove.h"
#include "Presets.h"
#include "PreviewSynth.h"
#include <atomic>

class KeirsChordsProcessor : public juce::AudioProcessor, private juce::Timer {
public:
    KeirsChordsProcessor();
    ~KeirsChordsProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- editor interface ----
    std::shared_ptr<const kc::Pattern> getPattern() const { std::lock_guard<std::mutex> l(patternMutex); return uiPattern; }
    double getLoopPosSteps() const { return loopPos.load(); }       // -1 when stopped
    bool isNoteOn(int n) const { return n >= 0 && n < 128 && (sounding[(size_t)(n >> 6)].load() >> (n & 63) & 1ull); }
    bool isStandalone() const { return wrapperType == wrapperType_Standalone; }
    std::atomic<bool> transportRunning{ false };                     // standalone play button
    juce::String getCurrentPresetName() const { return currentPreset; }
    void applyPreset(const kc::PresetData&);
    kc::PresetData captureState(const juce::String& name) const;
    bool exportMidi(const juce::File& f) const;                       // current loop as a .mid file
    void setProgByIndex(int i);
    void setRhythmByIndex(int i);
    void randomise(int genre, bool progression, bool rhythm);

    std::function<void()> onPresetLoaded;   // editor refresh hook (message thread)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    kc::Settings readSettings() const;
    kc::GrooveSettings readGroove() const;
    float pv(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    void setParamPlain(const juce::String& id, float plain);

    // pattern hand-over: message thread builds, audio thread reads the raw pointer
    std::atomic<kc::Pattern*> livePattern{ nullptr };
    std::vector<std::pair<std::unique_ptr<kc::Pattern>, juce::uint32>> retired;
    mutable std::mutex patternMutex; std::shared_ptr<const kc::Pattern> uiPattern;
    juce::String lastHash;

    // playback state (audio thread)
    struct Pending { double offPpq; int pitch, ch; };
    std::vector<Pending> pending;
    double sr = 44100, expectedPpq = -1, internalPpq = 0, holdPpq = 0; bool wasActive = false;
    std::array<bool, 128> held{}; int lastHeldNote = -1, heldCount = 0;
    std::array<std::atomic<juce::uint64>, 2> sounding{};
    std::atomic<double> loopPos{ -1 };
    void allNotesOff(juce::MidiBuffer& out, int offset);
    void markSounding(int pitch, bool on);
    std::array<int, 16 * 128> noteCount{};

    // MIDI clock (standalone)
    double clockTicks = 0, clockBpm = 120, samplesSinceTick = 0; bool clockRunning = false; double lastTickSample = -1; juce::int64 sampleCounter = 0;

    // preview synth
    juce::Synthesiser synth; std::atomic<int> previewMode{ 0 };

    // standalone virtual MIDI port
    std::unique_ptr<juce::MidiOutput> virtualOut;

    juce::String currentPreset{ "Init" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeirsChordsProcessor)
};
