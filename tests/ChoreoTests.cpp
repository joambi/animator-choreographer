// Konsolentest: Vorlagen exportieren, Plan-JSON-Rundlauf, Import/Export-Rundlauf einer Animator-Datei.
// Aufruf: ChoreoTests <ausgabeordner> [animator-export.xml ...]
#include "Model/EncoderImport.h"
#include "Model/RegionImport.h"
#include <JuceHeader.h>
#include "Model/Generator.h"
#include "Model/Preview.h"
#include "AI/PlanAI.h"

using namespace choreo;

static int failures = 0;
static void check(bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok    " : "  FEHLER ") << what << std::endl;
    if (! ok) ++failures;
}

int main(int argc, char* argv[])
{
    juce::File outDir = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[1])
                                 : juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("choreo-tests");
    outDir.createDirectory();

    for (auto& pd : presets())
    {
        std::cout << "Vorlage " << pd.key << std::endl;
        auto plan = buildPreset(pd.key);
        auto scene = planToScene(plan);
        auto issues = collectPlanIssues(plan, scene);
        int err = 0, warn = 0, note = 0;
        for (auto& i : issues)
        {
            if (i.level == Issue::error) ++err; else if (i.level == Issue::warn) ++warn; else ++note;
            std::cout << "    [" << (i.level == Issue::error ? "Fehler" : i.level == Issue::warn ? "Hinweis" : "Notiz") << "] " << i.text << std::endl;
        }
        check(err == 0, "keine Fehler (" + juce::String(warn) + " Hinweise, " + juce::String(note) + " Notizen)");

        auto file = outDir.getChildFile(pd.key + ".xml");
        juce::String e;
        check(writeSceneFile(scene, file, e), "Export " + file.getFullPathName());

        // erneut einlesen und wieder ausgeben: identisch
        auto parsed = parseSceneXml(file.loadFileAsString(), e);
        check(parsed.has_value(), "Import of own file");
        if (parsed) check(sceneToXmlString(*parsed) == sceneToXmlString(scene), "XML-Rundlauf identisch");

        // Plan-JSON-Rundlauf
        auto json = planToJson(plan);
        auto plan2 = planFromJson(json, e);
        check(plan2.has_value(), "Plan-JSON lesbar");
        if (plan2)
        {
            auto scene2 = planToScene(*plan2);
            check(sceneToXmlString(scene2) == sceneToXmlString(scene), "Plan-JSON-Rundlauf ergibt dieselbe Szene");
        }
        outDir.getChildFile(pd.key + ".choreo.json").replaceWithText(json);

        // Vorschau: stetige Bahn (keine Sprünge über 25 % des Radius in 1/900 der Dauer)
        double maxJump = 0;
        for (size_t gi = 0; gi < scene.timelines.size(); ++gi)
        {
            TimelinePreview pv(scene.timelines[gi], scene.startPositions[gi]);
            V3 prev = pv.sample(0).pos;
            for (int i = 1; i <= 900; ++i)
            {
                auto p = pv.sample(scene.duration * 1000.0 * i / 900.0).pos;
                maxJump = juce::jmax(maxJump, (p - prev).length());
                prev = p;
            }
        }
        check(maxJump < 0.25 * plan.radius, "Vorschau stetig (max. Schritt " + juce::String(maxJump, 3) + ")");
    }

    // Modellformat (lokale KI): Schema gültig, jede Vorlage übersteht toAi → JSON → fromAi
    {
        std::cout << "Modellformat" << std::endl;
        const auto schemaText = juce::JSON::toString(ai::schema());
        check(juce::JSON::parse(schemaText).isObject(), U("Schema is valid JSON"));
        check(ai::systemPrompt().contains("\"sections\""), U("Prompt contains example plan"));
        for (auto& pd : presets())
        {
            auto plan = buildPreset(pd.key);
            const auto text = juce::JSON::toString(ai::toAi(plan, "Test"));
            juce::String concept, err;
            auto back = ai::fromAi(juce::JSON::parse(text), concept, err);
            check(back.has_value(), "Vorlage " + pd.key + U(" read back ") + err);
            if (! back) continue;
            check(back->groups.size() == plan.groups.size() && back->sections.size() == plan.sections.size(),
                  "Vorlage " + pd.key + ": groups and sections preserved");
            int errs = 0;
            for (auto& i : collectPlanIssues(*back, planToScene(*back))) if (i.level == Issue::error) ++errs;
            check(errs == 0, "Vorlage " + pd.key + ": ohne Fehler");
            const bool same = sceneToXmlString(planToScene(*back)) == sceneToXmlString(planToScene(plan));
            std::cout << "    " << pd.key << (same ? ": Szene identisch" : ": Szene auf Raster gerundet") << std::endl;
        }
        // Namen mit Nummerierung werden gekürzt, unbekannte Gesten abgelehnt
        auto j = ai::toAi(buildPreset("explosion"), {});
        auto* secs = j.getProperty("sections", {}).getArray();
        secs->getReference(0).getDynamicObject()->setProperty("name", "Phase 1: Condensation");
        juce::String c, e;
        auto p1 = ai::fromAi(j, c, e);
        check(p1 && p1->sections[0].name == "Condensation", U("Prefix 'Phase 1:' removed"));
    }
    {
        // REAPER regions/markers CSV
        auto r = choreo::sectionsFromRegionCsv("type,index,name,start,end,length\nregion,1,\"Approach\",8.000000,16.000000,8.000000\nregion,2,Implosion,16.000000,24.500000,8.500000\nmarker,1,x,3.0,,\n");
        check(r.error.isEmpty() && r.sections.size() == 2 && std::abs(r.sections[1].dur - 8.5) < 1e-6 && std::abs(r.startOffset - 8.0) < 1e-6, "Region CSV: sections and offset");
        auto m = choreo::sectionsFromRegionCsv("#,Name,Start,End,Length\nM1,A,0:00.000,,\nM2,B,0:10.500,,\nM3,C,1:02.000,,\n");
        check(m.error.isEmpty() && m.sections.size() == 2 && std::abs(m.sections[1].dur - 51.5) < 1e-6, "Marker CSV (REAPER manager format)");
        {
            auto pl = choreo::buildPreset("kanon");
            auto base = choreo::planToScene(pl);
            pl.maxDistance = 15.6;
            auto sc = choreo::planToScene(pl);
            check(sc.maxDistanceFound > 0 && std::abs(sc.maxDistanceFound * sc.exportScale - 15.6) < 1e-6 && base.exportScale == 1.0, "Max distance scales the scene");
            auto sc2 = choreo::planToScene(pl);
            check(std::abs(sc2.exportScale - sc.exportScale) < 1e-9, "Max distance is deterministic");
        }
        {
            auto e = choreo::readEncoderPreset("<EncoderPreset><AmbiSourceSet><DistanceScaler Factor=\"5.7\"/><Groups>"
                "<Group Name=\"G1\" X=\"0\" Y=\"4.7\" Z=\"0\" Enabled=\"1\"><SubPoints><SubPoint Id=\"a\"/><SubPoint Id=\"b\"/></SubPoints><Stretch Factor=\"0.1624\"/></Group>"
                "<Group Name=\"G5\" X=\"0\" Y=\"0\" Z=\"0\" Enabled=\"0\"><SubPoints/><Stretch Factor=\"0.42\"/></Group></Groups></AmbiSourceSet></EncoderPreset>");
            check(e.error.isEmpty() && e.groups.size() == 1 && std::abs(e.groups[0].stretch - 0.1624) < 1e-9 && e.distanceScaler && std::abs(*e.distanceScaler - 5.7) < 1e-9, "Encoder preset: groups, stretch, scaler");
        }
        auto g = choreo::sectionsFromRegionCsv("type,index,name,start,end,length\nregion,1,A,1.0,5.0,4.0\nregion,2,B,23.0,30.0,7.0\n");
        check(g.sections.size() == 3 && g.sections[1].name == "(gap)" && std::abs(g.sections[1].dur - 18.0) < 1e-6, "Region CSV: gap becomes section");
        check(choreo::sectionsFromRegionCsv("foo\nbar").error.isNotEmpty(), "Region CSV: invalid file rejected");
    }

    for (int a = 2; a < argc; ++a)
    {
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile(argv[a]);
        if (f.getFileName().endsWith(".choreo.json"))
        {
            // Plan → Szene (für den Vergleich mit der Web-App)
            juce::String e;
            auto plan = planFromJson(f.loadFileAsString(), e);
            check(plan.has_value(), "Plan " + f.getFileName() + " " + e);
            if (! plan) continue;
            auto scene = planToScene(*plan);
            writeSceneFile(scene, outDir.getChildFile(f.getFileName().upToFirstOccurrenceOf(".", false, false) + ".xml"), e);
            continue;
        }
        std::cout << "Datei " << f.getFileName() << std::endl;
        juce::String e;
        auto sc = parseSceneXml(f.loadFileAsString(), e);
        check(sc.has_value(), "Import " + e);
        if (! sc) continue;
        int nm = 0, na = 0;
        for (auto& tl : sc->timelines) { nm += (int) tl.movement.size(); na += (int) tl.actions.size(); }
        std::cout << "    " << sc->timelines.size() << " Timelines, " << nm << " Movement, " << na << " Action" << std::endl;
        for (auto& i : validate(*sc, 0)) std::cout << "    [" << (i.level == Issue::error ? "Fehler" : "Hinweis") << "] " << i.text << std::endl;
        auto orig = juce::parseXML(f);
        check(orig != nullptr && orig->toString() == sceneToXmlString(*sc), "Import/export round trip identical to original");
        auto out = outDir.getChildFile("roundtrip-" + f.getFileName());
        check(writeSceneFile(*sc, out, e), "Rundlauf geschrieben " + out.getFullPathName());
    }

    std::cout << (failures == 0 ? "Alle Tests bestanden" : juce::String(failures) + " Fehler") << std::endl;
    return failures == 0 ? 0 : 1;
}
