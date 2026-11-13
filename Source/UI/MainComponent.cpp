#include "Model/RegionImport.h"
#include "Model/EncoderImport.h"
#include "Model/SceneToPlan.h"
#include "MainComponent.h"

static constexpr int kImportXmlId = 1000; // Eintrag "Import scene XML" im Aufklappmenue

namespace ui
{
using namespace choreo;
#define T(x) choreo::U(x)

// ------------------------------------------------------------------ Toast
void Toast::show(const juce::String& msg)
{
    message = msg;
    if (auto* p = getParentComponent())
    {
        const int w = preferredWidth();
        setBounds((p->getWidth() - w) / 2, p->getHeight() - 24 - 40, w, 40);
    }
    setVisible(true);
    toFront(false);
    repaint();
    startTimer(2200);
}

int Toast::preferredWidth() const { return juce::GlyphArrangement::getStringWidthInt(sans(14.0f), message) + 34; }

void Toast::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(col::surface3);
    g.fillRoundedRectangle(r, 9.0f);
    g.setColour(col::border);
    g.drawRoundedRectangle(r, 9.0f, 1.0f);
    g.setColour(col::text);
    g.setFont(sans(14.0f));
    g.drawText(message, getLocalBounds(), juce::Justification::centred);
}

void Toast::timerCallback()
{
    stopTimer();
    setVisible(false);
}

// ------------------------------------------------------------------ Hauptansicht
MainComponent::MainComponent()
    : topFrame("Top view", "front at top", topView), sideFrame("Side view", "from the right", sideView)
{
    setWantsKeyboardFocus(true);
    setOpaque(true);
    S.rebuild();

    addAndMakeVisible(panel);
    addAndMakeVisible(topFrame);
    addAndMakeVisible(sideFrame);
    addAndMakeVisible(legend);
    addAndMakeVisible(transport);
    addAndMakeVisible(timeline);
    addAndMakeVisible(stageNote);
    addChildComponent(toast);

    presetBox.addItem(T("Load template …"), 1);
    for (size_t i = 0; i < presets().size(); ++i) presetBox.addItem(presets()[i].label, (int) i + 2);
    presetBox.addSeparator();
    presetBox.addItem(T("Import scene XML …"), kImportXmlId);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.setTitle("Load template");
    presetBox.onChange = [this] {
        const int id = presetBox.getSelectedId();
        if (id < 2) return;
        juce::Component::SafePointer<MainComponent> self(this);
        juce::MessageManager::callAsync([self, id] {
            if (self == nullptr) return;
            if (id == kImportXmlId) self->importXml();
            else self->loadPreset(presets()[(size_t) id - 2].key);
            self->presetBox.setSelectedId(1, juce::dontSendNotification);
        });
    };
    addAndMakeVisible(presetBox);

    for (auto* b : { &btnOpen, &btnSave, &btnCheck })
    {
        setVariant(*b, "ghost");
        addAndMakeVisible(b);
    }
    setVariant(btnExport, "primary");
    addAndMakeVisible(btnExport);
    setVariant(btnDescribe, "ghost");
    btnDescribe.setTooltip(T("Describe the choreography in words and let a local model draft the plan"));
    addAndMakeVisible(btnDescribe);
    btnDescribe.onClick = [this] { openAiPanel(); };
    addChildComponent(aiPanel);
    aiPanel.onApply = [this] (const Plan& p) { applyPlan(p, T("Plan taken from description")); };
    aiPanel.onClose = [this] { grabKeyboardFocus(); };

    btnOpen.onClick = [this] { openPlan(); };
    btnSave.onClick = [this] { savePlan(); };
    btnCheck.onClick = [this] { openXml(); };
    btnExport.onClick = [this] { exportXml(); };

    transport.onPlay = [this] (bool on) { setPlaying(on); };
    transport.onSeek = [this] (double t) { seek(t); };
    timeline.onSeek = [this] (double t) { seek(t); };

    debounce.fn = [this] {
        S.rebuild();
        panel.updateBadges();
        panel.updateLiveTexts();
        refreshStage();
    };

    setSize(1440, 900);
    refreshStage();
}

MainComponent::~MainComponent()
{
    stopTimer();
    debounce.stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(col::bg);
    // Kopfleiste
    g.setColour(col::bg.withAlpha(0.94f));
    g.fillRect(0, 0, getWidth(), 60);
    g.setColour(col::divider);
    g.fillRect(0, 59, getWidth(), 1);
    g.setColour(col::text);
    g.setFont(sans(18.0f, 600));
    g.drawText("Animator Choreographer", 24, 10, 400, 22, juce::Justification::centredLeft);
    g.setColour(col::textMuted);
    g.setFont(sans(12.5f));
    const int maxW = presetBox.getX() - 24 - 16;
    g.drawText(T("Scene XML for the ICST Animator · AmbiEncoder 3.2.0.9"), 24, 32, juce::jmax(0, maxW), 18, juce::Justification::centredLeft, true);
    // Trennlinie Panel/Bühne
    g.setColour(col::divider);
    g.fillRect(panelWidth, 60, 1, getHeight() - 60);
}

void MainComponent::resized()
{
    const int W = getWidth(), Hh = getHeight();
    // Kopfleiste rechts
    {
        auto r = juce::Rectangle<int>(0, 0, W, 60).reduced(24, 12);
        auto place = [&r] (juce::Component& c, int w) {
            c.setBounds(r.removeFromRight(w).withSizeKeepingCentre(w, 36));
            r.removeFromRight(8);
        };
        auto bw = [] (juce::TextButton& b, int weight) { return juce::GlyphArrangement::getStringWidthInt(sans(14.0f, weight), b.getButtonText()) + 34; };
        place(btnExport, bw(btnExport, 600));
        place(btnCheck, bw(btnCheck, 500));
        place(btnSave, bw(btnSave, 500));
        place(btnOpen, bw(btnOpen, 500));
        place(btnDescribe, bw(btnDescribe, 500));
        // Vorlagenwahl nimmt den Rest, solange Platz neben dem Titel bleibt
        place(presetBox, juce::jlimit(150, 200, r.getWidth() - 260));
    }
    panelWidth = juce::jlimit(380, 500, (int) (W * 0.36));
    panel.setBounds(0, 60, panelWidth, Hh - 60);

    // Bühne
    auto st = juce::Rectangle<int>(panelWidth + 1, 60, W - panelWidth - 1, Hh - 60);
    st = st.withTrimmedLeft(20).withTrimmedRight(20).withTrimmedTop(16).withTrimmedBottom(20);
    const int noteH = stageNote.heightFor(st.getWidth());
    stageNote.setBounds(st.removeFromBottom(noteH));
    st.removeFromBottom(12);
    const int tlH = timeline.preferredHeight();
    timeline.setBounds(st.removeFromBottom(tlH));
    st.removeFromBottom(12);
    transport.setBounds(st.removeFromBottom(36));
    st.removeFromBottom(12);
    // Ansichten: Draufsicht quadratisch, Seitenspalte 200–300 px
    const int sideW = juce::jlimit(200, 300, (st.getWidth() - 16) / 3);
    auto side = st.removeFromRight(sideW);
    st.removeFromRight(16);
    const int sq = juce::jmax(100, juce::jmin(st.getWidth(), st.getHeight() - 26));
    topFrame.setBounds(st.getX(), st.getY(), sq, sq + 26);
    const int sideH = (int) std::round(sideW * 3.0 / 4.0) + 26;
    sideFrame.setBounds(side.removeFromTop(sideH));
    side.removeFromTop(12);
    legend.setBounds(side);

    toast.setBounds((W - toast.preferredWidth()) / 2, Hh - 64, toast.preferredWidth(), 40);
    aiPanel.setBounds(getLocalBounds());
}

void MainComponent::openAiPanel()
{
    if (S.playing) setPlaying(false);
    aiPanel.setBounds(getLocalBounds());
    aiPanel.open();
}

void MainComponent::refreshStage()
{
    stageNote.setText(! S.importMode
                          ? T("Large dots: group center (size = height). Small dots: schematic source constellation, shows rotation, width and jitter. Upper track: movement, lower: action.")
                          : T("Preview of the imported file. Upper track per group: movement clips, lower: action clips."),
                      sans(12.5f), col::textFaint);
    if (timeline.getHeight() != timeline.preferredHeight()) resized();
    transport.refresh();
    topView.repaint();
    sideView.repaint();
    legend.repaint();
    timeline.repaint();
}

// ------------------------------------------------------------------ Wiedergabe
void MainComponent::setPlaying(bool on)
{
    S.playing = on;
    if (on)
    {
        if (S.t >= S.duration - 0.01) S.t = 0;
        lastTick = juce::Time::getMillisecondCounterHiRes();
        startTimerHz(60);
    }
    else
        stopTimer();
    refreshStage();
}

void MainComponent::timerCallback()
{
    if (! S.playing) { stopTimer(); return; }
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = (now - lastTick) / 1000.0;
    lastTick = now;
    S.t += dt * S.speed;
    if (S.t >= S.duration)
    {
        if (S.loop) S.t = std::fmod(S.t, S.duration);
        else
        {
            S.t = S.duration;
            setPlaying(false);
            return;
        }
    }
    refreshStage();
}

bool MainComponent::keyPressed(const juce::KeyPress& k)
{
    if (aiPanel.isVisible()) return false;
    if (k == juce::KeyPress::spaceKey)
    {
        auto* f = juce::Component::getCurrentlyFocusedComponent();
        if (dynamic_cast<juce::TextEditor*>(f) || dynamic_cast<juce::ComboBox*>(f) || dynamic_cast<juce::Button*>(f)) return false;
        setPlaying(! S.playing);
        return true;
    }
    return false;
}

void MainComponent::seek(double t)
{
    S.t = juce::jlimit(0.0, S.duration, t);
    refreshStage();
}

// ------------------------------------------------------------------ PanelHost
void MainComponent::planChanged(bool rerenderPanel)
{
    debounce.stopTimer();
    S.rebuild();
    panel.updateBadges();
    refreshStage();
    if (rerenderPanel) panel.requestRebuild(true);
}

void MainComponent::planEdited()
{
    legend.repaint();
    timeline.repaint();
    debounce.startTimer(60);
}

void MainComponent::gotoStep(int step)
{
    if (S.importMode)
    {
        S.importMode = false;
        S.rebuild();
    }
    S.step = juce::jlimit(0, 4, step);
    panel.requestRebuild(false);
    refreshStage();
}

void MainComponent::leaveImport()
{
    S.importMode = false;
    S.rebuild();
    panel.requestRebuild(false);
    refreshStage();
}

void MainComponent::copyXml()
{
    juce::SystemClipboard::copyTextToClipboard(sceneToXmlString(S.scene));
    toast.show("XML copied to the clipboard");
}

void MainComponent::exportXml()
{
    if (S.importMode)
    {
        toast.show(T("Nothing is exported in review mode. Switch back to the plan."));
        return;
    }
    debounce.stopTimer();
    S.rebuild();
    for (auto& i : S.issues())
        if (i.level == Issue::error)
        {
            S.step = 4;
            panel.requestRebuild(false);
            refreshStage();
            toast.show("Please fix the errors first");
            return;
        }
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    chooser = std::make_unique<juce::FileChooser>("Export XML", dir.getChildFile(slug(S.plan.title) + (S.plan.encoderName.isNotEmpty() ? "_" + slug(S.plan.encoderName) : juce::String()) + ".xml"), "*.xml");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                             | juce::FileBrowserComponent::warnAboutOverwriting,
                         [this] (const juce::FileChooser& fc) {
                             auto f = fc.getResult();
                             if (f == juce::File()) return;
                             if (! f.hasFileExtension("xml")) f = f.withFileExtension("xml");
                             writeExport(f);
                         });
}

void MainComponent::writeExport(const juce::File& f)
{
    juce::String err;
    if (! writeSceneFile(S.scene, f, err))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Export failed",
                                               err.isNotEmpty() ? err : T("The file could not be written."));
        return;
    }
    // Begleitdatei: derselbe Name mit .choreo.json. Der Animator liest nur die XML; beim "Import scene XML" findet
    // die App den Plan hier wieder und oeffnet die Szene bearbeitbar.
    f.withFileExtension("choreo.json").replaceWithText(planToJson(S.plan), false, false, "\n");
#if JUCE_MAC
    const juce::String reveal = "Show in Finder";
#else
    const juce::String reveal = "Show in folder";
#endif
    auto opts = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::InfoIcon)
                    .withTitle("XML exported")
                    .withMessage(T("The scene was saved:\n") + f.getFullPathName()
                                 + T("\n\nIn the Animator: Import Scene… and choose this file."))
                    .withButton(reveal)
                    .withButton("OK")
                    .withAssociatedComponent(this);
    juce::AlertWindow::showAsync(opts, [f] (int result) {
        if (result == 1) f.revealToUser();
    });
    toast.show("XML saved: " + f.getFileName());
}

// ------------------------------------------------------------------ Dateien
void MainComponent::savePlan()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    chooser = std::make_unique<juce::FileChooser>("Save plan", dir.getChildFile(slug(S.plan.title) + ".choreo.json"), "*.json");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                             | juce::FileBrowserComponent::warnAboutOverwriting,
                         [this] (const juce::FileChooser& fc) {
                             auto f = fc.getResult();
                             if (f == juce::File()) return;
                             if (! f.hasFileExtension("json")) f = f.withFileExtension("json");
                             if (f.replaceWithText(planToJson(S.plan), false, false, "\n")) toast.show("Plan saved: " + f.getFileName());
                             else toast.show("Plan could not be saved");
                         });
}

void MainComponent::openPlan()
{
    chooser = std::make_unique<juce::FileChooser>(T("Open plan"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f.existsAsFile()) loadPlanFile(f);
    });
}

void MainComponent::importRegions()
{
    chooser = std::make_unique<juce::FileChooser>(T("Import regions (CSV)"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.csv");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        auto r = choreo::sectionsFromRegionCsv(f.loadFileAsString());
        if (r.error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Import regions", r.error);
            return;
        }
        S.plan.sections = r.sections;
        choreo::normalizePlan(S.plan);
        planChanged(true);
        juce::String msg = juce::String((int) r.sections.size()) + " sections from " + f.getFileName() + ". In the Animator use Insert at Cursor at " + choreo::formatTimecode(r.startOffset) + ".";
        for (auto& n : r.notes) msg << "\n" << n;
        if (r.notes.isEmpty()) toast.show(msg);
        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Import regions", msg);
    });
}

void MainComponent::importEncoder()
{
    chooser = std::make_unique<juce::FileChooser>(T("Import encoder preset (XML)"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        auto r = choreo::readEncoderPreset(f.loadFileAsString());
        if (r.error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Import encoder preset", r.error);
            return;
        }
        auto& P = S.plan;
        const int n = (int) r.groups.size();
        while ((int) P.groups.size() < n) P.groups.push_back(choreo::defaultGroup((int) P.groups.size()));
        while ((int) P.groups.size() > n) P.groups.pop_back();
        juce::String msg = f.getFileName() + ": " + juce::String(n) + " groups";
        if (r.distanceScaler) { P.distanceScaler = r.distanceScaler; msg << ", Distance Scaler " << juce::String(*r.distanceScaler, 3); }
        for (int i = 0; i < n; ++i)
        {
            auto& g = P.groups[(size_t) i];
            g.baseStretch = juce::jlimit(0.02, 4.0, r.groups[(size_t) i].stretch);
            if (r.groups[(size_t) i].name.isNotEmpty()) g.name = r.groups[(size_t) i].name;
            msg << "\n" << g.name << ": Stretch " << juce::String(r.groups[(size_t) i].stretch, 4) << ", " << r.groups[(size_t) i].sources << " sources";
        }
        for (auto& note : r.notes) msg << "\n" << note;
        choreo::normalizePlan(P);
        planChanged(true);
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Import encoder preset", msg);
    });
}

void MainComponent::openXml()
{
    chooser = std::make_unique<juce::FileChooser>(T("Review XML"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f.existsAsFile()) loadSceneFile(f);
    });
}

// "Import scene XML": mit Begleitplan (<name>.choreo.json) exakt bearbeitbar, sonst als Naeherung aus den Clips.
void MainComponent::importXml()
{
    chooser = std::make_unique<juce::FileChooser>(T("Import scene XML"), juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        auto side = f.withFileExtension("choreo.json");
        if (side.existsAsFile() && loadPlanFile(side, false))
        {
            toast.show("Plan loaded with the scene: " + side.getFileName());
            return;
        }
        juce::String err;
        auto sc = parseSceneXml(f.loadFileAsString(), err);
        if (! sc)
        {
            toast.show(err.isNotEmpty() ? err : T("The file could not be read"));
            return;
        }
        auto r = choreo::sceneToPlan(*sc, f.getFileNameWithoutExtension());
        choreo::normalizePlan(r.plan);
        applyPlan(r.plan, juce::String());
        juce::String msg = juce::String(r.clipsConverted) + " of " + juce::String(r.clipsTotal) + " clips converted into editable gestures.\n";
        for (auto& n : r.notes) msg << "\n- " << n;
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Import scene XML (approximation)", msg);
    });
}

bool MainComponent::loadPlanFile(const juce::File& f, bool announce)
{
    juce::String err;
    auto p = planFromJson(f.loadFileAsString(), err);
    if (! p)
    {
        if (announce) toast.show(T("The file is not a valid plan"));
        return false;
    }
    applyPlan(*p, announce ? T("Plan opened") : juce::String());
    return true;
}

void MainComponent::applyPlan(const Plan& plan, const juce::String& announcement)
{
    if (S.playing) setPlaying(false);
    S.plan = plan;
    S.importMode = false;
    S.step = 0;
    S.section = 0;
    S.t = 0;
    S.rebuild();
    panel.openState.clear();
    panel.requestRebuild(false);
    refreshStage();
    if (announcement.isNotEmpty()) toast.show(announcement);
}

bool MainComponent::loadSceneFile(const juce::File& f, bool announce)
{
    juce::String err;
    auto sc = parseSceneXml(f.loadFileAsString(), err);
    if (! sc)
    {
        if (announce) toast.show(err.isNotEmpty() ? err : T("The file could not be read"));
        return false;
    }
    S.imported = std::move(*sc);
    S.importName = f.getFileName();
    S.importMode = true;
    S.t = 0;
    S.rebuild();
    panel.requestRebuild(false);
    refreshStage();
    if (announce) toast.show("Scene loaded");
    return true;
}

void MainComponent::loadPreset(const juce::String& key, bool announce)
{
    auto fresh = buildPreset(key);
    if (key == "reset")
    {
        // keep the encoder-related settings of the current plan, so the reset matches this encoder
        fresh.baseStretch = S.plan.baseStretch;
        fresh.radius = S.plan.radius;
        fresh.distanceScaler = S.plan.distanceScaler;
        fresh.encoderName = S.plan.encoderName;
        for (size_t i = 0; i < fresh.groups.size(); ++i)
        {
            if (i < S.plan.groups.size()) fresh.groups[i].baseStretch = S.plan.groups[i].baseStretch;
            fresh.groups[i].cells[0].act.stretch = fresh.groups[i].baseStretch.value_or(fresh.baseStretch);
        }
    }
    S.plan = fresh;
    S.importMode = false;
    S.t = 0;
    S.section = 0;
    S.rebuild();
    panel.openState.clear();
    panel.requestRebuild(false);
    refreshStage();
    if (announce)
        for (auto& p : presets())
            if (p.key == key) toast.show(T("Template '") + p.label + T("' loaded"));
}

void MainComponent::setStepDirect(int step)
{
    S.step = juce::jlimit(0, 4, step);
    panel.rebuildNow(false);
    refreshStage();
}

void MainComponent::setTimeDirect(double t) { seek(t); }

void MainComponent::setSectionDirect(int s)
{
    S.section = s;
    panel.rebuildNow(false);
    refreshStage();
}
} // namespace ui
