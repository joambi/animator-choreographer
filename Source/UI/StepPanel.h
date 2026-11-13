#pragma once
#include "AppState.h"
#include "Widgets.h"
#include <map>

namespace ui
{
struct PanelBuilder;

struct PanelHost
{
    virtual ~PanelHost() = default;
    virtual void planChanged(bool rerenderPanel) = 0; // sofort neu berechnen
    virtual void planEdited() = 0;                    // Texteingabe: verzögert neu berechnen
    virtual void gotoStep(int step) = 0;
    virtual void exportXml() = 0;
    virtual void importRegions() = 0; // REAPER regions/markers CSV
    virtual void importEncoder() = 0; // AmbiEncoder preset XML
    virtual void copyXml() = 0;
    virtual void leaveImport() = 0;
    virtual void seek(double t) = 0;
};

class StepButton : public juce::Button
{
public:
    StepButton(int index, const juce::String& label);
    void paintButton(juce::Graphics&, bool over, bool down) override;
    int preferredWidth() const;
    juce::String badge;
    bool badgeWarn = false;

private:
    int index;
    juce::String label;
};

class StepPanel : public juce::Component, private juce::AsyncUpdater
{
public:
    StepPanel(AppState&, PanelHost&);
    ~StepPanel() override;
    void resized() override;
    void paint(juce::Graphics&) override;

    void requestRebuild(bool keepScroll = true); // asynchron
    void rebuildNow(bool keepScroll);
    void relayout();
    void updateBadges();
    void updateLiveTexts(); // z. B. Gesamtdauer
    void scrollToTop() { viewport.setViewPosition(0, 0); }

    std::map<juce::String, bool> openState; // aufgeklappte Bereiche über Neuaufbau hinweg

private:
    void handleAsyncUpdate() override;
    AppState& S;
    PanelHost& H;
    juce::OwnedArray<StepButton> steps;
    juce::Viewport viewport;
    std::unique_ptr<Flow> content;
    bool pendingKeepScroll = true;
    std::function<void()> liveUpdate;
    std::unique_ptr<PanelBuilder> builder; // langlebig: Rückrufe der Eingaben verweisen darauf

    friend struct PanelBuilder;
};
} // namespace ui
