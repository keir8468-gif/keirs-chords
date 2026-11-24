// Host simulation + UI snapshot. Built only with -DKC_TESTS=ON.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <map>

struct FakeHead : juce::AudioPlayHead {
    double ppq = 0, bpm = 124; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying(playing); p.setBpm(bpm); p.setPpqPosition(ppq); return p; }
};

static void setP(KeirsChordsProcessor& p, const char* id, float v) { auto* x = p.apvts.getParameter(id); x->setValueNotifyingHost(x->convertTo0to1(v)); }

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::PluginHostType::jucePlugInClientCurrentWrapperType = juce::AudioProcessor::wrapperType_VST3;
    auto proc = std::make_unique<KeirsChordsProcessor>();
    FakeHead head; proc->setPlayHead(&head);
    const double sr = 48000; const int bs = 480;
    proc->prepareToPlay(sr, bs);
    setP(*proc, "swingMode", 2); setP(*proc, "swingPct", 58); setP(*proc, "roll", 0); setP(*proc, "pocket", 0); setP(*proc, "preview", 1);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(80);   // let the pattern timer build
    juce::AudioBuffer<float> buf(2, bs);
    std::map<int, int> open; int ons = 0, offs = 0, stuckErr = 0; double peak = 0;
    std::vector<std::pair<double, int>> firstBar;
    const double beatsPerBlock = bs * head.bpm / (60.0 * sr);
    for (int b = 0; b < 2000; ++b) {
        juce::MidiBuffer midi;
        proc->processBlock(buf, midi);
        peak = std::max(peak, (double)buf.getMagnitude(0, bs));
        for (const auto m : midi) {
            auto msg = m.getMessage(); const double t = head.ppq + m.samplePosition / (double)bs * beatsPerBlock;
            const int key = msg.getChannel() * 128 + msg.getNoteNumber();
            if (msg.isNoteOn()) { ons++; open[key]++; if (t < 4.0) firstBar.push_back({ t, msg.getNoteNumber() }); }
            else if (msg.isNoteOff()) { offs++; if (open[key] > 0) open[key]--; }
        }
        head.ppq += beatsPerBlock;
    }
    head.playing = false; { juce::MidiBuffer midi; proc->processBlock(buf, midi); for (const auto m : midi) if (m.getMessage().isNoteOff()) { offs++; open[m.getMessage().getChannel() * 128 + m.getMessage().getNoteNumber()] = 0; } }
    for (auto& kv : open) if (kv.second) stuckErr++;
    printf("note-ons %d, note-offs %d, stuck after stop %d, preview peak %.3f\n", ons, offs, stuckErr, peak);
    auto P = proc->getPattern();
    printf("pattern: %s | %s | %s\n", P->styleName.c_str(), P->progName.c_str(), P->rhythmName.c_str());
    printf("first bar onsets (beats; 16th grid = .25, MPC 58%% off-16ths should sit +0.0417 late):\n");
    double last = -1; for (auto& e : firstBar) if (std::abs(e.first - last) > 1e-4) { printf("  %.4f (16th %.3f)\n", e.first, e.first * 4); last = e.first; }

    // hold mode: notes only while a key is held, root follows the key
    setP(*proc, "playMode", 1); juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    int holdOns = 0;
    for (int b = 0; b < 200; ++b) {
        juce::MidiBuffer midi; if (b == 10) midi.addEvent(juce::MidiMessage::noteOn(1, 62, (juce::uint8)100), 0); if (b == 150) midi.addEvent(juce::MidiMessage::noteOff(1, 62), 0);
        proc->processBlock(buf, midi); for (const auto m : midi) if (m.getMessage().isNoteOn() && b < 150) holdOns++;
    }
    printf("hold mode: %d note-ons while key held (stopped transport)\n", holdOns);

    // MIDI export
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("kc_test.mid");
    printf("export midi: %s (%d bytes)\n", proc->exportMidi(f) ? "ok" : "FAILED", (int)f.getSize());

    // UI snapshot
    if (argc > 1) {
        setP(*proc, "playMode", 0);
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc->createEditor());
        ed->setSize(1200, 790);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
        juce::File out(argv[1]); out.deleteFile(); juce::FileOutputStream os(out); juce::PNGImageFormat().writeImageToStream(img, os);
        printf("snapshot %s\n", argv[1]);
        ed.reset();
    }
    proc.reset();
    return 0;
}
