#pragma once
// Turns (progression + rhythm + settings) into a loop of note events. Deterministic for a given seed.
#include "Library.h"

namespace kc {

struct Settings {
    int prog = 0, rhythm = 0;
    int key = 0;              // tonic pitch class
    int voicing = 0;          // Voicing enum index (0 = style default)
    int octave = 0;           // -2..+2
    double complexity = 50;   // 0..100
    double variation = 25;    // 0..100
    int barsMode = 0;         // 0 auto, else 1/2/4/8/16
    int rateMode = 0;         // 0 auto, 1 = 2 per bar, 2 = 1 per bar, 3 = 1 per 2 bars, 4 = 1 per 4 bars
    int bass = 0;             // 0 off, 1 held, 2 off-beat, 3 organ bounce, 4 walking, 5 tonic pedal, 6 follow
    double dynamics = 50;     // 0..100
    double gate = 100;        // 10..150 %
    int shape = 0;            // parallel shape (0 auto)
    uint32_t seed = 1;
};

struct Event { double step; double len; int pitch; int vel; bool bass; int chord; bool eighthFeel; int voice = 0; };   // voice = order within its chord, low to high (for the roll)
struct ChordSlot { std::string name; double start, len; std::vector<int> pitches; };
struct Pattern {
    std::vector<Event> events;
    std::vector<ChordSlot> chords;  // one entry per chord change in the loop
    int lengthSteps = 16;
    bool eighthFeel = false;
    std::string progName, rhythmName, styleName;
};

struct Shape { std::vector<int> iv; const char* sym; double w; };
inline const std::vector<Shape>& parallelShapes() {
    static const std::vector<Shape> s = {
        { { 0, 3, 7, 10 }, "m7", 3 }, { { 3, 7, 10, 14 }, "m9", 3 }, { { 0, 5, 10, 15, 19 }, "m11", 2 }, { { 0, 3, 7, 12 }, "m", 2 },
        { { 0, 7, 10, 14 }, "7sus2", 2 }, { { -2, 0, 3, 5 }, "m7(11)", 2 }, { { 0, 4, 7, 11 }, "maj7", 1 }, { { 0, 7, 12, 15 }, "m(5)", 1 },
    };
    return s;
}
inline const char* shapeName(int i) { return i <= 0 ? "Auto" : parallelShapes()[(size_t)clampv(i - 1, 0, (int)parallelShapes().size() - 1)].sym; }

inline std::string pcName(int pc, bool flats) {
    static const char* sh[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* fl[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    return (flats ? fl : sh)[mod12(pc)];
}
inline bool useFlats(int key, bool minor) { return minor ? !(key == 1 || key == 4 || key == 6 || key == 8 || key == 11) : (key == 0 || key == 5 || key == 10 || key == 3 || key == 8 || key == 1); }

inline Pattern generate(const Settings& s) {
    const auto& L = Library::get();
    Pattern out;
    if (L.progs.empty() || L.rhythms.empty()) return out;
    const Progression& P = L.progs[(size_t)clampv(s.prog, 0, (int)L.progs.size() - 1)];
    const Rhythm& R = L.rhythms[(size_t)clampv(s.rhythm, 0, (int)L.rhythms.size() - 1)];
    const Style& st = styles()[(size_t)P.style];
    out.progName = P.name; out.rhythmName = R.name; out.styleName = st.name; out.eighthFeel = R.eighthFeel;

    Rng rProg(hashSeed(s.seed, 11 + (uint32_t)s.prog)), rVar(hashSeed(s.seed, 23 + (uint32_t)s.rhythm)), rVel(hashSeed(s.seed, 37));
    const double variation = clampv(s.variation, 0.0, 100.0);

    // ---- harmonic rhythm & loop length
    std::vector<int> slotChord; for (size_t i = 0; i < P.chords.size(); ++i) for (int k = 0; k < P.chords[i].slots; ++k) slotChord.push_back((int)i);
    static const int RATE[] = { 0, 8, 16, 32, 64 };
    int slotSteps = RATE[clampv(s.rateMode, 0, 4)];
    if (!slotSteps) {
        slotSteps = st.rateSteps;
        while ((int)slotChord.size() * slotSteps > 16 * 16 && slotSteps > 4) slotSteps /= 2;
    }
    // dance loops want 1/2/4/8/16 bars: in Auto, let the last chord ring on to the next power-of-two bar count
    if (s.rateMode == 0 && s.barsMode == 0 && st.name != std::string("Blues")) {
        int steps = (int)slotChord.size() * slotSteps;
        if (steps % 16 == 0 || steps > 16) {
            int barsNeeded = (steps + 15) / 16, pow2 = 1; while (pow2 < barsNeeded) pow2 *= 2;
            int extra = (pow2 * 16 - steps) / slotSteps;
            for (int k = 0; k < extra; ++k) slotChord.push_back(slotChord.back());
        }
    }
    int loop;
    if (s.barsMode > 0) loop = 16 * s.barsMode;
    else {
        loop = (int)slotChord.size() * slotSteps;
        loop = std::max(16, ((loop + 15) / 16) * 16);
        if (R.bars == 2 && (loop / 16) % 2) loop *= 2;
        loop = std::min(loop, 16 * 16);
    }
    out.lengthSteps = loop;
    const int nSlots = std::max(1, (int)std::ceil(loop / (double)slotSteps));

    // ---- chord qualities
    int level = clampv(st.level + (int)std::lround((s.complexity - 50) / 25.0), 0, 3);
    const bool jersey = st.name == std::string("Jersey Garage") || st.name == std::string("Deep House");
    struct C { int root; std::vector<int> iv; std::string sym; int bassPc; bool slash; };
    std::vector<C> chords;
    const int key = mod12(s.key);
    const bool flats = useFlats(key, P.minor);

    int shapeIdx = s.shape;
    if (P.parallel && shapeIdx <= 0) {
        std::vector<std::pair<int, double>> w;
        for (size_t i = 0; i < parallelShapes().size(); ++i) {
            double ww = parallelShapes()[i].w; auto sym = std::string(parallelShapes()[i].sym);
            if (s.complexity < 35 && (sym == "m9" || sym == "m11")) ww *= 0.25;
            if (s.complexity > 65 && (sym == "m" || sym == "m7" || sym == "m(5)")) ww *= 0.3;
            if (st.name == std::string("Rave Stabs") && (sym == "m7" || sym == "m")) ww *= 3;
            w.push_back({ (int)i + 1, ww });
        }
        shapeIdx = pickWeighted(rProg, w);
    }
    for (auto& pc : P.chords) {
        C c; c.root = mod12(key + pc.deg);
        if (P.parallel) {
            auto& sh = parallelShapes()[(size_t)clampv(shapeIdx - 1, 0, (int)parallelShapes().size() - 1)];
            c.iv = sh.iv; c.sym = sh.sym;
        } else {
            int lv = level;
            if (rProg.chance(variation / 100.0 * 0.25)) lv = clampv(lv + (rProg.chance(0.5) ? -1 : 1), 0, 3);
            std::string q = pc.fixed.empty() ? familyQuality(pc.fam, lv) : pc.fixed;
            auto& Q = quality(q); c.iv = Q.iv; c.sym = Q.sym;
            if (jersey && pc.fam == "m" && lv >= 1 && rProg.chance(0.4)) { c.iv = quality("m7_11").iv; c.sym = quality("m7_11").sym; }
            c.iv = trimTones(c.iv, st.maxVoices, true);
        }
        c.slash = pc.bass >= 0;
        c.bassPc = mod12(key + (pc.bass >= 0 ? pc.bass : pc.deg));
        chords.push_back(c);
    }

    // ---- voicings
    Voicing vm = s.voicing > 0 ? (Voicing)s.voicing : st.voicing;
    if (P.parallel && s.voicing == 0) vm = Voicing::Parallel;
    const double center = st.register_ + 12 * clampv(s.octave, -2, 2);
    std::vector<std::vector<int>> voiced;
    std::vector<int> refShape; int prevRoot = 0, firstRoot = 0;
    for (size_t i = 0; i < chords.size(); ++i) {
        auto& c = chords[i]; std::vector<int> v; const std::vector<int>* prev = voiced.empty() ? nullptr : &voiced.back();
        auto voiceWith = [&](Voicing m) -> std::vector<int> {
            switch (m) {
                case Voicing::Close:         return voiceClose(c.root, c.iv, center, prev, false);
                case Voicing::Rootless:      return voiceRootlessAB(c.root, c.iv, center, prev);
                case Voicing::SeventhBottom: return voiceSeventhBottom(c.root, c.iv, center, prev);
                case Voicing::Quartal:       return voiceQuartal(c.root, c.iv, center, prev, st.maxVoices);
                case Voicing::Spread:        return voiceSpread(c.root, c.iv, center, prev);
                case Voicing::Piano:         return voicePiano(c.root, c.iv, center, prev, c.bassPc);
                case Voicing::Shell:         return voiceShell(c.root, c.iv, center, prev);
                case Voicing::Drop2:         return voiceDrop2(c.root, c.iv, center, prev);
                default:                     return voiceClose(c.root, c.iv, center, prev, false);
            }
        };
        if (vm == Voicing::Parallel) {
            if (refShape.empty()) {
                if (P.parallel) refShape = c.iv;
                else { Voicing base = st.voicing == Voicing::Parallel ? Voicing::Close : st.voicing; auto v0 = voiceWith(base); int r0 = v0[0] - mod12(v0[0] - c.root); for (int x : v0) refShape.push_back(x - r0); }
                double m = meanOf(refShape);
                firstRoot = (int)std::lround(center - m);
                firstRoot += mod12(c.root - mod12(firstRoot));
                if (firstRoot - (center - m) > 6) firstRoot -= 12;
                prevRoot = firstRoot;
            }
            int best = prevRoot; double bs = 1e9;
            for (int k = -2; k <= 2; ++k) { int cand = prevRoot + 12 * k; cand += mod12(c.root - mod12(cand)); for (int adj : { -12, 0 }) { int x = cand + adj; double sc = std::abs(x - prevRoot) + 0.5 * std::abs(x - firstRoot); if (sc < bs) { bs = sc; best = x; } } }
            prevRoot = best;
            for (int x : refShape) v.push_back(x + best);
            if (!P.parallel) c.sym += "*";
        } else v = voiceWith(vm);
        v.erase(std::remove_if(v.begin(), v.end(), [](int x) { return x < 0 || x > 127; }), v.end());
        voiced.push_back(v);
    }

    // ---- chord display list
    for (int sl = 0; sl < nSlots; ++sl) {
        int ci = slotChord[(size_t)(sl % (int)slotChord.size())];
        double st0 = sl * (double)slotSteps, len = std::min((double)slotSteps, loop - st0);
        if (!out.chords.empty() && sl > 0 && slotChord[(size_t)((sl - 1) % (int)slotChord.size())] == ci && (sl % (int)slotChord.size()) != 0) { out.chords.back().len += len; continue; }
        ChordSlot cs; cs.start = st0; cs.len = len; cs.pitches = voiced[(size_t)ci];
        cs.name = pcName(chords[(size_t)ci].root, flats) + chords[(size_t)ci].sym + (chords[(size_t)ci].slash ? "/" + pcName(chords[(size_t)ci].bassPc, flats) : "");
        out.chords.push_back(cs);
    }
    auto chordAt = [&](double step) { int sl = (int)std::floor(step / slotSteps); return slotChord[(size_t)(((sl % (int)slotChord.size()) + (int)slotChord.size()) % (int)slotChord.size())]; };

    // ---- velocity shaping
    const double dyn = clampv(s.dynamics, 0.0, 100.0) / 50.0;
    // phrase dynamics: accent the phrase downbeat, build gently towards the end of each 4-bar phrase,
    // and lift syncopated hits a touch so the push is felt
    auto vel = [&](int v, bool top, double step = -1) {
        double x = 96 + (v - 96) * dyn + (top ? 6 : 0) + (rVel.next() * 2 - 1) * 3;
        if (step >= 0) {
            const int bar = (int)(step / 16), inBar = (int)std::lround(std::fmod(step, 16.0));
            x += (bar % 4) * 1.5 * dyn;                                // build through the phrase
            if (inBar == 0 && bar % 4 == 0) x += 6 * dyn;              // phrase downbeat
            if (inBar % 4 != 0) x += 2 * dyn;                          // off-beat / syncopated lift
        }
        return clampv((int)std::lround(x), 1, 127); };
    const double gate = clampv(s.gate, 10.0, 150.0) / 100.0;

    // ---- rhythm -> chord events
    struct Stab { double step, len; int vel, chord, transpose; };
    std::vector<Stab> stabs;
    const double anticip = std::max(st.anticip, 0.35) * (0.5 + variation / 200.0);
    const int pushWin = R.eighthFeel ? 2 : 1;
    const int bars = loop / 16;
    const bool fills = P.genre != Jazz;
    static const int FILLS_T[] = { 5, 6, -2, 3, 7 };
    if (R.sustain) {
        for (int sl = 0; sl < nSlots; ++sl) {
            double s0 = sl * (double)slotSteps, len = std::min((double)slotSteps, loop - s0);
            if (R.sustain == 2) for (double b = s0; b < s0 + len - 0.01; b += 16) stabs.push_back({ b, std::min(16.0, s0 + len - b) - 0.1, b == s0 ? 100 : 88, chordAt(s0), 0 });
            else stabs.push_back({ s0, len - 0.1, 98, chordAt(s0), 0 });
        }
    } else {
        for (int b = 0; b < bars; ++b) {
            const int rb = b % R.bars;
            const bool fillBar = fills && bars >= 2 && ((bars >= 4 && b % 4 == 3) || (bars < 4 && b == bars - 1)) && rVar.chance(variation / 100.0 * 0.6);
            const int fillIv = fillBar ? FILLS_T[rVar.below(5)] : 0;
            std::vector<Hit> hits;
            for (auto& h : R.hits) if (h.step >= rb * 16 && h.step < rb * 16 + 16) hits.push_back({ h.step - rb * 16, h.len, h.vel });
            if (fills && bars >= 8 && b % 8 == 7 && rVar.chance(variation / 100.0)) { hits.push_back({ 0, 1, 104 }); hits.push_back({ 1, 1, 92 }); }
            for (auto& h : hits) {
                double step = b * 16 + h.step;
                if (step >= loop) continue;
                int ci = chordAt(step);
                double boundary = (std::floor(step / slotSteps) + 1) * slotSteps;
                bool pushed = false;
                if (boundary - step <= pushWin + 0.01 && boundary < loop + 0.01) {
                    int nx = chordAt(std::fmod(boundary, (double)loop));
                    if (nx != ci && rVar.chance(anticip)) { ci = nx; pushed = true; }
                }
                double len = h.len;
                if (!pushed && step + len > boundary && chords.size() > 1 && chordAt(boundary) != ci) len = boundary - step - 0.1;
                stabs.push_back({ step, std::max(0.25, len), h.vel, ci, (fillBar && h.step >= 8) ? fillIv : 0 });
            }
        }
    }
    if (stabs.empty())   // e.g. a 2-bar rhythm whose hits all sit in bar 2 of a 1-bar loop: fall back to held chords
        for (int sl = 0; sl < nSlots; ++sl) { double s0 = sl * (double)slotSteps; stabs.push_back({ s0, std::min((double)slotSteps, loop - s0) - 0.1, 96, chordAt(s0), 0 }); }
    for (auto& sb : stabs) {
        auto& v = voiced[(size_t)sb.chord];
        for (size_t j = 0; j < v.size(); ++j)
            out.events.push_back({ sb.step, std::max(0.2, sb.len * gate), v[j] + sb.transpose, vel(sb.vel, j + 1 == v.size(), sb.step), false, sb.chord, R.eighthFeel, (int)j });
    }

    // ---- bass
    auto bassPitch = [&](int pc) { int x = 36 + mod12(pc); if (x > 43) x -= 12; return x + 12 * clampv(s.octave, -1, 1) * 0; };
    const int tonic = key;
    switch (s.bass) {
        case 1: for (int sl = 0; sl < nSlots; ++sl) { double s0 = sl * (double)slotSteps; int ci = chordAt(s0); out.events.push_back({ s0, std::min((double)slotSteps, loop - s0) - 0.25, bassPitch(chords[(size_t)ci].bassPc), vel(100, false), true, ci, false }); } break;
        case 2: case 5:
            for (int b = 0; b < bars; ++b) for (int o : { 2, 6, 10, 14 }) { double stp = b * 16 + o; int ci = chordAt(stp); out.events.push_back({ stp, 1.5 * gate, bassPitch(s.bass == 5 ? tonic : chords[(size_t)ci].bassPc), vel(o % 8 == 2 ? 108 : 100, false), true, ci, false }); }
            break;
        case 3: {   // organ bounce (M1-style octave riff)
            static const double ST[] = { 0, 3, 6, 10, 13 }; static const int OC[] = { 0, 12, 0, 12, 0 };
            for (int b = 0; b < bars; ++b) for (int k = 0; k < 5; ++k) { double stp = b * 16 + ST[k]; int ci = chordAt(stp); out.events.push_back({ stp, 1.5 * gate, bassPitch(chords[(size_t)ci].bassPc) + OC[k], vel(k == 0 ? 110 : 98, false), true, ci, false }); }
        } break;
        case 4: {   // walking: root, chord tones, chromatic approach into the next bar's root
            for (int b = 0; b < bars; ++b) {
                for (int q = 0; q < 4; ++q) {
                    double stp = b * 16 + q * 4; int ci = chordAt(stp); auto& c = chords[(size_t)ci];
                    int root = bassPitch(c.bassPc), p;
                    if (q == 0) p = root;
                    else if (q == 3) { int nci = chordAt(std::fmod(stp + 4, (double)loop)); int nr = bassPitch(chords[(size_t)nci].bassPc); p = nr + (rVar.chance(0.5) ? 1 : -1); if (rVar.chance(0.25)) p = nr + 7 - 12; }
                    else { auto pcs = pcsOf(c.iv); int tone = pcs[(size_t)rVar.below((int)pcs.size())]; p = root + tone; if (p > 55) p -= 12; }
                    out.events.push_back({ stp, 3.5 * gate, p, vel(q == 0 ? 104 : 94, false), true, ci, true });
                }
            }
        } break;
        case 6: for (auto& sb : stabs) out.events.push_back({ sb.step, std::max(0.2, sb.len * gate), bassPitch(chords[(size_t)sb.chord].bassPc) + sb.transpose, vel(sb.vel, false), true, sb.chord, R.eighthFeel }); break;
        default: break;
    }
    std::sort(out.events.begin(), out.events.end(), [](const Event& a, const Event& b) { return a.step < b.step; });
    return out;
}

} // namespace kc
