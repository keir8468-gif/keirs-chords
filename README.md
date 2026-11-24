# Keir's Chords

A chord-pattern sequencer for **House, Techno and Jazz**, built as a JUCE VST3 / AU plugin and a standalone app. Pick a progression, put it on a rhythm and a swing model, and send the MIDI to any instrument.

## What's in it
- **1,392 progressions**: House 683, Techno 378 and Jazz 331, across 22 styles. Examples include Chicago Piano, Jersey Garage, Deep House, Soulful Gospel, Organ House, Disco House, UK Garage, Detroit Strings, Chord Memory, Rave Stabs, Warehouse, Dub Techno, Melodic Techno, Hi-Tech Jazz, ii-V-I, Minor ii-V-i, Turnarounds, Modal, Neo-Soul, Bossa Nova, Ballad and Blues.
- **563 rhythm patterns**: House 213, Techno 171 and Jazz 179. Each one is scored against a four-on-the-floor kick and given a **groove rating**. Machine-made variants that fight the kick are thrown away.
- **Swing models**
  - **TR-909**: the six real shuffle positions (54.2 → 75%).
  - **MPC2000XL**: Linn swing at 50–75%, snapped to 96 PPQN, with the classic 54/58/62/66/71 buttons.
  - **Human**: tempo-relaxed swing ratio, slightly laid-back downbeats and tight micro-timing.
- **Feel controls**
  - Roll: strums the chord bottom-up.
  - Pocket: plays the stabs ahead of or behind the beat while the bass stays on the grid.
  - Kick Space: ducks stabs that land on the kick.
  - Phrase dynamics: builds through 4-bar phrases.
- **Voicings**: Rootless A/B, 7th-on-bottom, Quartal "So What", Spread pad, Chicago piano, Shell, Drop-2 and Chord Memory (parallel).
- **Key / scale**, so the plugin stays in the key of your track. **3,000 factory presets**, plus user presets and favourites.
- **Free** or **Hold-to-play** modes. Bass lines on a separate channel: held, off-beat, organ bounce, walking or pedal.
- **Drag MIDI** to a clip. The standalone app has a virtual MIDI port and MIDI-clock sync.

See **[INSTALL.md](INSTALL.md)** for setup in Ableton Live, and **[docs/RESEARCH.md](docs/RESEARCH.md)** for the research behind the groove engine.

## Building
macOS builds come from GitHub Actions (`.github/workflows/build-mac.yml`). For a local build:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release        # fetches JUCE 8.0.4
cmake --build build --target KeirsChords_VST3 KeirsChords_Standalone
```

Pass `-DKC_TESTS=ON` to build `plugin_test`, a host simulation that checks timing, stuck notes, hold mode and MIDI export. `engine_test` stress-tests every progression × rhythm combination.
