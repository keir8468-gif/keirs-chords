#pragma once
// The progression and rhythm libraries.
// A hand-written core (from the research in docs/RESEARCH.md) is expanded with genre-appropriate techniques
// (substitutions, borrowed chords, colour swaps, parallel/chord-memory versions, rotations; rhythmic
// displacement, anticipation, thinning, ghost fills, articulation, two-bar call & response) and de-duplicated.
#include "Theory.h"
#include <map>
#include <set>
#include <sstream>

namespace kc {

enum Genre { House = 0, Techno = 1, Jazz = 2, NumGenres = 3 };
inline const char* genreName(int g) { static const char* n[] = { "House", "Techno", "Jazz" }; return n[clampv(g, 0, 2)]; }

// ---------------------------------------------------------------- styles (voicing / register defaults)
struct Style {
    const char* name; int genre;
    Voicing voicing; double register_; int maxVoices; int level;   // level = default extension level 0..3
    double anticip;   // probability that a last-16th hit grabs the next chord
    int rateSteps;    // default 16ths per chord
};
inline const std::vector<Style>& styles() {
    static const std::vector<Style> s = {
        // house
        { "Chicago Piano",       House,  Voicing::Piano,         66, 4, 0, 0.30, 16 },
        { "Jersey Garage",       House,  Voicing::SeventhBottom, 62, 4, 1, 0.80, 8  },
        { "Deep House",          House,  Voicing::Rootless,      62, 5, 2, 0.40, 16 },
        { "Soulful Gospel",      House,  Voicing::Spread,        62, 5, 2, 0.30, 16 },
        { "Organ House",         House,  Voicing::Close,         64, 4, 1, 0.50, 16 },
        { "Disco House",         House,  Voicing::Close,         64, 4, 1, 0.40, 16 },
        { "UK Garage",           House,  Voicing::Rootless,      64, 4, 2, 0.60, 16 },
        // techno
        { "Detroit Strings",     Techno, Voicing::Spread,        62, 5, 1, 0.00, 16 },
        { "Chord Memory",        Techno, Voicing::Parallel,      64, 5, 1, 0.30, 8  },
        { "Rave Stabs",          Techno, Voicing::Parallel,      64, 4, 1, 0.20, 8  },
        { "Warehouse",           Techno, Voicing::Close,         60, 4, 0, 0.20, 32 },
        { "Dub Techno",          Techno, Voicing::Close,         60, 3, 1, 0.10, 32 },
        { "Melodic Techno",      Techno, Voicing::Spread,        62, 5, 0, 0.10, 16 },
        { "Hi-Tech Jazz",        Techno, Voicing::Rootless,      63, 5, 3, 0.50, 8  },
        // jazz
        { "ii-V-I",              Jazz,   Voicing::Rootless,      60, 4, 2, 0.50, 16 },
        { "Minor ii-V-i",        Jazz,   Voicing::Rootless,      60, 4, 2, 0.50, 16 },
        { "Turnarounds",         Jazz,   Voicing::Rootless,      60, 4, 2, 0.50, 8  },
        { "Modal",               Jazz,   Voicing::Quartal,       60, 5, 2, 0.30, 32 },
        { "Neo-Soul",            Jazz,   Voicing::Rootless,      62, 5, 3, 0.40, 16 },
        { "Bossa Nova",          Jazz,   Voicing::Drop2,         60, 4, 2, 0.40, 16 },
        { "Ballad",              Jazz,   Voicing::Spread,        60, 5, 3, 0.20, 16 },
        { "Blues",               Jazz,   Voicing::Shell,         58, 3, 1, 0.40, 16 },
    };
    return s;
}
inline int styleIndex(const char* name) { for (size_t i = 0; i < styles().size(); ++i) if (std::string(styles()[i].name) == name) return (int)i; return 0; }

// ---------------------------------------------------------------- progression tokens
// "deg:fam[*slots][/bassdeg]"  deg = semitones above the tonic; fam = family (Theory.h) or !quality for a fixed quality
struct PChord { int deg = 0; std::string fam = "m"; std::string fixed; int slots = 1; int bass = -1; };
struct Progression {
    std::string id;            // canonical token string (stable across versions)
    std::string name;          // roman numerals
    int genre = 0, style = 0;
    bool minor = true;         // tonic quality
    bool parallel = false;     // chord-memory: one fixed shape moved around
    std::string tag;           // technique that produced it ("core", "tritone sub", ...)
    std::vector<PChord> chords;
    int totalSlots() const { int t = 0; for (auto& c : chords) t += c.slots; return t; }
};

inline std::vector<PChord> parseProg(const std::string& s) {
    std::vector<PChord> out; std::istringstream is(s); std::string tok;
    while (is >> tok) {
        PChord c; auto slash = tok.find('/');
        if (slash != std::string::npos) { c.bass = std::stoi(tok.substr(slash + 1)); tok = tok.substr(0, slash); }
        auto star = tok.find('*');
        if (star != std::string::npos) { c.slots = std::max(1, std::stoi(tok.substr(star + 1))); tok = tok.substr(0, star); }
        auto colon = tok.find(':');
        c.deg = mod12(std::stoi(tok.substr(0, colon)));
        std::string f = tok.substr(colon + 1);
        if (!f.empty() && f[0] == '!') c.fixed = f.substr(1), c.fam = ""; else c.fam = f;
        out.push_back(c);
    }
    return out;
}
inline std::string tokenOf(const PChord& c) {
    std::string t = std::to_string(c.deg) + ":" + (c.fixed.empty() ? c.fam : "!" + c.fixed);
    if (c.slots > 1) t += "*" + std::to_string(c.slots);
    if (c.bass >= 0) t += "/" + std::to_string(c.bass);
    return t;
}
inline std::string idOf(const std::vector<PChord>& v, bool parallel) {
    std::string s = parallel ? "P|" : "";
    for (size_t i = 0; i < v.size(); ++i) s += (i ? " " : "") + tokenOf(v[i]);
    return s;
}

// quality classes for naming / filters
inline bool famIsMinor(const std::string& f) { return f == "m" || f == "mi" || f == "m6" || f == "h" || f == "o"; }
inline std::string romanOf(const PChord& c) {
    static const char* up[] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    std::string q = c.fixed.empty() ? familyQuality(c.fam, 1) : c.fixed;
    bool lower = (q[0] == 'm' && q.rfind("maj", 0) != 0) || q.rfind("dim", 0) == 0;
    std::string r = up[c.deg];
    if (lower) for (auto& ch : r) if (ch != 'b' && ch != '#') ch = (char)std::tolower(ch);
    std::string suf;
    const std::string f = c.fam;
    if (!c.fixed.empty()) suf = quality(c.fixed).sym, suf = (suf.size() && suf[0] == 'm' && suf.rfind("maj", 0) != 0) ? suf.substr(1) : suf;
    else if (f == "h") suf = "\xC3\xB8";             // ø
    else if (f == "o") suf = "\xC2\xB0";             // °
    else if (f == "D" || f == "V" || f == "Db9") suf = "7";
    else if (f == "Da") suf = "7alt";
    else if (f == "S") suf = "sus";
    else if (f == "s2") suf = "sus2";
    else if (f == "P5") suf = "5";
    else if (f == "L") suf = "maj7#11";
    else if (f == "M") suf = "maj7";
    else if (f == "m" || f == "mi") suf = "7";
    else if (f == "m6") suf = "6";
    else if (f == "6") suf = "6";
    std::string n = r + suf;
    if (c.bass >= 0) n += "/" + std::string(up[c.bass]);
    return n;
}
inline std::string nameOf(const std::vector<PChord>& v, bool parallel) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += " \xE2\x80\x93 ";   // en dash
        s += romanOf(v[i]);
        if (v[i].slots > 1) s += " x" + std::to_string(v[i].slots);
    }
    if (parallel) s = "\xE2\x87\x89 " + s;   // ⇉ marks a parallel (one-shape) progression
    return s;
}

// ---------------------------------------------------------------- rhythm patterns
struct Hit { double step; double len; int vel; };
struct Rhythm {
    std::string id, name;
    int genre = 0;
    int bars = 1;                  // 1 or 2
    int sustain = 0;               // 0 = hits, 1 = hold each chord for its slot, 2 = re-strike every bar
    bool eighthFeel = false;       // jazz/bossa: swing applies to 8ths
    std::vector<Hit> hits;         // step 0..(16*bars)-1
    double sync = 0;               // syncopation per bar (computed)
    double density = 0;            // hits per bar
    int syncClass = 1;             // 0 low, 1 medium, 2 high (per-genre tertiles)
    std::string tag;
    // relationship to a four-on-the-floor kick (per bar averages)
    double kickHits = 0, pickups = 0, offbeats = 0; bool counterMeter = false;
    double groove = 0;             // 0..5 groove rating (see grooveScore)
};

// How a chord rhythm sits against a 4/4 kick (steps 0,4,8,12) and off-beat hat (2,6,10,14):
//  + pickups (3,7,11,15) lead into the kick, + off-beats lock with the hat, + 3+3+2 / 3-step counter-meter figures,
//  - hits stacked on the kick mask it, and syncopation scores best in the medium range (inverted U).
inline void analyseAgainstKick(Rhythm& r) {
    const int N = 16 * r.bars; std::vector<int> on((size_t)N, 0);
    for (auto& h : r.hits) { int s = (int)std::lround(h.step); if (s >= 0 && s < N) on[(size_t)s] = 1; }
    int k = 0, p = 0, o = 0;
    for (int i = 0; i < N; ++i) if (on[(size_t)i]) { int b = i % 16; if (b % 4 == 0) ++k; else if (b % 4 == 3) ++p; else if (b % 4 == 2) ++o; }
    r.kickHits = (double)k / r.bars; r.pickups = (double)p / r.bars; r.offbeats = (double)o / r.bars;
    // counter-meter: three or more consecutive onsets spaced 3 steps apart
    int run = 0, best = 0, last = -100;
    for (int i = 0; i < N; ++i) if (on[(size_t)i]) { run = (i - last == 3) ? run + 1 : 1; best = std::max(best, run); last = i; }
    r.counterMeter = best >= 3;
}
inline double grooveScore(const Rhythm& r, double medianSync) {
    if (r.sustain) return 2.5;
    double sc = 3.0 - std::min(2.0, std::abs(r.sync - medianSync) / std::max(1.0, medianSync) * 2.0);  // inverted U around the genre median
    sc += std::min(1.0, r.pickups * 0.5) + std::min(0.8, r.offbeats * 0.3) + (r.counterMeter ? 0.6 : 0.0);
    sc -= std::max(0.0, r.kickHits - 1.0) * (r.genre == 2 ? 0.15 : 0.6);  // one downbeat hit is fine, stacking on every kick is not (jazz comping is freer)
    if (r.density > 8) sc -= (r.density - 8) * 0.15;                     // too busy masks the groove
    return clampv(sc, 0.0, 5.0);
}

// Longuet-Higgins & Lee style syncopation: a note on a weak position followed by silence on a stronger
// position counts (weight difference). Computed on the 16-step grid, looping.
inline double syncopationOf(const std::vector<Hit>& hits, int bars) {
    static const int W[16] = { 0, -4, -3, -4, -2, -4, -3, -4, -1, -4, -3, -4, -2, -4, -3, -4 };
    const int N = 16 * bars;
    std::vector<int> on((size_t)N, 0);
    for (auto& h : hits) { int s = (int)std::lround(h.step); if (s >= 0 && s < N) on[(size_t)s] = 1; }
    double total = 0;
    for (int i = 0; i < N; ++i) {
        if (!on[(size_t)i]) continue;
        int j = (i + 1) % N;                          // next note
        while (!on[(size_t)j] && j != i) j = (j + 1) % N;
        int k = (i + 1) % N, strongest = -99;
        while (k != j) { strongest = std::max(strongest, W[k % 16]); k = (k + 1) % N; }
        if (strongest > W[i % 16]) total += strongest - W[i % 16];
    }
    return total / bars;
}

// ================================================================= the library
class Library {
public:
    std::vector<Progression> progs;
    std::vector<Rhythm> rhythms;

    static const Library& get() { static Library L; return L; }

    int findProg(const std::string& id) const { for (size_t i = 0; i < progs.size(); ++i) if (progs[i].id == id) return (int)i; return -1; }
    int findRhythm(const std::string& id) const { for (size_t i = 0; i < rhythms.size(); ++i) if (rhythms[i].id == id) return (int)i; return -1; }
    int countProgs(int g) const { int n = 0; for (auto& p : progs) n += p.genre == g; return n; }
    int countRhythms(int g) const { int n = 0; for (auto& r : rhythms) n += r.genre == g; return n; }

private:
    std::set<std::string> seen, seenR;

    Library() { buildProgressions(); buildRhythms(); }

    // ------------------------------------------------------------ progressions
    void add(const std::string& s, const char* style, bool minor, const std::string& tag = "core", bool parallel = false) {
        auto ch = parseProg(s);
        addChords(ch, styleIndex(style), minor, tag, parallel);
    }
    bool addChords(const std::vector<PChord>& ch, int style, bool minor, const std::string& tag, bool parallel) {
        if (ch.empty()) return false;
        auto id = idOf(ch, parallel);
        if (!seen.insert(id).second) return false;
        Progression p; p.id = id; p.chords = ch; p.style = style; p.genre = styles()[(size_t)style].genre;
        p.minor = minor; p.parallel = parallel; p.tag = tag; p.name = nameOf(ch, parallel);
        progs.push_back(p);
        return true;
    }

    void corePaths(const std::vector<int>& path, const char* style, const std::string& fam) {
        std::vector<PChord> v; for (int d : path) { PChord c; c.deg = mod12(d); c.fam = fam; v.push_back(c); }
        addChords(v, styleIndex(style), true, "core", true);
    }

    void buildProgressions() {
        // ===================== HOUSE =====================
        // Chicago piano (Move Your Body-style one-bar minor riffs, piano anthems, spread chords over bass notes)
        for (auto s : { "0:m 5:m 7:m", "0:m 5:m", "0:m 10:Mx 8:M 7:m", "0:m 3:M 5:m 7:m", "0:m 8:M 10:Mx", "0:m 5:m 10:Mx 3:M",
                        "0:m 7:m 8:M 5:m", "0:m 10:Mx 5:m 7:m", "0:m 3:M 10:Mx 5:m", "5:m 7:m 0:m*2" })
            add(s, "Chicago Piano", true);
        for (auto s : { "0:M 7:V 9:m 5:M", "9:m 5:M 0:M 7:V", "0:M 5:M", "5:M 7:V 9:m", "0:M 4:m 5:M 7:V", "0:M 9:m 5:M 7:V",
                        "0:M 5:M 9:m 7:V", "5:M 0:M 7:V 9:m", "0:M 10:M/3 8:L 5:M", "0:M 10:Mx 5:M 0:M", "0:M 5:M 0:M 7:V", "2:m 7:V 0:M*2" })
            add(s, "Chicago Piano", false);
        // Jersey / Kerri-style two- and three-chord stab loops, borrowed chords, pedal ideas
        for (auto s : { "0:m 5:m", "0:m 8:!6", "0:m 7:m/10", "0:m 2:m 1:D", "0:m 7:m 5:m", "0:m 7:m 9:m", "0:m 10:m",
                        "0:m 5:m 7:m 2:m", "0:m 2:m 6:m 10:m", "0:m 8:!m6", "0:m 5:m 8:!6 7:m", "0:m 3:m", "0:m 5:D", "0:m 7:m/7 5:m/7",
                        "0:m 0:m 8:!6 10:m" })
            add(s, "Jersey Garage", true);
        // Deep house (Dorian / MAW / Larry Heard)
        for (auto s : { "0:m 5:D", "0:m 2:m", "0:m 10:M", "0:m 3:M 5:D 3:M", "0:m 7:m 5:m", "5:m 8:M 0:m*2", "0:m 8:M 10:S",
                        "3:M 2:m 0:m 10:M", "0:m 5:D 10:M 3:M", "0:m 5:m 8:M 10:Mx", "0:m 8:M", "0:m 3:M", "0:m 10:M 8:M 10:M", "0:m 5:m 3:M 2:m" })
            add(s, "Deep House", true);
        for (auto s : { "5:M 4:mi", "5:M 4:mi 2:mi 0:M", "0:M 4:mi 5:M 7:S", "2:mi 7:S", "0:M 9:mi 2:mi 7:S" })
            add(s, "Deep House", false);
        // Soulful / gospel house
        for (auto s : { "5:M 4:mi 2:mi 0:M", "2:mi 7:S 0:M 9:mi", "5:M 7:S 4:mi 9:mi", "9:mi 2:mi 7:D 0:M", "0:M 5:M", "5:M 7:D 4:mi 9:mi",
                        "2:mi 7:S", "0:M 9:mi 5:M 7:S", "5:M 5:mi 0:M*2", "0:M 0:D/4 5:M 5:mi", "2:mi 4:mi 5:M 7:S", "0:M 9:D 2:mi 7:D",
                        "5:M 4:D 9:mi 7:mi 0:D", "4:mi 9:mi 2:mi 7:S", "0:M 5:M/0 7:S 5:M", "8:L 10:D 0:M*2" })
            add(s, "Soulful Gospel", false);
        for (auto s : { "0:m 5:m 10:D 3:M", "0:m 8:M 10:D 0:m" }) add(s, "Soulful Gospel", true);
        // Organ house (90s M1)
        for (auto s : { "0:m 10:Mx 8:M 10:Mx", "0:m 5:m", "0:m 3:M 10:Mx 5:m", "0:m 8:M 3:M 10:Mx", "0:m 0:m 5:m 7:m", "0:m 10:Mx", "0:m 3:M",
                        "0:m 8:M 5:m 7:m" })
            add(s, "Organ House", true);
        for (auto s : { "0:M 7:V 9:m 5:M", "5:M 7:V 0:M*2" }) add(s, "Organ House", false);
        // Disco house (minor 7 vamps, dominant funk, maj7 two-chord Lydian loops)
        for (auto s : { "0:m 5:m", "0:m 5:D", "0:m 3:M 5:m 7:m", "0:m 10:D", "0:m 8:M 10:D 0:m", "5:m 0:m 7:m 0:m" })
            add(s, "Disco House", true);
        for (auto s : { "0:M*2 5:L*2", "0:M 9:mi", "2:mi 7:D", "0:M 4:mi 5:M 7:D", "5:M 4:mi 2:mi 7:D", "0:D 5:D" })
            add(s, "Disco House", false);
        // UK garage (i-v-VI-VII, maj7/m9 colours, sus2)
        for (auto s : { "0:m 7:m 8:M 10:Mx", "0:m 5:D 7:m 0:m", "0:m 8:M 3:M 10:Mx", "0:m 5:m 8:M 7:m", "0:s2 8:M 10:Mx 5:m", "0:m 3:M 8:M 10:S",
                        "8:M 10:Mx 0:m*2", "0:m 2:h 3:M 7:Da" })
            add(s, "UK Garage", true);
        for (auto s : { "5:M 7:S 4:mi 9:mi", "0:M 4:mi 5:M 5:mi" }) add(s, "UK Garage", false);

        // ===================== TECHNO =====================
        for (auto s : { "0:m 10:Mx 8:M 10:Mx", "0:m 5:m", "0:m 8:M 10:Mx 0:m", "0:m 3:M 10:Mx 5:m", "0:m 7:m", "0:s2 5:m 8:M 7:m", "0:m 8:!maj7s11",
                        "0:m 10:s2 8:M 7:S", "5:m 0:m 8:M 10:s2", "0:m*2 8:M 10:Mx", "0:m 3:M", "0:m 5:m 8:M 7:m", "0:m*3 8:M", "0:m 10:s2" })
            add(s, "Detroit Strings", true);
        // chord memory / sampled-chord paths (parallel)
        for (auto& p : std::vector<std::vector<int>>{ { 0, 5 }, { 0, 3 }, { 0, -2 }, { 0, 7 }, { 0, 2, 6, 10 }, { 0, 3, 5, 7 }, { 0, -2, -4, -5 },
                                                      { 0, 10, 8, 7 }, { 0, 5, 3, -2 }, { 0, 1 }, { 0, 4 }, { 0, 3, 0, -2 }, { 0, 7, 5, 3 }, { 0, 5, 7, 3 },
                                                      { 0, 2 }, { 0, -5, -3, -2 }, { 0, 8, 10, 3 }, { 0, 3, 7, 5 }, { 0, -1, -3, -5 } })
            corePaths(p, "Chord Memory", "m");
        // rave stabs: one sampled minor chord triggered at several pitches
        for (auto& p : std::vector<std::vector<int>>{ { 0, 7, 10, 5 }, { 0, 2, 4, 5, 7 }, { 0, 3, 5, 3 }, { 0, -2, 0, 3 }, { 0, 5, 3, 0 }, { 0, 7, 5, 10 },
                                                      { 0, 10, 7, 5 }, { 0, 3, 10, 8 } })
            corePaths(p, "Rave Stabs", "m");
        for (auto s : { "0:m", "0:m 1:m", "0:P5 3:P5", "0:m 5:m", "0:m*3 6:m", "0:m 10:m", "0:m 3:m", "0:P5 1:P5", "0:m 0:m 0:m 1:m", "0:m 8:m", "0:P5 5:P5 3:P5 6:P5" })
            add(s, "Warehouse", true);
        for (auto s : { "0:m", "0:m 5:m", "0:m 7:m", "0:m 10:Mx", "0:m*3 5:m", "0:m 3:M", "0:s2 5:m" }) add(s, "Dub Techno", true);
        for (auto s : { "0:m 8:M 3:M 10:Mx", "0:m 5:m 8:M 7:V", "0:m 8:M 5:m 7:m", "0:m 10:Mx 8:M 7:V", "0:m 3:M 10:Mx 8:M", "0:m 8:M 10:s2 5:m",
                        "0:m 3:M 8:M 10:Mx", "0:m 5:m 3:M 10:Mx", "8:M 10:Mx 0:m*2", "0:s2 3:M 10:Mx 8:M", "0:m 7:m 8:M 10:Mx", "0:m*2 8:M 10:S" })
            add(s, "Melodic Techno", true);
        for (auto s : { "0:m 5:D 10:M 3:M", "2:m 7:D 1:m 6:D", "0:m 5:D 10:m 3:D 8:m 1:D 7:m 0:D", "0:m 1:M", "5:M 4:m 3:M 2:m", "0:m 8:M 7:S 0:m",
                        "11:m 4:D 10:m 3:D 8:m 1:D 11:D 10:D", "0:m 2:m 3:M 5:D" })
            add(s, "Hi-Tech Jazz", true);

        // ===================== JAZZ =====================
        for (auto s : { "2:mi 7:D 0:M*2", "2:mi 7:D", "2:mi 7:D 0:M 9:Db9", "2:mi 7:D 0:M 0:M", "2:mi 7:D 0:M 5:M", "2:mi 1:D 0:M*2",
                        "2:mi 7:D 0:M*2 0:mi 5:D 10:M*2", "9:mi 2:D 7:mi 0:D 5:M*2", "2:mi 7:Db9 0:M*2", "0:M*2 2:mi 7:D", "4:mi 9:Db9 2:mi 7:D" })
            add(s, "ii-V-I", false);
        for (auto s : { "2:h 7:Da 0:m6*2", "2:h 7:Da", "0:m6 9:h 2:h 7:Da", "5:mi 10:D 3:M 8:M 2:h 7:Da 0:m6*2", "0:m 0:!mM7 0:!m7 0:!m6",
                        "0:m6 8:D 7:Da 0:m6", "2:h 7:Da 0:m6 8:D", "0:m6*2 2:h 7:Da", "5:mi 2:h 7:Da 0:m6", "0:m6 3:mi 8:D 1:M" })
            add(s, "Minor ii-V-i", true);
        for (auto s : { "0:M 9:mi 2:mi 7:D", "4:mi 9:mi 2:mi 7:D", "0:M 9:Db9 2:mi 7:D", "4:mi 9:Db9 2:mi 7:D", "4:D 9:D 2:D 7:D", "0:M 3:D 8:D 1:D",
                        "0:M 3:M 8:M 1:M", "0:M 9:Db9 2:mi 1:D", "5:mi 10:D 0:M*2", "5:M 5:mi 0:M*2", "0:M 1:o 2:mi 3:o", "0:M 5:M 11:h 4:mi 9:mi 2:mi 7:D 0:M",
                        "2:mi 7:D 4:mi 9:Db9", "0:M 9:D 2:mi 7:D" })
            add(s, "Turnarounds", false);
        for (auto s : { "0:m*4", "0:m*2 5:D*2", "0:m 1:M", "0:S*2 3:S*2", "0:m*4 1:m*4", "0:L*2 2:V*2", "0:m*2 10:Mx*2", "0:S 10:S 8:S 7:S",
                        "0:m 3:m", "0:m*2 2:m*2", "0:L 7:L" })
            add(s, "Modal", true);
        for (auto s : { "5:M 4:D 9:mi 7:mi 0:D", "5:M 4:mi 9:mi", "2:mi 7:S", "0:M 4:mi 9:mi 2:mi", "2:mi 4:mi 5:M", "0:M*2 5:M*2", "5:M 7:S 4:mi 9:mi",
                        "8:L 10:S 0:M*2", "2:mi 7:S 0:M 4:Db9" })
            add(s, "Neo-Soul", false);
        for (auto s : { "0:m 5:D", "8:M 10:S 0:m*2", "0:m 3:M 5:D 3:M", "0:m 8:M 3:M 10:S", "0:m 5:m 10:D 3:M" }) add(s, "Neo-Soul", true);
        for (auto s : { "0:M 2:D 2:mi 1:D", "2:mi 7:D 0:M*2", "0:M 1:o 2:mi 7:D", "0:M*2 9:Db9 2:mi 7:D", "2:mi 1:D 0:M 9:mi" }) add(s, "Bossa Nova", false);
        for (auto s : { "0:m6 5:mi 2:h 7:Da 0:m6 3:mi 8:D 1:M", "0:m6*2 5:mi 10:D", "5:mi 10:D 3:M 8:M 2:h 7:Da 0:m6*2", "0:m6 2:h 7:Da 0:m6" })
            add(s, "Bossa Nova", true);
        for (auto s : { "0:M 9:mi 2:mi 7:D", "0:M 5:L", "0:M 4:mi 5:M 5:mi", "4:mi 9:D 2:mi 7:D", "0:M 0:D 5:M 5:mi", "0:M*2 5:L*2", "5:M 4:mi 3:mi 2:mi" })
            add(s, "Ballad", false);
        for (auto s : { "0:m6 8:M 2:h 7:Da", "0:m 0:!mM7 0:!m7 0:!m6" }) add(s, "Ballad", true);
        add("0:D 5:D 0:D 0:D 5:D 5:D 0:D 9:D 2:mi 7:D 0:D 7:D", "Blues", false);
        add("0:D 5:D 0:D 0:D 5:D 5:o 0:D 4:mi 2:mi 7:D 0:D 9:D", "Blues", false);
        add("0:D 5:D 0:D 7:mi 0:D 5:D 5:o 0:D 4:mi 9:D 2:mi 7:D 4:mi 9:D 2:mi 7:D", "Blues", false);
        add("0:m 5:m 0:m 0:m 5:m 5:m 0:m 0:m 8:D 7:Da 0:m 7:Da", "Blues", true);
        add("0:D 5:D 0:D*2", "Blues", false);
        add("0:D*2 5:D*2", "Blues", false);
        add("0:m*2 5:m*2", "Blues", true);

        expandProgressions();
    }

    // genre-appropriate transformations of every core progression
    void expandProgressions() {
        const std::vector<Progression> core = progs;
        for (auto& p : core) {
            const int g = p.genre;
            auto ch = p.chords;
            if (p.parallel) {
                // parallel paths: other shapes are applied at voicing time; here add rotations, retrogrades and 2x-speed doubles
                for (size_t r = 1; r < ch.size(); ++r) { auto v = ch; std::rotate(v.begin(), v.begin() + (long)r, v.end()); int base = v[0].deg; for (auto& c : v) c.deg = mod12(c.deg - base); addChords(v, p.style, true, "rotation", true); }
                { auto v = ch; std::reverse(v.begin(), v.end()); int base = v[0].deg; for (auto& c : v) c.deg = mod12(c.deg - base); addChords(v, p.style, true, "retrograde", true); }
                if (ch.size() <= 4) { auto v = ch; for (auto c : ch) v.push_back(c); v.back().deg = mod12(v.back().deg + (ch.size() > 2 ? 5 : 2)); addChords(v, p.style, true, "turnaround fill", true); }
                continue;
            }
            // rotations (start the loop somewhere else)
            if (ch.size() >= 3 && ch.size() <= 4)
                for (size_t r = 1; r < ch.size(); ++r) { auto v = ch; std::rotate(v.begin(), v.begin() + (long)r, v.end()); addChords(v, p.style, p.minor, "rotation", false); }

            if (g == Jazz || p.style == styleIndex("Hi-Tech Jazz") || p.style == styleIndex("Soulful Gospel") || p.style == styleIndex("Neo-Soul")) {
                // tritone substitution of every dominant
                { auto v = ch; bool any = false; for (auto& c : v) if (c.fam == "D" || c.fam == "Db9" || c.fam == "Da" || c.fam == "V") { c.deg = mod12(c.deg + 6); c.fam = "D"; any = true; } if (any) addChords(v, p.style, p.minor, "tritone sub", false); }
                // altered dominants
                { auto v = ch; bool any = false; for (auto& c : v) if (c.fam == "D") { c.fam = p.minor ? "Da" : "Db9"; any = true; } if (any) addChords(v, p.style, p.minor, "altered", false); }
                // sus dominants (modern / neo-soul colour)
                { auto v = ch; bool any = false; for (auto& c : v) if (c.fam == "D" || c.fam == "Db9") { c.fam = "S"; any = true; } if (any) addChords(v, p.style, p.minor, "sus dominants", false); }
                // ii-V into the first chord when it's a target (I or i)
                if (ch.size() <= 6 && ch[0].deg == 0 && ch[0].slots >= 2) {
                    auto v = ch; v[0].slots -= 1; PChord ii; ii.deg = 2; ii.fam = p.minor ? "h" : "mi"; PChord V; V.deg = 7; V.fam = p.minor ? "Da" : "D";
                    std::vector<PChord> w = v; w.push_back(ii); w.push_back(V); addChords(w, p.style, p.minor, "ii-V lead-in", false);
                }
                // backdoor ending (iv - bVII7 - I) for major progressions ending on V
                if (!p.minor && ch.back().deg == 7) { auto v = ch; v.back().deg = 10; v.back().fam = "D"; PChord iv; iv.deg = 5; iv.fam = "mi"; v.insert(v.end() - 1, iv); addChords(v, p.style, p.minor, "backdoor", false); }
                // Lydian tonic colour
                { auto v = ch; bool any = false; for (auto& c : v) if (c.fam == "M" && (c.deg == 0 || c.deg == 5)) { c.fam = "L"; any = true; } if (any) addChords(v, p.style, p.minor, "lydian colour", false); }
                // diminished passing chord between I and ii
                if (!p.minor && ch.size() >= 2 && ch[0].deg == 0 && ch[1].deg == 2) { auto v = ch; PChord d; d.deg = 1; d.fam = "o"; v.insert(v.begin() + 1, d); addChords(v, p.style, p.minor, "dim passing", false); }
            }
            if (g == House || p.style == styleIndex("Detroit Strings") || p.style == styleIndex("Melodic Techno") || p.style == styleIndex("Dub Techno")) {
                // borrowed iv in major / Dorian IV7 in minor
                if (!p.minor) { auto v = ch; bool any = false; for (auto& c : v) if (c.deg == 5 && c.fam == "M") { c.fam = "mi"; any = true; } if (any) addChords(v, p.style, p.minor, "borrowed iv", false); }
                else { auto v = ch; bool any = false; for (auto& c : v) if (c.deg == 5 && c.fam == "m") { c.fam = "D"; any = true; } if (any) addChords(v, p.style, p.minor, "dorian IV7", false); }
                // suspended colours on the last chord (leaves the loop hanging)
                { auto v = ch; auto& c = v.back(); if (c.fam == "Mx" || c.fam == "V" || c.fam == "M") { c.fam = "S"; addChords(v, p.style, p.minor, "sus ending", false); } }
                // minor 11 tonic with the 5th swapped for the 11th (Jersey trick)
                if (p.minor && ch[0].deg == 0 && ch[0].fam == "m") { auto v = ch; v[0].fam = ""; v[0].fixed = "m7_11"; addChords(v, p.style, p.minor, "m7(11) tonic", false); }
                // pedal bass: everything over the tonic (or the 5th)
                if (ch.size() >= 2 && ch.size() <= 4) {
                    auto v = ch; for (auto& c : v) c.bass = 0; addChords(v, p.style, p.minor, "tonic pedal", false);
                    if (p.minor) { auto w = ch; for (auto& c : w) c.bass = 7; addChords(w, p.style, p.minor, "5th pedal", false); }
                }
                // parallel (chord-memory) version of the same root movement
                if (ch.size() >= 2) addChords(ch, p.style, p.minor, "parallel version", true);
                // double-time: each chord for half the time, twice through
                if (ch.size() <= 3) { auto v = ch; for (auto c : ch) v.push_back(c); addChords(v, p.style, p.minor, "doubled", false); }
            }
            if (g == Techno) {
                // power-fifth (raw) version
                { auto v = ch; for (auto& c : v) { c.fam = "P5"; c.fixed.clear(); } addChords(v, styleIndex("Warehouse"), true, "fifths", false); }
                // sus2 shimmer
                { auto v = ch; for (auto& c : v) if (c.fam == "m") c.fam = "s2"; addChords(v, p.style, true, "sus2 colour", false); }
                // disjunctive shift: whole loop then a minor-third / tritone jump (machine harmony)
                if (ch.size() <= 2) { auto v = ch; for (auto c : ch) { c.deg = mod12(c.deg + 3); v.push_back(c); } addChords(v, p.style, true, "shift +3", false);
                                      auto w = ch; for (auto c : ch) { c.deg = mod12(c.deg + 6); w.push_back(c); } addChords(w, p.style, true, "shift tritone", false); }
            }
        }
        // stable order: by genre, then style, then insertion
        std::stable_sort(progs.begin(), progs.end(), [](const Progression& a, const Progression& b) { return a.genre != b.genre ? a.genre < b.genre : a.style < b.style; });
    }

    // ------------------------------------------------------------ rhythms
    struct CoreR { const char* name; int genre; int bars; int sustain; bool eighth; std::vector<Hit> hits; const char* tag; };

    void addR(const CoreR& c, const std::string& suffix, std::vector<Hit> hits, const std::string& tag) {
        std::sort(hits.begin(), hits.end(), [](auto& a, auto& b) { return a.step < b.step; });
        // merge duplicates
        std::vector<Hit> h; for (auto& x : hits) { if (!h.empty() && std::abs(h.back().step - x.step) < 0.01) { h.back().vel = std::max(h.back().vel, x.vel); continue; } h.push_back(x); }
        if (c.sustain == 0 && h.empty()) return;
        std::string id = std::to_string(c.genre) + "|" + std::to_string(c.bars) + "|" + std::to_string(c.sustain) + "|";
        for (auto& x : h) id += std::to_string((int)std::lround(x.step * 4)) + "," + std::to_string((int)std::lround(x.len * 4)) + ";";
        if (!seenR.insert(id).second) return;
        Rhythm r; r.id = id; r.name = std::string(c.name) + suffix; r.genre = c.genre; r.bars = c.bars; r.sustain = c.sustain; r.eighthFeel = c.eighth;
        r.hits = h; r.tag = tag; r.sync = syncopationOf(h, c.bars); r.density = (double)h.size() / c.bars;
        rhythms.push_back(r);
    }

    static std::vector<Hit> H(std::initializer_list<std::initializer_list<double>> l) {
        std::vector<Hit> v; for (auto& x : l) { auto it = x.begin(); Hit h; h.step = *it++; h.len = *it++; h.vel = (int)*it; v.push_back(h); } return v;
    }

    void buildRhythms() {
        std::vector<CoreR> core = {
            // ---------------- HOUSE ----------------
            { "Sustained Pad",       House, 1, 1, false, {}, "pad" },
            { "Bar Re-strike",       House, 1, 2, false, {}, "pad" },
            { "Off-beat Stabs",      House, 1, 0, false, H({ {2,1.5,100},{6,1.5,96},{10,1.5,100},{14,1.5,96} }), "stab" },
            { "3-3-2 Stabs",         House, 1, 0, false, H({ {0,2,110},{3,2,92},{6,2,98},{10,2,104},{13,2,94} }), "stab" },
            { "Jersey Push",         House, 1, 0, false, H({ {3,1,92},{6,2,108},{10,1,96},{13,1.5,100},{15,1,90} }), "stab" },
            { "Jersey Lilt",         House, 1, 0, false, H({ {2,1,96},{5,2,110},{9,1,80},{11,2,100},{14,1,92} }), "stab" },
            { "Piano 8ths",          House, 1, 0, false, H({ {0,1.5,84},{2,1.5,112},{4,1.5,80},{6,1.5,108},{8,1.5,84},{10,1.5,112},{12,1.5,80},{14,1.5,106} }), "piano" },
            { "Chicago Spread",      House, 2, 0, false, H({ {0,1,108},{4,1,96},{8,2,104},{11,1,98},{14,1,92},{18,1,100},{21,1,96},{24,2,104},{27,1,92} }), "piano" },
            { "Piano Anthem",        House, 1, 0, false, H({ {0,3,112},{3,3,96},{6,2,100},{10,2,104},{12,2,92},{14,2,100} }), "piano" },
            { "Organ Bounce",        House, 1, 0, false, H({ {0,1,104},{3,1,92},{6,1,100},{8,1,88},{10,1,104},{13,1,96} }), "organ" },
            { "Disco Upstrokes",     House, 1, 0, false, H({ {2,1,100},{4,0.5,70},{6,1,104},{10,1,100},{12,0.5,72},{14,1,104} }), "disco" },
            { "Disco Downbeat",      House, 1, 0, false, H({ {0,4,108},{7,1,88},{8,4,104},{15,1,90} }), "disco" },
            { "Garage Skip",         House, 1, 0, false, H({ {0,1,104},{3,1,96},{7,1,90},{10,1,104},{11,1,84},{14,1,96} }), "garage" },
            { "2-Step Answer",       House, 2, 0, false, H({ {3,1,98},{6,2,108},{10,1,92},{19,1,96},{22,1,104},{26,2,100},{29,1,90},{31,1,88} }), "garage" },
            { "Pad + Stab",          House, 1, 0, false, H({ {0,7,96},{10,1,88},{14,2,104} }), "pad" },
            { "Half-bar Pads",       House, 1, 0, false, H({ {0,7.5,98},{8,7.5,94} }), "pad" },
            { "Gospel Push",         House, 1, 0, false, H({ {0,2,104},{3,1,90},{6,3,108},{11,1,92},{14,2,104} }), "gospel" },
            { "Deep Offbeat Chords", House, 2, 0, false, H({ {2,3,100},{10,3,96},{14,1,88},{18,3,100},{26,2,96},{30,2,104} }), "deep" },
            { "Call & Response",     House, 2, 0, false, H({ {0,1,106},{3,1,92},{6,1,100},{22,1,104},{26,1,96},{29,1,100},{31,1,90} }), "stab" },
            { "Sparse Stabs",        House, 1, 0, false, H({ {2,2,104},{11,1.5,90} }), "stab" },
            // ---------------- TECHNO ----------------
            { "Long Pad",            Techno, 1, 1, false, {}, "pad" },
            { "Bar Pad",             Techno, 1, 2, false, {}, "pad" },
            { "Machine Riff",        Techno, 1, 0, false, H({ {0,1,112},{3,1,96},{6,1,100},{8,1,92},{11,1,100},{14,1,94} }), "riff" },
            { "Dub Echo",            Techno, 1, 0, false, H({ {2,1,112},{5,1,74},{8,1,54},{11,1,38},{14,1,100} }), "dub" },
            { "Dub Answer",          Techno, 2, 0, false, H({ {3,1,108},{6,1,70},{9,1,48},{19,1,104},{22,1,68},{26,1,96},{29,1,62} }), "dub" },
            { "Sparse Hit",          Techno, 1, 0, false, H({ {2,2,104},{11,1.5,90} }), "stab" },
            { "16th Pulse",          Techno, 1, 0, false, H({ {0,0.5,112},{1,0.5,60},{2,0.5,84},{3,0.5,60},{4,0.5,104},{5,0.5,60},{6,0.5,88},{7,0.5,62},{8,0.5,110},{9,0.5,60},{10,0.5,86},{11,0.5,60},{12,0.5,104},{13,0.5,62},{14,0.5,90},{15,0.5,64} }), "pulse" },
            { "Polymeter 5",         Techno, 2, 0, false, H({ {0,1,110},{5,1,96},{10,1,104},{15,1,94},{20,1,108},{25,1,96},{30,1,100} }), "poly" },
            { "Polymeter 3",         Techno, 1, 0, false, H({ {0,1,108},{3,1,94},{6,1,100},{9,1,94},{12,1,104},{15,1,92} }), "poly" },
            { "Off-beat Stab",       Techno, 1, 0, false, H({ {2,1,104},{6,1,96},{10,1,104},{14,1,96} }), "stab" },
            { "Half-time Stab",      Techno, 2, 0, false, H({ {6,2,108},{22,2,104},{30,1,90} }), "stab" },
            { "Rave Riff",           Techno, 2, 0, false, H({ {0,1,112},{2,1,100},{4,1,104},{7,1,98},{10,1,104},{12,1,100},{16,1,112},{19,1,98},{22,1,104},{24,1,96},{27,1,102},{30,1,98} }), "rave" },
            { "Detroit Chord Riff",  Techno, 1, 0, false, H({ {0,1,108},{3,1,92},{4,1,100},{7,1,92},{10,1,104},{12,1,96} }), "riff" },
            { "Hypnotic 3+3+2",      Techno, 1, 0, false, H({ {1,1,100},{4,1,92},{7,1,104},{9,1,90},{12,1,98},{15,1,94} }), "riff" },
            { "Stutter",             Techno, 1, 0, false, H({ {0,0.5,110},{1,0.5,80},{2,0.5,96},{8,0.5,108},{9,0.5,78},{10,0.5,92},{14,1,100} }), "stab" },
            { "Swell & Stab",        Techno, 2, 0, false, H({ {0,14,90},{18,1,104},{22,1,96},{26,1,104},{30,2,100} }), "pad" },
            { "Minimal Click",       Techno, 1, 0, false, H({ {3,0.5,100},{7,0.5,84},{11,0.5,100} }), "stab" },
            // ---------------- JAZZ ----------------
            { "Whole Notes",         Jazz, 1, 1, true, {}, "pad" },
            { "Ballad Restrike",     Jazz, 1, 2, true, {}, "pad" },
            { "Charleston",          Jazz, 1, 0, true, H({ {0,5,108},{6,4,96} }), "comp" },
            { "Reverse Charleston",  Jazz, 1, 0, true, H({ {2,4,100},{8,6,104} }), "comp" },
            { "Charleston on 3",     Jazz, 1, 0, true, H({ {8,5,104},{14,2,98} }), "comp" },
            { "Shifted Charleston",  Jazz, 1, 0, true, H({ {2,5,102},{10,4,96} }), "comp" },
            { "Red Garland",         Jazz, 1, 0, true, H({ {6,2,100},{14,2,104} }), "comp" },
            { "Red Garland Push",    Jazz, 2, 0, true, H({ {6,2,100},{14,4,106},{22,2,98},{30,2,104} }), "comp" },
            { "Anticipated 4&",      Jazz, 1, 0, true, H({ {0,4,100},{14,4,108} }), "comp" },
            { "Four to the Bar",     Jazz, 1, 0, true, H({ {0,2,100},{4,2,92},{8,2,98},{12,2,92} }), "four" },
            { "Freddie Green",       Jazz, 1, 0, true, H({ {0,1.5,96},{4,1.5,104},{8,1.5,96},{12,1.5,104} }), "four" },
            { "Two-Feel",            Jazz, 1, 0, true, H({ {0,6,100},{8,6,96} }), "four" },
            { "Bossa Comp",          Jazz, 2, 0, false, H({ {0,2,100},{6,2,96},{12,3,104},{20,2,100},{26,4,104} }), "bossa" },
            { "Bossa Clave",         Jazz, 2, 0, false, H({ {0,2,104},{6,2,96},{12,2,100},{20,2,100},{26,2,96} }), "bossa" },
            { "Bossa 8ths",          Jazz, 1, 0, false, H({ {0,1,90},{3,1,100},{6,1,92},{8,1,88},{10,1,100},{13,1,96} }), "bossa" },
            { "Comp & Answer",       Jazz, 2, 0, true, H({ {0,5,104},{6,3,92},{18,2,98},{22,2,100},{30,2,106} }), "comp" },
            { "Neo-Soul Lay-back",   Jazz, 1, 0, false, H({ {0,6,100},{7,1,80},{10,4,96},{15,1,86} }), "neo" },
            { "Neo-Soul Push",       Jazz, 2, 0, false, H({ {0,5,100},{6,2,88},{11,4,98},{18,4,96},{23,1,80},{26,5,102},{31,1,84} }), "neo" },
            { "Waltz-ish 3s",        Jazz, 1, 0, true, H({ {0,2,104},{6,2,94},{12,2,98} }), "comp" },
            { "Stride Off-beats",    Jazz, 1, 0, true, H({ {2,1.5,96},{6,1.5,104},{10,1.5,96},{14,1.5,104} }), "comp" },
        };
        for (auto& c : core) expandRhythm(c);

        // per-genre syncopation classes (tertiles) and kick-aware groove ratings
        for (int g = 0; g < NumGenres; ++g) {
            std::vector<double> s; for (auto& r : rhythms) if (r.genre == g && r.sustain == 0) s.push_back(r.sync);
            std::sort(s.begin(), s.end());
            double t1 = s.empty() ? 0 : s[s.size() / 3], t2 = s.empty() ? 0 : s[(s.size() * 2) / 3], med = s.empty() ? 4 : s[s.size() / 2];
            for (auto& r : rhythms) if (r.genre == g) {
                r.syncClass = r.sustain ? 0 : (r.sync <= t1 ? 0 : (r.sync <= t2 ? 1 : 2));
                analyseAgainstKick(r); r.groove = grooveScore(r, med);
            }
        }
        // drop machine-made variants that fight the kick (keep every hand-written core pattern)
        rhythms.erase(std::remove_if(rhythms.begin(), rhythms.end(), [](const Rhythm& r) {
            return r.genre != 2 && r.sustain == 0 && r.groove < 0.75 && r.name.find("\xC2\xB7") != std::string::npos; }), rhythms.end());
        std::stable_sort(rhythms.begin(), rhythms.end(), [](const Rhythm& a, const Rhythm& b) { return a.genre < b.genre; });
    }

    void expandRhythm(const CoreR& c) {
        addR(c, "", c.hits, c.tag);
        if (c.sustain) {
            // pads: add pulse/swell variants
            CoreR v = c; v.sustain = 0; v.bars = 1;
            addR(v, " \xC2\xB7 16th push", { { 15, 16, 98 } }, "pad");       // each chord arrives a 16th early
            addR(v, " \xC2\xB7 8th push", { { 14, 16, 98 } }, "pad");
            addR(v, " \xC2\xB7 half notes", { { 0, 7.5, 98 }, { 8, 7.5, 90 } }, "pad");
            v.bars = 2;
            addR(v, " \xC2\xB7 swell + push", { { 0, 15.5, 92 }, { 31, 16, 100 } }, "pad");
            return;
        }
        const int N = 16 * c.bars;
        auto wrap = [&](double s) { while (s < 0) s += N; while (s >= N) s -= N; return s; };
        // displacement (the same figure a 16th or an 8th later / earlier)
        for (double d : { 1.0, -1.0, 2.0, -2.0 }) {
            if (c.eighth && std::abs(d) < 2) continue;
            std::vector<Hit> v = c.hits; for (auto& h : v) h.step = wrap(h.step + d);
            addR(c, d > 0 ? (d > 1 ? " \xC2\xB7 8th late" : " \xC2\xB7 16th late") : (d < -1 ? " \xC2\xB7 8th early" : " \xC2\xB7 16th early"), v, c.tag);
        }
        // anticipation: last hit moved to the final 16th (or 8th) so it grabs the next chord
        { std::vector<Hit> v = c.hits; auto last = std::max_element(v.begin(), v.end(), [](auto& a, auto& b) { return a.step < b.step; });
          double target = c.eighth ? N - 2 : N - 1; if (last->step < target) { last->step = target; last->len = std::min(last->len, 2.0); addR(c, " \xC2\xB7 anticipated", v, c.tag); } }
        // thinned: drop the weakest hit (keeps the skeleton)
        if (c.hits.size() >= 3) { std::vector<Hit> v = c.hits; auto w = std::min_element(v.begin(), v.end(), [](auto& a, auto& b) { return a.vel < b.vel; }); v.erase(w); addR(c, " \xC2\xB7 thinned", v, c.tag); }
        if (c.hits.size() >= 5) { std::vector<Hit> v; for (size_t i = 0; i < c.hits.size(); i += 2) v.push_back(c.hits[i]); addR(c, " \xC2\xB7 half", v, c.tag); }
        // ghost fill: soft 16ths before each accent
        { std::vector<Hit> v = c.hits; for (auto& h : c.hits) if (h.vel >= 100) { double s = wrap(h.step - 1); bool occ = false; for (auto& x : c.hits) if (std::abs(x.step - s) < 0.1) occ = true; if (!occ) v.push_back({ s, 0.5, 64 }); }
          addR(c, " \xC2\xB7 ghosts", v, c.tag); }
        // articulation
        { std::vector<Hit> v = c.hits; for (auto& h : v) h.len = std::min(h.len, 0.75); addR(c, " \xC2\xB7 staccato", v, c.tag); }
        { std::vector<Hit> v = c.hits; std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.step < b.step; });
          for (size_t i = 0; i < v.size(); ++i) { double nx = i + 1 < v.size() ? v[i + 1].step : v[0].step + N; v[i].len = std::max(0.5, nx - v[i].step - 0.25); }
          addR(c, " \xC2\xB7 legato", v, c.tag); }
        // accent flip (velocity shape inverted)
        { std::vector<Hit> v = c.hits; int mx = 0, mn = 127; for (auto& h : v) { mx = std::max(mx, h.vel); mn = std::min(mn, h.vel); } for (auto& h : v) h.vel = clampv(mx + mn - h.vel, 40, 127); addR(c, " \xC2\xB7 accent flip", v, c.tag); }
        // two-bar call & response: one-bar figure answered by its displaced / thinned version
        if (c.bars == 1) {
            CoreR two = c; two.bars = 2;
            std::vector<Hit> a = c.hits, b; for (auto& h : c.hits) b.push_back({ wrap(h.step + 2) + 16, h.len, h.vel });
            std::vector<Hit> v = a; v.insert(v.end(), b.begin(), b.end()); addR(two, " \xC2\xB7 answer 8th late", v, c.tag);
            std::vector<Hit> b2; for (size_t i = 0; i < c.hits.size(); ++i) if (i % 2 == 0) b2.push_back({ c.hits[i].step + 16, c.hits[i].len, c.hits[i].vel });
            b2.push_back({ 31, 1, 96 });
            std::vector<Hit> v2 = a; v2.insert(v2.end(), b2.begin(), b2.end()); addR(two, " \xC2\xB7 answer sparse + push", v2, c.tag);
            std::vector<Hit> b3; for (auto& h : c.hits) b3.push_back({ wrap(h.step - 1) + 16, h.len, h.vel });
            std::vector<Hit> v3 = a; v3.insert(v3.end(), b3.begin(), b3.end()); addR(two, " \xC2\xB7 answer 16th early", v3, c.tag);
        }
    }
};

} // namespace kc
