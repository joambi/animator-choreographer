// Plan aus Beschreibung über ein lokales Sprachmodell (Ollama), ohne Oberfläche.
//
//   ChoreoAI [Optionen] "Beschreibung" plan.choreo.json [szene.xml]
//
//   --model NAME     Modell (Vorgabe gemma4:12b)
//   --host H --port P  Ollama-Server (Vorgabe 127.0.0.1:11434)
//   --think          Denkphase einschalten (gründlicher, langsamer)
//   --groups N       genau N Gruppen
//   --dur S          Gesamtdauer etwa S Sekunden
//   --list           installierte Modelle anzeigen
//   --rules          Entwurf nach Regeln statt per Sprachmodell (ohne Ollama; automatisch, wenn Ollama fehlt)
//   --rules-list     Stichwörter des Regelplaners anzeigen
//   --print-prompt   Anweisung an das Modell ausgeben
//   --print-schema   JSON-Schema ausgeben
#include <JuceHeader.h>
#include "AI/PlanAI.h"
#include "AI/PromptPlanner.h"

using namespace choreo;

int main(int argc, char* argv[])
{
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add(juce::String::fromUTF8(argv[i]));

    ai::Settings st;
    ai::Hints hints;
    juce::StringArray pos;
    bool rules = false;
    for (int i = 0; i < args.size(); ++i)
    {
        const auto& a = args[i];
        auto next = [&] { return i + 1 < args.size() ? args[++i] : juce::String(); };
        if (a == "--model") st.model = next();
        else if (a == "--host") st.host = next();
        else if (a == "--port") st.port = next().getIntValue();
        else if (a == "--think") st.think = true;
        else if (a == "--rules") rules = true;
        else if (a == "--rules-list") { for (auto& l : plannerVocabulary()) std::cout << l << std::endl; return 0; }
        else if (a == "--groups") hints.groups = next().getIntValue();
        else if (a == "--dur") hints.duration = next().getDoubleValue();
        else if (a == "--print-prompt") { std::cout << ai::systemPrompt() << std::endl; return 0; }
        else if (a == "--print-schema") { std::cout << juce::JSON::toString(ai::schema()) << std::endl; return 0; }
        else if (a == "--list")
        {
            juce::String err;
            const auto ver = ai::serverVersion(st, err);
            if (err.isNotEmpty()) { std::cerr << err << std::endl; return 1; }
            std::cout << "Ollama " << ver << std::endl;
            for (auto& m : ai::listModels(st, err)) std::cout << "  " << m << std::endl;
            return 0;
        }
        else pos.add(a);
    }
    if (pos.size() < 2)
    {
        std::cerr << "Usage: ChoreoAI [--model NAME] [--think] [--rules] [--groups N] [--dur S] \"description\" plan.choreo.json [scene.xml]" << std::endl;
        return 2;
    }

    std::atomic<bool> cancel { false };
    int lastRound = 0, lastShown = -1;
    ai::Result res;
    auto runRules = [&]
    {
        PlannerOptions o;
        if (hints.groups > 0) o.groups = hints.groups;
        if (hints.duration > 0) o.duration = hints.duration;
        auto planned = planFromPrompt(pos[0], o);
        for (auto& l : planned.log) std::cout << "  " << l << std::endl;
        auto plan = planned.plan;
        auto scene = planToScene(plan);
        res.plan = plan;
        res.issues = collectPlanIssues(plan, scene);
        res.concept = U("Entwurf nach Regeln (ohne Sprachmodell).");
    };

    if (rules)
        runRules();
    else
    {
        res = ai::generate(st, pos[0], hints, cancel, [&] (const ai::Progress& p) {
            if (p.round != lastRound)
            {
                if (lastRound) std::cout << std::endl;
                std::cout << p.phase << " (round " << p.round << ") " << std::flush;
                lastRound = p.round;
                lastShown = -1;
            }
            const int k = (p.contentChars + p.thinkingChars) / 400;
            if (k != lastShown) { std::cout << "." << std::flush; lastShown = k; }
        });
        std::cout << std::endl;

        if (! res.plan && res.error.contains(U("not reachable")))
        {
            std::cerr << res.error << std::endl;
            std::cerr << U("Continuing with the rule planner (no language model).") << std::endl;
            runRules();
        }
    }

    if (! res.plan)
    {
        std::cerr << "Error: " << res.error << std::endl;
        if (res.raw.isNotEmpty()) std::cerr << "Last answer:\n" << res.raw << std::endl;
        return 1;
    }
    auto plan = *res.plan;
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(pos[1]);
    out.replaceWithText(planToJson(plan));
    std::cout << "Plan: " << out.getFullPathName() << std::endl;
    if (pos.size() > 2)
    {
        auto scene = planToScene(plan);
        const juce::File xml = juce::File::getCurrentWorkingDirectory().getChildFile(pos[2]);
        xml.replaceWithText(sceneToXmlString(scene));
        std::cout << "Scene: " << xml.getFullPathName() << std::endl;
    }

    std::cout << "\n" << plan.title << " (" << juce::String(totalDuration(plan), 1) << " s, " << (int) plan.groups.size()
              << " Gruppen, " << res.rounds << " Runde(n), " << juce::String(res.seconds, 1) << " s Rechenzeit)" << std::endl;
    if (res.concept.isNotEmpty()) std::cout << res.concept << std::endl;
    std::cout << "\nSections:";
    for (auto& s : plan.sections) std::cout << "  " << s.name << " " << juce::String(s.dur, 1) << " s";
    std::cout << std::endl;
    for (auto& g : plan.groups)
    {
        std::cout << "  " << g.name << " [" << g.relation.type << "]:";
        if (g.relation.type == "frei")
            for (auto& c : g.cells) std::cout << " " << c.move.type;
        std::cout << std::endl;
    }
    int errors = 0;
    for (auto& i : res.issues)
    {
        const char* lvl = i.level == Issue::error ? "Fehler" : i.level == Issue::warn ? "Hinweis" : "Notiz";
        if (i.level == Issue::error) ++errors;
        std::cout << "  [" << lvl << "] " << i.text << std::endl;
    }
    if (res.issues.empty()) std::cout << U("Review: all clear") << std::endl;
    return errors ? 3 : 0;
}
