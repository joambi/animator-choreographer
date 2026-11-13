#include "PlanAI.h"

namespace choreo::ai
{
// ------------------------------------------------------------------ Wortlisten
namespace
{
struct Word
{
    const char* word;
    const char* key;
};

const Word gestureWords[] = { { "rest", "ruhen" }, { "straight", "gerade" }, { "arc", "bogen" }, { "circle", "kreis" },
                              { "spiral", "spirale" }, { "helix", "helix" }, { "pendulum", "pendel" }, { "rose", "rose" },
                              { "lissajous", "lissajous" }, { "drift", "drift" }, { "stations", "stationen" } };

const Word easeWords[] = { { "even", "linear" }, { "accelerating", "in" }, { "decelerating", "out" },
                           { "smooth", "inout" }, { "breathing", "breath" } };

const Word dirWords[] = { { "clockwise", "cw" }, { "counter-clockwise", "ccw" } };

const Word relationWords[] = { { "independent", "frei" }, { "unison", "unisono" }, { "canon", "kanon" },
                               { "mirror left-right", "spiegel_lr" }, { "mirror front-back", "spiegel_vh" },
                               { "opposite", "umkehr" } };

template <size_t N>
juce::StringArray words(const Word (&list)[N])
{
    juce::StringArray a;
    for (auto& w : list) a.add(U(w.word));
    return a;
}

template <size_t N>
juce::String keyFor(const Word (&list)[N], const juce::String& word, const juce::String& def)
{
    const auto w = word.trim();
    for (auto& e : list)
        if (w.equalsIgnoreCase(U(e.word)) || w.equalsIgnoreCase(e.key)) return e.key;
    return def;
}

template <size_t N>
juce::String wordFor(const Word (&list)[N], const juce::String& key)
{
    for (auto& e : list)
        if (key == e.key) return U(e.word);
    return U(list[0].word);
}

juce::StringArray labels(const std::vector<NumOption>& opts)
{
    juce::StringArray a;
    for (auto& o : opts) a.add(o.label);
    return a;
}

double valueFor(const std::vector<NumOption>& opts, const juce::String& label, double def)
{
    const auto l = label.trim();
    for (auto& o : opts)
        if (l.equalsIgnoreCase(o.label)) return o.v;
    return def;
}

juce::String labelNear(const std::vector<NumOption>& opts, double v)
{
    const NumOption* best = &opts.front();
    for (auto& o : opts)
        if (std::abs(o.v - v) < std::abs(best->v - v)) best = &o;
    return best->label;
}

juce::String directionLabel(double az)
{
    const double a = nearestDirection(az);
    for (auto& d : directions())
        if (std::abs(d.v - a) < 1e-9) return d.label;
    return directions().front().label;
}

// ------------------------------------------------------------------ Schema-Bausteine
juce::var obj(std::initializer_list<std::pair<const char*, juce::var>> props, juce::StringArray required)
{
    auto* p = new juce::DynamicObject();
    for (auto& kv : props) p->setProperty(kv.first, kv.second);
    auto* o = new juce::DynamicObject();
    o->setProperty("type", "object");
    o->setProperty("properties", juce::var(p));
    juce::Array<juce::var> req;
    for (auto& r : required) req.add(r);
    o->setProperty("required", req);
    return juce::var(o);
}
juce::var typed(const char* type)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("type", type);
    return juce::var(o);
}
juce::var enumOf(const juce::StringArray& values)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("type", "string");
    juce::Array<juce::var> e;
    for (auto& v : values) e.add(v);
    o->setProperty("enum", e);
    return juce::var(o);
}
juce::var arrayOf(const juce::var& items)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("type", "array");
    o->setProperty("items", items);
    return juce::var(o);
}
juce::var posSchema()
{
    return obj({ { "direction", enumOf(labels(directions())) }, { "distance", enumOf(labels(distances())) },
                 { "height", enumOf(labels(heights())) } },
               { "direction", "distance", "height" });
}

// ------------------------------------------------------------------ Lesen der Modellantwort
juce::var prop(const juce::var& o, const char* k) { return o.isObject() ? o.getProperty(k, juce::var()) : juce::var(); }
bool has(const juce::var& o, const char* k) { return o.isObject() && o.hasProperty(k) && ! o[k].isVoid(); }

std::optional<double> numOf(const juce::var& v)
{
    if (v.isInt() || v.isInt64() || v.isDouble())
    {
        const double d = (double) v;
        if (std::isfinite(d)) return d;
        return std::nullopt;
    }
    if (v.isString())
    {
        auto s = v.toString().trim().replaceCharacter(',', '.');
        if (s.containsAnyOf("0123456789")) return s.getDoubleValue();
    }
    return std::nullopt;
}
double num(const juce::var& o, const char* k, double def, double lo, double hi)
{
    auto v = numOf(prop(o, k));
    return juce::jlimit(lo, hi, v ? *v : def);
}

PosSpec posOf(const juce::var& o, const PosSpec& def)
{
    if (! o.isObject()) return def;
    PosSpec p = def;
    if (has(o, "direction")) p.az = valueFor(directions(), o["direction"].toString(), def.az);
    if (has(o, "distance")) p.dist = valueFor(distances(), o["distance"].toString(), def.dist);
    if (has(o, "height")) p.elev = valueFor(heights(), o["height"].toString(), def.elev);
    return p;
}

juce::var posVar(const PosSpec& p)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("direction", directionLabel(p.az));
    o->setProperty("distance", labelNear(distances(), p.dist));
    o->setProperty("height", labelNear(heights(), p.elev));
    return juce::var(o);
}

juce::String shortName(const juce::String& s, const juce::String& def, int maxLen)
{
    auto t = s.trim();
    // »Phase 1: Name« → »Name«
    if (t.containsChar(':') && t.upToFirstOccurrenceOf(":", false, false).containsAnyOf("0123456789"))
        t = t.fromFirstOccurrenceOf(":", false, false).trim();
    if (t.isEmpty()) return def;
    return t.length() > maxLen ? t.substring(0, maxLen).trimEnd() : t;
}
} // namespace

// ------------------------------------------------------------------ Schema
juce::var schema()
{
    auto cell = obj({ { "gesture", enumOf(words(gestureWords)) },
                      { "target", posSchema() },
                      { "turns", typed("number") },
                      { "spin", enumOf(words(dirWords)) },
                      { "end_distance", enumOf(labels(distances())) },
                      { "rise", typed("number") },
                      { "repetitions", typed("number") },
                      { "petal_factor", typed("number") },
                      { "size", typed("number") },
                      { "extent", typed("number") },
                      { "restlessness", typed("number") },
                      { "stations", arrayOf(posSchema()) },
                      { "onset", typed("number") },
                      { "share", typed("number") },
                      { "curve", enumOf(words(easeWords)) },
                      { "rotation", typed("number") },
                      { "width", typed("number") },
                      { "jitter", typed("number") },
                      { "jitter_speed", typed("number") } },
                    { "gesture", "onset", "share", "curve", "width", "jitter", "jitter_speed", "rotation" });

    auto relation = obj({ { "kind", enumOf(words(relationWords)) }, { "leader", typed("integer") }, { "offset", typed("number") } },
                        { "kind", "leader", "offset" });

    auto group = obj({ { "name", typed("string") },
                       { "role", enumOf(roles()) },
                       { "start", posSchema() },
                       { "relation", relation },
                       { "cells", arrayOf(cell) } },
                     { "name", "role", "start", "relation", "cells" });

    auto section = obj({ { "name", typed("string") }, { "duration", typed("number") } }, { "name", "duration" });

    return obj({ { "title", typed("string") },
                 { "concept", typed("string") },
                 { "sections", arrayOf(section) },
                 { "groups", arrayOf(group) } },
               { "title", "concept", "sections", "groups" });
}

// ------------------------------------------------------------------ Anweisung
juce::String systemPrompt()
{
    juce::String s = U(R"(You design spatial choreographies for the Animator of the ICST AmbiEncoder (Ambisonics). Groups of sound sources move around a listening position at the center. You write a plan as JSON following the given schema. Software generates the movements from it; you calculate nothing.

SPACE (from the listener's point of view)
- direction: front, front right, right, back right, back, back left, left, front left.
- distance: center (at the listener, closest), near, medium, far (edge of the circle), beyond the circle (far outside).
- height: below ear level, ear level, raised, high, zenith (directly overhead).

FORM
- sections: 2 to 6 sections with a short name (one or two words, no number) and duration in seconds (usually 3 to 30).
- groups: every independent group has a start and exactly one cell per section, in the same order. Each gesture begins where the previous one ends.
- relation.kind: "independent" = own cells (leader 0, offset 0). Otherwise the group takes over the movement of its leader (number from 1, must be independent): "unison" (identical), "canon" (identical, offset seconds later), "mirror left-right", "mirror front-back", "opposite" (point reflection). Derived groups have an empty cells list.

GESTURES (gesture) and their fields
- rest: stays in place.
- straight: direct path to the target, also across the space or through the center.
- arc: path around the listener to the target.
- circle: turns around the listener at constant distance, spin.
- spiral: turns with spin; the distance moves to end_distance (nearer = circling in, farther = spreading out).
- helix: turns with spin, rises or falls by rise (fraction of the radius, e.g. 0.5 upward, -0.5 downward).
- pendulum: to the target and back, repetitions.
- rose: petal loops through the center, petal_factor (e.g. 3), turns, spin.
- lissajous: interlaced figure around the listener, size (fraction of the radius, 0.2 to 1), turns.
- drift: calm wandering around the current position, extent (0.1 to 0.6), restlessness (0.5 calm to 6 nervous).
- stations: path through 2 to 6 stations.
Fields that do not belong to the gesture are left out.

TIME WITHIN THE SECTION
- onset: fraction of the section at which the gesture begins (0 to 0.9). Staggered onsets let groups enter one after another.
- share: fraction of the section the gesture lasts (onset + share at most 1).
- curve: "even"; "accelerating" (slow start, fast end: suction, fall, implosion); "decelerating" (fast start, soft end: throw, explosion); "smooth" (at both ends); "breathing" (strongly delayed).

SHAPE OF THE GROUP (every cell)
- width: extent of the group at the end of the cell. 1 = original, 0.05 = contracted to a point, 2 = twice as wide.
- jitter: 0 calm, 0.03 light, 0.1 strong, 0.2 violent. jitter_speed: 0.5 slow to 8 frantic.
- rotation: the group's own rotation in degrees during the cell (0 none, 360 one full turn, negative = opposite direction).

SOUND IMAGES
- Approach, threat, encircling: spiral with end_distance near, low height, increasing jitter, opposing spins.
- Implosion, collapse: straight with target distance "center", curve "accelerating", width 0.05.
- Explosion, scattering: from the center straight to "beyond the circle" in different directions, curve "decelerating", large width.
- Floating, standstill: rest or drift with small extent.
- Ascent: helix with positive rise or straight to the zenith.
- Polyphony: groups in different directions, opposing spins, staggered onsets.

concept: two to three sentences in English on how the plan realizes the description spatially.
Answer only with the JSON object.

EXAMPLE for "Four shards condense in the center and explode into the space":
)");
    auto example = buildPreset("explosion");
    s << juce::JSON::toString(toAi(example, U("Four groups contract in the center with growing jitter, are hurled past the circle in four directions and heights, and then sink back in an arc to ear level.")), true);
    return s;
}

juce::String userPrompt(const juce::String& description, const Hints& h)
{
    juce::String s;
    s << U("Choreography description:\n") << description.trim() << "\n\n";
    if (h.groups > 0) s << U("Number of groups: exactly ") << h.groups << ".\n";
    else s << U("Number of groups: choose what fits (usually 2 to 6).\n");
    if (h.duration > 0) s << U("Total duration: about ") << juce::String(h.duration, 0) << U(" seconds (sum of all sections).\n");
    else s << U("Total duration: choose what fits (usually 20 to 60 seconds).\n");
    s << U("Output the plan as JSON.");
    return s;
}

// ------------------------------------------------------------------ Modell → Plan
std::optional<Plan> fromAi(const juce::var& j, juce::String& concept, juce::String& error)
{
    if (! j.isObject())
    {
        error = U("The answer is not a JSON object.");
        return std::nullopt;
    }
    Plan p;
    p.radius = 3;
    p.holdRests = true;
    p.title = shortName(prop(j, "title").toString(), U("Draft"), 60);
    concept = prop(j, "concept").toString().trim();

    if (auto* secs = prop(j, "sections").getArray())
        for (int i = 0; i < secs->size() && i < 8; ++i)
        {
            const auto& s = secs->getReference(i);
            const juce::String def = juce::String::charToString((juce::juce_wchar) ('A' + i));
            p.sections.push_back({ shortName(prop(s, "name").toString(), def, 32), num(s, "duration", 10, 1, 120) });
        }
    if (p.sections.empty())
    {
        error = U("The plan contains no sections.");
        return std::nullopt;
    }

    auto* groups = prop(j, "groups").getArray();
    if (groups == nullptr || groups->isEmpty())
    {
        error = U("The plan contains no groups.");
        return std::nullopt;
    }
    const int n = juce::jmin(8, groups->size());
    for (int gi = 0; gi < n; ++gi)
    {
        const auto& gv = groups->getReference(gi);
        Group g = defaultGroup(gi);
        g.name = shortName(prop(gv, "name").toString(), g.name, 32);
        const auto role = prop(gv, "role").toString().trim();
        if (roles().contains(role)) g.role = role;
        g.start = posOf(prop(gv, "start"), g.start);

        const auto rel = prop(gv, "relation");
        g.relation.type = keyFor(relationWords, prop(rel, "kind").toString(), "frei");
        g.relation.leader = (int) num(rel, "leader", 1, 0, 99) - 1;
        g.relation.delay = num(rel, "offset", 0, 0, 60);
        if (g.relation.type != "frei" && (g.relation.leader < 0 || g.relation.leader >= n || g.relation.leader == gi))
        {
            g.relation.type = "frei";
            g.relation.leader = 0;
        }
        if (g.relation.type == "frei") g.relation.leader = 0;

        auto* cells = prop(gv, "cells").getArray();
        for (size_t si = 0; si < p.sections.size(); ++si)
        {
            Cell c = defaultCell();
            if (cells != nullptr && (int) si < cells->size())
            {
                const auto& cv = cells->getReference((int) si);
                auto& m = c.move;
                m.type = keyFor(gestureWords, prop(cv, "gesture").toString(), "ruhen");
                m.to = posOf(prop(cv, "target"), m.to);
                m.turns = num(cv, "turns", 1, 0.05, 16);
                m.dir = keyFor(dirWords, prop(cv, "spin").toString(), "cw");
                if (has(cv, "end_distance")) m.toDist = valueFor(distances(), cv["end_distance"].toString(), m.toDist);
                m.rise = num(cv, "rise", 0.5, -2, 2) * p.radius;
                m.reps = std::round(num(cv, "repetitions", 2, 1, 16));
                m.petals = std::round(num(cv, "petal_factor", 3, 1, 9));
                m.size = num(cv, "size", 0.6, 0.1, 1.3);
                m.amount = num(cv, "extent", 0.4, 0.05, 1);
                m.speed = num(cv, "restlessness", 3, 0.1, 10);
                m.seed = gi * 10 + (int) si + 1;
                if (auto* st = prop(cv, "stations").getArray(); st != nullptr && ! st->isEmpty())
                {
                    m.stations.clear();
                    for (int k = 0; k < st->size() && k < 8; ++k) m.stations.push_back(posOf(st->getReference(k), { 0, 0.65, 0 }));
                }
                c.onset = num(cv, "onset", 0, 0, 0.9);
                c.span = num(cv, "share", 1, 0.05, 1);
                if (c.onset + c.span > 1) c.span = 1 - c.onset;
                c.ease = keyFor(easeWords, prop(cv, "curve").toString(), "inout");
                c.act.rotZ = num(cv, "rotation", 0, -1440, 1440);
                if (auto w = numOf(prop(cv, "width"))) c.act.stretch = juce::jlimit(0.02, 4.0, *w);
                c.act.jitter = num(cv, "jitter", 0, 0, 0.3);
                c.act.jitterSpeed = num(cv, "jitter_speed", 1, 0.1, 10);
            }
            g.cells.push_back(c);
        }
        p.groups.push_back(std::move(g));
    }
    normalizePlan(p);
    return p;
}

// ------------------------------------------------------------------ Plan → Modellformat (Beispiel)
juce::var toAi(const Plan& p, const juce::String& concept)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("title", p.title);
    root->setProperty("concept", concept);
    juce::Array<juce::var> secs;
    for (auto& s : p.sections)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("name", s.name);
        o->setProperty("duration", s.dur);
        secs.add(juce::var(o));
    }
    root->setProperty("sections", secs);

    juce::Array<juce::var> groups;
    for (auto& g : p.groups)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("name", g.name);
        o->setProperty("role", g.role.isNotEmpty() ? g.role : roles()[0]);
        o->setProperty("start", posVar(g.start));
        auto* r = new juce::DynamicObject();
        r->setProperty("kind", wordFor(relationWords, g.relation.type));
        r->setProperty("leader", g.relation.type == "frei" ? 0 : g.relation.leader + 1);
        r->setProperty("offset", g.relation.type == "frei" ? 0.0 : g.relation.delay);
        o->setProperty("relation", juce::var(r));
        juce::Array<juce::var> cells;
        if (g.relation.type == "frei")
            for (auto& c : g.cells)
            {
                auto* cv = new juce::DynamicObject();
                const auto& m = c.move;
                cv->setProperty("gesture", wordFor(gestureWords, m.type));
                const auto& t = m.type;
                if (t == "gerade" || t == "bogen" || t == "pendel") cv->setProperty("target", posVar(m.to));
                if (t == "kreis" || t == "spirale" || t == "helix" || t == "rose" || t == "lissajous") cv->setProperty("turns", m.turns);
                if (t == "kreis" || t == "spirale" || t == "helix" || t == "rose") cv->setProperty("spin", wordFor(dirWords, m.dir));
                if (t == "spirale") cv->setProperty("end_distance", labelNear(distances(), m.toDist));
                if (t == "helix") cv->setProperty("rise", m.rise / juce::jmax(0.001, p.radius));
                if (t == "pendel") cv->setProperty("repetitions", m.reps);
                if (t == "rose") cv->setProperty("petal_factor", m.petals);
                if (t == "lissajous") cv->setProperty("size", m.size);
                if (t == "drift") { cv->setProperty("extent", m.amount); cv->setProperty("restlessness", m.speed); }
                if (t == "stationen")
                {
                    juce::Array<juce::var> st;
                    for (auto& s : m.stations) st.add(posVar(s));
                    cv->setProperty("stations", st);
                }
                cv->setProperty("onset", c.onset);
                cv->setProperty("share", c.span);
                cv->setProperty("curve", wordFor(easeWords, c.ease));
                cv->setProperty("rotation", c.act.rotZ);
                cv->setProperty("width", c.act.stretch ? *c.act.stretch : 1.0);
                cv->setProperty("jitter", c.act.jitter);
                cv->setProperty("jitter_speed", c.act.jitterSpeed);
                cells.add(juce::var(cv));
            }
        o->setProperty("cells", cells);
        groups.add(juce::var(o));
    }
    root->setProperty("groups", groups);
    return juce::var(root);
}

// ------------------------------------------------------------------ HTTP
Http request(const Settings& st, const juce::String& method, const juce::String& path, const juce::String& body,
             int timeoutMs, std::atomic<bool>* cancel, std::function<void(const juce::String&)> onLine)
{
    Http r;
    juce::StreamingSocket sock;
    if (! sock.connect(st.host, st.port, 3000))
    {
        r.error = U("Ollama is not reachable at ") + st.host + ":" + juce::String(st.port)
                + U(". Is the Ollama app running (llama icon in the menu bar)?");
        return r;
    }
    const juce::MemoryBlock payload(body.toRawUTF8(), body.getNumBytesAsUTF8());
    juce::String head;
    head << method << " " << path << " HTTP/1.0\r\n"
         << "Host: " << st.host << ":" << st.port << "\r\n"
         << "Content-Type: application/json\r\n"
         << "Content-Length: " << (int) payload.getSize() << "\r\n"
         << "Connection: close\r\n\r\n";
    juce::MemoryBlock out(head.toRawUTF8(), head.getNumBytesAsUTF8());
    out.append(payload.getData(), payload.getSize());
    for (size_t sent = 0; sent < out.getSize();)
    {
        const int w = sock.write(static_cast<const char*>(out.getData()) + sent, (int) (out.getSize() - sent));
        if (w <= 0)
        {
            r.error = U("Sending to Ollama failed.");
            return r;
        }
        sent += (size_t) w;
    }

    juce::MemoryBlock data;
    size_t bodyStart = 0, lineStart = 0;
    bool headersDone = false, chunked = false;
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    char buf[8192];

    auto emitLines = [&] (bool final) {
        if (! onLine || chunked) return;
        const char* d = static_cast<const char*>(data.getData());
        for (size_t i = lineStart; i < data.getSize(); ++i)
            if (d[i] == '\n')
            {
                auto line = juce::String::fromUTF8(d + lineStart, (int) (i - lineStart)).trim();
                lineStart = i + 1;
                if (line.isNotEmpty()) onLine(line);
            }
        if (final && lineStart < data.getSize())
        {
            auto line = juce::String::fromUTF8(d + lineStart, (int) (data.getSize() - lineStart)).trim();
            lineStart = data.getSize();
            if (line.isNotEmpty()) onLine(line);
        }
    };

    for (;;)
    {
        if (cancel != nullptr && cancel->load())
        {
            r.error = U("Cancelled.");
            return r;
        }
        const int ready = sock.waitUntilReady(true, 200);
        if (ready < 0) break;
        if (ready == 0)
        {
            if (juce::Time::getMillisecondCounterHiRes() - t0 > timeoutMs)
            {
                r.error = U("Timeout: Ollama did not answer in time.");
                return r;
            }
            continue;
        }
        const int got = sock.read(buf, (int) sizeof(buf), false);
        if (got <= 0) break;
        data.append(buf, (size_t) got);
        if (! headersDone)
        {
            const char* d = static_cast<const char*>(data.getData());
            for (size_t i = 0; i + 3 < data.getSize(); ++i)
                if (d[i] == '\r' && d[i + 1] == '\n' && d[i + 2] == '\r' && d[i + 3] == '\n')
                {
                    const auto headers = juce::String::fromUTF8(d, (int) i);
                    r.status = headers.upToFirstOccurrenceOf("\r\n", false, false).fromFirstOccurrenceOf(" ", false, false).getIntValue();
                    chunked = headers.toLowerCase().contains("transfer-encoding: chunked");
                    headersDone = true;
                    bodyStart = lineStart = i + 4;
                    break;
                }
        }
        if (headersDone) emitLines(false);
    }
    if (! headersDone)
    {
        r.error = U("Ollama did not send a valid answer.");
        return r;
    }
    emitLines(true);

    const char* d = static_cast<const char*>(data.getData()) + bodyStart;
    const size_t len = data.getSize() - bodyStart;
    if (! chunked)
        r.body = juce::String::fromUTF8(d, (int) len);
    else
    {
        juce::MemoryBlock dec;
        size_t i = 0;
        while (i < len)
        {
            size_t e = i;
            while (e + 1 < len && ! (d[e] == '\r' && d[e + 1] == '\n')) ++e;
            const auto size = (size_t) juce::String::fromUTF8(d + i, (int) (e - i)).getHexValue64();
            i = e + 2;
            if (size == 0 || i + size > len) break;
            dec.append(d + i, size);
            i += size + 2;
        }
        r.body = juce::String::fromUTF8(static_cast<const char*>(dec.getData()), (int) dec.getSize());
        if (onLine)
            for (auto& line : juce::StringArray::fromLines(r.body))
                if (line.trim().isNotEmpty()) onLine(line.trim());
    }
    return r;
}

juce::String serverVersion(const Settings& st, juce::String& error)
{
    auto r = request(st, "GET", "/api/version", {}, 5000);
    if (r.error.isNotEmpty()) { error = r.error; return {}; }
    return juce::JSON::parse(r.body).getProperty("version", juce::var()).toString();
}

juce::StringArray listModels(const Settings& st, juce::String& error)
{
    juce::StringArray names;
    auto r = request(st, "GET", "/api/tags", {}, 5000);
    if (r.error.isNotEmpty()) { error = r.error; return names; }
    const auto parsed = juce::JSON::parse(r.body);          // muss leben, solange über das Array gelaufen wird
    const auto models = parsed.getProperty("models", juce::var());
    if (auto* arr = models.getArray())
        for (auto& m : *arr)
        {
            const auto name = m.getProperty("name", juce::var()).toString();
            // Cloud-Modelle laufen nicht lokal
            if (name.isNotEmpty() && ! name.containsIgnoreCase("cloud")) names.add(name);
        }
    names.sort(true);
    return names;
}

// ------------------------------------------------------------------ Entwurf mit Korrekturschleife
namespace
{
juce::var message(const juce::String& role, const juce::String& content)
{
    auto* o = new juce::DynamicObject();
    o->setProperty("role", role);
    o->setProperty("content", content);
    return juce::var(o);
}

juce::var parseLoose(const juce::String& text)
{
    auto v = juce::JSON::parse(text);
    if (v.isObject()) return v;
    const int a = text.indexOfChar('{'), b = text.lastIndexOfChar('}');
    if (a >= 0 && b > a) return juce::JSON::parse(text.substring(a, b + 1));
    return {};
}

// Abweichungen von der Anfrage, die das Modell korrigieren soll (zusätzlich zu Prüffehlern)
juce::StringArray structuralProblems(const juce::var& j, const Plan& p, const Hints& h)
{
    juce::StringArray out;
    const int nSec = (int) p.sections.size();
    if (auto* groups = prop(j, "groups").getArray())
    {
        if (h.groups > 0 && groups->size() != h.groups)
            out.add(U("Exactly ") + juce::String(h.groups) + U(" groups are required, the plan has ") + juce::String(groups->size()) + ".");
        for (int gi = 0; gi < groups->size() && gi < 8; ++gi)
        {
            const auto& gv = groups->getReference(gi);
            if (p.groups[(size_t) gi].relation.type != "frei") continue;
            const auto* cells = prop(gv, "cells").getArray();
            const int nc = cells ? cells->size() : 0;
            if (nc != nSec)
                out.add(U("Group ") + juce::String(gi + 1) + U(" »") + p.groups[(size_t) gi].name + U("' has ") + juce::String(nc)
                        + U(" cells, but there are ") + juce::String(nSec) + U(" sections. One cell per section."));
        }
    }
    if (h.duration > 0)
    {
        const double total = totalDuration(p);
        if (std::abs(total - h.duration) > 0.25 * h.duration)
            out.add(U("The total duration is ") + juce::String(total, 1) + U(" s, but about ") + juce::String(h.duration, 0) + " s is required.");
    }
    return out;
}
} // namespace

Result generate(const Settings& st, const juce::String& description, const Hints& hints, std::atomic<bool>& cancel,
                std::function<void(const Progress&)> onProgress)
{
    Result res;
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    auto finish = [&] () -> Result& { res.seconds = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0; return res; };

    juce::Array<juce::var> messages;
    messages.add(message("system", systemPrompt()));
    messages.add(message("user", userPrompt(description, hints)));
    const auto format = schema();
    bool sendThink = true;

    for (int round = 1; round <= st.maxRounds; ++round)
    {
        Progress pr;
        pr.round = round;
        pr.phase = round == 1 ? U("Draft") : U("Correction");
        if (onProgress) onProgress(pr);

        auto* body = new juce::DynamicObject();
        body->setProperty("model", st.model);
        body->setProperty("messages", messages);
        body->setProperty("stream", true);
        body->setProperty("format", format);
        if (sendThink) body->setProperty("think", st.think);
        auto* opts = new juce::DynamicObject();
        opts->setProperty("temperature", st.temperature);
        opts->setProperty("num_ctx", st.numCtx);
        opts->setProperty("num_predict", 8192);
        body->setProperty("options", juce::var(opts));

        juce::String content, streamError;
        auto http = request(st, "POST", "/api/chat", juce::JSON::toString(juce::var(body), true), st.timeoutMs, &cancel,
                            [&] (const juce::String& line) {
                                auto v = juce::JSON::parse(line);
                                if (! v.isObject()) return;
                                if (v.hasProperty("error")) streamError = v["error"].toString();
                                const auto msg = v.getProperty("message", juce::var());
                                content << msg.getProperty("content", juce::var()).toString();
                                pr.contentChars = content.length();
                                pr.thinkingChars += msg.getProperty("thinking", juce::var()).toString().length();
                                if (onProgress) onProgress(pr);
                            });
        if (cancel.load())
        {
            res.cancelled = true;
            res.error = U("Cancelled.");
            return finish();
        }
        if (http.error.isNotEmpty())
        {
            res.error = http.error;
            return finish();
        }
        if (http.status != 200 || streamError.isNotEmpty())
        {
            auto err = streamError.isNotEmpty() ? streamError
                                                : juce::JSON::parse(http.body).getProperty("error", juce::var()).toString();
            if (sendThink && err.containsIgnoreCase("think"))
            {
                sendThink = false; // Modell ohne Denkphase: Anfrage ohne den Schalter wiederholen
                --round;
                continue;
            }
            if (http.status == 404 || err.containsIgnoreCase("not found"))
                res.error = U("Model '") + st.model + U("' is not installed. In the terminal: ollama pull ") + st.model;
            else
                res.error = U("Ollama reports: ") + (err.isNotEmpty() ? err : U("HTTP ") + juce::String(http.status));
            return finish();
        }

        res.raw = content;
        res.rounds = round;
        juce::StringArray problems;
        const auto j = parseLoose(content);
        juce::String concept, err;
        auto plan = j.isObject() ? fromAi(j, concept, err) : std::nullopt;
        if (! j.isObject())
            problems.add(U("The answer was not a complete JSON object."));
        else if (! plan)
            problems.add(err);
        else
        {
            Plan copy = *plan;
            const auto scene = planToScene(copy);
            res.plan = *plan;
            res.concept = concept;
            res.issues = collectPlanIssues(*plan, scene);
            for (auto& i : res.issues)
                if (i.level == Issue::error) problems.add(i.text);
            problems.addArray(structuralProblems(j, *plan, hints));
        }
        if (problems.isEmpty()) return finish();
        if (round == st.maxRounds)
        {
            if (! res.plan) res.error = U("The model did not deliver a usable plan: ") + problems.joinIntoString(" ");
            return finish();
        }
        messages.add(message("assistant", content));
        juce::String fix = U("The review reports:\n");
        for (auto& pmsg : problems) fix << "- " << pmsg << "\n";
        fix << U("Fix these points and output the complete plan as JSON again.");
        messages.add(message("user", fix));
    }
    return finish();
}
} // namespace choreo::ai
