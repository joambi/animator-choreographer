#include "UI/MainComponent.h"

namespace
{
class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow(const juce::String& name)
        : juce::DocumentWindow(name, ui::col::bg, juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        content = new ui::MainComponent();
        setContentOwned(content, true);
        setResizable(true, true);
        setResizeLimits(1000, 680, 4000, 3000);
        centreWithSize(1440, 900);
        setVisible(true);
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    ui::MainComponent* content = nullptr;
};

// Hidden command line for screenshots without interaction:
// --snapshot out.png [--step N] [--preset key] [--xml file] [--plan file] [--t sec] [--section i] [--open key] [--size WxH]
struct SnapshotArgs
{
    juce::File out;
    int step = -1, section = -1, w = 1440, h = 900;
    juce::String preset;
    juce::File xml, plan;
    double t = -1;
    juce::StringArray open;
    bool ai = false;
    juce::String aiText;
    juce::File aiPlan;   // zeigt diesen Plan als Ergebnis im Beschreibungsfenster
};
} // namespace

class AnimatorChoreographerApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Animator Choreographer"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& cmd) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&laf);
        auto args = juce::StringArray::fromTokens(cmd, true);
        for (auto& a : args) a = a.unquoted();
        const int si = args.indexOf("--snapshot");
        if (si >= 0 && si + 1 < args.size())
        {
            SnapshotArgs s;
            s.out = juce::File::getCurrentWorkingDirectory().getChildFile(args[si + 1]);
            for (int i = 0; i + 1 < args.size(); ++i)
            {
                const auto& k = args[i];
                const auto& v = args[i + 1];
                if (k == "--ai") s.ai = true;
                if (k == "--step") s.step = v.getIntValue();
                else if (k == "--preset") s.preset = v;
                else if (k == "--xml") s.xml = juce::File::getCurrentWorkingDirectory().getChildFile(v);
                else if (k == "--plan") s.plan = juce::File::getCurrentWorkingDirectory().getChildFile(v);
                else if (k == "--t") s.t = v.getDoubleValue();
                else if (k == "--section") s.section = v.getIntValue();
                else if (k == "--open") s.open.add(v);
                else if (k == "--ai-text") { s.ai = true; s.aiText = v; }
                else if (k == "--ai-plan") { s.ai = true; s.aiPlan = juce::File::getCurrentWorkingDirectory().getChildFile(v); }
                else if (k == "--size") { s.w = v.upToFirstOccurrenceOf("x", false, false).getIntValue(); s.h = v.fromFirstOccurrenceOf("x", false, false).getIntValue(); }
            }
            snapshot(s);
            return;
        }
        window = std::make_unique<MainWindow>(getApplicationName());
    }

    void snapshot(SnapshotArgs s)
    {
        comp = std::make_unique<ui::MainComponent>();
        comp->setSize(s.w, s.h);
        comp->setVisible(true);
        comp->addToDesktop(0);
        if (s.preset.isNotEmpty()) comp->loadPreset(s.preset, false);
        if (s.plan.existsAsFile()) comp->loadPlanFile(s.plan, false);
        for (auto& k : s.open) comp->setOpen(k, true);
        if (s.section >= 0) comp->setSectionDirect(s.section);
        if (s.step >= 0) comp->setStepDirect(s.step);
        if (s.xml.existsAsFile()) comp->loadSceneFile(s.xml, false);
        if (s.t >= 0) comp->setTimeDirect(s.t);
        if (s.ai)
        {
            comp->openAiPanel();
            if (s.aiText.isNotEmpty()) comp->getAiPanel().setDescription(s.aiText);
            if (s.aiPlan.existsAsFile())
            {
                juce::String err;
                choreo::ai::Result r;
                r.plan = choreo::planFromJson(s.aiPlan.loadFileAsString(), err);
                if (r.plan)
                {
                    r.concept = juce::String::fromUTF8("Six layers push in from behind, condense with growing restlessness and plunge into the center together at the end.");
                    r.issues = choreo::collectPlanIssues(*r.plan, choreo::planToScene(*r.plan));
                }
                r.rounds = 2;
                r.seconds = 9.4;
                comp->getAiPanel().showResultForTest(r);
            }
        }
        juce::Timer::callAfterDelay(s.ai ? 1500 : 400, [this, s] {
            comp->getPanel().rebuildNow(false);
            comp->resized();
            auto img = comp->createComponentSnapshot(comp->getLocalBounds(), true, 1.0f);
            s.out.deleteFile();
            juce::FileOutputStream os(s.out);
            juce::PNGImageFormat png;
            png.writeImageToStream(img, os);
            os.flush();
            comp->removeFromDesktop();
            quit();
        });
    }

    void shutdown() override
    {
        window.reset();
        comp.reset();
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }
    void systemRequestedQuit() override { quit(); }

private:
    ui::LookAndFeel laf;
    std::unique_ptr<MainWindow> window;
    std::unique_ptr<ui::MainComponent> comp;
};

START_JUCE_APPLICATION(AnimatorChoreographerApp)
