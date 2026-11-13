#pragma once
#include "StepPanel.h"
#include "AiPanel.h"
#include "StageViews.h"

namespace ui
{
class Toast : public juce::Component, private juce::Timer
{
public:
    Toast() { setInterceptsMouseClicks(false, false); setAlwaysOnTop(true); }
    void show(const juce::String& msg);
    void paint(juce::Graphics&) override;
    int preferredWidth() const;
    juce::String message;

private:
    void timerCallback() override;
};

class MainComponent : public juce::Component, public PanelHost, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

    // PanelHost
    void planChanged(bool rerenderPanel) override;
    void planEdited() override;
    void gotoStep(int step) override;
    void exportXml() override;
    void importRegions() override;
    void importEncoder() override;
    void copyXml() override;
    void leaveImport() override;
    void seek(double t) override;

    // für Kommandozeile / Tests
    void loadPreset(const juce::String& key, bool announce = true);
    bool loadSceneFile(const juce::File&, bool announce = true);
    bool loadPlanFile(const juce::File&, bool announce = true);
    void applyPlan(const choreo::Plan&, const juce::String& announcement);
    AiPanel& getAiPanel() { return aiPanel; }
    void openAiPanel();
    void setStepDirect(int step);
    void setTimeDirect(double t);
    void setSectionDirect(int s);
    void setOpen(const juce::String& key, bool open) { panel.openState[key] = open; }
    StepPanel& getPanel() { return panel; }

private:
    void timerCallback() override;
    void setPlaying(bool);
    void refreshStage();
    void savePlan();
    void openPlan();
    void openXml();
    void importXml();
    void writeExport(const juce::File&);

    AppState S;
    StepPanel panel { S, *this };
    TopView topView { S };
    SideView sideView { S };
    Legend legend { S };
    ViewFrame topFrame, sideFrame;
    Transport transport { S };
    TimelineView timeline { S };
    TextBlock stageNote;
    juce::ComboBox presetBox;
    juce::TextButton btnDescribe { "Describe" };
    AiPanel aiPanel;
    juce::TextButton btnOpen { U8("Open plan") }, btnSave { "Save plan" }, btnCheck { U8("Review XML") }, btnExport { "Export XML" };
    Toast toast;
    std::unique_ptr<juce::FileChooser> chooser;
    double lastTick = 0;
    int panelWidth = 500;

    struct Debounce : juce::Timer
    {
        std::function<void()> fn;
        void timerCallback() override { stopTimer(); if (fn) fn(); }
    } debounce;

    static juce::String U8(const char* s) { return juce::String::fromUTF8(s); }
};
} // namespace ui
