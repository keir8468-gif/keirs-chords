#pragma once
// Chord vocabulary, voicing algorithms and small utilities. Pure C++17, no JUCE, so it can be unit-tested.
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace kc {

inline int mod12(int x) { return ((x % 12) + 12) % 12; }
template <typename T> inline T clampv(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---------------------------------------------------------------- RNG (deterministic, seedable)
struct Rng {
    uint32_t a;
    explicit Rng(uint32_t seed) : a(seed ? seed : 1u) {}
    double next() {                                  // mulberry32
        a += 0x6D2B79F5u; uint32_t t = a;
        t = (t ^ (t >> 15)) * (t | 1u);
        t ^= t + (t ^ (t >> 7)) * (t | 61u);
        return ((t ^ (t >> 14)) >> 0) / 4294967296.0;
    }
    int below(int n) { return n <= 0 ? 0 : (int)(next() * n) % n; }
    bool chance(double p) { return next() < p; }
};
inline uint32_t hashSeed(uint32_t a, uint32_t b) { uint32_t h = a * 2654435761u ^ (b * 40503u + 7919u); h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15; return h; }

template <typename T> const T& pickWeighted(Rng& r, const std::vector<std::pair<T, double>>& v) {
    double tot = 0; for (auto& p : v) tot += std::max(0.0, p.second);
    double x = r.next() * tot;
    for (auto& p : v) { x -= std::max(0.0, p.second); if (x <= 0) return p.first; }
    return v.back().first;
}

// ---------------------------------------------------------------- chord qualities
struct Quality { const char* id; std::vector<int> iv; const char* sym; };

inline const std::vector<Quality>& qualities() {
    static const std::vector<Quality> q = {
        { "M",      { 0, 4, 7 },                ""        },
        { "m",      { 0, 3, 7 },                "m"       },
        { "5",      { 0, 7, 12 },               "5"       },
        { "sus2",   { 0, 2, 7 },                "sus2"    },
        { "sus4",   { 0, 5, 7 },                "sus4"    },
        { "add9",   { 0, 4, 7, 14 },            "add9"    },
        { "madd9",  { 0, 3, 7, 14 },            "m(add9)" },
        { "6",      { 0, 4, 7, 9 },             "6"       },
        { "69",     { 0, 4, 7, 9, 14 },         "6/9"     },
        { "m6",     { 0, 3, 7, 9 },             "m6"      },
        { "m69",    { 0, 3, 7, 9, 14 },         "m6/9"    },
        { "m7",     { 0, 3, 7, 10 },            "m7"      },
        { "m9",     { 0, 3, 7, 10, 14 },        "m9"      },
        { "m11",    { 0, 3, 7, 10, 14, 17 },    "m11"     },
        { "m7_11",  { 0, 3, 10, 17 },           "m7(11)"  },
        { "mM7",    { 0, 3, 7, 11 },            "m(maj7)" },
        { "maj7",   { 0, 4, 7, 11 },            "maj7"    },
        { "maj9",   { 0, 4, 7, 11, 14 },        "maj9"    },
        { "maj7s11",{ 0, 4, 7, 11, 18 },        "maj7#11" },
        { "maj9s11",{ 0, 4, 11, 14, 18 },       "maj9#11" },
        { "dom7",   { 0, 4, 7, 10 },            "7"       },
        { "dom9",   { 0, 4, 7, 10, 14 },        "9"       },
        { "dom13",  { 0, 4, 10, 14, 21 },       "13"      },
        { "dom7b9", { 0, 4, 7, 10, 13 },        "7b9"     },
        { "dom7s9", { 0, 4, 10, 15 },           "7#9"     },
        { "alt",    { 0, 4, 10, 15, 20 },       "7alt"    },
        { "dom7s11",{ 0, 4, 10, 14, 18 },       "9#11"    },
        { "dom13b9",{ 0, 4, 10, 13, 21 },       "13b9"    },
        { "7sus4",  { 0, 5, 7, 10 },            "7sus4"   },
        { "9sus4",  { 0, 5, 7, 10, 14 },        "9sus4"   },
        { "13sus4", { 0, 5, 10, 14, 21 },       "13sus4"  },
        { "7sus2",  { 0, 2, 7, 10 },            "7sus2"   },
        { "dim",    { 0, 3, 6 },                "dim"     },
        { "dim7",   { 0, 3, 6, 9 },             "dim7"    },
        { "m7b5",   { 0, 3, 6, 10 },            "m7b5"    },
        { "m9b5",   { 0, 3, 6, 10, 14 },        "m9b5"    },
        { "m11b5",  { 0, 3, 6, 10, 17 },        "m11b5"   },
    };
    return q;
}
inline const Quality& quality(const std::string& id) {
    for (auto& q : qualities()) if (id == q.id) return q;
    return qualities()[1];
}

// chord families -> quality at extension level 0..3
inline std::string familyQuality(const std::string& fam, int level) {
    level = clampv(level, 0, 3);
    static const std::vector<std::pair<std::string, std::vector<std::string>>> F = {
        { "m",   { "m", "m7", "m9", "m11" } },
        { "mi",  { "m", "m7", "m9", "m9" } },           // minor without 11 (jazz ii, iii)
        { "M",   { "M", "maj7", "maj9", "maj9" } },     // I, IV, bIII, bVI
        { "L",   { "M", "maj7", "maj9", "maj9s11" } },  // lydian colour (IV, bVI)
        { "Mx",  { "M", "add9", "add9", "69" } },       // major, no maj7 (bVII in aeolian)
        { "V",   { "M", "dom7", "dom9", "dom13" } },     // dominant that may stay a triad
        { "D",   { "dom7", "dom9", "dom13", "dom13" } },
        { "Db9", { "dom7", "dom7b9", "dom7b9", "dom13b9" } },
        { "Da",  { "dom7", "dom7b9", "alt", "alt" } },   // minor-key / altered dominant
        { "S",   { "sus4", "7sus4", "9sus4", "13sus4" } },
        { "s2",  { "sus2", "sus2", "7sus2", "7sus2" } },
        { "h",   { "dim", "m7b5", "m7b5", "m9b5" } },
        { "o",   { "dim", "dim7", "dim7", "dim7" } },
        { "P5",  { "5", "5", "5", "5" } },
        { "m6",  { "m", "m6", "m69", "m69" } },          // minor tonic (jazz i)
        { "6",   { "M", "6", "69", "69" } },
    };
    for (auto& f : F) if (f.first == fam) return f.second[(size_t)level];
    return "m";
}

// ---------------------------------------------------------------- voicings
enum class Voicing { Auto, Close, Rootless, SeventhBottom, Quartal, Spread, Piano, Shell, Drop2, Parallel };
inline const std::vector<std::string>& voicingNames() {
    static const std::vector<std::string> v = { "Auto (style)", "Close", "Rootless A/B", "7th on bottom", "Quartal / So What",
                                                "Spread pad", "Piano spread", "Shell (1-3-7)", "Drop 2", "Parallel (chord memory)" };
    return v;
}

inline bool has(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }
inline std::vector<int> pcsOf(const std::vector<int>& iv) { std::vector<int> o; for (int i : iv) { int p = mod12(i); if (!has(o, p)) o.push_back(p); } return o; }
inline double meanOf(const std::vector<int>& v) { return v.empty() ? 0.0 : std::accumulate(v.begin(), v.end(), 0.0) / v.size(); }

// voice-leading distance between two voicings
inline double vlDist(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.empty() || b.empty()) return 0;
    double s = 0;
    for (int x : a) { int best = 99; for (int y : b) best = std::min(best, std::abs(x - y)); s += best; }
    for (int y : b) { int best = 99; for (int x : a) best = std::min(best, std::abs(x - y)); s += best; }
    return s / (a.size() + b.size());
}

// The top voice is heard as a melody: reward stepwise (or common-tone) motion, punish leaps.
inline double topLinePenalty(const std::vector<int>& v, const std::vector<int>& prev) {
    if (v.empty() || prev.empty()) return 0;
    int d = std::abs(v.back() - prev.back());
    return d == 0 ? 0.1 : (d <= 2 ? 0.0 : (d <= 4 ? 0.6 : 0.3 * d));
}

// keep the most important tones: 3rd & 7th, then tensions, then root, then 5th
inline std::vector<int> trimTones(const std::vector<int>& iv, int maxVoices, bool keepRoot) {
    if ((int)iv.size() <= maxVoices) return iv;
    const bool third = has(pcsOf(iv), 3) || has(pcsOf(iv), 4);
    std::vector<std::pair<int, double>> sc;
    for (size_t i = 0; i < iv.size(); ++i) {
        int pc = mod12(iv[i]); double pri;
        if (pc == 3 || pc == 4) pri = 10;
        else if (pc == 10 || pc == 11) pri = 9;
        else if ((pc == 5 || pc == 2) && !third) pri = 9;   // sus tone
        else if (pc == 1 || pc == 8) pri = 8;                // b9 / b13: the colour of altered chords
        else if (pc == 9 || pc == 2) pri = 7;
        else if (pc == 5 || pc == 6) pri = 6;
        else if (pc == 0) pri = keepRoot ? 8 : 4;
        else if (pc == 7) pri = 3;
        else pri = 5;
        sc.push_back({ iv[i], pri - i * 0.001 });
    }
    std::sort(sc.begin(), sc.end(), [](auto& a, auto& b) { return a.second > b.second; });
    std::vector<int> out; for (int i = 0; i < maxVoices; ++i) out.push_back(sc[(size_t)i].first);
    std::sort(out.begin(), out.end());
    return out;
}

inline std::vector<int> stackFrom(const std::vector<int>& pcs, int startPitch) {   // ascending close stack
    std::vector<int> out; int p = startPitch;
    for (size_t i = 0; i < pcs.size(); ++i) {
        if (i == 0) { out.push_back(p); continue; }
        int q = p + 1; while (mod12(q) != pcs[i]) ++q;
        out.push_back(q); p = q;
    }
    return out;
}
inline std::vector<int> placeNear(std::vector<int> shape, double center, const std::vector<int>* prev) {
    std::vector<int> best = shape; double bestS = 1e9;
    for (int k = -5; k <= 5; ++k) {
        std::vector<int> c = shape; for (auto& x : c) x += 12 * k;
        if (c.front() < 21 || c.back() > 108) continue;
        double s = std::abs(meanOf(c) - center) * 0.6 + (prev ? vlDist(c, *prev) : 0.0);
        if (s < bestS) { bestS = s; best = c; }
    }
    return best;
}
inline std::vector<int> orderFrom(std::vector<int> pcs, int start) {
    std::sort(pcs.begin(), pcs.end(), [&](int a, int b) { return mod12(a - start) < mod12(b - start); });
    return pcs;
}

inline std::vector<int> voiceClose(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev, bool dropRoot) {
    auto rel = pcsOf(iv);
    if (dropRoot && rel.size() >= 4) rel.erase(std::remove(rel.begin(), rel.end(), 0), rel.end());
    std::vector<int> pcs; for (int r : rel) pcs.push_back(mod12(r + root));
    std::vector<int> best; double bestS = 1e9;
    for (size_t r = 0; r < pcs.size(); ++r) {
        auto ord = orderFrom(pcs, pcs[r]);
        auto placed = placeNear(stackFrom(ord, 48 + ord[0]), center, prev);
        double s = std::abs(meanOf(placed) - center) * 0.6 + (prev ? vlDist(placed, *prev) + topLinePenalty(placed, *prev) : 0.0);
        if (s < bestS) { bestS = s; best = placed; }
    }
    return best;
}

// Bill Evans style rootless voicings: form A (3rd on the bottom) and form B (7th on the bottom); the
// form that moves least from the previous chord wins, which gives the classic A-B-A alternation.
inline std::vector<int> voiceRootlessAB(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev) {
    auto pcs = pcsOf(iv);
    int third = has(pcs, 4) ? 4 : (has(pcs, 3) ? 3 : (has(pcs, 5) ? 5 : 2));
    int sev = has(pcs, 10) ? 10 : (has(pcs, 11) ? 11 : (has(pcs, 9) ? 9 : -1));
    if (sev < 0) return voiceClose(root, iv, center, prev, true);
    int ninth = has(pcs, 2) ? 2 : (has(pcs, 1) ? 1 : (has(pcs, 3) && third == 4 ? 3 : 2));
    bool dom = (third == 4 && sev == 10);
    int color = dom ? (has(pcs, 8) ? 8 : 9) : (has(pcs, 6) && !has(pcs, 7) ? 6 : 7);  // 13 / b13 on dominants, 5 (or b5) otherwise
    if (has(pcs, 5) && third != 5 && !dom && third == 3) color = 5;                     // m11 colour
    std::vector<int> A = { third, color, sev, ninth }, B = { sev, ninth, third, color };
    std::vector<int> best; double bestS = 1e9;
    for (auto* f : { &A, &B }) {
        std::vector<int> p; for (int x : *f) p.push_back(mod12(x + root));
        auto placed = placeNear(stackFrom(p, 36 + p[0]), center, prev);
        double s = std::abs(meanOf(placed) - center) * 0.5 + (prev ? vlDist(placed, *prev) * 1.2 + topLinePenalty(placed, *prev) : 0.0);
        if (s < bestS) { bestS = s; best = placed; }
    }
    return best;
}

inline std::vector<int> voiceSeventhBottom(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev) {
    auto pcs = pcsOf(iv);
    int sev = has(pcs, 10) ? 10 : (has(pcs, 11) ? 11 : (has(pcs, 9) ? 9 : -1));
    if (sev < 0) return voiceClose(root, iv, center, prev, false);
    int third = has(pcs, 3) ? 3 : (has(pcs, 4) ? 4 : (has(pcs, 5) ? 5 : 2));
    int top = (has(pcs, 5) && third != 5) ? 5 : ((has(pcs, 2) && third != 2) ? 2 : 7);
    std::vector<int> ord; for (int x : { sev, 0, third, top }) ord.push_back(mod12(x + root));
    return placeNear(stackFrom(ord, 36 + ord[0]), center - 2, prev);
}

inline std::vector<int> voiceQuartal(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev, int maxVoices) {
    auto pcs = pcsOf(iv);
    bool m3 = has(pcs, 3), M3 = has(pcs, 4), b7 = has(pcs, 10);
    std::vector<int> rel;
    if (m3 && has(pcs, 6)) rel = { 0, 6, 10, 15, 17 };        // half-diminished in 4ths-ish
    else if (m3) rel = { 0, 5, 10, 15, 19 };                  // So What: three 4ths + a major 3rd
    else if (M3 && b7) rel = { 10, 16, 21, 26 };              // 7-3-13-9
    else if (M3) rel = { 4, 9, 14, 19, 23 };                  // 3-6-9-5-maj7 (6/9 sound)
    else rel = { 0, 5, 10, 14, 19 };
    rel.resize((size_t)std::max(3, std::min((int)rel.size(), maxVoices)));
    std::vector<int> shape; for (int x : rel) shape.push_back(x + root + 48);
    return placeNear(shape, center, prev);
}

inline std::vector<int> voiceSpread(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev) {
    auto v = voiceClose(root, iv, center + 5, prev, false);
    if (v.size() >= 3) v[v.size() - 2] -= 12;                       // drop 2
    if (has(pcsOf(iv), 0)) { int lo = *std::min_element(v.begin(), v.end()); int r = lo - 1; while (mod12(r) != mod12(root)) --r; v.push_back(r); }
    std::sort(v.begin(), v.end()); v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}
inline std::vector<int> voiceDrop2(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev) {
    auto v = voiceClose(root, trimTones(iv, 4, false), center + 4, prev, false);
    if (v.size() >= 4) v[v.size() - 2] -= 12;
    std::sort(v.begin(), v.end());
    return v;
}
inline std::vector<int> voiceShell(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev) {
    auto pcs = pcsOf(iv);
    int third = has(pcs, 3) ? 3 : (has(pcs, 4) ? 4 : (has(pcs, 5) ? 5 : 7));
    int sev = has(pcs, 10) ? 10 : (has(pcs, 11) ? 11 : (has(pcs, 9) ? 9 : 7));
    int r = (int)std::lround(center - 10); while (mod12(r) != mod12(root)) --r;
    std::vector<int> a = { r, r + third, r + sev }, b = { r, r + sev, r + third + 12 };   // 1-3-7 or 1-7-3
    if (!prev) return a;
    return vlDist(a, *prev) <= vlDist(b, *prev) ? a : b;
}
// Chicago "spread": close triad in the right hand, a single bass note well below (the bass note can turn the triad
// into a richer chord, e.g. Bb triad over Eb = Eb(add 11 colour)).
inline std::vector<int> voicePiano(int root, const std::vector<int>& iv, double center, const std::vector<int>* prev, int bassPc) {
    auto v = voiceClose(root, trimTones(iv, 4, true), center, prev, false);
    int top = v.back(), low = v.front();
    if (!has(v, low + 12) && low + 12 > top) v.push_back(low + 12);   // octave frame on top
    int b = (int)std::lround(center - 15); while (mod12(b) != mod12(bassPc)) --b;
    v.push_back(b);
    std::sort(v.begin(), v.end());
    return v;
}

} // namespace kc
