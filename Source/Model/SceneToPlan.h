#pragma once
#include "Plan.h"

namespace choreo
{
// Naeherung: Animator-Szene (Clips) -> bearbeitbarer Plan (Gesten). Verlustbehaftet: Beziehungen
// (Kanon, Spiegel ...) sind in der Szene schon eingerechnet und werden als "unabhaengig" uebernommen.
struct SceneToPlanResult
{
    Plan plan;
    juce::StringArray notes;
    int clipsTotal = 0;      // Bewegungsclips ohne Halte-/Anlauf-Clips
    int clipsConverted = 0;  // davon als Geste uebernommen
};

SceneToPlanResult sceneToPlan(const Scene&, const juce::String& title);
} // namespace choreo
