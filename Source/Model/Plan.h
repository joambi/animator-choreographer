#pragma once
#include "Scene.h"

namespace choreo
{
// Ortsangabe in Hörbegriffen: Richtung (Grad, 0 = vorne, im Uhrzeigersinn), Entfernung als Anteil des
// Radius »fern«, Höhe in Grad.
struct PosSpec
{
    double az = 0, dist = 0.65, elev = 0;
};

struct Move
{
    juce::String type = "ruhen";
    PosSpec to { 0, 0.65, 0 };
    double turns = 1;
    juce::String dir = "cw";
    double toDist = 0.35, rise = 1.5, reps = 2, petals = 3, a = 3, b = 2, size = 0.6, amount = 0.4, speed = 3, seed = 1;
    std::vector<PosSpec> stations { { 90, 0.65, 0 }, { 180, 0.65, 0 }, { 270, 0.65, 0 } };
    bool smooth = true, closed = false;
};

struct Act
{
    double rotZ = 0;
    std::optional<double> tiltX, tiltY, stretch; // leer = unverändert
    double jitter = 0, jitterSpeed = 1;
};

struct Cell
{
    Move move;
    double onset = 0, span = 1;
    juce::String ease = "inout";
    Act act;
};

struct Relation
{
    juce::String type = "frei";
    int leader = 0;
    double delay = 2;
};

struct Group
{
    juce::String name, role;
    std::optional<double> baseStretch; // group Stretch in the encoder; empty = Plan::baseStretch
    PosSpec start;
    Relation relation;
    std::vector<Cell> cells;
};

struct Section
{
    juce::String name;
    double dur = 10;
};

struct Plan
{
    juce::String title = "New choreography";
    double radius = 3;
    std::optional<double> distanceScaler;
    juce::String encoderName;                // label for the export file name (e.g. Encoder_01)
    std::optional<double> maxDistance;       // export: scale so the farthest point reaches this distance
    double baseStretch = 1;   // group stretch in the encoder; start and neutral value of all Width actions
    bool holdRests = true;
    std::vector<Section> sections;
    std::vector<Group> groups;
};

// ------------------------------------------------------------------ Kataloge
struct Option
{
    juce::String key;
    juce::String label;
};
struct NumOption
{
    double v;
    juce::String label;
};

extern const juce::uint32 groupColours[8];
juce::Colour groupColour(int gi);
const std::vector<NumOption>& directions();
const std::vector<NumOption>& distances();
const std::vector<NumOption>& heights();
const juce::StringArray& roles();

struct EaseDef
{
    juce::String key, label;
    bool on;
    double x1, y1, x2, y2;
};
const std::vector<EaseDef>& eases();
const EaseDef& easeFor(const juce::String& key);

struct GestureDef
{
    juce::String key, label, hint;
};
const std::vector<GestureDef>& gestures();
const GestureDef* gestureFor(const juce::String& key);

struct RelationDef
{
    juce::String key, label;
    int mx, my;
    bool mirror;
};
const std::vector<RelationDef>& relations();
const RelationDef& relationFor(const juce::String& key);

struct PresetDef
{
    juce::String key, label;
};
const std::vector<PresetDef>& presets();
Plan buildPreset(const juce::String& key);

// ------------------------------------------------------------------ Hilfen
Cell defaultCell();
Group defaultGroup(int i);
void normalizePlan(Plan&);
std::vector<double> sectionStarts(const Plan&);
double totalDuration(const Plan&);
V3 specToXYZ(const PosSpec&, double R);
double nearestDirection(double az);

// JSON wie die Web-App: { format: "raumchoreographie", version: 1, plan: {...} }
juce::String planToJson(const Plan&);
std::optional<Plan> planFromJson(const juce::String& text, juce::String& error);

juce::String slug(const juce::String&);
} // namespace choreo
