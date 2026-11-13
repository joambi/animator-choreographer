#pragma once
#include "Model/Generator.h"
#include <atomic>
#include <functional>

// Entwurf eines Plans aus einer Beschreibung durch ein lokales Sprachmodell (Ollama).
// Das Modell schreibt nur einen Plan in Hörbegriffen (Richtung, Abstand, Höhe, Geste, Kurve); die
// Szene erzeugt danach der Generator. Ein JSON-Schema beschränkt die Antwort auf gültige Werte,
// die Prüfung der App meldet Fehler zur Korrektur an das Modell zurück.
namespace choreo::ai
{
struct Settings
{
    juce::String host = "127.0.0.1";
    int port = 11434;
    juce::String model = "gemma4:12b";
    bool think = false;          // Denkphase des Modells (gründlicher, deutlich langsamer)
    double temperature = 0.4;
    int numCtx = 16384;          // Kontextlänge: Anweisung + Beispiel + Plan + Korrekturrunde
    int maxRounds = 3;           // erster Entwurf + höchstens zwei Korrekturen
    int timeoutMs = 600000;      // je Anfrage
};

struct Hints
{
    int groups = 0;        // 0 = Modell wählt
    double duration = 0;   // Gesamtdauer in s, 0 = Modell wählt
};

struct Progress
{
    int round = 1;
    int contentChars = 0;
    int thinkingChars = 0;
    juce::String phase;
};

struct Result
{
    std::optional<Plan> plan;
    juce::String concept;
    std::vector<Issue> issues;
    int rounds = 0;
    double seconds = 0;
    juce::String error;      // leer = Erfolg
    juce::String raw;        // letzte Modellantwort
    bool cancelled = false;
};

// Bausteine (auch für Tests und die Kommandozeile)
juce::var schema();
juce::String systemPrompt();
juce::String userPrompt(const juce::String& description, const Hints&);
std::optional<Plan> fromAi(const juce::var& j, juce::String& concept, juce::String& error);
juce::var toAi(const Plan&, const juce::String& concept);

// Minimaler HTTP-Client für den lokalen Server (HTTP/1.0, ohne externe Bibliotheken).
struct Http
{
    int status = 0;
    juce::String body;
    juce::String error;
};
Http request(const Settings&, const juce::String& method, const juce::String& path, const juce::String& body,
             int timeoutMs, std::atomic<bool>* cancel = nullptr,
             std::function<void(const juce::String& line)> onLine = {});

juce::String serverVersion(const Settings&, juce::String& error);
juce::StringArray listModels(const Settings&, juce::String& error);

Result generate(const Settings&, const juce::String& description, const Hints&, std::atomic<bool>& cancel,
                std::function<void(const Progress&)> onProgress = {});
} // namespace choreo::ai
