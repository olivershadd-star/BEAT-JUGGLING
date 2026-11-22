#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

// ============================================================================
//  Patterns et blocs de passe-passe (v0.2)
//
//  Un pattern a une longueur fixe (1 à 32 mesures de 4 temps) et une suite de
//  blocs joués l'un après l'autre :
//    - Lecture   : une platine, une zone du vinyle (départ + durée), un sens
//    - Mouvement : stop, scratch, backspin, démarrage, roll ; part par défaut
//                  de là où le bloc précédent s'est arrêté
//  Pattern pas plein : silence à la fin. Pattern qui déborde : le dernier bloc
//  est raccourci. Durées et départs sont comptés en doubles-croches
//  (1/4 de temps), soit 16 par mesure.
// ============================================================================

namespace bj
{
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

enum class BlockType { play = 0, move = 1 };
enum class Move { stop = 0, scratch, backspin, wind, roll, count };

constexpr int kNumPatterns = 4;
constexpr int kMaxBlocks = 64;
constexpr int kBarOptions[] = { 1, 2, 4, 8, 16, 32 };
constexpr int kNumBarOptions = 6;

struct Block
{
    BlockType type = BlockType::play;
    int   deck = 0;          // 0 = Gauche, 1 = Droite
    int   start16 = 0;       // départ dans le son, en doubles-croches depuis le 1er temps
    int   len16 = 16;        // durée en doubles-croches
    bool  reverse = false;
    Move  move = Move::stop;
    float amount = 0.4f;     // réglage du mouvement (0..1)
    bool  fromPrev = true;   // mouvement : part de la fin du bloc précédent
};

struct Pattern
{
    int bars = 4;
    std::vector<Block> blocks;

    double totalBeats() const { return bars * 4.0; }
    int total16() const { return bars * 16; }
    int used16() const { int s = 0; for (auto& b : blocks) s += b.len16; return s; }
};

// ---- affichage mesure.temps.double-croche ----
inline juce::String fmtPos16 (int s)   // position (commence à 1.1.1)
{
    if (s < 0) s = 0;
    return juce::String (s / 16 + 1) + "." + juce::String ((s % 16) / 4 + 1) + "." + juce::String (s % 4 + 1);
}

inline juce::String fmtDur16 (int n)   // durée (0.1.0 = un temps)
{
    if (n < 0) n = 0;
    return juce::String (n / 16) + "." + juce::String ((n % 16) / 4) + "." + juce::String (n % 4);
}

inline bool parse3 (const juce::String& t, int& a, int& b, int& c)
{
    juce::StringArray p;
    p.addTokens (t.trim(), ".:,; ", "");
    p.removeEmptyStrings();
    if (p.isEmpty() || p.size() > 3) return false;
    for (auto& x : p) if (! x.containsOnly ("0123456789")) return false;
    a = p[0].getIntValue();
    b = p.size() > 1 ? p[1].getIntValue() : -1;
    c = p.size() > 2 ? p[2].getIntValue() : -1;
    return true;
}

inline int parsePos16 (const juce::String& t, int fallback)
{
    int a, b, c;
    if (! parse3 (t, a, b, c)) return fallback;
    if (b < 0) b = 1;
    if (c < 0) c = 1;
    return juce::jmax (0, (a - 1) * 16 + (b - 1) * 4 + (c - 1));
}

inline int parseDur16 (const juce::String& t, int fallback)
{
    int a, b, c;
    if (! parse3 (t, a, b, c)) return fallback;
    if (b < 0) b = 0;
    if (c < 0) c = 0;
    return juce::jmax (1, a * 16 + b * 4 + c);
}

inline juce::String moveName (Move m)
{
    switch (m)
    {
        case Move::stop:     return "Stop";
        case Move::scratch:  return "Scratch";
        case Move::backspin: return "Backspin";
        case Move::wind:     return U ("Démarrage");
        case Move::roll:     return "Roll";
        default:             return {};
    }
}

inline juce::String amountName (Move m)
{
    switch (m)
    {
        case Move::stop:     return "Courbe";
        case Move::scratch:  return "Vitesse";
        case Move::backspin: return "Force";
        case Move::wind:     return "Courbe";
        case Move::roll:     return "Divisions";
        default:             return {};
    }
}

inline int rollDivisions (float a)
{
    static const int t[] = { 2, 3, 4, 6, 8, 12, 16 };
    return t[juce::jlimit (0, 6, (int) std::round (a * 6.0f))];
}

inline juce::String amountText (Move m, float a)
{
    switch (m)
    {
        case Move::stop:
        case Move::wind:     return a < 0.33f ? "douce" : (a < 0.66f ? "moyenne" : U ("sèche"));
        case Move::scratch:  return juce::String (1.0f + 3.0f * a, 1) + "/temps";
        case Move::backspin: return juce::String ((int) std::round (a * 100)) + " %";
        case Move::roll:     return "x" + juce::String (rollDivisions (a));
        default:             return {};
    }
}

// ---- mouvement : position u (en temps, depuis la base), vitesse v, gain ----
struct MoveState { double u, v, gain; };

inline MoveState evalMove (Move m, float a, double D, double tl, double fadeBeats)
{
    D = juce::jmax (1.0e-6, D);
    const double x = juce::jlimit (0.0, 1.0, tl / D);
    switch (m)
    {
        case Move::stop:
        {
            const double p = 0.5 + 3.0 * a;
            if (tl >= D) return { D / (p + 1.0), 0.0, 0.0 };
            const double y = 1.0 - x;
            const double v = std::pow (y, p);
            return { D * (1.0 - std::pow (y, p + 1.0)) / (p + 1.0), v, std::sqrt (v) };
        }
        case Move::wind:
        {
            const double p = 0.5 + 3.0 * a;
            if (tl >= D) return { D / (p + 1.0) + (tl - D), 1.0, 1.0 };
            const double v = std::pow (x, p);
            return { D * std::pow (x, p + 1.0) / (p + 1.0), v, std::sqrt (v) };
        }
        case Move::backspin:
        {
            const double k = 2.0 + 6.0 * a;
            if (tl >= D) return { -k * D / 3.0, 0.0, 0.0 };
            const double y = 1.0 - x;
            return { -k * D * (1.0 - y * y * y) / 3.0, -k * y * y, std::pow (y, 0.6) };
        }
        case Move::scratch:
        {
            const double perBeat = 1.0 + 3.0 * a;
            const int n = juce::jmax (1, (int) std::round (D * perBeat));
            const double A = juce::jmin (0.25, 0.5 * D / n + 0.06);
            const double w = juce::MathConstants<double>::twoPi * n / D;
            const double ph = w * tl;
            const double u = A * (1.0 - std::cos (ph)) * 0.5;
            const double v = A * 0.5 * std::sin (ph) * w;
            return { u, v, juce::jmin (1.0, std::abs (v) * 2.0 + 0.05) };
        }
        case Move::roll:
        {
            const int N = rollDivisions (a);
            const double seg = D / N;
            const double r = std::fmod (juce::jmax (0.0, tl), seg);
            const double wf = juce::jmin (seg * 0.15, fadeBeats);
            double g = 1.0;
            if (wf > 0) g = juce::jlimit (0.0, 1.0, juce::jmin (r / wf, (seg - r) / wf));
            return { r, 1.0, g };
        }
        default: break;
    }
    return { tl, 1.0, 1.0 };
}

// ---- segment prêt à jouer (calculé à chaque bloc audio) ----
struct RenderStep
{
    double start = 0, dur = 0;   // en temps, dans le pattern
    bool   silent = true;
    int    deck = 0;
    bool   isMove = false;
    Move   move = Move::stop;
    float  amount = 0.4f;
    double base = 0;             // position de départ dans le son (échantillons)
    double dir = 1;              // lecture : +1 avant, -1 arrière
    int    block = -1;           // index du bloc (-1 = silence)
    double zoneStart = 0, zoneLen = 0;  // pour l'affichage (échantillons)
};
} // namespace bj
