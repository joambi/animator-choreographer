#include "Scene.h"
#include "AnimatorMath.h"

namespace choreo
{
const char* const movementTypeNames[10] = { "MoveTo (Cartesian)", "MoveTo (Polar)", "Circle", "Spiral", "Spline",
                                             "Polygon", "Lissajous", "Rose", "Helix", "Random Walk" };

static void baseToIcst(Clip& c, const ClipBase& b)
{
    c.id = b.id;
    c.start = (ms_t) b.start;
    c.length = (ms_t) b.length;
    c.colour = juce::Colour(b.colour);
    c.muted = b.muted;
    c.palindrome = b.palindrome;
    c.repetitions = juce::jmax(1, b.repetitions);
    c.easingEnabled = b.easingEnabled;
    c.easeX1 = b.easeX1; c.easeY1 = b.easeY1; c.easeX2 = b.easeX2; c.easeY2 = b.easeY2;
    c.easePerRepetition = b.easePerRepetition;
    c.easePerDirection = b.easePerDirection;
}

static void baseFromIcst(ClipBase& b, const Clip& c)
{
    b.id = c.id;
    b.start = (juce::int64) c.start;
    b.length = (juce::int64) c.length;
    b.colour = c.colour.getARGB();
    b.muted = c.muted;
    b.palindrome = c.palindrome;
    b.repetitions = c.repetitions;
    b.easingEnabled = c.easingEnabled;
    b.easeX1 = c.easeX1; b.easeY1 = c.easeY1; b.easeX2 = c.easeX2; b.easeY2 = c.easeY2;
    b.easePerRepetition = c.easePerRepetition;
    b.easePerDirection = c.easePerDirection;
}

MovementClip toIcst(const MClip& m)
{
    MovementClip c;
    baseToIcst(c, m);
    c.movementType = (MovementType) juce::jlimit(0, (int) MovementType::RandomWalk, m.movementType);
    c.startPointGroup.setXYZ(m.sp.x, m.sp.y, m.sp.z, false);
    c.targetPointGroup.setXYZ(m.tp.x, m.tp.y, m.tp.z, false);
    c.useStartPoint = m.useStartPoint;
    c.count = m.count;
    c.radiusChange = m.radiusChange;
    for (auto& w : m.waypoints)
    {
        MovementWaypoint wp;
        wp.x = w.x; wp.y = w.y; wp.z = w.z; wp.startsNewSegment = w.newSegment;
        c.waypoints.add(wp);
    }
    c.tension = m.tension;
    c.closedPath = m.closedPath;
    c.heightRise = m.heightRise;
    c.freqRatioA = m.freqRatioA;
    c.freqRatioB = m.freqRatioB;
    c.phaseDeg = m.phaseDeg;
    c.randomSeed = m.randomSeed;
    return c;
}

ActionClip toIcst(const AClip& a)
{
    ActionClip c;
    baseToIcst(c, a);
    for (auto& d : a.actions)
        c.actions.add(ActionDefinition((ActionType) d.actionType, (TimingType) d.timingType, d.value, d.startValue,
                                       d.useStartValue, d.jitterSpeed));
    return c;
}

MClip fromIcst(const MovementClip& c)
{
    MClip m;
    baseFromIcst(m, c);
    m.movementType = (int) c.movementType;
    m.sp = { c.startPointGroup.getX(), c.startPointGroup.getY(), c.startPointGroup.getZ() };
    m.tp = { c.targetPointGroup.getX(), c.targetPointGroup.getY(), c.targetPointGroup.getZ() };
    m.useStartPoint = c.useStartPoint;
    m.count = c.count;
    m.radiusChange = c.radiusChange;
    for (auto& w : c.waypoints)
        m.waypoints.push_back({ w.x, w.y, w.z, w.startsNewSegment });
    m.tension = c.tension;
    m.closedPath = c.closedPath;
    m.heightRise = c.heightRise;
    m.freqRatioA = c.freqRatioA;
    m.freqRatioB = c.freqRatioB;
    m.phaseDeg = c.phaseDeg;
    m.randomSeed = c.randomSeed;
    return m;
}

AClip fromIcst(const ActionClip& c)
{
    AClip a;
    baseFromIcst(a, c);
    for (auto& d : c.actions)
        a.actions.push_back({ (int) d.getAction(), (int) d.getTiming(), d.getValue(), d.getStartValue(),
                              d.getUseStartValue(), d.getJitterSpeed() });
    return a;
}

V3 endPosition(const MClip& m, V3 reference)
{
    const auto clip = toIcst(m);
    const auto st = AnimatorMath::computeMovementStartState(clip, reference);
    return AnimatorMath::calculatePosition(clip, AnimatorMath::applyEasing(clip, 1.0), st, reference);
}

bool isHoldClip(const MClip& c)
{
    return c.movementType == 0 && ! c.palindrome && (c.sp - c.tp).length() < 1e-9;
}

// ------------------------------------------------------------------ XML
std::unique_ptr<juce::XmlElement> sceneToXml(const Scene& scene)
{
    // wie AnimatorMainView::exportAllScenes: Wurzel AnimatorTimelines, je Gruppe TimelineModel::toXml()
    auto root = std::make_unique<juce::XmlElement>("AnimatorTimelines");
    if (scene.distanceScaler.has_value())
        root->setAttribute("distanceScaler", *scene.distanceScaler);

    for (auto& tl : scene.timelines)
    {
        TimelineModel model;
        for (auto& m : tl.movement) model.movement.clips.add(toIcst(m));
        for (auto& a : tl.actions) model.actions.clips.add(toIcst(a));
        root->addChildElement(model.toXml().release());
    }
    return root;
}

juce::String sceneToXmlString(const Scene& scene)
{
    return sceneToXml(scene)->toString();
}

bool writeSceneFile(const Scene& scene, const juce::File& file, juce::String& error)
{
    auto root = sceneToXml(scene);
    if (! root->writeTo(file))
    {
        error = "The file could not be written: " + file.getFullPathName();
        return false;
    }
    return true;
}

std::optional<Scene> parseSceneXml(const juce::String& text, juce::String& error)
{
    auto root = juce::parseXML(text);
    if (root == nullptr)
    {
        error = "The file is not readable XML.";
        return std::nullopt;
    }
    if (! root->hasTagName("AnimatorTimelines"))
    {
        error = "Not an Animator export: root element is <" + root->getTagName() + ">, expected <AnimatorTimelines>.";
        return std::nullopt;
    }
    Scene sc;
    if (root->hasAttribute("distanceScaler"))
        sc.distanceScaler = root->getDoubleAttribute("distanceScaler");

    for (auto* x : root->getChildWithTagNameIterator("Timeline"))
    {
        TimelineModel model;
        model.fromXml(*x);
        Timeline tl;
        for (auto& c : model.movement.clips) tl.movement.push_back(fromIcst(c));
        for (auto& c : model.actions.clips) tl.actions.push_back(fromIcst(c));
        sc.timelines.push_back(std::move(tl));
    }
    if (sc.timelines.empty())
    {
        error = U("The file contains no timeline.");
        return std::nullopt;
    }
    juce::int64 end = 0;
    for (auto& tl : sc.timelines)
    {
        for (auto& c : tl.movement) end = juce::jmax(end, c.end());
        for (auto& c : tl.actions) end = juce::jmax(end, c.end());
    }
    sc.duration = juce::jmax(1.0, (double) end / 1000.0 + 0.5);
    sc.notes.resize(sc.timelines.size());
    return sc;
}

// ------------------------------------------------------------------ Prüfung
std::vector<Issue> validate(const Scene& scene, double durationSeconds)
{
    std::vector<Issue> issues;
    auto add = [&] (Issue::Level l, const juce::String& t) { issues.push_back({ l, t }); };
    const juce::String lq = U("\xe2\x80\x9e"), rq = U("\xe2\x80\x9c");
    auto q = [&] (const juce::String& s) { return lq + s + rq; };

    if (scene.timelines.size() > 8)
        add(Issue::error, U("The scene contains ") + juce::String((int) scene.timelines.size())
                              + U(" timelines; the Animator manages at most 8 groups."));

    for (size_t gi = 0; gi < scene.timelines.size(); ++gi)
    {
        const auto& tl = scene.timelines[gi];
        const juce::String g = "Group " + juce::String((int) gi + 1);

        auto check = [&] (std::vector<const ClipBase*> list, const juce::String& layer)
        {
            std::stable_sort(list.begin(), list.end(), [] (auto* a, auto* b) { return a->start < b->start; });
            for (size_t i = 0; i < list.size(); ++i)
            {
                auto* c = list[i];
                if (c->length < 10)
                    add(Issue::error, g + ", " + layer + " " + q(c->id) + ": " + U("Length ")
                                          + juce::String(c->length) + " ms is below the minimum of 10 ms.");
                if (c->start < 0)
                    add(Issue::error, g + ", " + layer + " " + q(c->id) + ": negative start time.");
                if (i > 0 && c->start < list[i - 1]->end())
                    add(Issue::error, g + ", " + layer + ": " + q(list[i - 1]->id) + " and " + q(c->id)
                                          + U(" overlap (the Animator marks this in red)."));
            }
        };
        std::vector<const ClipBase*> mv, ac;
        for (auto& c : tl.movement) mv.push_back(&c);
        for (auto& c : tl.actions) ac.push_back(&c);
        check(mv, "Movement");
        check(ac, "Action");

        for (auto& c : tl.movement)
        {
            if (c.movementType < 0 || c.movementType > 9)
                add(Issue::error, g + " " + q(c.id) + U(": invalid movementType ") + juce::String(c.movementType) + ".");
            if (! c.useStartPoint)
                add(Issue::warn, g + " " + q(c.id) + " starts without a fixed start point and does not play identically on every pass.");
            if ((c.movementType == 4 || c.movementType == 5) && c.waypoints.size() < 2)
                add(Issue::warn, g + " " + q(c.id) + ": " + movementTypeNames[c.movementType] + " without at least two waypoints.");
            if (c.repetitions > 1 && ! c.palindrome && (c.movementType == 0 || c.movementType == 1 || c.movementType == 9))
                add(Issue::warn, g + " " + q(c.id) + " repeats an open path without palindrome and jumps at every repetition.");
        }
        for (auto& c : tl.actions)
        {
            bool anyRot = false, anyStart = false;
            for (auto& a : c.actions)
            {
                if (a.actionType >= 1 && a.actionType <= 3 && a.timingType != 0)
                {
                    anyRot = true;
                    anyStart = anyStart || a.useStartValue;
                }
                if (a.actionType < 0 || a.actionType > 5)
                    add(Issue::error, g + " " + q(c.id) + U(": invalid actionType ") + juce::String(a.actionType) + ".");
            }
            if (anyRot && ! anyStart)
                add(Issue::warn, g + " " + q(c.id) + ": rotation without start value acts relatively and accumulates in the loop.");
        }
    }

    if (durationSeconds > 0)
    {
        for (size_t gi = 0; gi < scene.timelines.size(); ++gi)
        {
            auto chk = [&] (const ClipBase& c)
            {
                if ((double) c.end() > durationSeconds * 1000.0 + 0.5)
                    add(Issue::warn, "Group " + juce::String((int) gi + 1) + " " + q(c.id) + U(" extends beyond the form duration."));
            };
            for (auto& c : scene.timelines[gi].movement) chk(c);
            for (auto& c : scene.timelines[gi].actions) chk(c);
        }
    }
    return issues;
}
} // namespace choreo
