#pragma once
#include "Plan.h"

namespace choreo
{
// Sections from a REAPER regions/markers CSV (see reaper/Animator_export_regions_markers.lua).
// Regions become sections; gaps between regions become '(gap)' sections.
// Without regions, consecutive markers delimit the sections.
struct RegionImport
{
    std::vector<Section> sections;
    double startOffset = 0;   // project time of the first section (seconds)
    juce::StringArray notes;
    juce::String error;       // empty = ok
};

RegionImport sectionsFromRegionCsv(const juce::String& csv);
juce::String formatTimecode(double seconds); // m:ss.mmm
} // namespace choreo
