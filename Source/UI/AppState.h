#pragma once
#include "../Model/Generator.h"
#include "../Model/Preview.h"

namespace ui
{
// Zustand wie das Objekt S der Web-App
struct AppState
{
    choreo::Plan plan = choreo::buildPreset("kanon");
    int step = 0, section = 0;
    bool importMode = false;
    juce::String importName;
    choreo::Scene imported;
    choreo::Scene scene;

    std::vector<std::unique_ptr<choreo::TimelinePreview>> prep;
    std::vector<std::vector<choreo::V3>> paths;
    double duration = 1, t = 0;
    bool playing = false, loop = true;
    double speed = 1;

    const choreo::Scene& current() const { return importMode ? imported : scene; }
    void rebuild();
    std::vector<choreo::Issue> issues() const;
    double roomRadius() const;
    juce::StringArray groupNames() const;
    bool nonEmpty(int gi) const;
    choreo::Sample sample(int gi) const { return prep[(size_t) gi]->sample(t * 1000.0); }
};

juce::String fmt(double v, int decimals); // wie fmt der Web-App (toFixed nach Rundung)
} // namespace ui
