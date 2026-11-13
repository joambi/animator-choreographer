#include "EncoderImport.h"

namespace choreo
{
EncoderImport readEncoderPreset(const juce::String& xmlText)
{
    EncoderImport res;
    auto doc = juce::parseXML(xmlText);
    if (doc == nullptr) { res.error = "The file is not readable XML."; return res; }
    if (doc->getTagName() != "EncoderPreset") { res.error = "Not an encoder preset: root element is <" + doc->getTagName() + ">, expected <EncoderPreset>."; return res; }
    auto* set = doc->getChildByName("AmbiSourceSet");
    if (set == nullptr) { res.error = "The preset contains no AmbiSourceSet."; return res; }
    if (auto* ds = set->getChildByName("DistanceScaler"))
        if (ds->hasAttribute("Factor") && ds->getDoubleAttribute("Factor") > 0) res.distanceScaler = ds->getDoubleAttribute("Factor");
    if (auto* groups = set->getChildByName("Groups"))
        for (auto* g : groups->getChildWithTagNameIterator("Group"))
        {
            EncoderGroup eg;
            eg.name = g->getStringAttribute("Name");
            eg.enabled = g->getIntAttribute("Enabled", 1) != 0;
            eg.x = g->getDoubleAttribute("X"); eg.y = g->getDoubleAttribute("Y"); eg.z = g->getDoubleAttribute("Z");
            if (auto* st = g->getChildByName("Stretch")) eg.stretch = st->getDoubleAttribute("Factor", 1.0);
            if (auto* sp = g->getChildByName("SubPoints")) eg.sources = sp->getNumChildElements();
            if (eg.enabled && eg.sources > 0) res.groups.push_back(eg);
        }
    if (res.groups.empty()) res.error = "No enabled group with sources found.";
    else if (res.groups.size() > 8) { res.notes.add("More than 8 groups: only the first 8 are used."); res.groups.resize(8); }
    return res;
}
} // namespace choreo
