#include "PluginEditor.h"
#include "BinaryData.h"

namespace kcui {

using juce::String;
static String u8(const char* s) { return String::fromUTF8(s); }
static const String DOT = u8(" \xC2\xB7 ");

/* ---------------- fonts & look ---------------- */
Fonts::Fonts()
{
    display = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
    displayBold = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedBold_ttf, BinaryData::BarlowCondensedBold_ttfSize);
    body = juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowMedium_ttf, BinaryData::BarlowMedium_ttfSize);
    lcd = juce::Typeface::createSystemTypefaceFor(BinaryData::VT323Regular_ttf, BinaryData::VT323Regular_ttfSize);
}
juce::Font Fonts::d(float h, bool bold) const { return juce::Font(juce::FontOptions(bold ? displayBold : display).withHeight(h).withKerningFactor(0.08f)); }
juce::Font Fonts::b(float h) const { return juce::Font(juce::FontOptions(body).withHeight(h)); }
juce::Font Fonts::l(float h) const { return juce::Font(juce::FontOptions(lcd).withHeight(h)); }

Look::Look(const Fonts& f) : fonts(f)
{
    setColour(juce::PopupMenu::backgroundColourId, col::cap);
    setColour(juce::PopupMenu::textColourId, col::ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, col::accent);
    setColour(juce::PopupMenu::highlightedTextColourId, col::accentInk);
    setColour(juce::PopupMenu::headerTextColourId, col::ink2);
    setColour(juce::ListBox::backgroundColourId, col::bg);
    setColour(juce::TextEditor::backgroundColourId, col::cap);
    setColour(juce::TextEditor::textColourId, col::ink);
    setColour(juce::TextEditor::outlineColourId, col::capEdge);
    setColour(juce::TextEditor::focusedOutlineColourId, col::accent);
    setColour(juce::TextEditor::highlightColourId, col::accent.withAlpha(0.4f));
    setColour(juce::CaretComponent::caretColourId, col::accent);
    setColour(juce::ScrollBar::thumbColourId, col::groove);
    setColour(juce::AlertWindow::backgroundColourId, col::panel);
    setColour(juce::AlertWindow::textColourId, col::ink);
    setColour(juce::AlertWindow::outlineColourId, col::capEdge);
    setColour(juce::TextButton::buttonColourId, col::cap);
    setColour(juce::TextButton::textColourOffId, col::ink);
}
void Look::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const float size = (float)std::min(w, h), sc = size / 100.f;
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    auto pol = [&](float ang, float r) { return juce::Point<float>(cx + r * sc * std::sin(ang), cy - r * sc * std::cos(ang)); };
    g.setColour(col::ink3);
    for (int i = 0; i <= 10; i++) { const float ang = a0 + (a1 - a0) * i / 10.f; g.drawLine(juce::Line<float>(pol(ang, 44), pol(ang, 48)), 1.6f * sc); }
    const double detent = s.getProperties().getWithDefault("detent", s.getMinimum());
    const float aD = a0 + (float)s.valueToProportionOfLength(detent) * (a1 - a0), a = a0 + pos * (a1 - a0);
    if (std::abs(a - aD) > 0.01f) {
        juce::Path arc; arc.addCentredArc(cx, cy, 39 * sc, 39 * sc, 0, std::min(a, aD), std::max(a, aD), true);
        g.setColour(col::accent); g.strokePath(arc, juce::PathStrokeType(4 * sc, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour(col::cap); g.fillEllipse(cx - 33 * sc, cy - 33 * sc, 66 * sc, 66 * sc);
    g.setColour(col::capEdge); g.drawEllipse(cx - 33 * sc, cy - 33 * sc, 66 * sc, 66 * sc, 1.5f * sc);
    g.setColour(col::ink); g.drawLine(juce::Line<float>({ cx, cy }, pol(a, 26)), 3.5f * sc);
}
void Look::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool, bool)
{
    g.setColour(col::bg); g.fillRect(x, y, w, h);
    g.setColour(col::groove);
    if (vertical) g.fillRoundedRectangle((float)x + 2, (float)thumbStart, (float)w - 4, (float)thumbSize, 3);
    else g.fillRoundedRectangle((float)thumbStart, (float)y + 2, (float)thumbSize, (float)h - 4, 3);
}

/* ---------------- segmented ---------------- */
Segmented::Segmented(juce::RangedAudioParameter& p, juce::StringArray l, int columns, const Fonts& f, int offset, float fontH)
    : att(p, [this](float v) { idx = juce::roundToInt(v) - off; repaint(); }), labels(std::move(l)), cols(columns), off(offset), fonts(f), fh(fontH)
{
    att.sendInitialUpdate(); setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void Segmented::paint(juce::Graphics& g)
{
    const int rows = (labels.size() + cols - 1) / cols; const auto b = getLocalBounds().toFloat();
    const float cw = b.getWidth() / cols, rh = b.getHeight() / rows;
    for (int i = 0; i < labels.size(); i++) {
        const juce::Rectangle<float> cell(b.getX() + (i % cols) * cw, b.getY() + (i / cols) * rh, cw, rh);
        const bool sel = i == idx;
        g.setColour(sel ? accent : col::cap); g.fillRect(cell);
        g.setColour(sel ? col::accentInk : col::ink2); g.setFont(fonts.d(fh));
        g.drawText(labels[i], cell.reduced(2, 0), juce::Justification::centred);
    }
    g.setColour(col::capEdge);
    for (int c = 1; c < cols; c++) g.drawVerticalLine(juce::roundToInt(b.getX() + c * cw), b.getY(), b.getBottom());
    for (int r = 1; r < rows; r++) g.drawHorizontalLine(juce::roundToInt(b.getY() + r * rh), b.getX(), b.getRight());
    g.drawRect(b, 1.f);
}
void Segmented::mouseUp(const juce::MouseEvent& e)
{
    if (!contains(e.getPosition())) return;
    const int rows = (labels.size() + cols - 1) / cols;
    const int c = juce::jlimit(0, cols - 1, (int)(e.x * cols / (float)getWidth())), r = juce::jlimit(0, rows - 1, (int)(e.y * rows / (float)getHeight()));
    const int i = r * cols + c;
    if (i < labels.size()) att.setValueAsCompleteGesture((float)(i + off));
}

/* ---------------- choice box ---------------- */
ChoiceBox::ChoiceBox(juce::RangedAudioParameter& p, juce::StringArray l, const Fonts& f, int offset)
    : att(p, [this](float v) { idx = juce::roundToInt(v) - off; repaint(); }), labels(std::move(l)), off(offset), fonts(f)
{
    att.sendInitialUpdate(); setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void ChoiceBox::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(col::cap); g.fillRoundedRectangle(b, 3); g.setColour(col::capEdge); g.drawRoundedRectangle(b, 3, 1);
    g.setColour(col::ink); g.setFont(fonts.d(15.f));
    g.drawText(juce::isPositiveAndBelow(idx, labels.size()) ? labels[idx] : String(), b.reduced(8, 0).withTrimmedRight(14), juce::Justification::centredLeft);
    juce::Path tri; const float cx = b.getRight() - 11, cy = b.getCentreY();
    tri.addTriangle(cx - 4, cy - 2, cx + 4, cy - 2, cx, cy + 3); g.setColour(col::accent); g.fillPath(tri);
}
void ChoiceBox::mouseUp(const juce::MouseEvent& e)
{
    if (!contains(e.getPosition())) return;
    juce::PopupMenu m;
    for (int i = 0; i < labels.size(); ++i) m.addItem(i + 1, labels[i], true, i == idx);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(getWidth()),
                    [safe = juce::Component::SafePointer<ChoiceBox>(this)](int r) { if (safe && r > 0) safe->att.setValueAsCompleteGesture((float)(r - 1 + safe->off)); });
}

/* ---------------- toggle ---------------- */
Toggle::Toggle(juce::RangedAudioParameter& p, String t, const Fonts& f)
    : att(p, [this](float v) { on = v > 0.5f; repaint(); }), text(std::move(t)), fonts(f)
{
    att.sendInitialUpdate(); setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void Toggle::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(col::cap); g.fillRoundedRectangle(b, 3); g.setColour(on ? col::accent : col::capEdge); g.drawRoundedRectangle(b, 3, 1);
    const float cy = b.getCentreY(), r = 4.5f, lx = b.getX() + 12;
    if (on) { g.setColour(col::accent.withAlpha(0.35f)); g.fillEllipse(lx - r * 2, cy - r * 2, r * 4, r * 4); }
    g.setColour(on ? col::accent : col::groove); g.fillEllipse(lx - r, cy - r, r * 2, r * 2);
    g.setColour(on ? col::ink : col::ink2); g.setFont(fonts.d(14.5f));
    g.drawText(text, b.withTrimmedLeft(24), juce::Justification::centredLeft);
}
void Toggle::mouseUp(const juce::MouseEvent& e) { if (contains(e.getPosition())) att.setValueAsCompleteGesture(on ? 0.f : 1.f); }

void FlatButton::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced(0.5f);
    const bool hot = fill || lit;
    g.setColour(hot ? col::accent : col::cap); g.fillRoundedRectangle(b, 3);
    g.setColour(hot ? col::accent : col::capEdge); g.drawRoundedRectangle(b, 3, 1);
    g.setColour(hot ? col::accentInk : col::ink); g.setFont(fonts.d(14.5f));
    g.drawText(text, b, juce::Justification::centred);
}

/* ---------------- knob ---------------- */
Knob::Knob(juce::AudioProcessorValueTreeState& s, const char* id, String l, const Fonts& f, double detent, String u)
    : param(s.getParameter(id)), label(std::move(l)), unit(std::move(u)), fonts(f)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity(200);
    slider.getProperties().set("detent", detent);
    slider.setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible(slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, id, slider);
    slider.setDoubleClickReturnValue(true, detent);
    slider.onValueChange = [this] { repaint(); };
}
void Knob::resized() { auto b = getLocalBounds(); b.removeFromTop(14); b.removeFromBottom(16); slider.setBounds(b); }
void Knob::paint(juce::Graphics& g)
{
    g.setColour(col::ink2); g.setFont(fonts.d(13.f));
    g.drawText(label.toUpperCase(), getLocalBounds().removeFromTop(14), juce::Justification::centred);
    g.setColour(col::lcdInk); g.setFont(fonts.l(17.f));
    g.drawText(param->getCurrentValueAsText() + unit, getLocalBounds().removeFromBottom(16), juce::Justification::centred);
}

/* ---------------- LCD ---------------- */
void ChordDisplay::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(col::lcd); g.fillRoundedRectangle(b, 6); g.setColour(col::lcdEdge); g.drawRoundedRectangle(b.reduced(0.5f), 6, 1.5f);
    auto P = proc.getPattern();
    auto in = b.reduced(14, 10);
    if (!P) return;
    // header
    auto head = in.removeFromTop(22);
    g.setFont(fonts.l(21.f)); g.setColour(col::lcdInk);
    g.drawText(u8(P->progName.c_str()), head, juce::Justification::centredLeft);
    g.setColour(col::lcdDim);
    g.drawText(u8(P->styleName.c_str()) + DOT + u8(P->rhythmName.c_str()) + DOT + String(P->lengthSteps / 16) + (P->lengthSteps == 16 ? " bar" : " bars"), head, juce::Justification::centredRight);
    in.removeFromTop(6);
    // chord blocks
    auto lane = in.removeFromTop(58);
    const double pos = proc.getLoopPosSteps();
    for (auto& c : P->chords) {
        const float x0 = lane.getX() + (float)(c.start / P->lengthSteps) * lane.getWidth();
        const float w = (float)(c.len / P->lengthSteps) * lane.getWidth();
        auto cell = juce::Rectangle<float>(x0, lane.getY(), w, lane.getHeight()).reduced(2, 0);
        const bool now = pos >= c.start && pos < c.start + c.len;
        g.setColour(now ? col::lcdInk.withAlpha(0.18f) : col::lcdEdge); g.fillRoundedRectangle(cell, 3);
        g.setColour(now ? col::lcdHot : col::lcdInk.withAlpha(0.8f));
        g.setFont(fonts.l(w > 90 ? 30.f : (w > 50 ? 24.f : 18.f)));
        g.drawFittedText(u8(c.name.c_str()), cell.toNearestInt().reduced(4, 2), juce::Justification::centred, 1, 0.7f);
    }
    // bar ticks + playhead
    g.setColour(col::lcdDim.withAlpha(0.6f));
    for (int s = 0; s <= P->lengthSteps; s += 16) { float x = lane.getX() + (float)s / P->lengthSteps * lane.getWidth(); g.drawVerticalLine((int)x, lane.getBottom() + 2, lane.getBottom() + 7); }
    if (pos >= 0) { float x = lane.getX() + (float)(pos / P->lengthSteps) * lane.getWidth(); g.setColour(col::accent); g.fillRect(x - 1, lane.getY() - 3, 2.f, lane.getHeight() + 10); }
    in.removeFromTop(12);
    // keyboard C1..C7
    auto kb = in;
    const int lo = 36, hi = 96; int whites = 0; for (int n = lo; n <= hi; ++n) if (!juce::MidiMessage::isMidiNoteBlack(n)) ++whites;
    const float ww = kb.getWidth() / whites; float x = kb.getX();
    for (int n = lo; n <= hi; ++n) if (!juce::MidiMessage::isMidiNoteBlack(n)) {
        auto r = juce::Rectangle<float>(x, kb.getY(), ww - 1, kb.getHeight());
        g.setColour(proc.isNoteOn(n) ? col::accent : col::lcdInk.withAlpha(0.12f)); g.fillRect(r); x += ww;
    }
    x = kb.getX();
    for (int n = lo; n <= hi; ++n) {
        if (!juce::MidiMessage::isMidiNoteBlack(n)) { x += ww; continue; }
        auto r = juce::Rectangle<float>(x - ww * 0.32f, kb.getY(), ww * 0.64f, kb.getHeight() * 0.6f);
        g.setColour(proc.isNoteOn(n) ? col::accent : col::lcd); g.fillRect(r);
    }
}

/* ---------------- groove view ---------------- */
void GrooveView::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(col::lcd); g.fillRoundedRectangle(b, 4); g.setColour(col::lcdEdge); g.drawRoundedRectangle(b.reduced(0.5f), 4, 1);
    const auto& L = kc::Library::get();
    const int ri = juce::jlimit(0, (int)L.rhythms.size() - 1, (int)proc.apvts.getRawParameterValue("rhythm")->load());
    const auto& R = L.rhythms[(size_t)ri];
    kc::GrooveSettings gs; gs.mode = (kc::SwingMode)(int)proc.apvts.getRawParameterValue("swingMode")->load();
    gs.shuffle909 = (int)proc.apvts.getRawParameterValue("shuffle909")->load(); gs.swingPct = proc.apvts.getRawParameterValue("swingPct")->load();
    gs.resolution = (int)proc.apvts.getRawParameterValue("swingRes")->load(); gs.feel = 0;
    const int N = 16 * R.bars;
    auto in = b.reduced(8, 6);
    auto top = in.removeFromTop(14);
    g.setFont(fonts.l(16.f)); g.setColour(col::lcdDim);
    String stars; for (int i = 0; i < 5; ++i) stars += (R.groove >= i + 0.75 ? u8("\xE2\x96\xA0") : u8("\xC2\xB7"));
    g.drawText("GROOVE " + stars + "  SYNC " + String(R.sync, 1), top, juce::Justification::centredLeft);
    g.drawText("KICK 1 2 3 4", top, juce::Justification::centredRight);
    in.removeFromTop(4);
    const float cw = in.getWidth() / N;
    for (int s = 0; s < N; ++s) {
        auto cell = juce::Rectangle<float>(in.getX() + s * cw, in.getY(), cw, in.getHeight());
        g.setColour(s % 4 == 0 ? col::lcdDim.withAlpha(0.35f) : col::lcdEdge);   // kick positions
        g.fillRect(cell.reduced(1, 0).withTrimmedTop(in.getHeight() - 4));
        if (s % 16 == 0 && s) { g.setColour(col::lcdDim); g.drawVerticalLine((int)cell.getX(), in.getY(), in.getBottom()); }
    }
    if (R.sustain) { g.setColour(col::lcdInk.withAlpha(0.5f)); g.fillRect(in.withHeight(in.getHeight() - 8).reduced(2, 6)); return; }
    for (auto& h : R.hits) {
        const double d = kc::grooveDelay(gs, h.step, R.eighthFeel, 124, 0) * 4.0;
        const float x = in.getX() + (float)((h.step + d) * cw);
        const float hh = (in.getHeight() - 8) * (0.35f + 0.65f * h.vel / 127.f);
        g.setColour(col::lcdInk.withAlpha(0.85f));
        g.fillRect(juce::Rectangle<float>(x + 1, in.getBottom() - 6 - hh, std::max(2.f, (float)h.len * cw - 2), hh));
    }
}

/* ---------------- drag MIDI ---------------- */
void DragMidi::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(col::cap); g.fillRoundedRectangle(b, 4); g.setColour(col::accent); g.drawRoundedRectangle(b, 4, 1.5f);
    g.setColour(col::ink); g.setFont(fonts.d(16.f, true));
    g.drawText(u8("\xE2\x87\xB1  DRAG MIDI"), b, juce::Justification::centred);
}
void DragMidi::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("Keirs Chords").getChildFile(
        juce::File::createLegalFileName("Keirs Chords " + juce::Time::getCurrentTime().formatted("%H%M%S")) + ".mid");
    f.getParentDirectory().createDirectory();
    if (proc.exportMidi(f)) juce::DragAndDropContainer::performExternalDragDropOfFiles({ f.getFullPathName() }, false, this);
}

/* ---------------- browser ---------------- */
Browser::Browser(KeirsChordsProcessor& p, const Fonts& f) : proc(p), fonts(f)
{
    for (auto* b : { &tabP, &tabR, &tabS, &gAll, &gHouse, &gTechno, &gJazz, &f1, &f2, &f3, &rnd }) addAndMakeVisible(*b);
    tabP.onClick = [this] { setTab(Progressions); };
    tabR.onClick = [this] { setTab(Rhythms); };
    tabS.onClick = [this] { setTab(PresetsTab); };
    auto setG = [this](int g) { genre = g; styleFilter = -1; refresh(false); };
    gAll.onClick = [setG] { setG(-1); }; gHouse.onClick = [setG] { setG(0); }; gTechno.onClick = [setG] { setG(1); }; gJazz.onClick = [setG] { setG(2); };
    f1.onClick = [this] {
        if (tab == Progressions) {
            juce::PopupMenu m; m.addItem(1, "All styles", true, styleFilter < 0);
            for (size_t i = 0; i < kc::styles().size(); ++i) if (genre < 0 || kc::styles()[i].genre == genre) m.addItem((int)i + 2, kc::styles()[i].name, true, styleFilter == (int)i);
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&f1), [this](int r) { if (r > 0) { styleFilter = r - 2; refresh(false); } });
        } else if (tab == Rhythms) { syncFilter = syncFilter >= 2 ? -1 : syncFilter + 1; refresh(false); }
        else { favOnly = !favOnly; refresh(false); }
    };
    f2.onClick = [this] {
        if (tab == Progressions) { tonality = (tonality + 1) % 3; refresh(false); }
        else if (tab == Rhythms) { sortMode = (sortMode + 1) % 2; refresh(false); }
        else { // save handled by the panel; here: refresh user presets
            refresh(false); }
    };
    f3.onClick = [this] {
        if (tab == PresetsTab) {
            if (presetRows.empty()) return;
            int cur = -1; for (size_t i = 0; i < presetRows.size(); ++i) if (presetRows[i]->name == proc.getCurrentPresetName()) cur = (int)i;
            if (cur >= 0) { auto k = kc::Presets::keyOf(*presetRows[(size_t)cur]); if (favs.contains(k)) favs.removeString(k); else favs.add(k); kc::Presets::saveFavs(favs); refresh(); }
        }
    };
    rnd.onClick = [this] {
        if (tab == PresetsTab) { if (!presetRows.empty()) proc.applyPreset(*presetRows[(size_t)juce::Random::getSystemRandom().nextInt((int)presetRows.size())]); }
        else proc.randomise(genre, tab == Progressions, tab == Rhythms);
        syncSelection();
    };
    search.setFont(fonts.b(15.f)); search.setTextToShowWhenEmpty("Search...", col::ink3);
    search.setIndents(8, 6);
    search.onTextChange = [this] { refresh(false); };
    addAndMakeVisible(search);
    list.setRowHeight(36); list.setOutlineThickness(0);
    addAndMakeVisible(list);
    favs = kc::Presets::loadFavs();
    setTab(Progressions);
}
void Browser::setTab(Tab t)
{
    tab = t; tabP.lit = t == Progressions; tabR.lit = t == Rhythms; tabS.lit = t == PresetsTab;
    if (t == PresetsTab) userPresets = kc::Presets::loadUser();
    for (auto* b : { &tabP, &tabR, &tabS }) b->repaint();
    refresh(false);
}
void Browser::updateFilterButtons()
{
    gAll.lit = genre < 0; gHouse.lit = genre == 0; gTechno.lit = genre == 1; gJazz.lit = genre == 2;
    if (tab == Progressions) {
        f1.text = styleFilter < 0 ? "ALL STYLES" : String(kc::styles()[(size_t)styleFilter].name).toUpperCase();
        f2.text = tonality == 0 ? "MAJ + MIN" : (tonality == 1 ? "MINOR" : "MAJOR"); f3.text = ""; f3.setVisible(false);
    } else if (tab == Rhythms) {
        static const char* S[] = { "SYNC: LOW", "SYNC: MED", "SYNC: HIGH" };
        f1.text = syncFilter < 0 ? "SYNC: ALL" : S[syncFilter]; f2.text = sortMode == 0 ? "SORT: GROOVE" : "SORT: NAME"; f3.setVisible(false);
    } else { f1.text = favOnly ? u8("\xE2\x98\x85 FAVOURITES") : "ALL PRESETS"; f2.text = "RELOAD"; f3.text = u8("\xE2\x98\x85 CURRENT"); f3.setVisible(true); }
    for (auto* b : { &gAll, &gHouse, &gTechno, &gJazz, &f1, &f2, &f3 }) b->repaint();
}
void Browser::refresh(bool keepScroll)
{
    const auto& L = kc::Library::get();
    const String q = search.getText().trim().toLowerCase();
    auto match = [&](const String& s) { if (q.isEmpty()) return true; for (auto& w : juce::StringArray::fromTokens(q, " ", "")) if (w.isNotEmpty() && !s.toLowerCase().contains(w)) return false; return true; };
    rows.clear(); presetRows.clear();
    if (tab == Progressions) {
        for (size_t i = 0; i < L.progs.size(); ++i) {
            auto& p = L.progs[i];
            if (genre >= 0 && p.genre != genre) continue;
            if (styleFilter >= 0 && p.style != styleFilter) continue;
            if (tonality == 1 && !p.minor) continue;
            if (tonality == 2 && p.minor) continue;
            if (!match(u8(p.name.c_str()) + " " + kc::styles()[(size_t)p.style].name + " " + p.tag + " " + kc::genreName(p.genre))) continue;
            rows.push_back((int)i);
        }
    } else if (tab == Rhythms) {
        for (size_t i = 0; i < L.rhythms.size(); ++i) {
            auto& r = L.rhythms[i];
            if (genre >= 0 && r.genre != genre) continue;
            if (syncFilter >= 0 && (r.sustain || r.syncClass != syncFilter)) continue;
            if (!match(u8(r.name.c_str()) + " " + r.tag + " " + kc::genreName(r.genre))) continue;
            rows.push_back((int)i);
        }
        if (sortMode == 0) std::stable_sort(rows.begin(), rows.end(), [&](int a, int b) { return L.rhythms[(size_t)a].groove > L.rhythms[(size_t)b].groove; });
    } else {
        auto addP = [&](const kc::PresetData& p) {
            if (genre >= 0 && p.genre != genre) return;
            if (favOnly && !favs.contains(kc::Presets::keyOf(p))) return;
            if (!match(p.name)) return;
            presetRows.push_back(&p);
        };
        for (auto& p : userPresets) addP(p);
        for (auto& p : kc::Presets::factory()) addP(p);
        for (size_t i = 0; i < presetRows.size(); ++i) rows.push_back((int)i);
    }
    updateFilterButtons();
    auto pos = list.getVerticalPosition();
    list.updateContent();
    if (keepScroll) list.setVerticalPosition(pos); else list.scrollToEnsureRowIsOnscreen(0);
    lastProg = lastRhythm = -2;
    syncSelection();
    repaint();
}
void Browser::syncSelection()
{
    const int pi = (int)proc.apvts.getRawParameterValue("prog")->load(), ri = (int)proc.apvts.getRawParameterValue("rhythm")->load();
    if (pi == lastProg && ri == lastRhythm && tab != PresetsTab) return;
    lastProg = pi; lastRhythm = ri;
    int target = -1;
    if (tab == Progressions) { for (size_t i = 0; i < rows.size(); ++i) if (rows[i] == pi) target = (int)i; }
    else if (tab == Rhythms) { for (size_t i = 0; i < rows.size(); ++i) if (rows[i] == ri) target = (int)i; }
    else { for (size_t i = 0; i < presetRows.size(); ++i) if (presetRows[i]->name == proc.getCurrentPresetName()) target = (int)i; }
    if (target >= 0) { list.selectRow(target, true, true); }
    else list.deselectAllRows();
    list.repaint();
}
void Browser::paint(juce::Graphics& g)
{
    g.setColour(col::panel); g.fillRoundedRectangle(getLocalBounds().toFloat(), 6);
    g.setColour(col::line); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6, 1);
    g.setColour(col::ink3); g.setFont(fonts.d(13.f));
    String count = String((int)rows.size()) + (tab == Progressions ? " progressions" : (tab == Rhythms ? " rhythms" : " presets"));
    g.drawText(count.toUpperCase(), getLocalBounds().removeFromBottom(30).reduced(12, 0), juce::Justification::centredLeft);
}
void Browser::resized()
{
    auto b = getLocalBounds().reduced(10);
    auto r1 = b.removeFromTop(30); const int tw = r1.getWidth() / 3;
    tabP.setBounds(r1.removeFromLeft(tw).reduced(1, 0)); tabR.setBounds(r1.removeFromLeft(tw).reduced(1, 0)); tabS.setBounds(r1.reduced(1, 0));
    b.removeFromTop(8);
    auto r2 = b.removeFromTop(26); const int gw = r2.getWidth() / 4;
    gAll.setBounds(r2.removeFromLeft(gw).reduced(1, 0)); gHouse.setBounds(r2.removeFromLeft(gw).reduced(1, 0)); gTechno.setBounds(r2.removeFromLeft(gw).reduced(1, 0)); gJazz.setBounds(r2.reduced(1, 0));
    b.removeFromTop(6);
    auto r3 = b.removeFromTop(26);
    f1.setBounds(r3.removeFromLeft(150).reduced(1, 0)); f2.setBounds(r3.removeFromLeft(120).reduced(1, 0)); f3.setBounds(r3.removeFromLeft(110).reduced(1, 0));
    b.removeFromTop(6);
    search.setBounds(b.removeFromTop(28));
    b.removeFromTop(6);
    auto foot = b.removeFromBottom(26);
    rnd.setBounds(foot.removeFromRight(110));
    b.removeFromBottom(6);
    list.setBounds(b);
}
void Browser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (!juce::isPositiveAndBelow(row, (int)rows.size())) return;
    const auto& L = kc::Library::get();
    auto r = juce::Rectangle<int>(0, 0, w, h).reduced(0, 1);
    g.setColour(selected ? col::accent.withAlpha(0.18f) : (row % 2 ? col::bg : col::panel)); g.fillRect(r);
    if (selected) { g.setColour(col::accent); g.fillRect(r.removeFromLeft(3)); } else r.removeFromLeft(3);
    r.removeFromLeft(8);
    String main, sub; int genre_ = 0; bool fav = false;
    if (tab == Progressions) {
        auto& p = L.progs[(size_t)rows[(size_t)row]];
        main = u8(p.name.c_str()); sub = String(kc::styles()[(size_t)p.style].name) + DOT + (p.minor ? "minor" : "major") + DOT + p.tag; genre_ = p.genre;
    } else if (tab == Rhythms) {
        auto& rr = L.rhythms[(size_t)rows[(size_t)row]];
        main = u8(rr.name.c_str()); genre_ = rr.genre;
        String stars; for (int i = 0; i < 5; ++i) stars += (rr.groove >= i + 0.75 ? u8("\xE2\x98\x85") : u8("\xE2\x98\x86"));
        sub = stars + DOT + (rr.sustain ? String("pad") : (String("sync ") + String(rr.sync, 1))) + DOT + String(rr.bars) + (rr.bars > 1 ? " bars" : " bar");
        // mini step strip
        auto strip = juce::Rectangle<float>((float)w - 132, (float)h / 2 - 5, 124, 10);
        const int N = 16 * rr.bars; const float cw = strip.getWidth() / N;
        for (int s = 0; s < N; ++s) { g.setColour(s % 4 == 0 ? col::groove : col::cap); g.fillRect(strip.getX() + s * cw, strip.getY(), cw - 0.5f, strip.getHeight()); }
        g.setColour(col::genre(rr.genre));
        if (rr.sustain) g.fillRect(strip.reduced(0, 3));
        else for (auto& hit : rr.hits) g.fillRect(strip.getX() + (float)hit.step * cw, strip.getY() + (1 - hit.vel / 127.f) * 6, std::max(1.5f, (float)hit.len * cw - 0.5f), strip.getHeight() - (1 - hit.vel / 127.f) * 6);
        r.removeFromRight(140);
    } else {
        auto* p = presetRows[(size_t)row];
        main = p->name; genre_ = p->genre; fav = favs.contains(kc::Presets::keyOf(*p));
        sub = String(kc::genreName(p->genre)) + (p->user ? DOT + "user" : String());
    }
    g.setColour(col::genre(genre_)); g.fillEllipse((float)r.getX(), (float)h / 2 - 3, 6, 6); r.removeFromLeft(14);
    if (fav) { g.setColour(col::accent); g.setFont(fonts.d(15.f)); g.drawText(u8("\xE2\x98\x85"), r.removeFromRight(22), juce::Justification::centred); }
    g.setColour(selected ? col::ink : col::ink.withAlpha(0.9f)); g.setFont(fonts.d(17.f));
    g.drawText(main, r.withTrimmedBottom(h / 2 - 2), juce::Justification::bottomLeft, true);
    g.setColour(col::ink3); g.setFont(fonts.d(13.f));
    g.drawText(sub, r.withTrimmedTop(h / 2 + 1), juce::Justification::topLeft, true);
}
void Browser::activate(int row)
{
    if (!juce::isPositiveAndBelow(row, (int)rows.size())) return;
    if (tab == Progressions) proc.setProgByIndex(rows[(size_t)row]);
    else if (tab == Rhythms) proc.setRhythmByIndex(rows[(size_t)row]);
    else proc.applyPreset(*presetRows[(size_t)row]);
}
void Browser::listBoxItemClicked(int row, const juce::MouseEvent&) { activate(row); }
void Browser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&) { activate(row); }

/* ---------------- panel ---------------- */
static juce::StringArray nums(int a, int b) { juce::StringArray s; for (int i = a; i <= b; ++i) s.add(String(i)); return s; }
static juce::StringArray choicesOf(juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(id))) return c->choices;
    return {};
}

Panel::Panel(KeirsChordsProcessor& p) : proc(p)
{
    setLookAndFeel(&look);
    auto& s = proc.apvts;
    addAndMakeVisible(browser); addAndMakeVisible(lcd); addAndMakeVisible(grooveView); addAndMakeVisible(drag);
    key = std::make_unique<Segmented>(*s.getParameter("key"), juce::StringArray{ "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }, 12, fonts, 0, 14.f);
    octave = std::make_unique<Segmented>(*s.getParameter("octave"), juce::StringArray{ u8("\xE2\x88\x92" "2"), u8("\xE2\x88\x92" "1"), "0", "+1", "+2" }, 5, fonts, -2);
    bars = std::make_unique<Segmented>(*s.getParameter("bars"), juce::StringArray{ "AUTO", "1", "2", "4", "8", "16" }, 6, fonts);
    swingMode = std::make_unique<Segmented>(*s.getParameter("swingMode"), juce::StringArray{ "OFF", "TR-909", "MPC2000XL", "HUMAN" }, 4, fonts);
    shuffle = std::make_unique<Segmented>(*s.getParameter("shuffle909"), nums(1, 7), 7, fonts, 1);
    swingRes = std::make_unique<Segmented>(*s.getParameter("swingRes"), juce::StringArray{ "AUTO", "16THS", "8THS" }, 3, fonts);
    playMode = std::make_unique<Segmented>(*s.getParameter("playMode"), juce::StringArray{ "FREE", "HOLD" }, 2, fonts);
    previewVoice = std::make_unique<Segmented>(*s.getParameter("previewSound"), juce::StringArray{ "KEYS", "PAD", "ORGAN", "STAB" }, 4, fonts);
    clock = std::make_unique<Segmented>(*s.getParameter("clockSrc"), juce::StringArray{ "INTERNAL", "MIDI CLOCK" }, 2, fonts);
    voicing = std::make_unique<ChoiceBox>(*s.getParameter("voicing"), choicesOf(s, "voicing"), fonts);
    shape = std::make_unique<ChoiceBox>(*s.getParameter("shape"), choicesOf(s, "shape"), fonts);
    rate = std::make_unique<ChoiceBox>(*s.getParameter("rate"), choicesOf(s, "rate"), fonts);
    bass = std::make_unique<ChoiceBox>(*s.getParameter("bass"), choicesOf(s, "bass"), fonts);
    chordCh = std::make_unique<ChoiceBox>(*s.getParameter("chordCh"), nums(1, 16), fonts, 1);
    bassCh = std::make_unique<ChoiceBox>(*s.getParameter("bassCh"), nums(1, 16), fonts, 1);
    complexity = std::make_unique<Knob>(s, "complexity", "Complexity", fonts, 50);
    variation = std::make_unique<Knob>(s, "variation", "Variation", fonts, 25);
    gate = std::make_unique<Knob>(s, "gate", "Gate", fonts, 100, "%");
    dynamics = std::make_unique<Knob>(s, "dynamics", "Dynamics", fonts, 55);
    swingPct = std::make_unique<Knob>(s, "swingPct", "Swing", fonts, 56, "%");
    feel = std::make_unique<Knob>(s, "feel", "Human Feel", fonts, 30);
    roll = std::make_unique<Knob>(s, "roll", "Roll", fonts, 3, " ms");
    pocket = std::make_unique<Knob>(s, "pocket", "Pocket", fonts, 0, " ms");
    previewVol = std::make_unique<Knob>(s, "previewVol", "Level", fonts, -6, " dB");
    bpm = std::make_unique<Knob>(s, "bpm", "Tempo", fonts, 124, " bpm");
    kickSpace = std::make_unique<Toggle>(*s.getParameter("kickSpace"), "KICK SPACE", fonts);
    rootFollow = std::make_unique<Toggle>(*s.getParameter("rootFollow"), "KEY SETS ROOT", fonts);
    preview = std::make_unique<Toggle>(*s.getParameter("preview"), "PREVIEW SOUND", fonts);
    for (juce::Component* c : std::initializer_list<juce::Component*>{ key.get(), octave.get(), bars.get(), swingMode.get(), shuffle.get(), swingRes.get(), playMode.get(), previewVoice.get(),
                                                                       voicing.get(), shape.get(), rate.get(), bass.get(), chordCh.get(), bassCh.get(), complexity.get(), variation.get(), gate.get(),
                                                                       dynamics.get(), swingPct.get(), feel.get(), roll.get(), pocket.get(), previewVol.get(), kickSpace.get(), rootFollow.get(), preview.get(),
                                                                       &prevP, &nextP, &saveP, &favP, &dice })
        addAndMakeVisible(c);
    for (int v : { 54, 58, 62, 66, 71 }) {
        auto b = std::make_unique<FlatButton>(String(v), fonts);
        b->onClick = [this, v] { if (auto* p2 = proc.apvts.getParameter("swingPct")) p2->setValueNotifyingHost(p2->convertTo0to1((float)v)); if (auto* m = proc.apvts.getParameter("swingMode")) if ((int)proc.apvts.getRawParameterValue("swingMode")->load() == 0) m->setValueNotifyingHost(m->convertTo0to1(2)); };
        addAndMakeVisible(*b); quickSwing.push_back(std::move(b));
    }
    if (proc.isStandalone()) { addAndMakeVisible(*clock); addAndMakeVisible(*bpm); addAndMakeVisible(play); play.onClick = [this] { proc.transportRunning = !proc.transportRunning.load(); }; }
    prevP.onClick = [this] { stepPreset(-1); }; nextP.onClick = [this] { stepPreset(1); };
    saveP.onClick = [this] { savePreset(); };
    favP.onClick = [this] { browser.setTab(Browser::PresetsTab); };
    dice.onClick = [this] { if (auto* sd = proc.apvts.getParameter("seed")) sd->setValueNotifyingHost(sd->convertTo0to1((float)(1 + juce::Random::getSystemRandom().nextInt(9999)))); };
    proc.onPresetLoaded = [this] { juce::MessageManager::callAsync([sp = juce::Component::SafePointer<Panel>(this)] { if (sp) { sp->browser.syncSelection(); sp->repaint(); } }); };
    setSize(W, H);
    startTimerHz(30);
}
Panel::~Panel() { proc.onPresetLoaded = nullptr; setLookAndFeel(nullptr); }

void Panel::stepPreset(int dir)
{
    const auto& F = kc::Presets::factory();
    if (F.empty()) return;
    int cur = -1; for (size_t i = 0; i < F.size(); ++i) if (F[i].name == proc.getCurrentPresetName()) cur = (int)i;
    int next = cur < 0 ? 0 : (cur + dir + (int)F.size()) % (int)F.size();
    proc.applyPreset(F[(size_t)next]);
}
void Panel::savePreset()
{
    auto* w = new juce::AlertWindow("Save preset", "Name this preset (saved to your user folder):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc.getCurrentPresetName().startsWith("Init") ? "My Groove" : proc.getCurrentPresetName() + " (edit)");
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey)); w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int r) {
        if (r == 1) { auto name = w->getTextEditorContents("name").trim(); if (name.isNotEmpty()) { kc::Presets::saveUser(proc.captureState(name)); browser.setTab(Browser::PresetsTab); } }
    }), true);
}

juce::Rectangle<int> Panel::section(juce::Graphics* g, juce::Rectangle<int> r, const String& title)
{
    if (g) {
        g->setColour(col::panel); g->fillRoundedRectangle(r.toFloat(), 6);
        g->setColour(col::line); g->drawRoundedRectangle(r.toFloat().reduced(0.5f), 6, 1);
        g->setColour(col::accent); g->fillRect(r.getX() + 12, r.getY() + 13, 3, 12);
        g->setColour(col::ink); g->setFont(fonts.d(16.f, true));
        g->drawText(title, r.getX() + 22, r.getY() + 8, 300, 22, juce::Justification::centredLeft);
    }
    return r.reduced(12).withTrimmedTop(26);
}

void Panel::paint(juce::Graphics& g)
{
    g.fillAll(col::bg);
    // header
    g.setColour(col::ink); g.setFont(fonts.d(30.f, true));
    g.drawText("KEIR'S CHORDS", 16, 10, 260, 38, juce::Justification::centredLeft);
    g.setColour(col::ink3); g.setFont(fonts.d(13.f));
    g.drawText("HOUSE " + u8("\xC2\xB7") + " TECHNO " + u8("\xC2\xB7") + " JAZZ   CHORD SEQUENCER", 16, 42, 300, 14, juce::Justification::centredLeft);
    auto pr = juce::Rectangle<int>(400, 12, 420, 36);
    g.setColour(col::lcd); g.fillRoundedRectangle(pr.toFloat(), 4); g.setColour(col::lcdEdge); g.drawRoundedRectangle(pr.toFloat().reduced(0.5f), 4, 1);
    g.setColour(col::lcdInk); g.setFont(fonts.l(22.f)); g.drawFittedText(proc.getCurrentPresetName(), pr.reduced(10, 2), juce::Justification::centred, 1, 0.7f);

    section(&g, rHarm, "HARMONY");
    section(&g, rGroove, "GROOVE & FEEL");
    section(&g, rArr, "RHYTHM & BASS");
    section(&g, rOut, "PLAY & OUTPUT");
    auto lbl = [&](juce::Component& c, const char* t) { g.setColour(col::ink2); g.setFont(fonts.d(13.f)); g.drawText(t, c.getX(), c.getY() - 15, c.getWidth(), 14, juce::Justification::centredLeft); };
    lbl(*key, "KEY"); lbl(*voicing, "VOICING"); lbl(*shape, "MEMORY SHAPE"); lbl(*octave, "OCTAVE");
    lbl(*swingMode, "SWING MODEL"); lbl(*shuffle, "909 SHUFFLE (1 = OFF)"); lbl(*swingRes, "SWING GRID");
    { g.setColour(col::ink2); g.setFont(fonts.d(13.f)); g.drawText("MPC CLASSICS %", quickSwing[0]->getX(), quickSwing[0]->getY() - 15, 160, 14, juce::Justification::centredLeft); }
    lbl(*bars, "LOOP LENGTH"); lbl(*rate, "CHORD CHANGE"); lbl(*bass, "BASS LINE");
    lbl(*playMode, "PLAY MODE"); lbl(*chordCh, "CHORD CH"); lbl(*bassCh, "BASS CH"); lbl(*previewVoice, "PREVIEW VOICE");
    if (proc.isStandalone()) lbl(*clock, "SYNC");
}

void Panel::resized()
{
    rHead = { 0, 0, W, 58 };
    prevP.setBounds(360, 16, 34, 28); nextP.setBounds(826, 16, 34, 28); favP.setBounds(866, 16, 34, 28); saveP.setBounds(906, 16, 70, 28);
    if (proc.isStandalone()) { play.setBounds(990, 14, 80, 32); }
    browser.setBounds(12, 64, 440, H - 76);
    const int x0 = 464, cw = W - x0 - 12;
    rLcd = { x0, 64, cw, 168 }; lcd.setBounds(rLcd);
    const int colW = (cw - 10) / 2;
    rHarm = { x0, 242, colW, 262 }; rGroove = { x0 + colW + 10, 242, colW, 262 };
    rArr = { x0, 514, colW, H - 526 }; rOut = { x0 + colW + 10, 514, colW, H - 526 };

    // HARMONY
    auto h = section(nullptr, rHarm, "");
    h.removeFromTop(14); key->setBounds(h.removeFromTop(26)); h.removeFromTop(20);
    auto row = h.removeFromTop(28); voicing->setBounds(row.removeFromLeft(row.getWidth() / 2 - 4)); row.removeFromLeft(8); shape->setBounds(row);
    h.removeFromTop(20); octave->setBounds(h.removeFromTop(24).removeFromLeft(h.getWidth() / 2 - 4));
    h.removeFromTop(6);
    auto kn = h; const int kw = kn.getWidth() / 2;
    complexity->setBounds(kn.removeFromLeft(kw).withSizeKeepingCentre(100, kn.getHeight())); variation->setBounds(kn.withSizeKeepingCentre(100, kn.getHeight()));

    // GROOVE
    auto gr = section(nullptr, rGroove, "");
    gr.removeFromTop(14); swingMode->setBounds(gr.removeFromTop(26)); gr.removeFromTop(20);
    auto r2 = gr.removeFromTop(24); shuffle->setBounds(r2.removeFromLeft(r2.getWidth() / 2 - 4)); r2.removeFromLeft(8);
    { const int qw = r2.getWidth() / 5; for (auto& b : quickSwing) b->setBounds(r2.removeFromLeft(qw).reduced(1, 0)); }
    gr.removeFromTop(6);
    grooveView.setBounds(gr.removeFromTop(52));
    gr.removeFromTop(4);
    auto kr = gr; const int k4 = kr.getWidth() / 4;
    swingPct->setBounds(kr.removeFromLeft(k4)); feel->setBounds(kr.removeFromLeft(k4)); roll->setBounds(kr.removeFromLeft(k4)); pocket->setBounds(kr);

    // RHYTHM & BASS
    auto a = section(nullptr, rArr, "");
    a.removeFromTop(14); bars->setBounds(a.removeFromTop(24)); a.removeFromTop(20);
    auto r3 = a.removeFromTop(28); rate->setBounds(r3.removeFromLeft(r3.getWidth() / 2 - 4)); r3.removeFromLeft(8); bass->setBounds(r3);
    a.removeFromTop(20);
    auto r4 = a.removeFromTop(26); kickSpace->setBounds(r4.removeFromLeft(r4.getWidth() / 2 - 4)); r4.removeFromLeft(8); swingRes->setBounds(r4);
    a.removeFromTop(4);
    auto kr2 = a; const int k3 = kr2.getWidth() / 3;
    gate->setBounds(kr2.removeFromLeft(k3)); dynamics->setBounds(kr2.removeFromLeft(k3)); dice.setBounds(kr2.withSizeKeepingCentre(kr2.getWidth() - 10, 30));

    // PLAY & OUTPUT
    auto o = section(nullptr, rOut, "");
    o.removeFromTop(14);
    auto r5 = o.removeFromTop(26); playMode->setBounds(r5.removeFromLeft(r5.getWidth() / 2 - 4)); r5.removeFromLeft(8); rootFollow->setBounds(r5);
    o.removeFromTop(20);
    auto r6 = o.removeFromTop(28); chordCh->setBounds(r6.removeFromLeft(70)); r6.removeFromLeft(8); bassCh->setBounds(r6.removeFromLeft(70)); r6.removeFromLeft(8); preview->setBounds(r6);
    o.removeFromTop(20);
    auto r7 = o.removeFromTop(24); previewVoice->setBounds(r7);
    o.removeFromTop(8);
    auto r8 = o;
    if (proc.isStandalone()) {
        previewVol->setBounds(r8.removeFromLeft(80)); bpm->setBounds(r8.removeFromLeft(80)); r8.removeFromLeft(6);
        auto cr = r8.removeFromTop(r8.getHeight() / 2); clock->setBounds(cr.withTrimmedTop(16).withHeight(24));
        drag.setBounds(r8.withSizeKeepingCentre(r8.getWidth(), 34));
    } else {
        previewVol->setBounds(r8.removeFromLeft(90)); r8.removeFromLeft(10);
        drag.setBounds(r8.withSizeKeepingCentre(r8.getWidth(), 40));
    }
    (void)rHead;
}

void Panel::timerCallback()
{
    lcd.repaint(); grooveView.repaint();
    browser.syncSelection();
    if (proc.isStandalone()) { play.text = proc.transportRunning.load() ? "STOP" : "PLAY"; play.repaint(); }
    const auto nm = proc.getCurrentPresetName();
    if (nm != presetName) { presetName = nm; repaint(400, 12, 420, 36); }
}

/* ---------------- editor ---------------- */
KeirsChordsEditor::KeirsChordsEditor(KeirsChordsProcessor& p) : juce::AudioProcessorEditor(p), panel(p)
{
    addAndMakeVisible(panel);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio((double)Panel::W / Panel::H);
    setResizeLimits(Panel::W / 2, Panel::H / 2, Panel::W * 2, Panel::H * 2);
    setSize(1080, juce::roundToInt(1080.0 * Panel::H / Panel::W));
}
void KeirsChordsEditor::resized()
{
    const float s = getWidth() / (float)Panel::W;
    panel.setTransform(juce::AffineTransform::scale(s));
}

} // namespace kcui
