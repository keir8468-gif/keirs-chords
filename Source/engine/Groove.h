#pragma once
// Swing / groove models. Every model returns a non-negative delay (in beats) for an event on the 16th grid,
// so playback never has to look ahead of the grid.
//
//  TR-909    : the 909 delays each even-numbered 16th by 2/96, 4/96 ... 12/96 of a beat for shuffle
//              settings 1..6 (key 1 on the original = no shuffle; here 1 = off, 7 = maximum).
//  MPC2000XL : Roger Linn swing: 50% = straight, 66% = triplet; the 2nd 16th of each pair is placed at
//              swing% of the pair and the position snaps to the MPC's 96 PPQN grid.
//  Human     : swing ratio that straightens as the tempo rises (as measured in jazz drummers), slightly late
//              downbeats, small drifting micro-timing and velocity life. Research shows groove holds up when
//              deviations stay small and drops when they are exaggerated, so "Feel" stays tight by default.
#include "Theory.h"

namespace kc {

enum class SwingMode { Off = 0, TR909 = 1, MPC = 2, Human = 3 };

struct GrooveSettings {
    SwingMode mode = SwingMode::Off;
    int shuffle909 = 3;        // 1..7  (1 = none)
    double swingPct = 58;      // 50..75 (MPC and Human)
    int resolution = 0;        // 0 auto (pattern decides), 1 = 16ths, 2 = 8ths
    double feel = 30;          // Human: 0..100
};

// position within the beat of a 16th step: is it the "off" note of a swing pair?
inline bool isSwungStep(int step, bool eighths) {
    return eighths ? ((step % 4 + 4) % 4 == 2) : ((step % 2 + 2) % 2 == 1);
}

// delay in beats for an event at 'step' (16th index). 'occurrence' makes the Human model vary per pass.
inline double grooveDelay(const GrooveSettings& g, double step, bool patternEighths, double bpm, uint32_t occurrence) {
    const int istep = (int)std::lround(step);
    const bool eighths = g.resolution == 2 || (g.resolution == 0 && patternEighths);
    const bool onGrid = std::abs(step - istep) < 0.01;
    const bool swung = onGrid && isSwungStep(istep, eighths);
    const double unit = eighths ? 0.5 : 0.25;            // length of one note of the swing pair, in beats
    double d = 0;
    switch (g.mode) {
        case SwingMode::TR909:
            if (swung) d = (clampv(g.shuffle909, 1, 7) - 1) * (2.0 / 96.0) * (eighths ? 2.0 : 1.0);
            break;
        case SwingMode::MPC:
            if (swung) {
                double pct = clampv(g.swingPct, 50.0, 75.0) / 100.0;
                double ticks = std::round((2 * unit * pct - unit) * 96.0);   // snap to 96 PPQN
                d = ticks / 96.0;
            }
            break;
        case SwingMode::Human: {
            double pct = clampv(g.swingPct, 50.0, 75.0) / 100.0;
            // swing ratio relaxes towards straight as tempo rises (reference 100 bpm)
            double tempoScale = clampv(std::sqrt(100.0 / std::max(40.0, bpm)), 0.75, 1.25);
            double eff = 0.5 + (pct - 0.5) * tempoScale;
            if (swung) d = 2 * unit * eff - unit;
            const double feel = clampv(g.feel, 0.0, 100.0) / 100.0;
            const double msToBeats = bpm / 60000.0;
            Rng r(hashSeed(occurrence, (uint32_t)(istep * 977 + 13)));
            double laid = 6.0 * feel;                                       // ms: everything sits slightly back
            double downbeat = (onGrid && istep % 4 == 0) ? 4.0 * feel : 0;  // late downbeats
            double jitter = (r.next() * 2 - 1) * 5.0 * feel;                // ms, tight
            d += std::max(0.0, laid + downbeat + jitter) * msToBeats;
        } break;
        default: break;
    }
    return std::max(0.0, d);
}

inline int grooveVelocity(const GrooveSettings& g, int vel, double step, uint32_t occurrence) {
    if (g.mode != SwingMode::Human) return vel;
    Rng r(hashSeed(occurrence * 31u + 7u, (uint32_t)std::lround(step * 4)));
    double f = clampv(g.feel, 0.0, 100.0) / 100.0;
    return clampv((int)std::lround(vel + (r.next() * 2 - 1) * 10 * f), 1, 127);
}

inline const std::vector<std::string>& swingModeNames() { static const std::vector<std::string> v = { "Off", "TR-909", "MPC2000XL", "Human" }; return v; }

} // namespace kc
