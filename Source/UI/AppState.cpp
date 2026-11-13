#include "AppState.h"

namespace ui
{
using namespace choreo;

juce::String fmt(double v, int d)
{
    const double f = std::pow(10.0, d);
    double r = std::floor(v * f + 0.5) / f;
    if (r == 0) r = 0; // -0 vermeiden
    return juce::String(r, d);
}

void AppState::rebuild()
{
    if (! importMode)
    {
        normalizePlan(plan);
        scene = planToScene(plan);
        duration = juce::jmax(0.1, scene.duration);
    }
    else
    {
        juce::int64 end = 0;
        for (auto& tl : imported.timelines)
        {
            for (auto& c : tl.movement) end = juce::jmax(end, c.end());
            for (auto& c : tl.actions) end = juce::jmax(end, c.end());
        }
        duration = juce::jmax(1.0, (double) end / 1000.0 + 0.5);
    }
    const auto& sc = current();
    prep.clear();
    paths.clear();
    for (size_t i = 0; i < sc.timelines.size(); ++i)
    {
        std::optional<V3> init;
        if (i < sc.startPositions.size()) init = sc.startPositions[i];
        prep.push_back(std::make_unique<TimelinePreview>(sc.timelines[i], init));
    }
    const int N = 900;
    const double dt = duration * 1000.0 / N;
    for (auto& p : prep)
    {
        std::vector<V3> pts;
        pts.reserve(N + 1);
        for (int i = 0; i <= N; ++i) pts.push_back(p->sample(i * dt).pos);
        paths.push_back(std::move(pts));
    }
    t = juce::jmin(t, duration);
}

std::vector<Issue> AppState::issues() const
{
    if (importMode) return validate(imported, 0);
    return collectPlanIssues(plan, scene);
}

double AppState::roomRadius() const
{
    if (! importMode) return plan.radius > 0 ? plan.radius : 3.0;
    double m = 1;
    for (auto& pts : paths)
        for (auto& p : pts) m = juce::jmax(m, std::hypot(p.x, p.y));
    return m / 1.1;
}

juce::StringArray AppState::groupNames() const
{
    juce::StringArray n;
    if (! importMode)
        for (auto& g : plan.groups) n.add(g.name);
    else
        for (size_t i = 0; i < imported.timelines.size(); ++i) n.add("Timeline " + juce::String((int) i + 1));
    return n;
}

bool AppState::nonEmpty(int gi) const
{
    const auto& tl = current().timelines[(size_t) gi];
    return ! tl.movement.empty() || ! tl.actions.empty();
}
} // namespace ui
