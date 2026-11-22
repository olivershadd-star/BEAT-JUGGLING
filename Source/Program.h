#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>

// ============================================================================
//  Programme de passe-passe
//
//  Syntaxe (séparateurs : espaces, | ou virgules ; majuscules/minuscules égales)
//    1        un segment d'un temps (alternance automatique A, B, A, B...)
//    1/2 3/4 1.5   durées en temps (fractions ou décimales)
//    3+2      groupe : un segment de 3 temps puis un de 2
//    x3       répète le segment précédent (3 fois au total)   ex : 1 x3  ou  1x3
//    A1 B1/2  force la platine
//    _1       silence d'un temps
//    @4       départ forcé (en 1/16 depuis le début de la sélection) ex : B1@4
//  Mouvements (lettres après la durée, combinables avec @ et x) :
//    r  reverse          s  stop platine (ralentit jusqu'à l'arrêt)
//    w  démarrage platine (part de zéro)      k  backspin (retour arrière)
//    z  scratch (aller-retour)                c  continue (ne revient pas au départ)
//    *4 roll : rejoue 4 fois le début du segment
// ============================================================================

namespace bj
{
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

enum class Move { normal, reverse, stop, wind, backspin, scratch };

struct Step
{
    double start = 0.0;     // en temps, depuis le début du programme
    double dur   = 1.0;     // en temps
    int    deck  = 0;       // 0 = A, 1 = B
    bool   silent = false;
    double cue16 = -1.0;    // < 0 : départ de la platine
    Move   move  = Move::normal;
    int    roll  = 1;
    bool   cont  = false;
    double offset = 0.0;    // en temps, pour "continue"
};

struct Program
{
    std::vector<Step> steps;
    double total = 0.0;
    juce::String error;     // vide si tout va bien

    // index du segment qui contient p (0 <= p < total)
    int find (double p) const
    {
        int lo = 0, hi = (int) steps.size() - 1;
        while (lo < hi)
        {
            int mid = (lo + hi + 1) / 2;
            if (steps[(size_t) mid].start <= p) lo = mid; else hi = mid - 1;
        }
        return lo;
    }
};

namespace detail
{
    inline bool parseNumber (const juce::String& s, double& out)
    {
        if (s.isEmpty()) return false;
        if (s.containsChar ('/'))
        {
            auto a = s.upToFirstOccurrenceOf ("/", false, false);
            auto b = s.fromFirstOccurrenceOf ("/", false, false);
            if (a.isEmpty()) a = "1";
            if (! a.containsOnly ("0123456789.") || ! b.containsOnly ("0123456789.") || b.isEmpty())
                return false;
            double d = b.getDoubleValue();
            if (d <= 0.0) return false;
            out = a.getDoubleValue() / d;
            return out > 0.0;
        }
        if (! s.containsOnly ("0123456789.")) return false;
        out = s.getDoubleValue();
        return out > 0.0;
    }
}

inline Program parseProgram (const juce::String& text)
{
    Program prog;
    juce::StringArray tokens;
    auto cleaned = text.replaceCharacters ("|,;\n\r\t", "      ");
    tokens.addTokens (cleaned, " ", "");
    tokens.removeEmptyStrings();

    struct Raw { int deck = -1; bool silent = false; double dur = 1; double cue16 = -1; Move move = Move::normal; int roll = 1; bool cont = false; };
    std::vector<Raw> raws;
    size_t lastGroupStart = 0;   // pour que « 3+2 x3 » répète tout le groupe

    auto fail = [&prog] (const juce::String& msg) { prog.error = msg; prog.steps.clear(); prog.total = 0; return prog; };

    for (auto tok : tokens)
    {
        tok = tok.toLowerCase().trim();
        if (tok.isEmpty()) continue;

        // répétition isolée : x3
        if (tok[0] == 'x' && tok.length() > 1 && tok.substring (1).containsOnly ("0123456789"))
        {
            if (raws.empty()) return fail (bj::U ("« ") + tok + bj::U (" » : rien à répéter avant"));
            int n = tok.substring (1).getIntValue();
            if (n < 1 || n > 256) return fail (bj::U ("répétition invalide : ") + tok);
            const std::vector<Raw> group (raws.begin() + (long) lastGroupStart, raws.end());
            for (int i = 1; i < n; ++i) raws.insert (raws.end(), group.begin(), group.end());
            if (raws.size() > 1024) return fail (bj::U ("programme trop long (1024 segments max)"));
            continue;
        }

        Raw r;
        int i = 0;
        auto len = tok.length();
        if (tok[i] == 'a') { r.deck = 0; ++i; }
        else if (tok[i] == 'b') { r.deck = 1; ++i; }
        if (i < len && tok[i] == '_') { r.silent = true; ++i; }

        // durée (avec groupes 3+2)
        int j = i;
        while (j < len && juce::String ("0123456789./+").containsChar (tok[j])) ++j;
        auto durStr = tok.substring (i, j);
        if (durStr.isEmpty()) return fail (bj::U ("« ") + tok + bj::U (" » : durée manquante"));

        juce::StringArray parts;
        parts.addTokens (durStr, "+", "");
        parts.removeEmptyStrings();
        std::vector<double> durs;
        for (auto& p : parts)
        {
            double d;
            if (! detail::parseNumber (p, d)) return fail (bj::U ("« ") + tok + bj::U (" » : durée illisible"));
            if (d < 1.0 / 64.0) return fail (bj::U ("« ") + tok + bj::U (" » : durée trop courte (min 1/64)"));
            durs.push_back (d);
        }

        // suffixes
        int repeat = 1;
        i = j;
        while (i < len)
        {
            auto c = tok[i];
            auto readNum = [&] (int from, int& to) {
                to = from;
                while (to < len && juce::String ("0123456789.").containsChar (tok[to])) ++to;
                return tok.substring (from, to);
            };
            int k;
            switch (c)
            {
                case 'r': r.move = Move::reverse;  ++i; break;
                case 's': r.move = Move::stop;     ++i; break;
                case 'w': r.move = Move::wind;     ++i; break;
                case 'k': r.move = Move::backspin; ++i; break;
                case 'z': r.move = Move::scratch;  ++i; break;
                case 'c': r.cont = true;           ++i; break;
                case '@': { auto n = readNum (i + 1, k); if (n.isEmpty()) return fail (bj::U ("« ") + tok + bj::U (" » : nombre attendu après @")); r.cue16 = n.getDoubleValue(); i = k; break; }
                case '*': { auto n = readNum (i + 1, k); int v = n.getIntValue(); if (v < 1 || v > 64) return fail (bj::U ("« ") + tok + bj::U (" » : roll entre *1 et *64")); r.roll = v; i = k; break; }
                case 'x': { auto n = readNum (i + 1, k); int v = n.getIntValue(); if (v < 1 || v > 256) return fail (bj::U ("« ") + tok + bj::U (" » : répétition invalide")); repeat = v; i = k; break; }
                default: return fail (bj::U ("« ") + tok + bj::U (" » : lettre inconnue '") + juce::String::charToString (c) + "'");
            }
        }

        lastGroupStart = raws.size();
        for (int rep = 0; rep < repeat; ++rep)
            for (auto d : durs) { auto rr = r; rr.dur = d; raws.push_back (rr); }

        if (raws.size() > 1024) return fail ("programme trop long (1024 segments max)");
    }

    if (raws.empty()) return fail ("programme vide");

    // alternance automatique + offsets "continue"
    int nextAuto = 0;
    double t = 0;
    double elapsed[2] = { 0, 0 };
    double lastCue[2] = { -1, -1 };
    for (auto& r : raws)
    {
        Step s;
        s.deck = r.deck >= 0 ? r.deck : nextAuto;
        nextAuto = 1 - s.deck;
        s.start = t; s.dur = r.dur; s.silent = r.silent;
        s.move = r.move; s.roll = r.roll; s.cont = r.cont;
        s.cue16 = r.cue16;

        if (! s.silent)
        {
            auto& el = elapsed[s.deck];
            if (s.cont)
            {
                s.offset = el;
                if (s.cue16 < 0) s.cue16 = lastCue[s.deck];
                el += s.dur;
            }
            else
            {
                s.offset = 0;
                el = s.dur;
                lastCue[s.deck] = s.cue16;
            }
        }
        t += r.dur;
        prog.steps.push_back (s);
    }
    prog.total = t;
    return prog;
}

// Position (en temps relatifs au départ) et vitesse d'un mouvement à l'instant tl du segment
struct MoveState { double u; double v; double gain; };

inline MoveState evalMove (const Step& s, double tl)
{
    const double D = s.dur;
    double x = juce::jlimit (0.0, 1.0, tl / D);
    double seg = s.roll > 1 ? D / s.roll : D;
    double tr  = s.roll > 1 ? std::fmod (tl, seg) : tl;
    juce::ignoreUnused (x);
    if (s.move != Move::normal && s.move != Move::scratch)
        tr = juce::jmin (tr, seg);   // au-delà de la fin (fondu), on reste figé
    double xr  = juce::jlimit (0.0, 1.0, tr / seg);

    switch (s.move)
    {
        case Move::normal:   return { tr, 1.0, 1.0 };
        case Move::reverse:  return { seg - tr, -1.0, 1.0 };
        case Move::stop:     { double u = tr - tr * tr / (2.0 * seg);
                               double v = 1.0 - xr; return { u, v, std::sqrt (juce::jmax (0.0, v)) }; }
        case Move::wind:     { double u = tr * tr / (2.0 * seg); double v = xr; return { u, v, std::sqrt (xr) }; }
        case Move::backspin: { double y = 1.0 - xr; double u = seg * y * y * y; double v = -3.0 * y * y;
                               return { u, v, juce::jmin (1.0, y * 1.5) }; }
        case Move::scratch:  { int n = juce::jmax (1, (int) std::round (seg * 2.0));
                               double A = juce::jmin (0.25, seg * 0.25);
                               double ph = juce::MathConstants<double>::twoPi * n * xr;
                               double u = A * (1.0 - std::cos (ph)) * 0.5;
                               double v = A * 0.5 * std::sin (ph) * juce::MathConstants<double>::twoPi * n / seg;
                               return { u, v, juce::jmin (1.0, std::abs (v) * 3.0 + 0.05) }; }
    }
    return { tr, 1.0, 1.0 };
}

inline const char* moveLetter (Move m)
{
    switch (m)
    {
        case Move::reverse:  return "R";
        case Move::stop:     return "S";
        case Move::wind:     return "W";
        case Move::backspin: return "K";
        case Move::scratch:  return "Z";
        default:             return "";
    }
}
} // namespace bj
