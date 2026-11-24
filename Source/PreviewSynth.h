#pragma once
// A small built-in synth so the plugin makes sound on its own (switch it off when it drives your own synths).
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace kc {

struct PreviewSound : public juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class PreviewVoice : public juce::SynthesiserVoice {
public:
    std::atomic<int>* mode = nullptr;   // 0 keys, 1 pad, 2 organ, 3 stab
    bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<PreviewSound*>(s) != nullptr; }

    void startNote(int note, float velocity, juce::SynthesiserSound*, int) override {
        freq = juce::MidiMessage::getMidiNoteInHertz(note); vel = velocity; t = 0; p1 = p2 = p3 = 0; lp = 0; releasing = false; relGain = 1;
        m = mode ? mode->load() : 0;
        const double sr = getSampleRate();
        switch (m) { case 1: atk = 0.35 * sr; dec = 1e9; break; case 2: atk = 0.004 * sr; dec = 1e9; break; case 3: atk = 0.002 * sr; dec = 0.22 * sr; break; default: atk = 0.002 * sr; dec = 1.6 * sr; }
        rel = (m == 1 ? 0.6 : (m == 2 ? 0.05 : 0.25)) * sr;
    }
    void stopNote(float, bool allowTailOff) override { if (allowTailOff) releasing = true; else { clearCurrentNote(); } }
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& out, int start, int num) override {
        if (!isVoiceActive()) return;
        const double sr = getSampleRate(), inc = freq / sr, twoPi = juce::MathConstants<double>::twoPi;
        for (int i = 0; i < num; ++i) {
            double env = t < atk ? t / atk : std::exp(-(t - atk) / dec);
            if (releasing) { relGain -= 1.0 / rel; if (relGain <= 0) { clearCurrentNote(); break; } }
            double s = 0;
            switch (m) {
                case 1: {   // pad: two detuned saws through a soft low-pass
                    p1 += inc * 1.003; p2 += inc * 0.997; p1 -= std::floor(p1); p2 -= std::floor(p2);
                    double raw = (2 * p1 - 1) + (2 * p2 - 1);
                    lp += (raw - lp) * 0.06; s = lp * 0.35;
                } break;
                case 2: {   // organ: drawbar-ish sines + key click
                    p1 += inc; p1 -= std::floor(p1);
                    s = 0.55 * std::sin(twoPi * p1) + 0.35 * std::sin(twoPi * 2 * p1) + 0.18 * std::sin(twoPi * 3 * p1) + 0.12 * std::sin(twoPi * 4 * p1);
                    if (t < sr * 0.004) s += 0.15 * std::sin(twoPi * 9 * p1) * (1 - t / (sr * 0.004));
                    s *= 0.45;
                } break;
                case 3: {   // stab: saw + square with a closing filter
                    p1 += inc; p1 -= std::floor(p1);
                    double raw = (2 * p1 - 1) * 0.7 + (p1 < 0.5 ? 0.3 : -0.3);
                    double cut = 0.02 + 0.5 * std::exp(-t / (0.06 * sr));
                    lp += (raw - lp) * cut; s = lp * 0.6;
                } break;
                default: {  // keys: two-operator FM electric piano
                    p1 += inc; p1 -= std::floor(p1);
                    double idx = 1.6 * std::exp(-t / (0.25 * sr));
                    s = std::sin(twoPi * p1 + idx * std::sin(twoPi * p1)) * 0.5;
                } break;
            }
            float v = (float)(s * env * relGain * (0.25 + 0.75 * vel) * 0.22);
            for (int c = 0; c < out.getNumChannels(); ++c) out.addSample(c, start + i, v);
            t += 1;
        }
    }
private:
    double freq = 440, vel = 1, t = 0, p1 = 0, p2 = 0, p3 = 0, lp = 0, atk = 1, dec = 1, rel = 1, relGain = 1; int m = 0; bool releasing = false;
};

} // namespace kc
