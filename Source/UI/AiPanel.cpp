#include "AiPanel.h"

namespace ui
{
using namespace choreo;
#define T(x) choreo::U(x)

namespace
{
const int durationValues[] = { 0, 15, 20, 30, 45, 60, 90, 120 };

juce::PropertiesFile::Options propertyOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Animator Choreographer";
    o.filenameSuffix = ".settings";
    o.folderName = "Animator Choreographer";
    o.osxLibrarySubFolder = "Application Support";
    return o;
}

juce::String seconds(double s) { return juce::String(s, s < 10 ? 1 : 0) + " s"; }
} // namespace

// ------------------------------------------------------------------ Hintergrundarbeit
class AiPanel::Worker : public juce::Thread
{
public:
    Worker(AiPanel& p, ai::Settings s, juce::String d, ai::Hints h)
        : juce::Thread("ChoreoAI"), owner(&p), st(std::move(s)), desc(std::move(d)), hints(h) {}

    void run() override
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        double lastPost = 0;
        auto res = ai::generate(st, desc, hints, cancel, [&] (const ai::Progress& p) {
            const double now = juce::Time::getMillisecondCounterHiRes();
            if (now - lastPost < 120) return;
            lastPost = now;
            auto sp = owner;
            const double el = (now - t0) / 1000.0;
            juce::MessageManager::callAsync([sp, p, el] { if (sp != nullptr) sp->showProgress(p, el); });
        });
        auto sp = owner;
        juce::MessageManager::callAsync([sp, res] { if (sp != nullptr) sp->showResult(res); });
    }

    std::atomic<bool> cancel { false };

private:
    juce::Component::SafePointer<AiPanel> owner;   // auf dem Nachrichten-Thread angelegt
    ai::Settings st;
    juce::String desc;
    ai::Hints hints;
};

class AiPanel::Probe : public juce::Thread
{
public:
    Probe(AiPanel& p, ai::Settings s) : juce::Thread("ChoreoAI-Probe"), owner(&p), st(std::move(s)) {}
    void run() override
    {
        juce::String err;
        const auto ver = ai::serverVersion(st, err);
        juce::StringArray models;
        if (err.isEmpty()) models = ai::listModels(st, err);
        auto sp = owner;
        juce::MessageManager::callAsync([sp, ver, models, err] { if (sp != nullptr) sp->probeFinished(ver, models, err); });
    }

private:
    juce::Component::SafePointer<AiPanel> owner;
    ai::Settings st;
};

// ------------------------------------------------------------------ Aufbau
AiPanel::AiPanel()
{
    setWantsKeyboardFocus(true);
    props = std::make_unique<juce::PropertiesFile>(propertyOptions());

    title.setText(T("Describe choreography"), sans(18.0f, 600), col::text);
    intro.setText(T("Describe the course, character and spatial movement in your own words. A local language model (Ollama) "
                    "drafts a plan from it and the app checks it. The description never leaves your computer."),
                  sans(13.5f), col::textMuted);
    for (auto* b : { &title, &intro, &status })
        addAndMakeVisible(b);

    description.setMultiLine(true, true);
    description.setReturnKeyStartsNewLine(true);
    description.setFont(sans(14.0f));
    description.setIndents(10, 8);
    description.setScrollbarsShown(true);
    description.setTextToShowWhenEmpty(T("e.g. Build-up of a threatening sound cluster that pushes in from behind, "
                                         "gets denser and more restless, and implodes at the end."),
                                       col::textFaint);
    description.setText(props->getValue("aiDescription"), false);
    description.onEscapeKey = [this] { close(); };
    addAndMakeVisible(description);

    lblGroups.setText("Groups", sans(12.5f, 500), col::textMuted);
    lblDur.setText("Duration", sans(12.5f, 500), col::textMuted);
    lblModel.setText("Model", sans(12.5f, 500), col::textMuted);
    for (auto* b : { &lblGroups, &lblDur, &lblModel }) addAndMakeVisible(b);

    groups.addItem("automatic", 1);
    for (int i = 1; i <= 8; ++i) groups.addItem(juce::String(i), i + 1);
    groups.setSelectedItemIndex(juce::jlimit(0, 8, props->getIntValue("aiGroups", 0)), juce::dontSendNotification);
    duration.addItem("automatic", 1);
    for (int i = 1; i < (int) std::size(durationValues); ++i) duration.addItem(juce::String(durationValues[i]) + " s", i + 1);
    duration.setSelectedItemIndex(juce::jlimit(0, (int) std::size(durationValues) - 1, props->getIntValue("aiDuration", 0)),
                                  juce::dontSendNotification);
    model.setTextWhenNothingSelected(T("Choose model"));
    model.setTextWhenNoChoicesAvailable(T("no models found"));
    const auto savedModel = props->getValue("aiModel", ai::Settings().model);
    model.addItem(savedModel, 1);
    model.setSelectedId(1, juce::dontSendNotification);
    for (auto* c : { &groups, &duration, &model })
    {
        c->onChange = [this] { saveSettings(); };
        addAndMakeVisible(c);
    }

    think.setButtonText(T("Think (more thorough, much slower)"));
    think.setToggleState(props->getBoolValue("aiThink", false), juce::dontSendNotification);
    think.onClick = [this] { saveSettings(); };
    addAndMakeVisible(think);

    btnRefresh.setButtonText("Reload");
    btnClose.setButtonText(T("Close"));
    btnStop.setButtonText("Stop");
    btnRun.setButtonText("Draft");
    btnApply.setButtonText(T("Apply"));
    btnRules.setButtonText(T("Without AI (rules)"));
    setVariant(btnRefresh, "ghost");
    setVariant(btnClose, "ghost");
    setVariant(btnStop, "danger");
    setVariant(btnRun, "primary");
    setVariant(btnApply, "primary");
    setVariant(btnRules, "ghost");
    btnRefresh.onClick = [this] { refreshModels(); };
    btnClose.onClick = [this] { close(); };
    btnStop.onClick = [this] { stop(); };
    btnRun.onClick = [this] { start(); };
    btnApply.onClick = [this] { apply(); };
    btnRules.onClick = [this] { startRules(); };
    for (auto* b : { &btnRefresh, &btnClose, &btnStop, &btnRun, &btnApply, &btnRules }) addChildComponent(b);

    resultHolder.block = &result;
    resultHolder.addAndMakeVisible(result);
    resultView.setViewedComponent(&resultHolder, false);
    resultView.setScrollBarsShown(true, false);
    addChildComponent(resultView);

    updateButtons();
}

AiPanel::~AiPanel()
{
    saveSettings();
    if (worker) { worker->cancel = true; worker->stopThread(4000); }
    if (probe) probe->stopThread(6000);
}

// ------------------------------------------------------------------ Einstellungen
ai::Settings AiPanel::currentSettings() const
{
    ai::Settings st;
    if (model.getText().isNotEmpty()) st.model = model.getText();
    st.think = think.getToggleState();
    return st;
}

ai::Hints AiPanel::currentHints() const
{
    ai::Hints h;
    h.groups = juce::jmax(0, groups.getSelectedItemIndex());
    const int di = duration.getSelectedItemIndex();
    h.duration = di > 0 && di < (int) std::size(durationValues) ? durationValues[di] : 0;
    return h;
}

void AiPanel::saveSettings()
{
    if (! props) return;
    props->setValue("aiDescription", description.getText());
    props->setValue("aiGroups", groups.getSelectedItemIndex());
    props->setValue("aiDuration", duration.getSelectedItemIndex());
    if (model.getText().isNotEmpty()) props->setValue("aiModel", model.getText());
    props->setValue("aiThink", think.getToggleState());
    props->saveIfNeeded();
}

// ------------------------------------------------------------------ Ablauf
void AiPanel::open()
{
    setVisible(true);
    toFront(true);
    description.grabKeyboardFocus();
    if (phase != Phase::running) refreshModels();
}

void AiPanel::close()
{
    if (worker) stop();
    saveSettings();
    setVisible(false);
    if (onClose) onClose();
}

void AiPanel::setDescription(const juce::String& s) { description.setText(s, false); }

void AiPanel::refreshModels()
{
    if (probe || worker) return;
    phase = Phase::probing;
    setStatus(T("Connecting to Ollama …"), col::textMuted);
    probe = std::make_unique<Probe>(*this, currentSettings());
    probe->startThread();
    updateButtons();
}

void AiPanel::probeFinished(const juce::String& version, const juce::StringArray& models, const juce::String& error)
{
    if (probe) { probe->stopThread(1000); probe.reset(); }
    if (phase == Phase::probing) phase = pending ? Phase::done : Phase::idle;
    if (error.isNotEmpty())
    {
        serverInfo = {};
        setStatus(error, col::error);
        updateButtons();
        return;
    }
    serverInfo = "Ollama " + version;
    const auto wanted = model.getText().isNotEmpty() ? model.getText() : props->getValue("aiModel", ai::Settings().model);
    model.clear(juce::dontSendNotification);
    for (int i = 0; i < models.size(); ++i) model.addItem(models[i], i + 1);
    const int idx = models.indexOf(wanted);
    if (idx >= 0) model.setSelectedItemIndex(idx, juce::dontSendNotification);
    else if (! models.isEmpty())
    {
        // bevorzugt eine gemma4-Variante, sonst das erste Modell
        int pick = 0;
        for (int i = 0; i < models.size(); ++i)
            if (models[i].startsWith("gemma4")) { pick = i; break; }
        model.setSelectedItemIndex(pick, juce::dontSendNotification);
    }

    if (models.isEmpty())
        setStatus(serverInfo + T(" is running, but no local model is installed. In the terminal: ollama pull gemma4:12b"), col::warn);
    else if (idx < 0 && wanted.isNotEmpty())
        setStatus(serverInfo + T(" ready. '") + wanted + T("' is not installed, selected: ") + model.getText(), col::warn);
    else if (! pending)
        setStatus(serverInfo + T(" ready, ") + juce::String(models.size()) + (models.size() == 1 ? " model" : " models") + T(" installed."),
                  col::ok);
    updateButtons();
}

void AiPanel::start()
{
    if (worker || probe) return;
    const auto text = description.getText().trim();
    if (text.length() < 8)
    {
        setStatus(T("Please describe the choreography first."), col::warn);
        description.grabKeyboardFocus();
        return;
    }
    if (model.getText().isEmpty())
    {
        setStatus(T("No model selected. 'Reload' asks Ollama for the installed models."), col::warn);
        return;
    }
    saveSettings();
    pending.reset();
    result.setText({}, sans(13.5f), col::text);
    resultView.setVisible(false);
    phase = Phase::running;
    setStatus(T("Requesting draft … (loading the model can take 10–30 s the first time)"), col::textMuted);
    worker = std::make_unique<Worker>(*this, currentSettings(), text, currentHints());
    worker->startThread();
    updateButtons();
    resized();
}

void AiPanel::startRules()
{
    if (worker) return;
    const auto text = description.getText().trim();
    if (text.length() < 8)
    {
        setStatus(T("Please describe the choreography first."), col::warn);
        description.grabKeyboardFocus();
        return;
    }
    saveSettings();
    choreo::PlannerOptions o;
    const auto h = currentHints();
    if (h.groups > 0) o.groups = h.groups;
    if (h.duration > 0) o.duration = h.duration;
    auto planned = choreo::planFromPrompt(text, o);
    auto plan = planned.plan;
    auto scene = choreo::planToScene(plan);
    ai::Result r;
    r.plan = plan;
    r.issues = choreo::collectPlanIssues(plan, scene);
    r.concept = planned.log.joinIntoString("\n");
    r.rounds = 0;
    showResult(r);
}

void AiPanel::stop()
{
    if (! worker) return;
    worker->cancel = true;
    setStatus(T("Cancelling …"), col::textMuted);
}

void AiPanel::showProgress(const ai::Progress& p, double elapsed)
{
    if (phase != Phase::running) return;
    juce::String s = p.phase;
    if (p.round > 1) s << " (round " << p.round << ")";
    s << T(" · ") << seconds(elapsed);
    if (p.thinkingChars > 0) s << T(" · thinking ") << p.thinkingChars << " chars";
    if (p.contentChars > 0) s << T(" · plan ") << p.contentChars << " chars";
    setStatus(s, col::textMuted);
}

void AiPanel::showResultForTest(const ai::Result& r)
{
    phase = Phase::running;
    showResult(r);
}

void AiPanel::showResult(ai::Result r)
{
    if (worker) { worker->stopThread(2000); worker.reset(); }
    phase = Phase::done;
    pending.reset();

    if (r.cancelled)
    {
        phase = Phase::idle;
        setStatus("Cancelled.", col::textMuted);
        updateButtons();
        resized();
        return;
    }
    if (! r.plan)
    {
        phase = Phase::idle;
        setStatus(r.error.isNotEmpty() ? r.error : T("The model did not deliver a valid plan."), col::error);
        updateButtons();
        resized();
        return;
    }

    pending = r.plan;
    const auto& plan = *r.plan;
    int errors = 0, warns = 0;
    for (auto& i : r.issues)
    {
        if (i.level == Issue::error) ++errors;
        else if (i.level == Issue::warn) ++warns;
    }
    juce::String head;
    if (r.rounds == 0) head << T("Draft by rules ready (no language model).");
    else head << "Draft ready: " << r.rounds << (r.rounds == 1 ? " round, " : " rounds, ") << seconds(r.seconds) << " compute time.";
    if (errors) head << " " << errors << (errors == 1 ? " error remains" : " errors remain") << T(", please check after applying.");
    else if (warns) head << " " << warns << (warns == 1 ? " notice." : " notices.");
    else head << T(" Review without findings.");
    setStatus(head, errors ? col::warn : col::ok);

    // Zusammenfassung
    const auto fB = sans(14.0f, 600), fT = sans(13.5f), fL = sans(12.5f, 500);
    result.setText(plan.title.isNotEmpty() ? plan.title : juce::String("Untitled"), sans(15.0f, 600), col::text);
    result.append(T("   ") + seconds(totalDuration(plan)) + T(" · ") + juce::String((int) plan.groups.size())
                      + (plan.groups.size() == 1 ? " group" : " groups") + "\n",
                  fT, col::textMuted);
    if (r.concept.isNotEmpty()) result.append(r.concept + "\n", fT, col::text);

    result.append("\nSECTIONS\n", fL, col::textFaint);
    juce::String secs;
    for (size_t i = 0; i < plan.sections.size(); ++i)
        secs << (i ? T("  ·  ") : juce::String()) << plan.sections[i].name << " " << seconds(plan.sections[i].dur);
    result.append(secs + "\n", fT, col::text);

    result.append("\nGROUPS\n", fL, col::textFaint);
    for (size_t gi = 0; gi < plan.groups.size(); ++gi)
    {
        const auto& g = plan.groups[gi];
        result.append(g.name, fB.withHeight(13.5f), col::text);
        juce::String line;
        if (g.relation.type == "frei")
        {
            juce::StringArray ges;
            for (auto& c : g.cells)
            {
                const auto* gd = gestureFor(c.move.type);
                ges.add(gd ? gd->label : c.move.type);
            }
            line << T(": ") << ges.joinIntoString(T(" → "));
        }
        else
        {
            const auto leader = g.relation.leader >= 0 && g.relation.leader < (int) plan.groups.size()
                                    ? plan.groups[(size_t) g.relation.leader].name : juce::String("?");
            line << T(": ") << relationFor(g.relation.type).label << " " << leader;
            if (g.relation.type == "kanon") line << ", " << seconds(g.relation.delay) << " later";
        }
        result.append(line + "\n", fT, col::textMuted);
    }

    if (! r.issues.empty())
    {
        result.append(T("\nREVIEW\n"), fL, col::textFaint);
        for (auto& i : r.issues)
        {
            const auto c = i.level == Issue::error ? col::error : i.level == Issue::warn ? col::warn : col::textMuted;
            result.append(T("• ") + i.text + "\n", fT, c);
        }
    }
    resultView.setVisible(true);
    updateButtons();
    resized();
    btnApply.grabKeyboardFocus();
}

void AiPanel::apply()
{
    if (! pending) return;
    auto plan = *pending;
    saveSettings();
    setVisible(false);
    if (onApply) onApply(plan);
    if (onClose) onClose();
}

void AiPanel::setStatus(const juce::String& text, juce::Colour colour)
{
    status.setText(text, sans(13.0f, 500), colour);
    resized();
    repaint();
}

void AiPanel::updateButtons()
{
    const bool running = phase == Phase::running;
    btnStop.setVisible(running);
    btnRun.setVisible(! running);
    btnRun.setButtonText(pending ? "Draft again" : "Draft");
    setVariant(btnRun, pending ? "ghost" : "primary");
    btnRun.setEnabled(probe == nullptr);
    btnApply.setVisible(! running && pending.has_value());
    btnClose.setVisible(true);
    btnRefresh.setVisible(true);
    btnRefresh.setEnabled(! running && probe == nullptr);
    btnRules.setVisible(true);
    btnRules.setEnabled(! running);
    for (juce::Component* c : { (juce::Component*) &description, (juce::Component*) &groups, (juce::Component*) &duration,
                                (juce::Component*) &model, (juce::Component*) &think })
        c->setEnabled(! running);
    resized();
}

// ------------------------------------------------------------------ Darstellung
int AiPanel::descHeight() const { return getHeight() < 820 ? 96 : 150; }

juce::Rectangle<int> AiPanel::cardBounds() const
{
    const int w = juce::jmin(720, getWidth() - 48);
    const int pad = 24, inner = w - 2 * pad;
    int h = pad;
    h += const_cast<TextBlock&>(title).heightFor(inner) + 6;
    h += const_cast<TextBlock&>(intro).heightFor(inner) + 14;
    h += descHeight() + 14;   // Beschreibung
    h += 18 + 4 + 34 + 12;    // Gruppen / Dauer / Modell
    h += 26 + 14;             // Nachdenken
    h += const_cast<TextBlock&>(status).heightFor(inner);
    if (resultView.isVisible())
        h += 12 + juce::jmin(360, const_cast<TextBlock&>(result).heightFor(inner - 12) + 4);
    h += 18 + 36 + pad;
    h = juce::jmin(h, getHeight() - 48);
    return { (getWidth() - w) / 2, juce::jmax(24, (getHeight() - h) / 2), w, h };
}

void AiPanel::resized()
{
    if (getWidth() <= 0) return;
    auto card = cardBounds();
    auto r = card.reduced(24);

    title.setBounds(r.removeFromTop(title.heightFor(r.getWidth())));
    r.removeFromTop(6);
    intro.setBounds(r.removeFromTop(intro.heightFor(r.getWidth())));
    r.removeFromTop(14);

    // Knöpfe unten
    auto bar = r.removeFromBottom(36);
    r.removeFromBottom(18);
    auto bw = [] (juce::TextButton& b, int weight) { return juce::GlyphArrangement::getStringWidthInt(sans(14.0f, weight), b.getButtonText()) + 34; };
    btnClose.setBounds(bar.removeFromLeft(bw(btnClose, 500)));
    bar.removeFromLeft(8);
    btnRules.setBounds(bar.removeFromLeft(bw(btnRules, 500)));
    for (auto* b : { &btnApply, &btnRun, &btnStop })
        if (b->isVisible())
        {
            b->setBounds(bar.removeFromRight(bw(*b, 600)));
            bar.removeFromRight(8);
        }

    description.setBounds(r.removeFromTop(descHeight()));
    r.removeFromTop(14);

    auto labels = r.removeFromTop(18);
    r.removeFromTop(4);
    auto row = r.removeFromTop(34);
    r.removeFromTop(12);
    const int gap = 12;
    const int colW = (row.getWidth() - 2 * gap) / 4;
    auto place = [&] (TextBlock& l, juce::Component& c, int w) {
        l.setBounds(labels.removeFromLeft(w));
        c.setBounds(row.removeFromLeft(w));
        labels.removeFromLeft(gap);
        row.removeFromLeft(gap);
    };
    place(lblGroups, groups, colW);
    place(lblDur, duration, colW);
    const int refreshW = bw(btnRefresh, 500);
    lblModel.setBounds(labels);
    btnRefresh.setBounds(row.removeFromRight(refreshW));
    row.removeFromRight(8);
    model.setBounds(row);

    think.setBounds(r.removeFromTop(26));
    r.removeFromTop(14);
    status.setBounds(r.removeFromTop(status.heightFor(r.getWidth())));
    if (resultView.isVisible())
    {
        r.removeFromTop(12);
        resultView.setBounds(r);
        const int inner = r.getWidth() - 12;
        resultHolder.setSize(inner, result.heightFor(inner) + 4);
    }
}

void AiPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.55f));
    auto c = cardBounds().toFloat();
    g.setColour(col::surface);
    g.fillRoundedRectangle(c, 12.0f);
    g.setColour(col::border);
    g.drawRoundedRectangle(c.reduced(0.5f), 12.0f, 1.0f);
    if (resultView.isVisible())
    {
        auto rv = resultView.getBounds().toFloat().expanded(0, 6);
        g.setColour(col::divider);
        g.drawHorizontalLine((int) rv.getY(), rv.getX(), rv.getRight());
    }
}

bool AiPanel::keyPressed(const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { close(); return true; }
    if (k == juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier, 0))
    {
        if (phase != Phase::running) start();
        return true;
    }
    // Leertaste nicht an die Hauptansicht durchreichen (dort Wiedergabe), Tab weiter für die Fokusfolge
    return k == juce::KeyPress::spaceKey;
}

void AiPanel::mouseDown(const juce::MouseEvent&) {}
} // namespace ui
