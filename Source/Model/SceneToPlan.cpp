#include "SceneToPlan.h"
#include <algorithm>
#include <cmath>

namespace choreo
{
namespace
{
constexpr double kDeg = 180.0 / juce::MathConstants<double>::pi;

double r2(double v) { return std::floor(v * 100.0 + 0.5) / 100.0 + 0.0; }
double len3(V3 p) { return std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z); }

PosSpec toSpec(V3 p, double R)
{
    PosSpec s;
    const double d = len3(p), h = std::hypot(p.x, p.y);
    s.dist = R > 0 ? r2(d / R) : 0;
    s.elev = d > 1e-9 ? r2(std::atan2(p.z, h) * kDeg) : 0;
    double az = h > 1e-9 ? std::atan2(p.x, p.y) * kDeg : 0;
    az = std::fmod(std::fmod(az, 360.0) + 360.0, 360.0);
    s.az = r2(az);
    return s;
}

juce::String easeKeyFor(const ClipBase& c)
{
    if (! c.easingEnabled) return "linear";
    juce::String best = "inout";
    double bd = 1e9;
    for (auto& e : eases())
    {
        if (! e.on) continue;
        const double d = std::abs(e.x1 - c.easeX1) + std::abs(e.y1 - c.easeY1) + std::abs(e.x2 - c.easeX2) + std::abs(e.y2 - c.easeY2);
        if (d < bd) { bd = d; best = e.key; }
    }
    return best;
}

bool isRunUp(const MClip& c) { return c.id.endsWithIgnoreCase("(run-up)"); }

struct Item
{
    const MClip* c;
    V3 start, end;
    bool gesture;   // false: Halte-, Anlauf- oder Endpunkt-Clip
};

std::vector<Item> walk(const Timeline& tl)
{
    std::vector<const MClip*> v;
    for (auto& c : tl.movement) v.push_back(&c);
    std::stable_sort(v.begin(), v.end(), [] (auto* a, auto* b) { return a->start < b->start; });
    std::vector<Item> out;
    V3 prev { 0, 0, 0 };
    for (auto* c : v)
    {
        V3 st = c->useStartPoint ? c->sp : prev;
        if (! c->useStartPoint && ! c->waypoints.empty() && (c->movementType == 4 || c->movementType == 5))
            st = { c->waypoints[0].x, c->waypoints[0].y, c->waypoints[0].z };
        const V3 en = endPosition(*c, st);
        out.push_back({ c, st, en, ! isHoldClip(*c) && ! isRunUp(*c) });
        prev = en;
    }
    return out;
}

// Geste aus einem Clip lesen. false = Typ nicht abbildbar.
bool gestureFrom(const Item& it, double R, Move& m)
{
    const auto& c = *it.c;
    const double turns = std::max(0.05, std::abs(c.count));
    const bool cw = c.count >= 0;
    m = Move();
    m.dir = cw ? "cw" : "ccw";
    switch (c.movementType)
    {
        case 0:
            m.to = toSpec(c.tp, R);
            if (c.palindrome) { m.type = "pendel"; m.reps = std::max(1, c.repetitions); }
            else m.type = "gerade";
            return true;
        case 1: m.type = "bogen"; m.to = toSpec(c.tp, R); return true;
        case 2: m.type = "kreis"; m.turns = turns; return true;
        case 3:
        {
            m.type = "spirale"; m.turns = turns;
            const double horiz = std::hypot(it.start.x, it.start.y);
            m.toDist = std::max(0.0, r2((horiz + c.radiusChange * turns) / (R > 0 ? R : 1)));
            return true;
        }
        case 8: m.type = "helix"; m.turns = turns; m.rise = c.heightRise; return true;
        case 7: m.type = "rose"; m.turns = turns; m.petals = std::max(0.5, c.freqRatioA); return true;
        case 6:
            m.type = "lissajous"; m.turns = std::max(0.05, c.count);
            m.a = std::max(0.5, c.freqRatioA); m.b = std::max(0.5, c.freqRatioB);
            m.size = std::max(0.05, r2(std::abs(c.sp.x) / (R > 0 ? R : 1)));
            return true;
        case 9:
            m.type = "drift";
            m.amount = std::max(0.02, r2(2.0 * std::abs(it.start.x - c.tp.x) / (R > 0 ? R : 1)));
            m.speed = std::max(0.2, c.count); m.seed = c.randomSeed;
            return true;
        case 4:
        case 5:
        {
            m.type = "stationen"; m.smooth = (c.movementType == 4); m.closed = c.closedPath;
            m.stations.clear();
            for (size_t i = 1; i < c.waypoints.size(); ++i)
                m.stations.push_back(toSpec({ c.waypoints[i].x, c.waypoints[i].y, c.waypoints[i].z }, R));
            if (m.stations.empty()) return false;
            return true;
        }
        default: return false;
    }
}
} // namespace

SceneToPlanResult sceneToPlan(const Scene& sc, const juce::String& title)
{
    SceneToPlanResult res;
    Plan& P = res.plan;
    P.title = title.isNotEmpty() ? title : "Imported scene";
    P.distanceScaler = sc.distanceScaler;
    P.holdRests = true;

    // Radius "fern": weitester Punkt der Szene
    std::vector<std::vector<Item>> items;
    double far = 0;
    for (auto& tl : sc.timelines)
    {
        items.push_back(walk(tl));
        for (auto& it : items.back())
        {
            far = std::max({ far, len3(it.start), len3(it.end) });
            for (auto& w : it.c->waypoints) far = std::max(far, len3({ w.x, w.y, w.z }));
        }
    }
    P.radius = far > 0.05 ? r2(far) : 3.0;
    const double R = P.radius;

    juce::int64 totalMs = (juce::int64) std::llround(sc.duration * 1000.0);
    for (auto& g : items) for (auto& it : g) totalMs = std::max(totalMs, it.c->end());
    totalMs = std::max<juce::int64>(totalMs, 1000);

    // Abschnitte bevorzugt aus den Praefixen der Clip-IDs ("Exposition: kreis" -> "Exposition"), sonst aus Zeitgrenzen.
    auto prefixOf = [] (const MClip& c) { return c.id.contains(": ") ? c.id.upToFirstOccurrenceOf(": ", false, false).trim() : juce::String(); };
    std::vector<juce::String> names;
    std::vector<juce::int64> firstStart;
    bool byName = true;
    for (auto& g : items)
        for (auto& it : g)
        {
            if (! it.gesture) continue;
            auto n = prefixOf(*it.c);
            if (n.isEmpty()) { byName = false; continue; }
            int k = -1;
            for (size_t i = 0; i < names.size(); ++i) if (names[i] == n) k = (int) i;
            if (k < 0) { names.push_back(n); firstStart.push_back(it.c->start); }
            else firstStart[(size_t) k] = std::min(firstStart[(size_t) k], it.c->start);
        }
    byName = byName && names.size() >= 2 && names.size() <= 24;

    std::vector<juce::int64> bounds { 0 };
    std::vector<size_t> order;
    const juce::int64 tol = std::max<juce::int64>(300, totalMs / 24);
    if (byName)
    {
        for (size_t i = 0; i < names.size(); ++i) order.push_back(i);
        std::sort(order.begin(), order.end(), [&] (size_t a, size_t c) { return firstStart[a] < firstStart[c]; });
        for (size_t i = 1; i < order.size(); ++i) bounds.push_back(firstStart[order[i]]);
        bounds.push_back(totalMs);
    }
    else
    {
        std::vector<juce::int64> b { 0, totalMs };
        for (auto& g : items)
            for (auto& it : g)
                if (it.gesture) { b.push_back(it.c->start); b.push_back(it.c->end()); }
        std::sort(b.begin(), b.end());
        for (auto t : b)
            if (t - bounds.back() >= tol && totalMs - t >= tol) bounds.push_back(t);
        bounds.push_back(totalMs);
    }

    const size_t ns = bounds.size() - 1;
    for (size_t si = 0; si < ns; ++si)
    {
        Section s;
        s.name = byName ? names[order[si]] : "Section " + juce::String((int) si + 1);
        s.dur = r2((double) (bounds[si + 1] - bounds[si]) / 1000.0);
        if (s.dur < 0.1) s.dur = 0.1;
        P.sections.push_back(s);
    }

    auto sectionOf = [&] (const MClip& c) -> size_t
    {
        if (byName)
        {
            auto n = prefixOf(c);
            for (size_t si = 0; si < ns; ++si) if (names[order[si]] == n) return si;
        }
        size_t si = 0;
        while (si + 1 < ns && c.start >= bounds[si + 1] - tol / 2) ++si;
        return si;
    };

    int multi = 0, lost = 0, partial = 0;
    for (size_t gi = 0; gi < sc.timelines.size(); ++gi)
    {
        Group g = defaultGroup((int) gi);
        g.cells.assign(ns, defaultCell());
        if (! items[gi].empty()) g.start = toSpec(items[gi].front().start, R);

        std::vector<int> used(ns, 0);
        for (auto& it : items[gi])
        {
            if (! it.gesture) continue;
            ++res.clipsTotal;
            const size_t si = sectionOf(*it.c);
            Move m;
            if (! gestureFrom(it, R, m)) { ++lost; continue; }
            if (used[si]++ > 0) { ++multi; continue; }
            const double s0 = (double) bounds[si], len = (double) (bounds[si + 1] - bounds[si]);
            auto& cell = g.cells[si];
            cell.move = m;
            cell.onset = std::min(0.95, std::max(0.0, r2(((double) it.c->start - s0) / len)));
            cell.span = std::min(1.0, std::max(0.05, r2((double) it.c->length / len)));
            if ((double) it.c->end() > (double) bounds[si + 1] + (double) tol) ++partial;
            cell.ease = easeKeyFor(*it.c);
            if (! P.sections[si].name.contains(":") && it.c->id.contains(": "))
                P.sections[si].name = it.c->id.upToFirstOccurrenceOf(": ", false, false);
            ++res.clipsConverted;
        }

        // Aktionen (Drehung, Neigung, Breite, Zittern) dem Abschnitt zuordnen, in dem sie beginnen
        for (auto& ac : sc.timelines[gi].actions)
        {
            if (ac.id.startsWithIgnoreCase("Hold:")) continue;
            size_t si = 0;
            while (si + 1 < ns && ac.start >= bounds[si + 1] - tol / 2) ++si;
            auto& act = g.cells[si].act;
            for (auto& d : ac.actions)
            {
                const double base = d.useStartValue ? d.startValue : 0.0;
                switch (d.actionType)
                {
                    case 1: if (d.value != base || d.value != 0) act.tiltX = d.value; break;
                    case 2: if (d.value != base || d.value != 0) act.tiltY = d.value; break;
                    case 3: act.rotZ += d.useStartValue ? d.value - d.startValue : d.value; break;
                    case 4: if (d.value > 0 && (d.value != 1.0 || d.startValue != d.value)) act.stretch = d.value; break;
                    case 5: act.jitter = d.value; act.jitterSpeed = d.jitterSpeed; break;
                    default: break;
                }
            }
        }
        P.groups.push_back(std::move(g));
    }

    res.notes.add("Relations between groups (canon, mirror ...) are not recovered; all groups are independent.");
    if (lost) res.notes.add(juce::String(lost) + " clip(s) of an unsupported type were skipped.");
    if (multi) res.notes.add(juce::String(multi) + " further clip(s) fell into an already used section and were dropped.");
    if (partial) res.notes.add(juce::String(partial) + " clip(s) extend past their section and are shortened.");
    res.notes.add("Section names and times are derived from the clip boundaries. Please check the result in the preview.");
    return res;
}
} // namespace choreo
