#pragma once
#include "Model/Plan.h"

namespace choreo
{
// Regelbasierter Planer: deutscher Freitext -> Plan (Abschnitte, Gruppen, Gesten, Beziehungen).
// Kein Netzwerk, kein Schlüssel; gleiche Eingabe ergibt immer denselben Plan.
struct PlannerOptions
{
    int groups = 4;         // 1..16
    double duration = 30;   // Sekunden, gesamt
    juce::String title;     // leer = aus dem Prompt
};

struct PlannerResult
{
    Plan plan;
    juce::StringArray log;  // was erkannt wurde (Phasen, Modifikatoren, Annahmen)
};

PlannerResult planFromPrompt(const juce::String& prompt, const PlannerOptions& opt);

// Lesbare Übersicht der erkannten Begriffe (für --list)
juce::StringArray plannerVocabulary();
} // namespace choreo
