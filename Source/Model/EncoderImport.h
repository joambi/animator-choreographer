#pragma once
#include "Plan.h"

namespace choreo
{
// Reads an AmbiEncoder preset (<EncoderPreset>, "import/export" in the encoder's Sources panel).
struct EncoderGroup
{
    juce::String name;
    bool enabled = false;
    int sources = 0;          // number of SubPoints
    double stretch = 1;       // <Stretch Factor>
    double x = 0, y = 0, z = 0; // group point in encoder units
};

struct EncoderImport
{
    std::optional<double> distanceScaler;
    std::vector<EncoderGroup> groups; // only groups that are enabled and have sources
    juce::StringArray notes;
    juce::String error;
};

EncoderImport readEncoderPreset(const juce::String& xmlText);
} // namespace choreo
