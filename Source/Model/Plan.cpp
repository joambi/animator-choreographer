#include "Plan.h"

namespace choreo
{
const juce::uint32 groupColours[8] = { 0xe8a33d, 0x5fb3d9, 0xd9675f, 0x7cc48a, 0xb58ad9, 0xd9c75f, 0x5fd9c0, 0xd98ab8 };

juce::Colour groupColour(int gi) { return juce::Colour(0xff000000u | groupColours[((gi % 8) + 8) % 8]); }

const std::vector<NumOption>& directions()
{
    static const std::vector<NumOption> v { { 0, "front" }, { 45, "front right" }, { 90, "right" }, { 135, "back right" },
                                            { 180, "back" }, { 225, "back left" }, { 270, "left" }, { 315, "front left" } };
    return v;
}
const std::vector<NumOption>& distances()
{
    static const std::vector<NumOption> v { { 0, "center" }, { 0.35, "near" }, { 0.65, "medium" }, { 1, "far" }, { 1.3, "beyond the circle" } };
    return v;
}
const std::vector<NumOption>& heights()
{
    static const std::vector<NumOption> v { { -20, U("below ear level") }, { 0, U("ear level") }, { 30, U("raised") }, { 60, "high" }, { 90, "zenith" } };
    return v;
}
const juce::StringArray& roles()
{
    static const juce::StringArray r { "Soloist", "Counter-voice", "Accompaniment", "Choir", "Drone", "Echo" };
    return r;
}

const std::vector<EaseDef>& eases()
{
    static const std::vector<EaseDef> v {
        { "linear", U("even"), false, 0, 0, 0, 0 },
        { "in", "gentle start", true, 0.42, 0, 1, 1 },
        { "out", "gentle end", true, 0, 0, 0.58, 1 },
        { "inout", "smooth at both ends", true, 0.42, 0, 0.58, 1 },
        { "breath", U("breathing (strongly delayed)"), true, 0.7, 0, 0.3, 1 } };
    return v;
}
const EaseDef& easeFor(const juce::String& key)
{
    for (auto& e : eases())
        if (e.key == key) return e;
    return eases().front();
}

const std::vector<GestureDef>& gestures()
{
    static const std::vector<GestureDef> v {
        { "ruhen", "Rest", "The group stays in place." },
        { "gerade", "Move straight", U("Direct path to a target; may cross the space.") },
        { "bogen", "Move in an arc", U("Path around the listening position (shorter angle); distance and height glide along.") },
        { "kreis", "Circle", U("Orbit around the listening position at constant distance.") },
        { "spirale", "Spiral", "Orbit with growing or shrinking distance." },
        { "helix", "Rising helix", "Orbit that rises or falls while turning." },
        { "pendel", "Pendulum", U("To a target and back, repeatedly.") },
        { "rose", "Rose", U("Petal-shaped loops through the center.") },
        { "lissajous", "Lissajous figure", "Interlaced figure from two frequencies; with automatic run-up." },
        { "drift", "Random drift", "Calm, reproducible wandering around the current position." },
        { "stationen", U("Path via stations"), U("Path through several directions, angular or rounded.") } };
    return v;
}
const GestureDef* gestureFor(const juce::String& key)
{
    for (auto& g : gestures())
        if (g.key == key) return &g;
    return nullptr;
}

const std::vector<RelationDef>& relations()
{
    static const std::vector<RelationDef> v {
        { "frei", U("independent"), 1, 1, false },
        { "unisono", "in unison with", 1, 1, false },
        { "kanon", "in canon after", 1, 1, false },
        { "spiegel_lr", U("als Spiegelbild (links \xe2\x86\x94 rechts) von"), -1, 1, true },
        { "spiegel_vh", U("als Spiegelbild (vorne \xe2\x86\x94 hinten) von"), 1, -1, true },
        { "umkehr", U("opposite (point reflection) of"), -1, -1, false } };
    return v;
}
const RelationDef& relationFor(const juce::String& key)
{
    for (auto& r : relations())
        if (r.key == key) return r;
    return relations().front();
}

// ------------------------------------------------------------------ Hilfen
Cell defaultCell() { return Cell {}; }

Group defaultGroup(int i)
{
    static const double az[8] = { 315, 45, 225, 135, 0, 90, 180, 270 };
    Group g;
    g.name = "Group " + juce::String(i + 1);
    g.role = roles()[juce::jmin(i, roles().size() - 1)];
    g.start = { az[i % 8], 0.65, 0 };
    return g;
}

void normalizePlan(Plan& p)
{
    if (p.sections.empty()) p.sections.push_back({ "A", 10 });
    for (auto& g : p.groups)
        g.cells.resize(p.sections.size(), defaultCell());
}

std::vector<double> sectionStarts(const Plan& p)
{
    std::vector<double> r;
    double t = 0;
    for (auto& s : p.sections) { r.push_back(t); t += s.dur; }
    return r;
}

double totalDuration(const Plan& p)
{
    double t = 0;
    for (auto& s : p.sections) t += std::isfinite(s.dur) ? s.dur : 0.0;
    return t;
}

V3 specToXYZ(const PosSpec& s, double R)
{
    const double d = s.dist * R, el = s.elev * juce::MathConstants<double>::pi / 180.0, az = s.az * juce::MathConstants<double>::pi / 180.0;
    const double h = d * std::cos(el);
    return { round6(h * std::sin(az)), round6(h * std::cos(az)), round6(d * std::sin(el)) };
}

double nearestDirection(double az)
{
    const double a = std::fmod(std::fmod(az, 360.0) + 360.0, 360.0);
    double best = 0, bd = 1e9;
    for (auto& d : directions())
        if (std::abs(d.v - a) < bd) { bd = std::abs(d.v - a); best = d.v; }
    return best;
}

juce::String slug(const juce::String& s)
{
    auto t = s.toLowerCase();
    t = t.replace(U("ä"), "ae").replace(U("ö"), "oe").replace(U("ü"), "ue").replace(U("ß"), "ss")
         .replace(U("é"), "e").replace(U("è"), "e").replace(U("à"), "a");
    juce::String out;
    bool dash = false;
    for (auto c : t)
    {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) { out << juce::String::charToString(c); dash = false; }
        else if (! dash && out.isNotEmpty()) { out << "-"; dash = true; }
    }
    while (out.endsWithChar('-')) out = out.dropLastCharacters(1);
    return out.isEmpty() ? juce::String("szene") : out;
}

// ------------------------------------------------------------------ Vorlagen
const std::vector<PresetDef>& presets()
{
    static const std::vector<PresetDef> v { { "kanon", "Canon in a circle" }, { "duett", "Mirror duet" },
                                            { "zerstreuung", "Gather and scatter" }, { "aufstieg", "Ascent" },
                                            { "explosion", "Explosion" },
                                            { "reset", "Reset width (Encoder)" },
                                            { "leer", "Empty plan" } };
    return v;
}

static Plan basePlan(const juce::String& title, std::vector<Section> sections, int n)
{
    Plan p;
    p.title = title;
    p.sections = std::move(sections);
    for (int i = 0; i < n; ++i) p.groups.push_back(defaultGroup(i));
    normalizePlan(p);
    return p;
}

static Cell cell(const juce::String& type, std::function<void(Cell&)> f = {})
{
    Cell c;
    c.move.type = type;
    if (f) f(c);
    return c;
}

Plan buildPreset(const juce::String& key)
{
    if (key == "reset")
    {
        // Sets the group Stretch of all 8 groups back to Plan::baseStretch (default 1); no movement, no hold clips.
        auto p = basePlan("Reset width", { { "Reset", 2 } }, 8);
        p.holdRests = false;
        for (auto& g : p.groups)
        {
            g.cells[0].move.type = "ruhen";
            g.cells[0].act.stretch = p.baseStretch;
        }
        return p;
    }
    if (key == "kanon")
    {
        auto p = basePlan("Canon in a circle", { { "Exposition", 8 }, { U("Development"), 12 }, { "Coda", 6 } }, 4);
        p.groups[0].start = { 0, 1, 0 };
        p.groups[0].cells = {
            cell("kreis", [] (Cell& c) { c.move.turns = 0.5; c.move.dir = "cw"; c.ease = "inout"; }),
            cell("spirale", [] (Cell& c) { c.move.turns = 1.5; c.move.dir = "cw"; c.move.toDist = 0.35; c.ease = "linear"; c.act.rotZ = 180; }),
            cell("bogen", [] (Cell& c) { c.move.to = { 0, 1, 30 }; c.ease = "out"; c.act.stretch = 1.6; }) };
        for (int i : { 1, 2, 3 }) p.groups[(size_t) i].relation = { "kanon", 0, i * 1.5 };
        p.groups[1].name = "Comes 1"; p.groups[2].name = "Comes 2"; p.groups[3].name = "Comes 3"; p.groups[0].name = "Dux";
        return p;
    }
    if (key == "duett")
    {
        auto p = basePlan("Mirror duet", { { U("Approach"), 6 }, { "Intertwine", 10 }, { "Separation", 6 } }, 2);
        p.groups[0].start = { 270, 1, 0 };
        p.groups[0].cells = {
            cell("gerade", [] (Cell& c) { c.move.to = { 315, 0.35, 0 }; c.ease = "out"; }),
            cell("lissajous", [] (Cell& c) { c.move.a = 1; c.move.b = 2; c.move.size = 0.5; c.move.turns = 2; c.ease = "linear"; c.act.rotZ = 90; }),
            cell("bogen", [] (Cell& c) { c.move.to = { 225, 1, 30 }; c.ease = "in"; }) };
        p.groups[1].relation = { "spiegel_lr", 0, 0 };
        p.groups[0].name = "Left"; p.groups[1].name = "Right"; p.groups[1].role = "Counter-voice";
        return p;
    }
    if (key == "zerstreuung")
    {
        auto p = basePlan("Gather and scatter", { { "Gather", 8 }, { "Float", 10 }, { "Scatter", 8 } }, 4);
        const double az[4] = { 0, 90, 180, 270 };
        for (size_t i = 0; i < p.groups.size(); ++i)
        {
            auto& g = p.groups[i];
            const double a = az[i];
            const double fi = (double) i;
            g.start = { a, 1, 0 };
            g.role = "Choir";
            g.cells = {
                cell("gerade", [a, fi] (Cell& c) { c.move.to = { a, 0.35, 0 }; c.onset = fi * 0.1; c.span = 1 - fi * 0.1; c.ease = "inout"; c.act.stretch = 0.5; }),
                cell("drift", [fi] (Cell& c) { c.move.amount = 0.3; c.move.speed = 2.5; c.move.seed = fi + 1; c.ease = "linear"; c.act.jitter = 0.05; c.act.jitterSpeed = 1.5; }),
                cell("bogen", [a] (Cell& c) { c.move.to = { a + 135, 1, 30 }; c.ease = "in"; c.act.stretch = 1.5; }) };
        }
        return p;
    }
    if (key == "aufstieg")
    {
        auto p = basePlan("Ascent", { { "Ground", 6 }, { "Ascent", 14 }, { "zenith", 6 } }, 2);
        p.groups[0].start = { 180, 0.65, -20 };
        p.groups[0].cells = {
            cell("ruhen", [] (Cell& c) { c.act.stretch = 0.6; }),
            cell("helix", [] (Cell& c) { c.move.turns = 2; c.move.dir = "ccw"; c.move.rise = 3; c.ease = "inout"; c.act.rotZ = -360; c.act.stretch = 1.2; }),
            cell("gerade", [] (Cell& c) { c.move.to = { 0, 0.65, 90 }; c.ease = "out"; }) };
        p.groups[1].relation = { "umkehr", 0, 3 };
        p.groups[0].name = "Rising"; p.groups[1].name = U("Opposite");
        return p;
    }
    if (key == "explosion")
    {
        // Vier Gruppen ziehen sich unter Spannung im Zentrum zusammen, werden in vier Richtungen und
        // Höhen über den Kreis hinausgeschleudert und sinken danach im Bogen auf Ohrhöhe zurück.
        auto p = basePlan("Explosion", { { "Condensation", 5 }, { "Detonation", 2 }, { "Aftermath", 9 } }, 4);
        const double az[4] = { 45, 135, 225, 315 };
        const double up[4] = { 30, 60, -20, 30 };
        const double spin[4] = { 360, -360, 360, -360 };
        const char* names[4] = { "Shard 1", "Shard 2", "Shard 3", "Shard 4" };
        for (size_t i = 0; i < 4; ++i)
        {
            auto& g = p.groups[i];
            const double a = az[i], e = up[i], r = spin[i], fi = (double) i;
            g.name = names[i];
            g.role = "Choir";
            g.start = { a, 0.65, 0 };
            g.cells = {
                cell("gerade", [a] (Cell& c) { c.move.to = { a, 0, 0 }; c.ease = "in"; c.act.stretch = 0.2; c.act.jitter = 0.08; c.act.jitterSpeed = 4; }),
                cell("gerade", [a, e, r] (Cell& c) { c.move.to = { a, 1.3, e }; c.ease = "out"; c.act.stretch = 2; c.act.rotZ = r; }),
                cell("bogen", [a, r, fi] (Cell& c) { c.move.to = { a + (r > 0 ? 30.0 : -30.0), 1, 0 }; c.onset = fi * 0.05; c.span = 1 - fi * 0.05;
                                                     c.ease = "inout"; c.act.stretch = 1.4; c.act.jitter = 0.02; c.act.jitterSpeed = 0.5; }) };
        }
        return p;
    }
    return basePlan("New choreography", { { "A", 10 }, { "B", 10 } }, 2);
}

// ------------------------------------------------------------------ JSON
static juce::var posToVar(const PosSpec& s)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("az", s.az); o->setProperty("dist", s.dist); o->setProperty("elev", s.elev);
    return juce::var(o);
}
static juce::var optToVar(const std::optional<double>& v) { return v ? juce::var(*v) : juce::var(juce::String()); }

juce::String planToJson(const Plan& p)
{
    auto* plan = new juce::DynamicObject();
    plan->setProperty("title", p.title);
    plan->setProperty("radius", p.radius);
    plan->setProperty("distanceScaler", optToVar(p.distanceScaler));
    plan->setProperty("baseStretch", p.baseStretch);
    plan->setProperty("encoderName", p.encoderName);
    plan->setProperty("maxDistance", optToVar(p.maxDistance));
    plan->setProperty("holdRests", p.holdRests);
    juce::Array<juce::var> secs;
    for (auto& s : p.sections)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("name", s.name); o->setProperty("dur", s.dur);
        secs.add(juce::var(o));
    }
    plan->setProperty("sections", secs);
    juce::Array<juce::var> groups;
    for (auto& g : p.groups)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("name", g.name);
        o->setProperty("role", g.role);
        if (g.baseStretch) o->setProperty("baseStretch", *g.baseStretch);
        o->setProperty("start", posToVar(g.start));
        auto* r = new juce::DynamicObject();
        r->setProperty("type", g.relation.type); r->setProperty("leader", g.relation.leader); r->setProperty("delay", g.relation.delay);
        o->setProperty("relation", juce::var(r));
        juce::Array<juce::var> cells;
        for (auto& c : g.cells)
        {
            auto* co = new juce::DynamicObject();
            auto* m = new juce::DynamicObject();
            const auto& mv = c.move;
            m->setProperty("type", mv.type); m->setProperty("to", posToVar(mv.to)); m->setProperty("turns", mv.turns);
            m->setProperty("dir", mv.dir); m->setProperty("toDist", mv.toDist); m->setProperty("rise", mv.rise);
            m->setProperty("reps", mv.reps); m->setProperty("petals", mv.petals); m->setProperty("a", mv.a); m->setProperty("b", mv.b);
            m->setProperty("size", mv.size); m->setProperty("amount", mv.amount); m->setProperty("speed", mv.speed); m->setProperty("seed", mv.seed);
            juce::Array<juce::var> st;
            for (auto& s : mv.stations) st.add(posToVar(s));
            m->setProperty("stations", st);
            m->setProperty("smooth", mv.smooth); m->setProperty("closed", mv.closed);
            co->setProperty("move", juce::var(m));
            co->setProperty("onset", c.onset); co->setProperty("span", c.span); co->setProperty("ease", c.ease);
            auto* a = new juce::DynamicObject();
            a->setProperty("rotZ", c.act.rotZ); a->setProperty("tiltX", optToVar(c.act.tiltX)); a->setProperty("tiltY", optToVar(c.act.tiltY));
            a->setProperty("stretch", optToVar(c.act.stretch)); a->setProperty("jitter", c.act.jitter); a->setProperty("jitterSpeed", c.act.jitterSpeed);
            co->setProperty("act", juce::var(a));
            cells.add(juce::var(co));
        }
        o->setProperty("cells", cells);
        groups.add(juce::var(o));
    }
    plan->setProperty("groups", groups);
    auto* root = new juce::DynamicObject();
    root->setProperty("format", "raumchoreographie");
    root->setProperty("version", 1);
    root->setProperty("plan", juce::var(plan));
    return juce::JSON::toString(juce::var(root), false);
}

static double num(const juce::var& o, const char* k, double def)
{
    if (! o.hasProperty(k)) return def;
    const auto& v = o[k];
    if (v.isString()) { auto s = v.toString().trim(); return s.isEmpty() ? 0.0 : s.getDoubleValue(); }
    if (v.isBool()) return (bool) v ? 1.0 : 0.0;
    const double d = (double) v;
    return std::isfinite(d) ? d : def;
}
static std::optional<double> optNum(const juce::var& o, const char* k, std::optional<double> def)
{
    if (! o.hasProperty(k)) return def;
    const auto& v = o[k];
    if (v.isVoid() || (v.isString() && v.toString().trim().isEmpty())) return std::nullopt;
    const double d = v.isString() ? v.toString().getDoubleValue() : (double) v;
    if (! std::isfinite(d)) return std::nullopt;
    return d;
}
static juce::String str(const juce::var& o, const char* k, const juce::String& def)
{
    return o.hasProperty(k) ? o[k].toString() : def;
}
static bool flag(const juce::var& o, const char* k, bool def)
{
    return o.hasProperty(k) ? (bool) o[k] : def;
}
static PosSpec pos(const juce::var& o, PosSpec def)
{
    if (! o.isObject()) return def;
    return { num(o, "az", def.az), num(o, "dist", def.dist), num(o, "elev", def.elev) };
}

std::optional<Plan> planFromJson(const juce::String& text, juce::String& error)
{
    juce::var root;
    auto res = juce::JSON::parse(text, root);
    if (res.failed()) { error = U("The file is not valid JSON."); return std::nullopt; }
    juce::var pv = root.hasProperty("plan") ? root["plan"] : root;
    if (! pv.isObject() || ! pv["groups"].isArray() || ! pv["sections"].isArray())
    {
        error = U("The file is not a valid plan.");
        return std::nullopt;
    }
    Plan p;
    p.title = str(pv, "title", p.title);
    p.radius = num(pv, "radius", 3);
    p.distanceScaler = optNum(pv, "distanceScaler", std::nullopt);
    p.encoderName = str(pv, "encoderName", "");
    p.maxDistance = optNum(pv, "maxDistance", std::nullopt);
    if (p.maxDistance && ! (*p.maxDistance > 0)) p.maxDistance.reset();
    p.baseStretch = juce::jlimit(0.02, 4.0, optNum(pv, "baseStretch", std::nullopt).value_or(1.0));
    p.holdRests = ! pv.hasProperty("holdRests") || ! (pv["holdRests"].isBool() && ! (bool) pv["holdRests"]);
    for (auto& s : *pv["sections"].getArray())
        p.sections.push_back({ str(s, "name", "A"), num(s, "dur", 10) });
    for (auto& gv : *pv["groups"].getArray())
    {
        Group g = defaultGroup((int) p.groups.size());
        g.name = str(gv, "name", g.name);
        g.role = str(gv, "role", g.role);
        g.baseStretch = optNum(gv, "baseStretch", std::nullopt);
        if (g.baseStretch) g.baseStretch = juce::jlimit(0.02, 4.0, *g.baseStretch);
        g.start = pos(gv["start"], g.start);
        if (gv["relation"].isObject())
        {
            auto r = gv["relation"];
            g.relation.type = str(r, "type", "frei");
            g.relation.leader = (int) num(r, "leader", 0);
            g.relation.delay = num(r, "delay", 2);
        }
        if (gv["cells"].isArray())
        {
            for (auto& cv : *gv["cells"].getArray())
            {
                Cell c = defaultCell();
                auto m = cv["move"];
                if (m.isObject())
                {
                    auto& mv = c.move;
                    mv.type = str(m, "type", mv.type);
                    mv.to = pos(m["to"], mv.to);
                    mv.turns = num(m, "turns", mv.turns);
                    mv.dir = str(m, "dir", mv.dir);
                    mv.toDist = num(m, "toDist", mv.toDist);
                    mv.rise = num(m, "rise", mv.rise);
                    mv.reps = num(m, "reps", mv.reps);
                    mv.petals = num(m, "petals", mv.petals);
                    mv.a = num(m, "a", mv.a);
                    mv.b = num(m, "b", mv.b);
                    mv.size = num(m, "size", mv.size);
                    mv.amount = num(m, "amount", mv.amount);
                    mv.speed = num(m, "speed", mv.speed);
                    mv.seed = num(m, "seed", mv.seed);
                    if (m["stations"].isArray())
                    {
                        mv.stations.clear();
                        for (auto& s : *m["stations"].getArray()) mv.stations.push_back(pos(s, { 0, 0.65, 0 }));
                    }
                    mv.smooth = flag(m, "smooth", mv.smooth);
                    mv.closed = flag(m, "closed", mv.closed);
                }
                c.onset = num(cv, "onset", c.onset);
                c.span = num(cv, "span", c.span);
                c.ease = str(cv, "ease", c.ease);
                auto a = cv["act"];
                if (a.isObject())
                {
                    c.act.rotZ = num(a, "rotZ", 0);
                    c.act.tiltX = optNum(a, "tiltX", std::nullopt);
                    c.act.tiltY = optNum(a, "tiltY", std::nullopt);
                    c.act.stretch = optNum(a, "stretch", std::nullopt);
                    c.act.jitter = num(a, "jitter", 0);
                    c.act.jitterSpeed = num(a, "jitterSpeed", 1);
                }
                g.cells.push_back(c);
            }
        }
        p.groups.push_back(g);
    }
    if (p.groups.empty()) { error = U("The plan contains no group."); return std::nullopt; }
    normalizePlan(p);
    return p;
}
} // namespace choreo
