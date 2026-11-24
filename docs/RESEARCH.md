# The research behind Keir's Chords

The goal: patterns that make people **move** and **feel** something. This file explains what the engine does and the evidence behind each choice.

## 1. Groove and syncopation: aim for the middle
- Witek et al. (2014, *PLOS ONE*, "Syncopation, Body-Movement and Pleasure in Groove Music") found an **inverted U**. Medium syncopation makes people want to move most; too little is boring and too much loses the beat.
  → Every rhythm gets a Longuet-Higgins & Lee (1984) syncopation score. The groove rating rewards the middle band and penalises both extremes.
- Matthews et al. (2019/2020, *PLOS ONE*) found the same inverted U for **harmonic complexity**, and it interacts with rhythm. Moderately complex chords (7ths and 9ths) over a moderately syncopated rhythm gave the strongest urge to move.
  → The *Complexity* knob defaults to the middle. Styles are tuned so that busy rhythms get simpler chords and sparse rhythms get richer ones.
- Janata, Tomic & Haberman (2012) found that groove is felt as pleasurable sensorimotor coupling: people *want* to move, and it feels good.

## 2. Moving around the four-on-the-floor kick
The kick sits on every beat (16th steps 0, 4, 8 and 12) and the closed hat sits on the off-beats (2, 6, 10 and 14). Stabs groove when they **talk to** that grid instead of doubling it.
- **Pickups** on the last 16th before a beat (steps 3, 7, 11 and 15) pull the ear forward into the kick. This is the "push" in house piano and garage organ.
- **Off-beat stabs** lock with the hat and the bass. This is the classic Chicago and Detroit pump.
- **Counter-meter runs**, such as hits spaced 3 steps apart and the tresillo 3+3+2, set up a second pulse against the kick. That tension is what drives Detroit chord-memory riffs.
- **Kick collisions** are penalised. A stab on the kick masks both sounds and flattens the groove.
  → `analyseAgainstKick()` counts these features and `grooveScore()` turns them into the 0–5 rating shown in the browser. **Kick Space** also ducks the velocity of any hit that does land on the kick.
- Studies of low-frequency content report that bass and low end strengthen the urge to move. → The bass line has its own channel and stays near the grid while the stabs carry the push.

## 3. Swing: small is groovy, exaggerated isn't
- The **TR-909** shuffle has six positions. Each delays the even 16ths by 2/96 of a beat more than the last: **54.2, 58.3, 62.5, 66.7, 70.8 and 75%**.
- Roger Linn's swing on the LM-1, LinnDrum and MPC delays every second 16th, and Linn has described the 54–62% range as where the feel lives. The **MPC2000XL** snaps to 96 PPQN, so 54% is literally a 2-tick delay.
- Senn et al. (Lucerne Groove Research Institute) found that **exaggerated microtiming lowers groove ratings**. Listeners preferred patterns close to the grid or with small, consistent deviations.
  → *Human* mode keeps jitter tight. It lands downbeats a hair late and relaxes the swing ratio as tempo rises, which is what drummers do: swing ratios get straighter at faster tempos (Friberg & Sundström 2002).
- Tempo: the preference for moving to music peaks around **2 Hz, roughly 120–130 bpm** (van Noorden & Moelants 1999; Moelants 2002). House and techno sit right on it.

## 4. Chords that carry emotion
- Lahdelma & Eerola (2016) found that **minor 7th, major 7th and minor triads** are heard as nostalgic, longing and tender, the emotional core of deep house and Detroit. → These are the default qualities for the Deep, Detroit, Soulful and Melodic styles.
- **Rootless and 7th-on-bottom voicings** (Bill Evans; Kerri Chandler's organ and keys) keep the colour tones in the middle register and leave room for the bass.
- **Smooth voice leading**: the engine chooses each voicing to minimise movement and penalises jumps in the top line, so the melody you hear on top stays singable.
- **Phrase dynamics**: 4-bar phrases build in velocity and accent their downbeat. This gives the tension-and-release arc that DJs use to work a floor.

## 5. Styles studied
Breakdowns of the harmony and rhythm in Chicago piano house (Larry Heard, Frankie Knuckles), Jersey and garage (Kerri Chandler, Masters At Work, the Todd Edwards pushes), Detroit (Derrick May's chord memory, Carl Craig, Underground Resistance's "Hi-Tech Jazz"), dub techno (Basic Channel) and jazz (ii-V-I, backdoor and tritone substitutions, Bill Evans voicings, modal "So What", neo-soul and bossa). These were used to write the hand-made core progressions and rhythms. The expansion rules then generate idiomatic variations: substitutions, lead-ins, pedals, displacements, anticipations and two-bar answers.

## References
- Witek, M. A. G., Clarke, E. F., Wallentin, M., Kringelbach, M. L., Vuust, P. (2014). Syncopation, body-movement and pleasure in groove music. *PLOS ONE* 9(4).
- Matthews, T. E., Witek, M. A. G., et al. (2019). The sensation of groove is affected by the interaction of rhythmic and harmonic complexity. *PLOS ONE* 14(1).
- Janata, P., Tomic, S. T., Haberman, J. M. (2012). Sensorimotor coupling in music and the psychology of the groove. *JEP: General* 141(1).
- Senn, O., Kilchenmann, L., et al. — microtiming and groove studies, Lucerne University of Applied Sciences and Arts.
- Longuet-Higgins, H. C., Lee, C. S. (1984). The rhythmic interpretation of monophonic music. *Music Perception* 1(4).
- Lahdelma, I., Eerola, T. (2016). Single chords convey distinct emotional qualities to both naïve and expert listeners. *Psychology of Music* 44(1).
- van Noorden, L., Moelants, D. (1999). Resonance in the perception of musical pulse. *JNMR* 28(1). Moelants, D. (2002). Preferred tempo reconsidered.
- Friberg, A., Sundström, A. (2002). Swing ratios and ensemble timing in jazz performance. *Music Perception* 19(3).
- Roland TR-909 owner's manual (shuffle); Roger Linn interviews on MPC swing.
