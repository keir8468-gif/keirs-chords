# Installing Keir's Chords (macOS)

## 1. Download
Go to the repo's **Actions** tab and open the latest green **Build macOS** run. Download the **KeirsChords-mac** artifact and unzip it. If the repo has a tagged release, the zip is also on the Releases page.

## 2. Remove the download quarantine
The build is ad-hoc signed and not notarised, so macOS will block it until you clear the quarantine flag. Run this in Terminal from inside the unzipped folder:

```
xattr -dr com.apple.quarantine "Keir's Chords.vst3" "Keir's Chords.component" "Keir's Chords.app"
```

## 3. Copy into place
| File | Goes to |
|---|---|
| `Keir's Chords.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |
| `Keir's Chords.component` | `~/Library/Audio/Plug-Ins/Components/` |
| `Keir's Chords.app` | `/Applications/` |

Then restart Live. In **Preferences → Plug-Ins**, make sure *Use VST3 Plug-in Custom Folder* or *System Folders* is on, and press **Rescan**.

## 4. Route it in Ableton Live
Live doesn't host third-party MIDI-effect plugins, so Keir's Chords loads as an **instrument** that also outputs MIDI.

1. Make a MIDI track called **Chords** and drop **Keir's Chords** (VST3) on it.
2. On each synth track that should play the chords, set **MIDI From** to *Chords*, choose *Keir's Chords* in the second box, and set **Monitor** to *In*.
3. Turn **Preview Sound** off in the plugin once your real synths are playing. While it's on, you'll hear the plugin's built-in keys/pad/organ/stab sound.
4. **Bass:** set a bass synth's *MIDI From* to the same track, and in the plugin set **Bass Ch** to 2. On the synth's track, pick channel 2 in the second *MIDI From* box. Chords go out on **Chord Ch** (1 by default).
5. Press play in Live. The plugin follows Live's transport and tempo.

**Hold mode:** switch Play Mode to **HOLD**, arm the Chords track and play your keyboard. The pattern only runs while a key is held. With *Key Sets Root* on, the lowest key you hold transposes the progression.

**Drag MIDI:** drag the **DRAG MIDI** button into an empty clip slot to get the current pattern, with swing applied, as a MIDI clip.

## 5. Standalone app
Open `Keir's Chords.app`. It creates a virtual MIDI port called **Keir's Chords** that any app, including Live, can receive from. Set the tempo with the Tempo knob, or set Clock to *MIDI Clock* to sync from another device.
