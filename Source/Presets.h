#pragma once
// Factory presets (generated deterministically from the libraries) + user presets on disk + favourites.
#include <juce_audio_processors/juce_audio_processors.h>
#include "engine/Engine.h"
#include "engine/Groove.h"

namespace kc {

struct PresetData {
    juce::String name; int genre = 0; bool user = false;
    std::map<juce::String, float> values;   // parameter id -> plain value
    juce::String progId, rhythmId;
};

class Presets {
public:
    static const std::vector<PresetData>& factory() { static std::vector<PresetData> f = build(); return f; }

    static juce::File userDir() {
        auto d = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
                     .getChildFile("Application Support")
#endif
                     .getChildFile("Keir's Chords").getChildFile("User Presets");
        d.createDirectory(); return d;
    }
    static juce::File favFile() { return userDir().getParentDirectory().getChildFile("favourites.txt"); }

    static std::vector<PresetData> loadUser() {
        std::vector<PresetData> v;
        for (auto& f : userDir().findChildFiles(juce::File::findFiles, false, "*.kcpreset")) {
            if (auto xml = juce::XmlDocument::parse(f)) {
                PresetData p; p.user = true; p.name = f.getFileNameWithoutExtension(); p.genre = xml->getIntAttribute("genre", 0);
                p.progId = xml->getStringAttribute("progId"); p.rhythmId = xml->getStringAttribute("rhythmId");
                for (auto* e : xml->getChildWithTagNameIterator("P")) p.values[e->getStringAttribute("id")] = (float)e->getDoubleAttribute("v");
                v.push_back(p);
            }
        }
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.name.compareIgnoreCase(b.name) < 0; });
        return v;
    }
    static bool saveUser(const PresetData& p) {
        juce::XmlElement x("KeirsChordsPreset");
        x.setAttribute("genre", p.genre); x.setAttribute("progId", p.progId); x.setAttribute("rhythmId", p.rhythmId);
        for (auto& kv : p.values) { auto* e = x.createNewChildElement("P"); e->setAttribute("id", kv.first); e->setAttribute("v", kv.second); }
        return x.writeTo(userDir().getChildFile(juce::File::createLegalFileName(p.name) + ".kcpreset"));
    }
    static juce::StringArray loadFavs() { juce::StringArray a; a.addLines(favFile().loadFileAsString()); a.removeEmptyStrings(); return a; }
    static void saveFavs(const juce::StringArray& a) { favFile().replaceWithText(a.joinIntoString("\n")); }
    static juce::String keyOf(const PresetData& p) { return (p.user ? "U:" : "F:") + p.name; }

private:
    static std::vector<PresetData> build() {
        static const std::vector<std::vector<const char*>> ADJ = {
            { "Warehouse", "Velvet", "Basement", "Sunday", "Midnight", "Jackin'", "Soulful", "Golden", "Hazy", "Lofty", "After-Hours", "Glowing", "Smoky", "Silk", "Gospel", "Disco", "Garage", "Deep", "Loose", "Bumpin'" },
            { "Futurist", "Belle Isle", "Chrome", "Orbital", "Concrete", "Hypnotic", "Machine", "Sub-Zero", "Nocturnal", "Industrial", "Dub", "Infinite", "Neon", "Steel", "Tunnel", "Signal", "Grid", "Static", "Cosmic", "Magnetic" },
            { "Blue", "Smoky", "Late", "Uptown", "Lazy", "Velvet", "Bebop", "Modal", "Autumn", "Rainy", "Satin", "Midnight", "Brushed", "Cool", "Bossa", "Moonlit", "Quiet", "Swinging", "Ballroom", "Cellar" } };
        static const std::vector<std::vector<const char*>> NOUN = {
            { "Jack", "Groove", "Shuffle", "Stabs", "Organ", "Keys", "Sermon", "Rhodes", "Bounce", "Floor", "Push", "Strut", "Session", "Sway", "Vamp", "Lift", "Glide", "Pulse", "Rapture", "Testament" },
            { "Strings", "Transmission", "Drift", "Circuit", "Pulse", "Memory", "Signal", "Corridor", "Engine", "Horizon", "Echo", "Ritual", "Orbit", "Haze", "Voltage", "Panorama", "Depth", "Spiral", "Relay", "Frequency" },
            { "Changes", "Turnaround", "Comp", "Ballad", "Cadence", "Lament", "Standard", "Interlude", "Reverie", "Voicing", "Walk", "Sketch", "Nocturne", "Bounce", "Waltz", "Serenade", "Session", "Motif", "Coda", "Reflection" } };
        const auto& L = Library::get();
        std::vector<PresetData> out;
        std::set<juce::String> used;
        for (int g = 0; g < NumGenres; ++g) {
            std::vector<int> progs, rh;
            for (size_t i = 0; i < L.progs.size(); ++i) if (L.progs[i].genre == g) progs.push_back((int)i);
            for (size_t i = 0; i < L.rhythms.size(); ++i) if (L.rhythms[i].genre == g) rh.push_back((int)i);
            Rng r(hashSeed(2026, (uint32_t)g + 1));
            for (int n = 0; n < 1000; ++n) {
                PresetData p; p.genre = g;
                // prefer core progressions and medium-syncopation rhythms (the groove sweet spot)
                std::vector<std::pair<int, double>> pw, rw;
                int pi = progs[(size_t)r.below((int)progs.size())];
                if (L.progs[(size_t)pi].tag != "core" && r.chance(0.35)) { for (int k = 0; k < 6; ++k) { int c = progs[(size_t)r.below((int)progs.size())]; if (L.progs[(size_t)c].tag == "core") { pi = c; break; } } }
                int ri = rh[(size_t)r.below((int)rh.size())];
                for (int k = 0; k < 4 && L.rhythms[(size_t)ri].syncClass != 1; ++k) ri = rh[(size_t)r.below((int)rh.size())];
                const auto& P = L.progs[(size_t)pi]; const auto& R = L.rhythms[(size_t)ri];
                p.progId = P.id; p.rhythmId = R.id;
                p.values["prog"] = (float)pi; p.values["rhythm"] = (float)ri;
                p.values["voicing"] = r.chance(0.75) ? 0.f : (float)(1 + r.below(9));
                p.values["complexity"] = (float)clampv(50 + (r.next() * 2 - 1) * 30, 5.0, 95.0);
                p.values["variation"] = (float)(10 + r.below(50));
                p.values["dynamics"] = (float)(35 + r.below(40));
                p.values["gate"] = (float)(R.tag == "pad" ? 100 : 60 + r.below(70));
                p.values["octave"] = (float)(r.chance(0.8) ? 0 : (r.chance(0.5) ? -1 : 1));
                p.values["bars"] = 0; p.values["rate"] = 0; p.values["shape"] = 0;
                p.values["seed"] = (float)(1 + r.below(9999));
                // groove by genre
                if (g == House) {
                    int m = r.below(10); p.values["swingMode"] = (float)(m < 5 ? 2 : (m < 8 ? 1 : (m < 9 ? 3 : 0)));
                    p.values["swingPct"] = (float)(54 + r.below(9)); p.values["shuffle909"] = (float)(2 + r.below(3));
                    p.values["bass"] = (float)(r.chance(0.5) ? 0 : (r.chance(0.5) ? 2 : 3));
                } else if (g == Techno) {
                    int m = r.below(10); p.values["swingMode"] = (float)(m < 4 ? 1 : (m < 7 ? 0 : 2));
                    p.values["swingPct"] = (float)(52 + r.below(5)); p.values["shuffle909"] = (float)(1 + r.below(3));
                    p.values["bass"] = (float)(r.chance(0.6) ? 0 : (r.chance(0.5) ? 5 : 2));
                } else {
                    p.values["swingMode"] = (float)(R.eighthFeel ? 3 : (r.chance(0.5) ? 3 : 0));
                    p.values["swingPct"] = (float)(60 + r.below(7)); p.values["shuffle909"] = 1;
                    p.values["bass"] = (float)(r.chance(0.6) ? 4 : (r.chance(0.5) ? 1 : 0));
                }
                p.values["feel"] = (float)(15 + r.below(30)); p.values["swingRes"] = 0;
                juce::String nm = juce::String(styles()[(size_t)P.style].name) + juce::String::fromUTF8(" \xC2\xB7 ") + ADJ[(size_t)g][(size_t)r.below(20)] + " " + NOUN[(size_t)g][(size_t)r.below(20)];
                juce::String fin = nm; int k = 2; while (used.count(fin)) fin = nm + " " + juce::String(k++);
                used.insert(fin); p.name = fin;
                out.push_back(p);
            }
        }
        std::stable_sort(out.begin(), out.end(), [](auto& a, auto& b) { return a.genre != b.genre ? a.genre < b.genre : a.name.compareNatural(b.name) < 0; });
        return out;
    }
};

} // namespace kc
