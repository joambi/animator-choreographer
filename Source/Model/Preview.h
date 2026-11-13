#pragma once
#include "Scene.h"
#include <vector>
#include <array>

namespace choreo
{
// Vorschau einer Timeline mit den Originalformeln (AnimatorMath). Bewegungen werden Clip für Clip
// verkettet: ein Clip ohne festen Startpunkt beginnt dort, wo der vorige endet.
struct Sample
{
    V3 pos;
    juce::Quaternion<double> q { juce::Vector3D<double>(0, 0, 0), 1.0 };
    double stretch = 1, jitter = 0, jitterSpeed = 1;
    const MClip* activeMove = nullptr;
    const AClip* activeAct = nullptr;
};

class TimelinePreview
{
public:
    TimelinePreview(const Timeline& tl, std::optional<V3> initialPos);
    ~TimelinePreview();
    TimelinePreview(const TimelinePreview&) = delete;
    TimelinePreview& operator=(const TimelinePreview&) = delete;
    Sample sample(double tMs) const;

private:
    struct PreparedMove;
    struct PreparedAct;
    std::vector<PreparedMove> moves; // nach dem Konstruktor unverändert, Zeiger bleiben gültig
    std::vector<PreparedAct> acts;
    V3 firstPos;
};

V3 rotate(const juce::Quaternion<double>& q, V3 v);

// Schematische Quellenkonstellation: vier Punkte um das Gruppenzentrum, zeigt Drehung, Weite, Zittern
std::array<V3, 4> satellites(const Sample&, double R, int gi, double tSec);
} // namespace choreo
