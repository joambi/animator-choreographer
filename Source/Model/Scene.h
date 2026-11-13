#pragma once
#include <JuceHeader.h>
#include <optional>
#include <vector>

// Original-Datenmodell des ICST Animators (GPLv3, Version 3.2.0.9)
#include "TimelineModel.h"

namespace choreo
{
using V3 = juce::Vector3D<double>;

// UTF-8-Literal (juce::String(const char*) erwartet ASCII)
inline juce::String U(const char* s) { return juce::String::fromUTF8(s); }

// Arbeitskopien der Animator-Clips. Sie sind frei kopier- und sortierbar; für Berechnung und
// Export werden sie in die Originalstrukturen MovementClip/ActionClip übersetzt.
struct Waypoint
{
    double x = 0, y = 0, z = 0;
    bool newSegment = false;
};

struct ClipBase
{
    juce::String id;
    juce::int64 start = 0, length = 250;
    juce::uint32 colour = 0xff6495ed;
    bool muted = false, palindrome = false;
    int repetitions = 1;
    bool easingEnabled = false;
    double easeX1 = 1.0 / 3.0, easeY1 = 1.0 / 3.0, easeX2 = 2.0 / 3.0, easeY2 = 2.0 / 3.0;
    bool easePerRepetition = true, easePerDirection = true;
    juce::int64 end() const { return start + length; }
};

struct MClip : ClipBase
{
    int movementType = 0;
    V3 sp, tp;
    bool useStartPoint = false;
    double count = 1, radiusChange = 0, tension = 0;
    bool closedPath = false;
    double heightRise = 0, freqRatioA = 3, freqRatioB = 2, phaseDeg = 0;
    int randomSeed = 1;
    std::vector<Waypoint> waypoints;
};

struct ADef
{
    int actionType = 0, timingType = 0;
    double value = 0, startValue = 0;
    bool useStartValue = false;
    double jitterSpeed = 1;
};

struct AClip : ClipBase
{
    std::vector<ADef> actions;
};

struct Timeline
{
    std::vector<MClip> movement;
    std::vector<AClip> actions;
};

struct Scene
{
    std::optional<double> distanceScaler;
    std::vector<Timeline> timelines;
    std::vector<V3> startPositions; // leer bei importierten Szenen
    std::vector<juce::StringArray> notes;
    double duration = 0;            // s
    double exportScale = 1;         // applied by planToScene (Plan::maxDistance)
    double maxDistanceFound = 0;    // farthest point before scaling
};

// Übersetzung in die Originalstrukturen und zurück
MovementClip toIcst(const MClip&);
ActionClip toIcst(const AClip&);
MClip fromIcst(const MovementClip&);
AClip fromIcst(const ActionClip&);

// Endpunkt eines Clips nach der Formel des Animators (AnimatorMath::calculatePosition bei Fortschritt 1)
V3 endPosition(const MClip&, V3 reference);

inline double round6(double v) { return std::floor(v * 1e6 + 0.5) / 1e6 + 0.0; }
inline V3 r6v(V3 p) { return { round6(p.x), round6(p.y), round6(p.z) }; }
inline double jsRound(double v) { return std::floor(v + 0.5); }

bool isHoldClip(const MClip&);

// XML wie AnimatorMainView::exportAllScenes / importScenes
std::unique_ptr<juce::XmlElement> sceneToXml(const Scene&);
juce::String sceneToXmlString(const Scene&);
bool writeSceneFile(const Scene&, const juce::File&, juce::String& error);
std::optional<Scene> parseSceneXml(const juce::String& text, juce::String& error);

// Prüfung
struct Issue
{
    enum Level { error, warn, note } level;
    juce::String text;
};
std::vector<Issue> validate(const Scene&, double durationSeconds /* 0 = keine Formdauer */);

extern const char* const movementTypeNames[10];
} // namespace choreo
