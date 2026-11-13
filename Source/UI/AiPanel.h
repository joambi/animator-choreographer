#pragma once
#include "Widgets.h"
#include "AI/PlanAI.h"
#include "AI/PromptPlanner.h"

namespace ui
{
// Überlagerndes Fenster: Choreographie in Worten beschreiben, Plan durch ein lokales Modell
// (Ollama) entwerfen lassen, Ergebnis prüfen und übernehmen.
class AiPanel : public juce::Component
{
public:
    AiPanel();
    ~AiPanel() override;

    // wird mit dem geprüften Plan aufgerufen, wenn »Übernehmen« gedrückt wird
    std::function<void(const choreo::Plan&)> onApply;
    std::function<void()> onClose;

    void open();               // zeigen, Server und Modelle abfragen
    void close();
    bool isBusy() const { return worker != nullptr; }

    // für Bildschirmfotos und Tests
    void setDescription(const juce::String&);
    void showResultForTest(const choreo::ai::Result&);

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    class Worker;
    class Probe;
    enum class Phase { idle, probing, running, done };

    void start();
    void startRules();   // Entwurf nach Regeln, ohne Sprachmodell
    void stop();
    void apply();
    void refreshModels();
    void setStatus(const juce::String& text, juce::Colour colour);
    void showProgress(const choreo::ai::Progress&, double elapsed);
    void showResult(choreo::ai::Result);
    void probeFinished(const juce::String& version, const juce::StringArray& models, const juce::String& error);
    void updateButtons();
    void saveSettings();
    choreo::ai::Settings currentSettings() const;
    choreo::ai::Hints currentHints() const;
    juce::Rectangle<int> cardBounds() const;
    int descHeight() const;

    Phase phase = Phase::idle;
    std::unique_ptr<juce::PropertiesFile> props;

    TextBlock title, intro, status, result;
    juce::TextEditor description;
    TextBlock lblGroups, lblDur, lblModel;
    juce::ComboBox groups, duration, model;
    juce::TextButton btnRefresh, btnClose, btnStop, btnRun, btnApply, btnRules;
    juce::ToggleButton think;
    juce::Viewport resultView;
    struct ResultHolder : juce::Component
    {
        TextBlock* block = nullptr;
        void resized() override { if (block) block->setBounds(getLocalBounds()); }
    } resultHolder;

    std::unique_ptr<Worker> worker;
    std::unique_ptr<Probe> probe;
    std::optional<choreo::Plan> pending;
    juce::String serverInfo;
};
} // namespace ui
