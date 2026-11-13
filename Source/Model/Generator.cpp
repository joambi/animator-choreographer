#include "Generator.h"
#include "Preview.h"

namespace choreo
{
namespace
{
// Entspricht dem JavaScript-Ausdruck (+v || d): 0 und ungültige Werte fallen auf den Vorgabewert.
double orDef(double v, double d) { return (std::isfinite(v) && v != 0.0) ? v : d; }

juce::uint32 dimHex(juce::uint32 hex)
{
    auto f = [] (juce::uint32 c) { return (juce::uint32) jsRound(c * 0.45 + 40); };
    return (f((hex >> 16) & 255) << 16) | (f((hex >> 8) & 255) << 8) | f(hex & 255);
}
juce::uint32 lightenHex(juce::uint32 hex)
{
    auto f = [] (juce::uint32 c) { return (juce::uint32) jsRound(c + (255 - c) * 0.35); };
    return (f((hex >> 16) & 255) << 16) | (f((hex >> 8) & 255) << 8) | f(hex & 255);
}
juce::uint32 argb(juce::uint32 rgb) { return 0xff000000u | rgb; }

void applyEase(ClipBase& c, const juce::String& key)
{
    const auto& e = easeFor(key);
    if (! e.on) { c.easingEnabled = false; return; }
    c.easingEnabled = true;
    c.easeX1 = e.x1; c.easeY1 = e.y1; c.easeX2 = e.x2; c.easeY2 = e.y2;
    c.easePerRepetition = true; c.easePerDirection = true;
}

MClip mk(juce::uint32 rgb)
{
    MClip c;
    c.colour = argb(rgb);
    c.useStartPoint = true;
    return c;
}
void finish(MClip& c) { c.sp = r6v(c.sp); c.tp = r6v(c.tp); }

MClip hold(V3 pos, juce::int64 t0, juce::int64 t1, juce::uint32 rgb, const juce::String& id)
{
    auto c = mk(dimHex(rgb));
    c.id = id; c.start = t0; c.length = t1 - t0;
    c.movementType = (int) MovementType::MoveToCartesian;
    c.sp = pos; c.tp = pos;
    finish(c);
    return c;
}

struct GestureResult
{
    std::vector<MClip> clips;
    V3 end;
    juce::StringArray notes;
    bool rest = false;
};

GestureResult buildGesture(const Cell& cell, V3 pos, juce::int64 t0, juce::int64 t1, double R, juce::uint32 rgb,
                           const juce::String& idBase, int seedOffset)
{
    const auto& m = cell.move;
    GestureResult r;
    const juce::int64 L = t1 - t0;
    const double horiz = std::hypot(pos.x, pos.y);
    const V3 centreZ { 0, 0, pos.z };
    const bool cw = m.dir != "ccw";
    const double turns = juce::jmax(0.05, orDef(m.turns, 1));

    auto push = [&] (std::function<void(MClip&)> f)
    {
        auto c = mk(rgb);
        c.id = idBase; c.start = t0; c.length = L; c.sp = pos;
        applyEase(c, cell.ease);
        f(c);
        finish(c);
        r.clips.push_back(c);
    };

    const auto& t = m.type;
    if (t == "gerade")
        push([&] (MClip& c) { c.movementType = 0; c.tp = specToXYZ(m.to, R); });
    else if (t == "bogen")
    {
        if (horiz < 1e-6) r.notes.add(U("Arc from the center has no defined starting angle; the movement runs straight."));
        push([&] (MClip& c) { c.movementType = 1; c.tp = specToXYZ(m.to, R); });
    }
    else if (t == "kreis")
    {
        if (horiz < 0.05) r.notes.add(U("Circling from the center has radius 0; the group stays put. Move outward first."));
        push([&] (MClip& c) { c.movementType = 2; c.tp = centreZ; c.count = cw ? turns : -turns; });
    }
    else if (t == "spirale")
    {
        const double target = juce::jmax(0.0, m.toDist) * R;
        push([&] (MClip& c) { c.movementType = 3; c.tp = centreZ; c.count = cw ? turns : -turns; c.radiusChange = (target - horiz) / turns; });
        if (horiz < 0.05) r.notes.add(U("Spiral from the center: the start angle is undefined (right)."));
    }
    else if (t == "helix")
    {
        push([&] (MClip& c) { c.movementType = 8; c.tp = { 0, 0, 0 }; c.count = cw ? turns : -turns; c.heightRise = std::isfinite(m.rise) ? m.rise : 0.0; });
        if (horiz < 0.05) r.notes.add(U("Helix from the center has radius 0 and only rises vertically."));
    }
    else if (t == "pendel")
        push([&] (MClip& c) { c.movementType = 0; c.tp = specToXYZ(m.to, R); c.palindrome = true; c.repetitions = (int) juce::jmax(1.0, jsRound(orDef(m.reps, 1))); });
    else if (t == "rose")
    {
        if (horiz < 0.05) r.notes.add(U("Rose from the center has radius 0."));
        push([&] (MClip& c) { c.movementType = 7; c.tp = centreZ; c.count = cw ? turns : -turns; c.freqRatioA = juce::jmax(0.5, orDef(m.petals, 3)); c.phaseDeg = 0; });
    }
    else if (t == "lissajous")
    {
        const double A = juce::jmax(0.05, m.size) * R;
        const V3 figStart { A, 0, pos.z };
        const double dist = (figStart - pos).length();
        juce::int64 ts = t0;
        if (dist > 0.02)
        {
            const juce::int64 appr = (juce::int64) juce::jmax(10.0, juce::jmin(1000.0, jsRound((double) L * 0.2)));
            auto c = mk(rgb);
            c.id = idBase + " (run-up)"; c.start = t0; c.length = appr; c.movementType = 1; c.sp = pos; c.tp = figStart;
            applyEase(c, "inout");
            finish(c);
            r.clips.push_back(c);
            ts = t0 + appr;
        }
        auto c = mk(rgb);
        c.id = idBase; c.start = ts; c.length = t1 - ts; c.movementType = 6;
        applyEase(c, cell.ease);
        c.sp = { A, A, pos.z }; c.tp = centreZ; c.count = turns;
        c.freqRatioA = juce::jmax(0.5, orDef(m.a, 3)); c.freqRatioB = juce::jmax(0.5, orDef(m.b, 2));
        c.phaseDeg = 90; c.closedPath = true;
        finish(c);
        r.clips.push_back(c);
    }
    else if (t == "drift")
    {
        const double amp = juce::jmax(0.02, m.amount) * R;
        push([&] (MClip& c)
        {
            c.movementType = 9; c.tp = { pos.x - amp / 2, pos.y, pos.z };
            c.count = juce::jmax(0.2, orDef(m.speed, 3));
            c.randomSeed = (int) orDef(jsRound(m.seed), 1) + seedOffset;
        });
    }
    else if (t == "stationen")
    {
        std::vector<Waypoint> wps;
        wps.push_back({ round6(pos.x), round6(pos.y), round6(pos.z), false });
        for (auto& s : m.stations)
        {
            auto p = specToXYZ(s, R);
            wps.push_back({ round6(p.x), round6(p.y), round6(p.z), false });
        }
        push([&] (MClip& c) { c.movementType = m.smooth ? 4 : 5; c.waypoints = wps; c.closedPath = m.closed; c.tension = 0; });
        if (wps.size() < 2) r.notes.add("Path without stations.");
    }
    else
    {
        r.rest = true;
        r.end = pos;
        return r;
    }

    V3 end = pos;
    for (auto& k : r.clips) end = endPosition(k, end);
    r.end = r6v(end);
    return r;
}

struct RotState
{
    double rx = 0, ry = 0, rz = 0, stretch = 1, base = 1;
};

std::optional<AClip> buildActions(const Cell& cell, RotState& state, juce::int64 t0, juce::int64 t1, juce::uint32 rgb, const juce::String& idBase)
{
    const auto& a = cell.act;
    std::vector<ADef> acts;
    const double rz = std::isfinite(a.rotZ) ? a.rotZ : 0.0;
    const V3 target { a.tiltX ? *a.tiltX : state.rx, a.tiltY ? *a.tiltY : state.ry, state.rz + rz };
    const bool rotChange = target.x != state.rx || target.y != state.ry || target.z != state.rz;
    const bool rotHeld = state.rx != 0 || state.ry != 0 || state.rz != 0;
    if (rotChange)
    {
        // Absolute Form mit Startwert: reproduzierbar, kein Aufsummieren im Loop. Achsen mit
        // Zustand ungleich 0 werden mitgeführt, sonst würde der Animator sie auf 0 setzen.
        const double cur[3] = { state.rx, state.ry, state.rz }, tgt[3] = { target.x, target.y, target.z };
        for (int k = 0; k < 3; ++k)
            if (cur[k] != 0 || tgt[k] != 0)
                acts.push_back({ 1 + k, 1, tgt[k], cur[k], true, 1 });
        state.rx = target.x; state.ry = target.y; state.rz = target.z;
    }
    else if (rotHeld)
    {
        // Keep the angles of earlier clips: without an action the Animator would reset the axis to 0 and the group would jump.
        const double cur[3] = { state.rx, state.ry, state.rz };
        for (int k = 0; k < 3; ++k)
            if (cur[k] != 0) acts.push_back({ 1 + k, 1, cur[k], cur[k], true, 1 });
    }
    if (a.stretch && *a.stretch > 0)
    {
        acts.push_back({ 4, 1, *a.stretch, state.stretch, true, 1 });
        state.stretch = *a.stretch;
    }
    else if (state.stretch != state.base)
        acts.push_back({ 4, 1, state.stretch, state.stretch, true, 1 }); // hold the width of the previous clip
    const double jit = std::isfinite(a.jitter) ? a.jitter : 0.0;
    if (jit > 0)
        acts.push_back({ 5, 1, jit, 0, false, juce::jmax(0.05, orDef(a.jitterSpeed, 1)) });
    if (acts.empty()) return std::nullopt;
    AClip c;
    c.id = idBase; c.start = t0; c.length = t1 - t0; c.colour = argb(lightenHex(rgb));
    c.actions = acts;
    applyEase(c, cell.ease);
    return c;
}

juce::String actionLabel(const Cell& cell)
{
    juce::StringArray parts;
    if (cell.act.rotZ != 0) parts.add("Rotation");
    if (cell.act.tiltX || cell.act.tiltY) parts.add("Tilt");
    if (cell.act.stretch) parts.add("Width");
    if (cell.act.jitter != 0) parts.add("Jitter");
    return parts.isEmpty() ? juce::String("Action") : parts.joinIntoString(" + ");
}

void sortByStart(std::vector<MClip>& v) { std::stable_sort(v.begin(), v.end(), [] (auto& a, auto& b) { return a.start < b.start; }); }
void sortByStart(std::vector<AClip>& v) { std::stable_sort(v.begin(), v.end(), [] (auto& a, auto& b) { return a.start < b.start; }); }

// Ohne Halteclips: kurzer 40-ms-Halt am exakten Endpunkt, wenn danach nichts folgt
void addEndGuards(std::vector<MClip>& movement, juce::uint32 rgb, juce::int64 totalMs)
{
    auto sorted = movement;
    sortByStart(sorted);
    std::vector<MClip> extra;
    for (size_t i = 0; i < sorted.size(); ++i)
    {
        const auto& c = sorted[i];
        const juce::int64 end = c.end();
        const juce::int64 gap = (i + 1 < sorted.size() ? sorted[i + 1].start : totalMs) - end;
        if (gap >= 10)
            extra.push_back(hold(r6v(endPosition(c, c.sp)), end, end + juce::jmin<juce::int64>(40, gap), rgb, c.id + " (end point)"));
    }
    movement.insert(movement.end(), extra.begin(), extra.end());
    sortByStart(movement);
}

struct GroupBuild
{
    std::vector<MClip> movement;
    std::vector<AClip> actions;
    V3 startPos;
    juce::StringArray notes;
};

GroupBuild buildFreeGroup(const Plan& plan, int gi)
{
    const auto& g = plan.groups[(size_t) gi];
    const double R = orDef(plan.radius, 3);
    const juce::uint32 rgb = groupColours[gi % 8];
    const auto starts = sectionStarts(plan);
    const juce::int64 totalMs = (juce::int64) jsRound(totalDuration(plan) * 1000);
    GroupBuild out;
    V3 pos = specToXYZ(g.start, R);
    out.startPos = pos;
    RotState state;
    state.stretch = juce::jlimit(0.02, 4.0, g.baseStretch.value_or(plan.baseStretch));
    state.base = state.stretch;
    const double baseStretch = state.base;
    const bool holdRests = plan.holdRests;
    juce::int64 cursor = 0;
    juce::int64 actCursor = 0;

    auto carryClip = [&] (juce::int64 t0, juce::int64 t1)
    {
        if (! holdRests || t1 - t0 < 10) return;
        std::vector<ADef> acts;
        const double cur[3] = { state.rx, state.ry, state.rz };
        for (int k = 0; k < 3; ++k)
            if (cur[k] != 0) acts.push_back({ 1 + k, 1, cur[k], cur[k], true, 1 });
        if (state.stretch != baseStretch) acts.push_back({ 4, 1, state.stretch, state.stretch, true, 1 });
        if (acts.empty()) return;
        AClip c;
        c.id = "Hold: Rotation + Width"; c.start = t0; c.length = t1 - t0; c.colour = argb(lightenHex(rgb));
        c.actions = acts;
        out.actions.push_back(c);
    };

    auto fillHold = [&] (juce::int64 until, const juce::String& label)
    {
        if (until - cursor >= 10 && holdRests) out.movement.push_back(hold(pos, cursor, until, rgb, label));
        cursor = juce::jmax(cursor, until);
    };

    for (size_t si = 0; si < plan.sections.size(); ++si)
    {
        const auto& sec = plan.sections[si];
        const auto& cell = g.cells[si];
        const juce::int64 s0 = (juce::int64) jsRound(starts[si] * 1000), s1 = (juce::int64) jsRound((starts[si] + sec.dur) * 1000);
        const double onset = juce::jmin(0.95, juce::jmax(0.0, std::isfinite(cell.onset) ? cell.onset : 0.0));
        const juce::int64 t0 = (juce::int64) jsRound((double) s0 + (double) (s1 - s0) * onset);
        const juce::int64 t1 = juce::jmin(s1, (juce::int64) jsRound((double) t0 + (double) (s1 - s0) * juce::jmax(0.05, juce::jmin(1.0, orDef(cell.span, 1)))));
        const auto* gd = gestureFor(cell.move.type);
        const juce::String label = sec.name + ": " + (gd ? gd->label : juce::String("Gesture"));
        if (t1 - t0 < 10)
        {
            out.notes.add(sec.name + U(": time window under 10 ms, gesture skipped."));
            continue;
        }
        auto r = buildGesture(cell, pos, t0, t1, R, rgb, label, gi * 101);
        for (auto& n : r.notes) out.notes.add(sec.name + ": " + n);
        if (r.rest)
        {
            fillHold(t0, sec.name + ": Hold");
            if (holdRests) out.movement.push_back(hold(pos, t0, t1, rgb, label));
            cursor = t1;
        }
        else
        {
            fillHold(t0, sec.name + ": Hold");
            for (auto& c : r.clips) out.movement.push_back(c);
            pos = r.end;
            cursor = t1;
        }
        carryClip(actCursor, t0);
        if (auto ac = buildActions(cell, state, t0, t1, rgb, sec.name + ": " + actionLabel(cell)))
            out.actions.push_back(*ac);
        actCursor = t1;
    }
    carryClip(actCursor, totalMs);
    if (totalMs - cursor >= 10 && holdRests)
        out.movement.push_back(hold(pos, cursor, totalMs, rgb, "End: Hold"));
    if (! holdRests) addEndGuards(out.movement, rgb, totalMs);
    return out;
}

V3 rotSigns(const juce::String& type)
{
    // Konjugation der Rotationen mit der Spiegelung: welche Achsen kehren ihren Drehsinn um
    if (type == "spiegel_lr") return { 1, -1, -1 };
    if (type == "spiegel_vh") return { -1, 1, -1 };
    if (type == "umkehr") return { -1, -1, 1 };
    return { 1, 1, 1 };
}

// Nach einer Spiegelung können entartete Startwinkel (Gruppe im Zentrum) andere Endpunkte ergeben.
// Darum wird die Kette mit der exakten Animator-Formel neu verknüpft.
void resyncChain(std::vector<MClip>& movement)
{
    sortByStart(movement);
    std::optional<V3> prevEnd;
    juce::int64 prevT = 0;
    for (auto& c : movement)
    {
        if (prevEnd && c.start == prevT && c.movementType != 6)
        {
            if (isHoldClip(c)) { c.sp = *prevEnd; c.tp = *prevEnd; }
            else if (! c.waypoints.empty() && (c.movementType == 4 || c.movementType == 5))
            {
                c.waypoints[0].x = prevEnd->x; c.waypoints[0].y = prevEnd->y; c.waypoints[0].z = prevEnd->z;
            }
            else c.sp = *prevEnd;
        }
        prevEnd = r6v(endPosition(c, c.sp));
        prevT = c.end();
    }
}

GroupBuild transformClips(const GroupBuild& src, const juce::String& type, juce::int64 delayMs, juce::int64 totalMs, juce::uint32 rgb, int gi)
{
    const auto& rel = relationFor(type);
    const double mx = rel.mx, my = rel.my;
    auto T = [&] (V3 p) { return V3 { round6(p.x * mx) + 0.0, round6(p.y * my) + 0.0, p.z }; };
    GroupBuild out;
    auto shift = [&] (ClipBase& c)
    {
        c.start += delayMs;
        if (c.start >= totalMs) return false;
        if (c.end() > totalMs) c.length = totalMs - c.start;
        return c.length >= 10;
    };
    for (const auto& s : src.movement)
    {
        auto c = s;
        c.sp = T(c.sp); c.tp = T(c.tp);
        for (auto& w : c.waypoints)
        {
            auto p = T({ w.x, w.y, w.z });
            w.x = p.x; w.y = p.y; w.z = p.z;
        }
        if (rel.mirror && (c.movementType == 2 || c.movementType == 3 || c.movementType == 7 || c.movementType == 8))
            c.count = -c.count;
        if (c.movementType == 9) c.randomSeed = c.randomSeed + 7 * (gi + 1);
        c.colour = argb(isHoldClip(s) ? dimHex(rgb) : rgb);
        if (shift(c)) out.movement.push_back(c);
    }
    resyncChain(out.movement);
    const auto sg = rotSigns(type);
    for (const auto& s : src.actions)
    {
        auto c = s;
        for (auto& a : c.actions)
        {
            const double k = a.actionType == 1 ? sg.x : a.actionType == 2 ? sg.y : a.actionType == 3 ? sg.z : 0.0;
            if (k != 0.0) { a.value *= k; a.startValue *= k; }
        }
        c.colour = argb(lightenHex(rgb));
        if (shift(c)) out.actions.push_back(c);
    }
    out.startPos = T(src.startPos);
    return out;
}

GroupBuild buildGroup(const Plan& plan, int gi, int depth = 0)
{
    const auto& g = plan.groups[(size_t) gi];
    const auto& rel = g.relation;
    if (rel.type == "frei" || depth > 8) return buildFreeGroup(plan, gi);
    const int li = juce::jmin((int) plan.groups.size() - 1, juce::jmax(0, rel.leader));
    if (li == gi)
    {
        auto b = buildFreeGroup(plan, gi);
        b.notes = juce::StringArray { "A group cannot relate to itself." };
        return b;
    }
    auto leader = buildGroup(plan, li, depth + 1);
    const juce::int64 totalMs = (juce::int64) jsRound(totalDuration(plan) * 1000);
    const juce::int64 delayMs = (juce::int64) juce::jmax(0.0, jsRound((std::isfinite(rel.delay) ? rel.delay : 0.0) * 1000)) * (rel.type == "unisono" ? 0 : 1);
    const juce::uint32 rgb = groupColours[gi % 8];
    auto out = transformClips(leader, rel.type, delayMs, totalMs, rgb, gi);
    if (delayMs > 0 && plan.holdRests)
        out.movement.insert(out.movement.begin(), hold(out.startPos, 0, juce::jmin(delayMs, totalMs), rgb, "Wait for entry"));
    if (delayMs > 0)
    {
        int cut = 0;
        for (auto& c : leader.movement) if (c.start + delayMs >= totalMs) ++cut;
        if (cut) out.notes.add(juce::String(cut) + U(" clip(s) of the lead group fall past the end of the form because of the offset and are dropped."));
    }
    return out;
}
} // namespace

static Scene buildScene(Plan& plan)
{
    normalizePlan(plan);
    Scene sc;
    sc.distanceScaler = plan.distanceScaler;
    for (int gi = 0; gi < (int) plan.groups.size(); ++gi)
    {
        auto b = buildGroup(plan, gi);
        Timeline tl;
        tl.movement = std::move(b.movement);
        tl.actions = std::move(b.actions);
        sortByStart(tl.movement);
        sortByStart(tl.actions);
        sc.timelines.push_back(std::move(tl));
        sc.startPositions.push_back(b.startPos);
        sc.notes.push_back(b.notes);
    }
    sc.duration = totalDuration(plan);
    return sc;
}

// Farthest distance from the listening position over all timelines (sampled every 25 ms plus the start points).
static double farthestPoint(const Scene& sc)
{
    double far = 0;
    for (size_t i = 0; i < sc.timelines.size(); ++i)
    {
        std::optional<V3> init;
        if (i < sc.startPositions.size()) init = sc.startPositions[i];
        TimelinePreview pv(sc.timelines[i], init);
        for (double t = 0; t <= sc.duration * 1000.0 + 1; t += 25.0)
        {
            const auto s = pv.sample(t);
            far = juce::jmax(far, std::sqrt(s.pos.x * s.pos.x + s.pos.y * s.pos.y + s.pos.z * s.pos.z));
        }
    }
    return far;
}

Scene planToScene(Plan& plan)
{
    auto sc = buildScene(plan);
    if (plan.maxDistance && *plan.maxDistance > 0)
    {
        const double found = farthestPoint(sc);
        if (found > 1e-6)
        {
            const double f = *plan.maxDistance / found;
            Plan scaled = plan;
            scaled.radius = plan.radius * f;
            auto s2 = buildScene(scaled);
            s2.exportScale = f;
            s2.maxDistanceFound = found;
            return s2;
        }
    }
    return sc;
}

std::vector<Issue> collectPlanIssues(const Plan& plan, const Scene& sc)
{
    auto iss = validate(sc, juce::jmax(0.1, sc.duration));
    if (sc.exportScale != 1.0 && plan.maxDistance)
        iss.push_back({ Issue::note, "Scene scaled by " + juce::String(sc.exportScale, 3) + ": farthest point "
                                         + juce::String(sc.maxDistanceFound, 2) + " to " + juce::String(*plan.maxDistance, 2) + "." });
    if (plan.encoderName.isNotEmpty())
        iss.push_back({ Issue::note, "For encoder: " + plan.encoderName });
    for (size_t gi = 0; gi < sc.notes.size(); ++gi)
        for (auto& n : sc.notes[gi])
            iss.push_back({ Issue::note, plan.groups[gi].name + ": " + n });
    for (auto& s : plan.sections)
        if (! (s.dur > 0))
        {
            iss.push_back({ Issue::error, U("At least one section has no valid duration.") });
            break;
        }
    return iss;
}
} // namespace choreo
