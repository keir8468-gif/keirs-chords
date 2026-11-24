#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace kcui {

namespace col {
const juce::Colour bg{ 0xff0a0a0b }, panel{ 0xff111113 }, ink{ 0xffe6e6e3 }, ink2{ 0xff85858a }, ink3{ 0xff55555a }, line{ 0xff1f1f22 },
    groove{ 0xff2a2a2e }, cap{ 0xff161618 }, capEdge{ 0xff2e2e33 }, accent{ 0xffff7a1a }, accentInk{ 0xff000000 },
    lcd{ 0xff0e1311 }, lcdEdge{ 0xff16201b }, lcdInk{ 0xffa6e3c3 }, lcdDim{ 0xff4c6659 }, lcdHot{ 0xffe8ffef },
    house{ 0xffff7a1a }, techno{ 0xff5fb3ff }, jazz{ 0xffe0c35a };
inline juce::Colour genre(int g) { return g == 0 ? house : (g == 1 ? techno : jazz); }
}

struct Fonts {
    juce::Typeface::Ptr display, displayBold, body, lcd;
    Fonts();
    juce::Font d(float h, bool bold = false) const;
    juce::Font b(float h) const;
    juce::Font l(float h) const;
};

class Look : public juce::LookAndFeel_V4 {
public:
    explicit Look(const Fonts& f);
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    juce::Font getPopupMenuFont() override { return fonts.d(16.f); }
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool, bool) override;
private:
    const Fonts& fonts;
};

/* row/grid of flat cells bound to a choice/int parameter (offset = plain value of the first cell) */
class Segmented : public juce::Component {
public:
    Segmented(juce::RangedAudioParameter& p, juce::StringArray labels, int columns, const Fonts& f, int offset = 0, float fontH = 13.5f);
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
    juce::Colour accent = col::accent;
private:
    juce::ParameterAttachment att; juce::StringArray labels; int cols, idx = 0, off; const Fonts& fonts; float fh;
};

/* compact drop-down bound to a choice/int parameter */
class ChoiceBox : public juce::Component {
public:
    ChoiceBox(juce::RangedAudioParameter& p, juce::StringArray labels, const Fonts& f, int offset = 0);
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    juce::ParameterAttachment att; juce::StringArray labels; int idx = 0, off; const Fonts& fonts;
};

/* on/off pill bound to a bool parameter */
class Toggle : public juce::Component {
public:
    Toggle(juce::RangedAudioParameter& p, juce::String text, const Fonts& f);
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    juce::ParameterAttachment att; juce::String text; bool on = false; const Fonts& fonts;
};

class FlatButton : public juce::Component {
public:
    FlatButton(juce::String t, const Fonts& f, bool accentFill = false) : text(std::move(t)), fonts(f), fill(accentFill) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
    std::function<void()> onClick;
    juce::String text; bool lit = false;
private:
    const Fonts& fonts; bool fill;
};

/* knob with a label and a value readout */
class Knob : public juce::Component {
public:
    Knob(juce::AudioProcessorValueTreeState& s, const char* id, juce::String label, const Fonts& f, double detent, juce::String unit = {});
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    juce::RangedAudioParameter* param; juce::String label, unit; const Fonts& fonts;
};

/* the green LCD: chords of the loop, playhead, keyboard of sounding notes */
class ChordDisplay : public juce::Component {
public:
    ChordDisplay(KeirsChordsProcessor& p, const Fonts& f) : proc(p), fonts(f) {}
    void paint(juce::Graphics&) override;
private:
    KeirsChordsProcessor& proc; const Fonts& fonts;
};

/* the current rhythm on its step grid, with the swing displacement drawn in */
class GrooveView : public juce::Component {
public:
    GrooveView(KeirsChordsProcessor& p, const Fonts& f) : proc(p), fonts(f) {}
    void paint(juce::Graphics&) override;
private:
    KeirsChordsProcessor& proc; const Fonts& fonts;
};

/* drag the current loop out as a MIDI file */
class DragMidi : public juce::Component {
public:
    DragMidi(KeirsChordsProcessor& p, const Fonts& f) : proc(p), fonts(f) { setMouseCursor(juce::MouseCursor::DraggingHandCursor); setTooltip("Drag into a Live clip slot / arrangement"); }
    void paint(juce::Graphics&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override { dragging = false; }
    void setTooltip(const juce::String&) {}
private:
    KeirsChordsProcessor& proc; const Fonts& fonts; bool dragging = false;
};

/* the library browser: progressions / rhythms / presets */
class Browser : public juce::Component, private juce::ListBoxModel {
public:
    Browser(KeirsChordsProcessor& p, const Fonts& f);
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh(bool keepScroll = true);
    void syncSelection();
    enum Tab { Progressions, Rhythms, PresetsTab };
    void setTab(Tab t);
private:
    int getNumRows() override { return (int)rows.size(); }
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void activate(int row);

    KeirsChordsProcessor& proc; const Fonts& fonts;
    Tab tab = Progressions; int genre = -1, styleFilter = -1, tonality = 0, syncFilter = -1, sortMode = 0; bool favOnly = false;
    juce::TextEditor search; juce::ListBox list{ "lib", this };
    FlatButton tabP{ "PROGRESSIONS", fonts }, tabR{ "RHYTHMS", fonts }, tabS{ "PRESETS", fonts };
    FlatButton gAll{ "ALL", fonts }, gHouse{ "HOUSE", fonts }, gTechno{ "TECHNO", fonts }, gJazz{ "JAZZ", fonts };
    FlatButton f1{ "", fonts }, f2{ "", fonts }, f3{ "", fonts }, rnd{ "RANDOM", fonts, true };
    std::vector<int> rows;                       // indices into the active source
    std::vector<kc::PresetData> userPresets; juce::StringArray favs;
    std::vector<const kc::PresetData*> presetRows;
    int lastProg = -1, lastRhythm = -1;
    void updateFilterButtons();
};

class Panel : public juce::Component, private juce::Timer {
public:
    explicit Panel(KeirsChordsProcessor&);
    ~Panel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    static constexpr int W = 1200, H = 790;
private:
    void timerCallback() override;
    void stepPreset(int dir);
    void savePreset();
    juce::Rectangle<int> section(juce::Graphics*, juce::Rectangle<int> r, const juce::String& title);

    KeirsChordsProcessor& proc;
    Fonts fonts; Look look{ fonts };
    Browser browser{ proc, fonts };
    ChordDisplay lcd{ proc, fonts };
    GrooveView grooveView{ proc, fonts };
    DragMidi drag{ proc, fonts };
    // harmony
    std::unique_ptr<Segmented> key, octave, bars, swingMode, shuffle, swingRes, playMode, previewVoice, clock;
    std::unique_ptr<ChoiceBox> voicing, shape, rate, bass, chordCh, bassCh;
    std::unique_ptr<Knob> complexity, variation, gate, dynamics, swingPct, feel, roll, pocket, previewVol, bpm;
    std::unique_ptr<Toggle> kickSpace, rootFollow, preview;
    FlatButton prevP{ "<", fonts }, nextP{ ">", fonts }, saveP{ "SAVE", fonts }, favP{ juce::String::fromUTF8("\xE2\x98\x85"), fonts }, dice{ "NEW SEED", fonts }, play{ "PLAY", fonts, true };
    std::vector<std::unique_ptr<FlatButton>> quickSwing;
    juce::String presetName; int presetIndex = -1;
    juce::Rectangle<int> rLcd, rHarm, rGroove, rArr, rOut, rHead;
};

class KeirsChordsEditor : public juce::AudioProcessorEditor {
public:
    explicit KeirsChordsEditor(KeirsChordsProcessor&);
    void resized() override;
private:
    Panel panel;
};

} // namespace kcui
using KeirsChordsEditor = kcui::KeirsChordsEditor;
