#include "PromptPlanner.h"

namespace choreo
{
namespace
{
enum class Kind { gatherMid, gather, implode, scatter, tension, spiralIn, spiralOut, rise, descend,
                  circle, pendulum, rose, lissajous, hold, drift, arc };

struct Kw { const char* stem; Kind kind; const char* label; };

// Order = priority. English stems first, German stems (normalised ae/oe/ue/ss, lower case) after.
const Kw kTable[] = {
    { "implo", Kind::implode, "Implosion" },    { "collaps", Kind::implode, "Implosion" },
    { "cave in", Kind::implode, "Implosion" },  { "contract", Kind::implode, "Implosion" },
    { "explo", Kind::scatter, "Scatter" },      { "scatter", Kind::scatter, "Scatter" },
    { "disperse", Kind::scatter, "Scatter" },   { "burst", Kind::scatter, "Scatter" },
    { "fly apart", Kind::scatter, "Scatter" },  { "shatter", Kind::scatter, "Scatter" },
    { "disintegrat", Kind::scatter, "Scatter" },{ "spread out", Kind::scatter, "Scatter" },
    { "cluster", Kind::gather, "Gather" },      { "build-up", Kind::gather, "Gather" },
    { "buildup", Kind::gather, "Gather" },      { "build up", Kind::gather, "Gather" },
    { "condens", Kind::gather, "Gather" },      { "gather", Kind::gather, "Gather" },
    { "converg", Kind::gather, "Gather" },      { "approach", Kind::gather, "Gather" },
    { "push in", Kind::gather, "Gather" },      { "closing in", Kind::gather, "Gather" },
    { "together", Kind::gather, "Gather" },
    { "spiral", Kind::spiralOut, "Spiral" },
    { "ascen", Kind::rise, "Ascent" },          { "rise", Kind::rise, "Ascent" },
    { "rising", Kind::rise, "Ascent" },         { "climb", Kind::rise, "Ascent" },
    { "upward", Kind::rise, "Ascent" },         { "heaven", Kind::rise, "Ascent" },
    { "descen", Kind::descend, "Descent" },     { "sink", Kind::descend, "Descent" },
    { "abyss", Kind::descend, "Descent" },      { "fall", Kind::descend, "Descent" },
    { "circl", Kind::circle, "Circle" },        { "orbit", Kind::circle, "Circle" },
    { "revolv", Kind::circle, "Circle" },       { "rotat", Kind::circle, "Circle" },
    { "carousel", Kind::circle, "Circle" },     { "whirl", Kind::circle, "Circle" },
    { "pendulum", Kind::pendulum, "Pendulum" }, { "back and forth", Kind::pendulum, "Pendulum" },
    { "swing", Kind::pendulum, "Pendulum" },
    { "rosette", Kind::rose, "Rose" },          { "flower", Kind::rose, "Rose" },
    { "petal", Kind::rose, "Rose" },
    { "interlace", Kind::lissajous, "Interlace" },{ "intertwin", Kind::lissajous, "Interlace" },
    { "weave", Kind::lissajous, "Interlace" },  { "entangl", Kind::lissajous, "Interlace" },
    { "freeze", Kind::hold, "Rest" },           { "standstill", Kind::hold, "Rest" },
    { "stand still", Kind::hold, "Rest" },      { "hold", Kind::hold, "Rest" },
    { "motionless", Kind::hold, "Rest" },
    { "float", Kind::drift, "Float" },          { "hover", Kind::drift, "Float" },
    { "wander", Kind::arc, "Wander" },          { "sweep", Kind::arc, "Wander" },
    { "glide", Kind::arc, "Wander" },           { "move", Kind::arc, "Wander" },
    { "implod", Kind::implode, "Implosion" },   { "kollabier", Kind::implode, "Implosion" },
    { "zusammenbrech", Kind::implode, "Implosion" }, { "einstuerz", Kind::implode, "Implosion" },
    { "kollaps", Kind::implode, "Implosion" },  { "kontrahier", Kind::implode, "Implosion" },
    { "explod", Kind::scatter, "Scatter" },  { "zerstreu", Kind::scatter, "Scatter" },
    { "auseinander", Kind::scatter, "Scatter" }, { "zerfall", Kind::scatter, "Scatter" },
    { "zersprengt", Kind::scatter, "Scatter" }, { "ausbreit", Kind::scatter, "Scatter" },
    { "zerstob", Kind::scatter, "Scatter" },
    { "cluster", Kind::gather, "Gather" },     { "aufbau", Kind::gather, "Gather" },
    { "verdicht", Kind::gather, "Gather" },    { "sammel", Kind::gather, "Gather" },
    { "konvergier", Kind::gather, "Gather" },  { "zusammenzieh", Kind::gather, "Gather" },
    { "annaeher", Kind::gather, "Gather" },    { "zusammen", Kind::gather, "Gather" },
    { "spiral", Kind::spiralOut, "Spiral" },
    { "aufsteig", Kind::rise, "Ascent" },     { "steigen", Kind::rise, "Ascent" },
    { "empor", Kind::rise, "Ascent" },        { "zenit", Kind::rise, "Ascent" },
    { "himmel", Kind::rise, "Ascent" },
    { "absteig", Kind::descend, "Descent" },    { "sinken", Kind::descend, "Descent" },
    { "abgrund", Kind::descend, "Descent" },    { "versink", Kind::descend, "Descent" },
    { "kreis", Kind::circle, "Circle" },       { "umlauf", Kind::circle, "Circle" },
    { "rotier", Kind::circle, "Circle" },      { "umkreis", Kind::circle, "Circle" },
    { "karussell", Kind::circle, "Circle" },   { "wirbel", Kind::circle, "Circle" },
    { "pendel", Kind::pendulum, "Pendulum" },    { "hin und her", Kind::pendulum, "Pendulum" },
    { "schaukel", Kind::pendulum, "Pendulum" },
    { "rosette", Kind::rose, "Rose" },       { "blume", Kind::rose, "Rose" },
    { "bluete", Kind::rose, "Rose" },
    { "lissajous", Kind::lissajous, "Interlace" }, { "verschlung", Kind::lissajous, "Interlace" },
    { "verflecht", Kind::lissajous, "Interlace" }, { "geflecht", Kind::lissajous, "Interlace" },
    { "ruhen", Kind::hold, "Rest" },           { "verharr", Kind::hold, "Rest" },
    { "stillstand", Kind::hold, "Rest" },      { "erstarr", Kind::hold, "Rest" },
    { "einfrier", Kind::hold, "Rest" },
    { "drift", Kind::drift, "Float" },       { "schweb", Kind::drift, "Float" },
    { "treib", Kind::drift, "Float" },       { "umherirr", Kind::drift, "Float" },
    { "diffus", Kind::drift, "Float" },      { "ruhig", Kind::drift, "Float" },
    { "atmen", Kind::drift, "Float" },       { "still", Kind::drift, "Float" },
    { "wander", Kind::arc, "Wander" },         { "ziehen", Kind::arc, "Wander" },
    { "bewegen", Kind::arc, "Wander" },        { "wechsel", Kind::arc, "Wander" },
};

bool anyOf(const juce::String& s, std::initializer_list<const char*> stems)
{
    for (auto* k : stems) if (s.contains(k)) return true;
    return false;
}

juce::String normalise(juce::String s)
{
    s = s.toLowerCase();
    return s.replace(U("ä"), "ae").replace(U("ö"), "oe").replace(U("ü"), "ue").replace(U("ß"), "ss")
            .replace(U("é"), "e").replace(U("è"), "e").replace(U("à"), "a");
}

struct Ctx
{
    double anchorAz = 0;
    bool anchorGiven = false;
    bool threat = false, airy = false, nervous = false, slow = false, fast = false;
    double clusterElev = 0;
};

struct Phase
{
    Kind kind;
    juce::String name;
    double weight = 2;
};

juce::String phaseName(Kind k)
{
    switch (k)
    {
        case Kind::gatherMid:  return U("Approach");
        case Kind::gather:     return "Condensation";
        case Kind::implode:    return "Implosion";
        case Kind::scatter:    return "Scattering";
        case Kind::tension:    return "Tension";
        case Kind::spiralIn:   return "Encircling";
        case Kind::spiralOut:  return "Swing-out";
        case Kind::rise:       return "Ascent";
        case Kind::descend:    return "Descent";
        case Kind::circle:     return "Circle";
        case Kind::pendulum:   return "Pendulum";
        case Kind::rose:       return "Rose";
        case Kind::lissajous:  return "Interlace";
        case Kind::hold:       return "Rest";
        case Kind::drift:      return "Float";
        case Kind::arc:        return "Wander";
    }
    return "A";
}

double wrapAz(double a) { return std::fmod(std::fmod(a, 360.0) + 360.0, 360.0); }
double lerpAz(double a, double b, double t) { return a + (std::fmod(b - a + 540.0, 360.0) - 180.0) * t; }

double baseWeight(Kind k)
{
    switch (k)
    {
        case Kind::implode: return 1.0;
        case Kind::scatter: return 1.2;
        case Kind::tension: return 1.0;
        case Kind::gatherMid: case Kind::gather: return 1.5;
        default: return 2.0;
    }
}

std::vector<Phase> detectPhases(const juce::String& norm, Ctx& ctx, juce::StringArray& log)
{
    juce::String t = " " + norm + " ";
    for (auto* d : { " and at the end ", " and finally ", " at the end ", " in the end ", " finally ", " eventually ", " then ", " after that ", " afterwards ", " until ", " whereupon ", " und am schluss ", " am schluss ", " zum schluss ", " am ende ", " zum ende ", " und dann ",
                     " um dann ", " woraufhin ", " worauf ", " anschliessend ", " schliesslich ", " danach ",
                     " dann ", " zuletzt ", " and ", " bis " })
        t = t.replace(d, "|");
    for (auto* d : { ",", ";", ".", "/", " - " }) t = t.replace(d, "|");

    std::vector<Phase> out;
    for (auto& raw : juce::StringArray::fromTokens(t, "|", ""))
    {
        const auto clause = raw.trim();
        if (clause.isEmpty()) continue;
        const Kw* hit = nullptr;
        for (auto& kw : kTable)
            if (clause.contains(kw.stem)) { hit = &kw; break; }
        if (! hit) { if (clause.length() > 4) log.add("Clause without gesture skipped: \"" + clause + "\""); continue; }
        Kind k = hit->kind;
        if (k == Kind::spiralOut && anyOf(clause, { "inward", "into the", "shrink", "narrow", "tighten", "hinein", "innen", "schrumpf", "zusammen" }) &&
            ! anyOf(clause, { "outward", "expand", "widen", "grow", "open", "hinaus", "aussen", "oeffn", "wachs", "weit" }))
            k = Kind::spiralIn;
        Phase p { k, phaseName(k), baseWeight(k) };
        if (anyOf(clause, { "sudden", "abrupt", "instant", "ploetzlich", "schlagartig", "jaeh" })) p.weight *= 0.6;
        if (anyOf(clause, { "slow", "gradual", "long", "langsam", "allmaehlich", "zaeh", "lange" })) p.weight *= 1.5;
        log.add("Section: " + juce::String(hit->label) + "  <- \"" + clause + "\"");
        out.push_back(p);
    }
    juce::ignoreUnused(ctx);
    return out;
}

// Ein Eintrag je Abschnitt und Gruppe
Cell makeCell(const Phase& ph, int gi, int n, const Ctx& c, bool relationDerived)
{
    Cell cell;
    auto& m = cell.move;
    const double az0 = 360.0 * gi / n;
    const double spread = 12.0 * (gi - (n - 1) / 2.0);
    const double target = wrapAz(c.anchorAz + spread);
    const bool cw = ! (gi % 2 == 1 && ! relationDerived && c.nervous);
    m.dir = cw ? "cw" : "ccw";
    cell.ease = c.slow ? "breath" : (c.fast ? "out" : "inout");

    switch (ph.kind)
    {
        case Kind::gatherMid:
            m.type = "gerade";
            m.to = { wrapAz(lerpAz(az0, target, 0.6)), 0.65, c.clusterElev * 0.5 };
            cell.onset = juce::jmin(0.4, 0.08 * gi);
            cell.span = 1.0 - cell.onset;
            cell.ease = c.threat ? "in" : "inout";
            cell.act.stretch = c.threat ? 0.8 : 1.0;
            break;
        case Kind::gather:
            m.type = "gerade";
            m.to = { target, 0.35, c.clusterElev };
            cell.ease = c.threat ? "breath" : "inout";
            cell.act.stretch = c.threat ? 0.5 : 0.7;
            if (c.threat) { cell.act.jitter = 0.04; cell.act.jitterSpeed = 1.5; }
            break;
        case Kind::tension:
            m.type = "drift";
            m.amount = 0.12; m.speed = 2.5 + gi * 0.4; m.seed = gi + 1;
            cell.ease = "linear";
            cell.act.stretch = 0.4;
            cell.act.jitter = 0.08 + 0.02 * (gi % 3); cell.act.jitterSpeed = 3;
            break;
        case Kind::implode:
            m.type = "gerade";
            m.to = { target, 0.0, 0.0 };
            cell.ease = "in";
            cell.act.stretch = 0.2;
            cell.act.jitter = 0.12; cell.act.jitterSpeed = 4;
            break;
        case Kind::scatter:
            m.type = "gerade";
            m.to = { wrapAz(az0 + 20 * (gi % 2 ? 1 : -1)), 1.3, 30 };
            cell.ease = "in";
            cell.act.stretch = 1.8;
            break;
        case Kind::spiralIn:
            m.type = "spirale"; m.turns = 1.5; m.toDist = 0.35; cell.ease = "linear";
            cell.act.rotZ = 180;
            break;
        case Kind::spiralOut:
            m.type = "spirale"; m.turns = 1.5; m.toDist = 1.2; cell.ease = "linear";
            cell.act.rotZ = 90;
            break;
        case Kind::rise:
            m.type = "helix"; m.turns = c.fast ? 3 : 2; m.rise = 2.5; cell.act.rotZ = 180;
            break;
        case Kind::descend:
            m.type = "helix"; m.turns = 2; m.rise = -2.5; m.dir = "ccw"; cell.act.rotZ = -180;
            break;
        case Kind::circle:
            m.type = "kreis"; m.turns = c.fast ? 2 : 1; cell.ease = "linear"; cell.act.rotZ = 90;
            break;
        case Kind::pendulum:
            m.type = "pendel"; m.to = { wrapAz(az0 + 90), 0.65, 0 }; m.reps = 3;
            break;
        case Kind::rose:
            m.type = "rose"; m.petals = 3 + gi % 3; m.turns = 1; cell.ease = "linear";
            break;
        case Kind::lissajous:
            m.type = "lissajous"; m.a = 3; m.b = 2; m.size = 0.5; m.turns = 2; cell.ease = "linear";
            break;
        case Kind::hold:
            m.type = "ruhen";
            break;
        case Kind::drift:
            m.type = "drift"; m.amount = 0.3; m.speed = c.nervous ? 4 : 2.5; m.seed = gi + 1;
            cell.ease = "linear";
            if (c.nervous) { cell.act.jitter = 0.08; cell.act.jitterSpeed = 3; }
            break;
        case Kind::arc:
            m.type = "bogen"; m.to = { wrapAz(az0 + 120), 0.65, c.airy ? 30.0 : 0.0 };
            break;
    }
    if (c.nervous && (ph.kind == Kind::hold)) { cell.act.jitter = 0.06; cell.act.jitterSpeed = 3; }
    return cell;
}
} // namespace

PlannerResult planFromPrompt(const juce::String& prompt, const PlannerOptions& opt)
{
    PlannerResult res;
    auto& log = res.log;
    const auto norm = normalise(prompt);
    const int n = juce::jlimit(1, 16, opt.groups);
    const double dur = juce::jmax(4.0, opt.duration);

    Ctx c;
    c.threat = anyOf(norm, { "threat", "menac", "dark", "ominous", "sinister", "eerie", "oppress", "bedroh", "dunkel", "duester", "unheimlich", "drohend", "finster", "beklemm", "bedrueck" });
    c.airy = anyOf(" " + norm, { "airy", " light", " bright", "delicate", "weightless", "luftig", " hell", " leicht", "zart", "schwerelos" });
    c.nervous = anyOf(norm, { "hectic", "nervous", "restless", "tremb", "shiver", "jitter", "flicker", "tense", "threat", "menac", "hektisch", "nervoes", "unruhig", "zitter", "flirr", "angespannt", "bedroh" });
    c.slow = anyOf(norm, { "slow", "sluggish", "leisurely", "langsam", "zaeh", "gemaechlich", "traege" });
    c.fast = anyOf(norm, { "fast", "quick", "rapid", "hectic", "racing", "schnell", "rasch", "hektisch", "rasend" });
    if (c.slow && c.fast) c.fast = false;
    c.clusterElev = c.threat ? -15 : (c.airy ? 30 : 0);

    struct Dir { const char* stem; double az; } dirs[] = { { "behind", 180 }, { "back", 180 }, { "right", 90 }, { "left", 270 }, { "front", 0 }, { "hinten", 180 }, { "rechts", 90 }, { "links", 270 }, { "vorn ", 0 } };
    for (auto& d : dirs)
        if (norm.contains(d.stem)) { c.anchorAz = d.az; c.anchorGiven = true; break; }
    if (! c.anchorGiven && c.threat) c.anchorAz = 180;

    if (c.threat) log.add(U("Modifier: threatening -> low cluster (-15°), jitter, compressed, target behind"));
    if (c.airy) log.add(U("Modifier: airy -> raised (30°)"));
    if (c.nervous) log.add("Modifier: restless -> faster jitter/drift");
    if (c.slow) log.add("Modifier: slow -> breathing curve");
    if (c.fast) log.add(U("Modifier: fast -> shorter sections, more turns"));
    if (c.anchorGiven) log.add("Target direction from the text: " + juce::String(c.anchorAz, 0) + U("°"));

    // ---- Phasen
    auto phases = detectPhases(norm, c, log);
    if (phases.empty())
    {
        log.add("No gestures recognised; default arc: Gather, Circle, Scatter.");
        phases = { { Kind::gather, "", 2 }, { Kind::circle, "", 2 }, { Kind::scatter, "", 1.2 } };
    }
    // Sammeln = Annäherung + Verdichtung
    std::vector<Phase> expanded;
    for (size_t i = 0; i < phases.size(); ++i)
    {
        auto p = phases[i];
        if (p.kind == Kind::gather)
        {
            Phase a { Kind::gatherMid, phaseName(Kind::gatherMid), p.weight };
            Phase b { Kind::gather, phaseName(Kind::gather), p.weight };
            expanded.push_back(a); expanded.push_back(b);
        }
        else
        {
            if ((p.kind == Kind::implode || (p.kind == Kind::scatter && c.threat)) && ! expanded.empty() &&
                expanded.back().kind != Kind::tension && expanded.back().kind != Kind::drift && expanded.back().kind != Kind::hold)
                expanded.push_back({ Kind::tension, phaseName(Kind::tension), baseWeight(Kind::tension) });
            p.name = phaseName(p.kind);
            expanded.push_back(p);
        }
    }
    phases = expanded;

    // ---- Abschnittsdauern: proportional, auf 0.5 s gerundet, Summe = dur
    double wsum = 0;
    for (auto& p : phases) wsum += p.weight;
    Plan plan;
    double acc = 0;
    for (size_t i = 0; i < phases.size(); ++i)
    {
        double d = (i + 1 == phases.size()) ? dur - acc : std::round(dur * phases[i].weight / wsum * 2.0) / 2.0;
        d = juce::jmax(1.0, d);
        plan.sections.push_back({ phases[i].name, d });
        acc += d;
    }
    // Rundungsrest dem längsten Abschnitt zuschlagen, falls die Summe abweicht
    double total = 0;
    size_t longest = 0;
    for (size_t i = 0; i < plan.sections.size(); ++i)
    {
        total += plan.sections[i].dur;
        if (plan.sections[i].dur > plan.sections[longest].dur) longest = i;
    }
    plan.sections[longest].dur = juce::jmax(1.0, plan.sections[longest].dur + (dur - total));

    // ---- Beziehung
    juce::String relType = "frei";
    if (anyOf(norm, { "canon", "staggered", "offset", "kanon", "versetzt", "nachfolg" })) relType = "kanon";
    else if (anyOf(norm, { "mirror", "spiegel" })) relType = "spiegel_lr";
    else if (anyOf(norm, { "opposite", "inverse", "point reflection", "gegenueber", "umkehr", "punktspiegel" })) relType = "umkehr";
    else if (anyOf(norm, { "unison", "synchron", "together with", "in sync", "unisono", "gemeinsam", "im gleichen" })) relType = "unisono";
    if (relType != "frei") log.add("Relation: " + relType);

    // ---- Gruppen
    const Kind firstKind = phases.front().kind;
    plan.title = opt.title.isNotEmpty() ? opt.title : prompt.trim().substring(0, 60);
    plan.radius = 3;
    plan.holdRests = true;
    for (int gi = 0; gi < n; ++gi)
    {
        Group g = defaultGroup(gi);
        g.role = "Choir";
        g.start.az = wrapAz(360.0 * gi / n);
        g.start.dist = (firstKind == Kind::gatherMid || firstKind == Kind::implode) ? 1.0
                       : (firstKind == Kind::scatter || firstKind == Kind::tension) ? 0.35 : 0.65;
        g.start.elev = (firstKind == Kind::scatter || firstKind == Kind::tension) ? c.clusterElev : (c.airy ? 30 : 0);

        bool derived = false;
        if (gi > 0 && relType == "kanon") { g.relation = { "kanon", 0, 1.5 * gi }; derived = true; }
        else if (gi > 0 && relType == "unisono") { g.relation = { "unisono", 0, 0 }; derived = true; }
        else if (gi > 0 && relType == "umkehr") { g.relation = { "umkehr", 0, 0 }; derived = true; }
        else if (gi % 2 == 1 && relType == "spiegel_lr") { g.relation = { "spiegel_lr", gi - 1, 0 }; derived = true; }
        for (auto& p : phases) g.cells.push_back(makeCell(p, gi, n, c, derived));
        g.name = gi == 0 && relType == "kanon" ? "Dux" : (relType == "kanon" ? "Comes " + juce::String(gi) : "Group " + juce::String(gi + 1));
        plan.groups.push_back(std::move(g));
    }
    normalizePlan(plan);
    res.plan = std::move(plan);
    log.add(juce::String(n) + " groups, " + juce::String((int) res.plan.sections.size()) + " sections, " + juce::String(dur, 1) + " s");
    return res;
}

juce::StringArray plannerVocabulary()
{
    juce::StringArray v;
    v.add("Gestures (keywords; German stems are normalised ae/oe/ue):");
    juce::String last, line;
    for (auto& kw : kTable)
    {
        if (juce::String(kw.label) != last)
        {
            if (line.isNotEmpty()) v.add(line);
            last = kw.label;
            line = "  " + juce::String(kw.label) + ": ";
        }
        else line << ", ";
        line << kw.stem << "*";
    }
    if (line.isNotEmpty()) v.add(line);
    v.add("Clauses are split at: comma, semicolon, full stop, 'then', 'afterwards', 'and', 'until', 'at the end', ...");
    v.add("Modifiers: threatening/dark/sinister; airy/light/delicate; restless/hectic/trembling; slow/sluggish; fast/quick; sudden/abrupt (shorter section)");
    v.add("Direction: front, back, left, right (target of the cluster)");
    v.add("Relation: canon, mirror, opposite/inverse, unison/in sync");
    v.add("Templates of the model:");
    for (auto& p : presets()) v.add("  " + p.key + "  (" + p.label + ")");
    return v;
}
} // namespace choreo
