#include "StepPanel.h"

namespace ui
{
using namespace choreo;
#define T(x) choreo::U(x)

static const char* const stepLabels[5] = { "Form", "Groups", "Gestures", "Relations", "Review & Export" };

// ------------------------------------------------------------------ Schrittknopf
StepButton::StepButton(int i, const juce::String& l) : juce::Button(l), index(i), label(l) { setTitle(l); }

int StepButton::preferredWidth() const
{
    int w = 12 + 20 + 8 + juce::GlyphArrangement::getStringWidthInt(sans(14.0f), label) + 12;
    if (badge.isNotEmpty()) w += 8 + juce::GlyphArrangement::getStringWidthInt(sans(10.5f, 600), badge) + 12;
    return w;
}

void StepButton::paintButton(juce::Graphics& g, bool over, bool)
{
    const bool cur = getToggleState();
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    if (cur)
    {
        g.setColour(col::surface2);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(col::border);
        g.drawRoundedRectangle(r, 6.0f, 1.0f);
    }
    if (hasKeyboardFocus(false))
    {
        g.setColour(col::primary.withAlpha(0.8f));
        g.drawRoundedRectangle(r.reduced(1.0f), 6.0f, 1.5f);
    }
    const float cy = r.getCentreY();
    juce::Rectangle<float> num(12.5f, cy - 10, 20, 20);
    if (cur)
    {
        g.setColour(col::primary);
        g.fillEllipse(num);
    }
    else
    {
        g.setColour(col::border);
        g.drawEllipse(num.reduced(0.5f), 1.0f);
    }
    g.setColour(cur ? col::primaryInk : (over ? col::text : col::textMuted));
    g.setFont(mono(11.0f, cur ? 500 : 400));
    g.drawText(juce::String(index + 1), num, juce::Justification::centred);
    g.setColour(cur || over ? col::text : col::textMuted);
    g.setFont(sans(14.0f));
    const int lw = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), label);
    g.drawText(label, 40, 0, lw + 2, getHeight(), juce::Justification::centredLeft);
    if (badge.isNotEmpty())
    {
        g.setFont(sans(10.5f, 600));
        const int bw = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), badge) + 12;
        juce::Rectangle<float> b((float) (40 + lw + 8), cy - 8, (float) bw, 16);
        g.setColour(badgeWarn ? col::warn : col::error);
        g.fillRoundedRectangle(b, 8.0f);
        g.setColour(juce::Colour(0xff1a0d0b));
        g.drawText(badge, b, juce::Justification::centred);
    }
}

// ------------------------------------------------------------------ Bausteine
namespace
{
const juce::Font hintFont() { return sans(12.5f); }

// Text mit `mono`-Abschnitten
std::unique_ptr<TextBlock> rich(const juce::String& text, juce::Font base, juce::Colour c, juce::Colour monoColour = {})
{
    auto tb = std::make_unique<TextBlock>();
    tb->lineSpacing = 3.0f;
    tb->setText({}, base, c);
    auto parts = juce::StringArray::fromTokens(text, "`", "");
    for (int i = 0; i < parts.size(); ++i)
        if (parts[i].isNotEmpty())
            tb->append(parts[i], i % 2 ? mono(base.getHeight() * 0.92f) : base, i % 2 && ! monoColour.isTransparent() ? monoColour : c);
    tb->setInterceptsMouseClicks(false, false);
    return tb;
}
std::unique_ptr<TextBlock> hint(const juce::String& text) { return rich(text, hintFont(), col::textMuted); }
std::unique_ptr<TextBlock> label(const juce::String& text) { return rich(text, sans(15.0f, 500), col::text); }

struct Kpis : public Block
{
    std::vector<std::pair<juce::String, juce::String>> items;
    int heightFor(int) override { return 64; }
    void paint(juce::Graphics& g) override
    {
        const int n = (int) items.size(), gap = 8;
        const float w = (getWidth() - gap * (n - 1)) / (float) n;
        for (int i = 0; i < n; ++i)
        {
            juce::Rectangle<float> r(i * (w + gap), 0, w, 64);
            g.setColour(col::surface);
            g.fillRoundedRectangle(r.reduced(0.5f), 6.0f);
            g.setColour(col::border);
            g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
            g.setColour(col::text);
            g.setFont(mono(19.0f, 500));
            g.drawText(items[(size_t) i].first, r.reduced(12, 0).withTrimmedTop(9).withHeight(26), juce::Justification::centredLeft);
            g.setColour(col::textMuted);
            g.setFont(sans(12.5f));
            g.drawText(items[(size_t) i].second, r.reduced(12, 0).withTrimmedTop(36).withHeight(18), juce::Justification::centredLeft, true);
        }
    }
};

struct IssuesList : public Block
{
    std::vector<Issue> list;
    bool ok = false;
    static juce::String lvl(Issue::Level l) { return l == Issue::error ? "ERROR" : l == Issue::warn ? "NOTICE" : "NOTE"; }
    static juce::Colour lvlCol(Issue::Level l) { return l == Issue::error ? col::error : l == Issue::warn ? col::warn : col::textMuted; }
    std::vector<std::tuple<juce::String, juce::Colour, juce::String>> rows() const
    {
        std::vector<std::tuple<juce::String, juce::Colour, juce::String>> r;
        if (list.empty())
            r.emplace_back("ALL CLEAR", col::ok, T("No overlaps; every clip has fixed start points and start values."));
        for (auto& i : list) r.emplace_back(lvl(i.level), lvlCol(i.level), i.text);
        return r;
    }
    static juce::TextLayout layoutFor(const juce::String& s, int w)
    {
        juce::AttributedString as;
        as.setWordWrap(juce::AttributedString::byWord);
        as.setLineSpacing(2.0f);
        as.append(s, sans(13.5f), col::text);
        juce::TextLayout tl;
        tl.createLayout(as, (float) juce::jmax(20, w));
        return tl;
    }
    int heightFor(int width) override
    {
        int h = 0;
        for (auto& [a, c, t] : rows()) h += juce::jmax(18, (int) std::ceil(layoutFor(t, width - 24 - 82).getHeight())) + 18 + 8;
        return juce::jmax(0, h - 8);
    }
    void paint(juce::Graphics& g) override
    {
        int y = 0;
        for (auto& [a, c, t] : rows())
        {
            auto tl = layoutFor(t, getWidth() - 24 - 82);
            const int h = juce::jmax(18, (int) std::ceil(tl.getHeight())) + 18;
            juce::Rectangle<float> r(0, (float) y, (float) getWidth(), (float) h);
            g.setColour(col::surface);
            g.fillRoundedRectangle(r.reduced(0.5f), 6.0f);
            g.setColour(col::border);
            g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
            g.setColour(c);
            juce::Font f = sans(11.5f, 600);
            f.setExtraKerningFactor(0.04f);
            g.setFont(f);
            g.drawText(a, 12, y + 9, 76, 18, juce::Justification::centredLeft, false);
            tl.draw(g, juce::Rectangle<float>(12 + 82.0f, (float) y + 9, (float) getWidth() - 24 - 82, (float) h - 18));
            y += h + 8;
        }
    }
};

struct ClipTable : public Block
{
    struct RowData
    {
        juce::String k, id, what, start, len;
    };
    std::vector<RowData> rows;
    std::array<int, 5> widths(int w) const
    {
        const int k = 22, st = 58, ln = 62;
        const int rest = juce::jmax(80, w - k - st - ln);
        const int id = (int) (rest * 0.36);
        return { k, id, rest - id, st, ln };
    }
    static juce::TextLayout cell(const juce::String& s, int w, juce::Font f, juce::Colour c, bool byChar = false)
    {
        juce::AttributedString as;
        as.setWordWrap(byChar ? juce::AttributedString::byChar : juce::AttributedString::byWord);
        as.append(s, f, c);
        juce::TextLayout tl;
        tl.createLayout(as, (float) juce::jmax(10, w - 12));
        return tl;
    }
    int rowHeight(const RowData& r, const std::array<int, 5>& w) const
    {
        const float h = juce::jmax(cell(r.id, w[1], sans(12.5f), col::text, true).getHeight(), cell(r.what, w[2], sans(12.5f), col::text).getHeight());
        return (int) std::ceil(juce::jmax(16.0f, h)) + 12;
    }
    int heightFor(int width) override
    {
        auto w = widths(width);
        int h = 30;
        if (rows.empty()) return h + 28;
        for (auto& r : rows) h += rowHeight(r, w);
        return h;
    }
    void paint(juce::Graphics& g) override
    {
        auto w = widths(getWidth());
        const char* const heads[5] = { "", "Clip", "Content", "Start s", "Length s" };
        int x = 0;
        g.setFont(sans(12.5f, 500));
        g.setColour(col::textMuted);
        for (int i = 0; i < 5; ++i)
        {
            g.drawText(T(heads[i]), x + 6, 0, w[(size_t) i] - 6, 30, juce::Justification::centredLeft, true);
            x += w[(size_t) i];
        }
        g.setColour(col::divider);
        g.fillRect(0, 29, getWidth(), 1);
        int y = 30;
        if (rows.empty())
        {
            g.setColour(col::textMuted);
            g.setFont(sans(12.5f));
            g.drawText("no clips", 6, y, getWidth() - 6, 28, juce::Justification::centredLeft);
            return;
        }
        for (auto& r : rows)
        {
            const int h = rowHeight(r, w);
            x = 0;
            const juce::String vals[5] = { r.k, r.id, r.what, r.start, r.len };
            for (int i = 0; i < 5; ++i)
            {
                const bool m = i >= 3;
                auto tl = cell(vals[i], w[(size_t) i], m ? mono(12.0f) : sans(12.5f), col::text, i == 1);
                tl.draw(g, juce::Rectangle<float>((float) x + 6, (float) y + 6, (float) w[(size_t) i] - 12, (float) h - 12));
                x += w[(size_t) i];
            }
            y += h;
            g.setColour(col::divider);
            g.fillRect(0, y - 1, getWidth(), 1);
        }
    }
};

struct Banner : public Block
{
    std::unique_ptr<TextBlock> text;
    juce::TextButton button;
    Banner(const juce::String& name, std::function<void()> onLeave) : button("Plan")
    {
        button.setButtonText(T("Back to plan"));
        text = std::make_unique<TextBlock>();
        text->lineSpacing = 2.0f;
        const juce::Colour c(0xffecd9b0);
        text->setText(T("Imported scene "), sans(13.5f), c);
        text->append(name, sans(13.5f, 600), c);
        text->append(T(" is shown. Your plan is unchanged."), sans(13.5f), c);
        addAndMakeVisible(*text);
        setVariant(button, "small");
        button.onClick = std::move(onLeave);
        addAndMakeVisible(button);
    }
    int buttonW() const { return juce::GlyphArrangement::getStringWidthInt(sans(12.5f, 500), button.getButtonText()) + 28; }
    int heightFor(int width) override { return juce::jmax(30, text->heightFor(width - 32 - buttonW() - 12)) + 24; }
    void resized() override
    {
        const int bw = buttonW();
        const int th = text->heightFor(getWidth() - 32 - bw - 12);
        text->setBounds(16, (getHeight() - th) / 2, getWidth() - 32 - bw - 12, th);
        button.setBounds(getWidth() - 16 - bw, (getHeight() - 30) / 2, bw, 30);
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(juce::Colour(0xff2a2416));
        g.fillRoundedRectangle(r, 9.0f);
        g.setColour(juce::Colour(0xff5a4a2a));
        g.drawRoundedRectangle(r, 9.0f, 1.0f);
    }
};

// Ordnungszahl + Text (ol.howto)
struct HowTo : public Block
{
    std::vector<std::unique_ptr<TextBlock>> items;
    void add(const juce::String& s)
    {
        items.push_back(rich(s, sans(13.5f), col::text));
        addAndMakeVisible(*items.back());
    }
    int heightFor(int w) override
    {
        int h = 0;
        for (auto& i : items) h += i->heightFor(w - 22) + 4;
        return juce::jmax(0, h - 4);
    }
    void resized() override
    {
        int y = 0;
        for (auto& i : items)
        {
            const int h = i->heightFor(getWidth() - 22);
            i->setBounds(22, y, getWidth() - 22, h);
            y += h + 4;
        }
    }
    void paint(juce::Graphics& g) override
    {
        int y = 0, n = 1;
        g.setFont(mono(12.0f));
        g.setColour(col::textFaint);
        for (auto& i : items)
        {
            g.drawText(juce::String(n++) + ".", 0, y + 1, 20, 18, juce::Justification::centredLeft);
            y += i->heightFor(getWidth() - 22) + 4;
        }
    }
};

struct NumLabel : public juce::Component
{
    juce::String s;
    explicit NumLabel(juce::String t) : s(std::move(t)) {}
    void paint(juce::Graphics& g) override
    {
        g.setFont(mono(12.0f));
        g.setColour(col::textFaint);
        g.drawText(s, getLocalBounds(), juce::Justification::centredLeft);
    }
};

int closestIndex(const std::vector<NumOption>& o, double v)
{
    int best = 0;
    for (size_t i = 0; i < o.size(); ++i)
        if (std::abs(o[i].v - v) < std::abs(o[(size_t) best].v - v)) best = (int) i;
    return best;
}

juce::String stripRelSuffix(juce::String s)
{
    for (auto suf : { " of", " with", " after" })
        if (s.endsWith(suf)) return s.dropLastCharacters((int) strlen(suf));
    return s;
}

juce::String relHint(const juce::String& t)
{
    if (t == "unisono") return T("Same path, same time. Useful when two groups should sound different but stay spatially coupled.");
    if (t == "kanon") return T("Same path, later. Several groups with staggered offsets create a spatial imitation.");
    if (t == "spiegel_lr") return T("Left and right swapped. Orbits and rotations reverse their direction of rotation.");
    if (t == "spiegel_vh") return T("Front and back swapped. Orbits and rotations reverse their direction of rotation.");
    if (t == "umkehr") return T("Point reflection through the listening position: the group always stays opposite. The direction of rotation is kept.");
    return {};
}
} // namespace

using StrOpts = std::vector<std::pair<juce::String, juce::String>>;

struct PanelBuilder
{
    AppState& S;
    PanelHost& H;
    StepPanel& panel;

    template <typename T>
    std::unique_ptr<Field> field(const juce::String& l, std::unique_ptr<T> c, int h = 34)
    {
        return std::make_unique<Field>(l, std::move(c), h);
    }

    std::unique_ptr<juce::TextEditor> num(std::function<double&()> ref, bool live = true)
    {
        auto ed = makeTextEditor(formatNumber(ref(), 6), [this, ref, live] (const juce::String& t) {
            ref() = parseNumber(t).value_or(0.0);
            if (live) H.planEdited();
        }, true);
        return ed;
    }
    std::unique_ptr<juce::TextEditor> optNum(std::function<std::optional<double>&()> ref, const juce::String& placeholder)
    {
        auto v = ref();
        auto ed = makeTextEditor(v ? formatNumber(*v, 6) : juce::String(), [this, ref] (const juce::String& t) {
            ref() = parseNumber(t);
            H.planEdited();
        }, true);
        ed->setTextToShowWhenEmpty(placeholder, col::textFaint);
        return ed;
    }
    std::unique_ptr<juce::TextEditor> text(std::function<juce::String&()> ref)
    {
        return makeTextEditor(ref(), [this, ref] (const juce::String& t) {
            ref() = t;
            H.planEdited();
        });
    }
    std::unique_ptr<juce::ComboBox> numCombo(const std::vector<NumOption>& opts, std::function<double&()> ref)
    {
        juce::StringArray labels;
        for (auto& o : opts) labels.add(o.label);
        return makeCombo(labels, closestIndex(opts, ref()), [this, opts, ref] (int i) {
            if (i < 0) return;
            ref() = opts[(size_t) i].v;
            H.planChanged(true);
        });
    }
    std::unique_ptr<juce::ComboBox> strCombo(const StrOpts& opts, std::function<juce::String&()> ref, std::function<void()> after = {})
    {
        juce::StringArray labels;
        int sel = -1;
        for (size_t i = 0; i < opts.size(); ++i)
        {
            labels.add(opts[i].second);
            if (opts[i].first == ref()) sel = (int) i;
        }
        return makeCombo(labels, sel, [this, opts, ref, after] (int i) {
            if (i < 0) return;
            ref() = opts[(size_t) i].first;
            if (after) after();
            H.planChanged(true);
        });
    }
    std::unique_ptr<juce::ToggleButton> toggle(const juce::String& l, std::function<bool&()> ref, bool small = false)
    {
        return makeToggle(l, ref(), [this, ref] (bool on) {
            ref() = on;
            H.planChanged(true);
        }, small);
    }
    std::unique_ptr<Flow> q(const juce::String& l, bool isLabel = true)
    {
        auto f = std::make_unique<Flow>(8);
        if (l.isNotEmpty()) f->add(isLabel ? label(l) : hint(l));
        return f;
    }
    std::unique_ptr<Flow> head(const juce::String& title, const juce::String& t)
    {
        auto f = std::make_unique<Flow>(4);
        f->add(std::make_unique<TextBlock>(title, sans(24.0f, 600), col::text));
        auto p = std::make_unique<TextBlock>(t, sans(14.0f), col::textMuted);
        p->lineSpacing = 3.0f;
        p->setText(t, sans(14.0f), col::textMuted);
        f->add(std::move(p));
        return f;
    }
    std::unique_ptr<Row> grid(int) { return std::make_unique<Row>(12); }
    std::unique_ptr<juce::TextButton> button(const juce::String& t, const juce::String& variant, std::function<void()> fn)
    {
        return makeButton(t, variant, std::move(fn));
    }
    static int textW(const juce::String& t, juce::Font f) { return juce::GlyphArrangement::getStringWidthInt(f, t); }

    std::unique_ptr<Disclosure> disclosure(const juce::String& key, const juce::String& summary, bool defaultOpen, const juce::String& badge = {},
                                           juce::Colour swatch = {})
    {
        auto it = panel.openState.find(key);
        const bool open = it != panel.openState.end() ? it->second : defaultOpen;
        auto d = std::make_unique<Disclosure>(summary, open, badge, swatch);
        auto* sp = &panel;
        d->onToggle = [sp, key] (bool o) {
            sp->openState[key] = o;
            sp->relayout();
        };
        return d;
    }

    std::unique_ptr<Row> posPicker(std::function<PosSpec&()> ref, bool noCentre = false)
    {
        auto r = grid(3);
        std::vector<NumOption> dists;
        for (auto& d : distances())
            if (! noCentre || d.v > 0) dists.push_back(d);
        r->add(field("Direction", numCombo(directions(), [ref] () -> double& { return ref().az; })));
        r->add(field("Distance", numCombo(dists, [ref] () -> double& { return ref().dist; })));
        r->add(field(T("Height"), numCombo(heights(), [ref] () -> double& { return ref().elev; })));
        return r;
    }

    std::unique_ptr<Flow> footerNav()
    {
        auto f = std::make_unique<Flow>(0, juce::BorderSize<int>(8, 0, 0, 0));
        auto r = std::make_unique<Row>(8);
        const int st = S.step;
        if (st > 0)
        {
            auto t = T("← ") + T(stepLabels[st - 1]);
            r->add(button(t, "", [this, st] { H.gotoStep(st - 1); }), (float) -(textW(t, sans(14.0f, 500)) + 36), 36);
        }
        r->addSpacer();
        if (st < 4)
        {
            auto t = T(stepLabels[st + 1]) + T(" →");
            r->add(button(t, "primary", [this, st] { H.gotoStep(st + 1); }), (float) -(textW(t, sans(14.0f, 600)) + 36), 36);
        }
        f->add(std::move(r));
        return f;
    }

    // -------------------------------------------------------------- 1 Form
    void form(Flow& out)
    {
        auto& P = S.plan;
        out.add(head("Form", T("The form sets the time grid. Each group gets one gesture per section.")));
        {
            auto f = q("What is the choreography called?");
            f->addFixed(text([this] () -> juce::String& { return S.plan.title; }), 34);
            out.add(std::move(f));
        }
        {
            auto f = q("Which sections does the form divide into?");
            auto hr = std::make_unique<Row>(8);
            hr->add(hint("Name"));
            hr->add(hint("Duration (s)"), -110.0f);
            hr->addSpacer(-36.0f);
            f->add(std::move(hr));
            for (size_t i = 0; i < P.sections.size(); ++i)
            {
                auto r = std::make_unique<Row>(8);
                r->alignCentre = true;
                auto nameEd = text([this, i] () -> juce::String& { return S.plan.sections[i].name; });
                nameEd->setTitle("Name of section " + juce::String((int) i + 1));
                r->add(std::move(nameEd), 1.0f, 34);
                auto durEd = num([this, i] () -> double& { return S.plan.sections[i].dur; });
                durEd->setTitle("Duration of section " + juce::String((int) i + 1) + " in seconds");
                r->add(std::move(durEd), -110.0f, 34);
                auto del = button(T("×"), "danger", [this, i] {
                    S.plan.sections.erase(S.plan.sections.begin() + (long) i);
                    for (auto& g : S.plan.groups)
                        if (i < g.cells.size()) g.cells.erase(g.cells.begin() + (long) i);
                    H.planChanged(true);
                });
                del->setTitle("Remove section");
                del->setEnabled(P.sections.size() >= 2);
                r->add(std::move(del), -36.0f, 34);
                f->add(std::move(r), 8);
            }
            auto r = std::make_unique<Row>(8);
            r->alignCentre = true;
            r->add(button("+ Section", "small", [this] {
                Section s;
                s.name = juce::String::charToString((juce::juce_wchar) ('A' + (int) S.plan.sections.size() % 26));
                s.dur = 8;
                S.plan.sections.push_back(s);
                H.planChanged(true);
            }), -112.0f, 30);
            auto imp = button("Import regions (CSV)", "small", [this] { H.importRegions(); });
            imp->setTitle("Import sections from a REAPER regions and markers CSV");
            r->add(std::move(imp), -176.0f, 30);
            auto total = std::make_unique<TextBlock>();
            auto* tp = total.get();
            auto upd = [this, tp] {
                double t = 0;
                for (auto& s : S.plan.sections) t += s.dur;
                tp->setText("Total " + fmt(t, 2) + " s", mono(12.0f), col::textMuted);
                tp->setJustification(juce::Justification::centredRight);
            };
            upd();
            panel.liveUpdate = upd;
            r->add(std::move(total), 1.0f);
            f->add(std::move(r));
            out.add(std::move(f));
        }
        {
            auto f = q("How large is the space?");
            auto g = grid(2);
            g->add(field(T("Radius 'far' (scene units)"), num([this] () -> double& { return S.plan.radius; })));
            g->add(field("Global scaling factor (optional)",
                         optNum([this] () -> std::optional<double>& { return S.plan.distanceScaler; }, "e.g. 4.1")));
            g->add(field("Base stretch (group Stretch in the Encoder)", num([this] () -> double& { return S.plan.baseStretch; })));
            f->add(std::move(g));
            auto impEnc = button("Import encoder preset (XML)", "small", [this] { H.importEncoder(); });
            impEnc->setTitle("Read scaling factor, groups and group Stretch from an AmbiEncoder preset");
            f->addFixed(std::move(impEnc), 30, 8);
            f->add(hint(T("'Near' and 'medium' are 35 % and 65 % of this radius. You find the scaling factor in the `distanceScaler` attribute of an Animator export file. If it differs from the project's on import, the Animator offers to convert the coordinates.")));
            out.add(std::move(f));
        }
        {
            auto f = q("Which encoder is this scene for?");
            auto g = grid(2);
            g->add(field("Encoder name (e.g. Encoder_01)", text([this] () -> juce::String& { return S.plan.encoderName; })));
            g->add(field("Max distance (optional)", optNum([this] () -> std::optional<double>& { return S.plan.maxDistance; }, "e.g. 15.6")));
            f->add(std::move(g));
            f->add(hint(T("At export the whole scene is scaled so that its farthest point reaches the max distance. The plan itself is not changed. The encoder name becomes part of the file name.")));
            out.add(std::move(f));
        }
        {
            auto f = q(T("What happens while a group rests?"));
            f->addFixed(toggle("Groups hold their position (hold clips)", [this] () -> bool& { return S.plan.holdRests; }), 28);
            f->add(hint(T("With hold clips every group sits at a fixed position at all times, even after a jump in the transport. During these times the Animator wins over the automation. Without hold clips the group follows the REAPER automation outside its gestures; the app then only sets short 40 ms holds at end points.")));
            out.add(std::move(f));
        }
        out.add(footerNav());
    }

    // -------------------------------------------------------------- 2 Gruppen
    void groupsStep(Flow& out)
    {
        auto& P = S.plan;
        const int n = (int) P.groups.size();
        out.add(head("Groups", "Each group corresponds to one source group in the AmbiEncoder Multi and one timeline in the Animator."));
        {
            auto f = q("How many source groups take part?");
            auto seg = std::make_unique<WrapButtons>(4, 34);
            for (int i = 1; i <= 8; ++i)
                seg->add(juce::String(i), 38, [this, i] { setGroups(i); }, i == n, "seg");
            f->add(std::move(seg));
            f->add(hint(T("The Animator only plays timelines for groups that are active in the project. Create the groups in the AmbiEncoder before importing.")));
            out.add(std::move(f));
        }
        for (int gi = 0; gi < n; ++gi)
        {
            auto& g = P.groups[(size_t) gi];
            const bool derived = g.relation.type != "frei";
            auto card = std::make_unique<Card>(groupColour(gi), "Timeline " + juce::String(gi + 1), derived);
            auto G = [this, gi] () -> Group& { return S.plan.groups[(size_t) gi]; };
            auto r = grid(2);
            r->add(field("Name", text([G] () -> juce::String& { return G().name; })));
            StrOpts roleOpts;
            for (auto& ro : roles()) roleOpts.push_back({ ro, ro });
            r->add(field("What role does it have?", strCombo(roleOpts, [G] () -> juce::String& { return G().role; })));
            r->add(field("Group Stretch in the Encoder (empty = Base stretch)", optNum([G] () -> std::optional<double>& { return G().baseStretch; }, "e.g. 0.163")));
            card->body.add(std::move(r));
            if (derived)
            {
                const auto ln = g.relation.leader >= 0 && g.relation.leader < n ? P.groups[(size_t) g.relation.leader].name : juce::String();
                card->body.add(hint(T("Start position and movement follow ") + ln + " (" + relationFor(g.relation.type).label + T("). Change this under 'Relations'.")));
            }
            else
            {
                auto f = q(T("Where does ") + g.name + "?", false);
                f->add(posPicker([G] () -> PosSpec& { return G().start; }));
                card->body.add(std::move(f));
            }
            out.add(std::move(card));
        }
        out.add(footerNav());
    }

    void setGroups(int n)
    {
        auto& P = S.plan;
        while ((int) P.groups.size() < n) P.groups.push_back(defaultGroup((int) P.groups.size()));
        P.groups.resize((size_t) n);
        for (int gi = 0; gi < n; ++gi)
        {
            auto& r = P.groups[(size_t) gi].relation;
            if (r.leader >= n || r.leader == gi)
            {
                r.type = "frei";
                r.leader = 0;
            }
        }
        if (n == 1) P.groups[0].relation.type = "frei";
        H.planChanged(true);
    }

    // -------------------------------------------------------------- 3 Gesten
    std::unique_ptr<Flow> gestureParams(int gi, int si)
    {
        auto M = [this, gi, si] () -> Move& { return S.plan.groups[(size_t) gi].cells[(size_t) si].move; };
        auto& m = M();
        auto out = std::make_unique<Flow>(12);
        auto dir = [&] { return field("Direction", strCombo({ { "cw", "clockwise" }, { "ccw", "counter-clockwise" } }, [M] () -> juce::String& { return M().dir; })); };
        auto turns = [&] (const juce::String& l = T("Turns")) { return field(l, num([M] () -> double& { return M().turns; })); };
        auto numF = [&] (const juce::String& l, double Move::*mem) { return field(l, num([M, mem] () -> double& { return M().*mem; })); };
        const auto& t = m.type;
        if (t == "gerade" || t == "bogen")
        {
            auto f = q("Where to?", false);
            f->add(posPicker([M] () -> PosSpec& { return M().to; }));
            out->add(std::move(f));
        }
        else if (t == "kreis")
        {
            auto r = grid(2);
            r->add(turns());
            r->add(dir());
            out->add(std::move(r));
        }
        else if (t == "spirale")
        {
            auto r = grid(3);
            r->add(turns());
            r->add(dir());
            r->add(field("endet", numCombo(distances(), [M] () -> double& { return M().toDist; })));
            out->add(std::move(r));
        }
        else if (t == "helix")
        {
            auto r = grid(3);
            r->add(turns());
            r->add(dir());
            r->add(numF(T("Height change"), &Move::rise));
            out->add(std::move(r));
        }
        else if (t == "pendel")
        {
            auto f = q("Where does it swing to?", false);
            f->add(posPicker([M] () -> PosSpec& { return M().to; }));
            out->add(std::move(f));
            auto r = grid(2);
            r->add(numF(T("Back and forth (×)"), &Move::reps));
            r->addSpacer();
            out->add(std::move(r));
        }
        else if (t == "rose")
        {
            auto r = grid(3);
            r->add(numF("Petal factor k", &Move::petals));
            r->add(turns());
            r->add(dir());
            out->add(std::move(r));
            out->add(hint(T("Odd k gives k petals, even k gives 2k.")));
        }
        else if (t == "lissajous")
        {
            auto r1 = grid(2);
            r1->add(numF("Frequency left/right", &Move::a));
            r1->add(numF("Frequency front/back", &Move::b));
            out->add(std::move(r1));
            auto r2 = grid(2);
            r2->add(numF(T("Size (fraction of radius)"), &Move::size));
            r2->add(turns(T("Passes")));
            out->add(std::move(r2));
            out->add(hint(T("The figure is centered on the listening position and starts on the right. A short run-up (at most 1 s) leads from the current position to it; the path is closed.")));
        }
        else if (t == "drift")
        {
            auto r = grid(3);
            r->add(numF(T("Extent (fraction of radius)"), &Move::amount));
            r->add(numF("Restlessness", &Move::speed));
            r->add(numF("Random seed", &Move::seed));
            out->add(std::move(r));
        }
        else if (t == "stationen")
        {
            auto f = q(T("Which stations does the path pass through?"), false);
            for (size_t k = 0; k < m.stations.size(); ++k)
            {
                auto St = [M, k] () -> PosSpec& { return M().stations[k]; };
                auto r = std::make_unique<Row>(8);
                r->alignCentre = true;
                r->add(std::make_unique<NumLabel>(juce::String((int) k + 1)), -14.0f, 34);
                auto c1 = numCombo(directions(), [St] () -> double& { return St().az; });
                c1->setTitle("Direction");
                r->add(std::move(c1), 1.0f, 34);
                auto c2 = numCombo(distances(), [St] () -> double& { return St().dist; });
                c2->setTitle("Distance");
                r->add(std::move(c2), 1.0f, 34);
                auto c3 = numCombo(heights(), [St] () -> double& { return St().elev; });
                c3->setTitle(T("Height"));
                r->add(std::move(c3), 1.0f, 34);
                auto del = button(T("×"), "danger", [this, M, k] {
                    auto& st = M().stations;
                    if (k < st.size()) st.erase(st.begin() + (long) k);
                    H.planChanged(true);
                });
                del->setTitle("Remove station");
                r->add(std::move(del), -32.0f, 34);
                f->add(std::move(r), 8);
            }
            auto w = std::make_unique<WrapButtons>(10, 30);
            w->add("+ Station", 96, [this, M] {
                auto& st = M().stations;
                st.push_back({ std::fmod(st.size() * 90.0, 360.0), 0.65, 0 });
                H.planChanged(true);
            }, false, "small");
            w->addComponent(toggle("rounded (spline)", [M] () -> bool& { return M().smooth; }, true), 140);
            w->addComponent(toggle(T("back to start"), [M] () -> bool& { return M().closed; }, true), 160);
            f->add(std::move(w));
            out->add(std::move(f));
        }
        return out;
    }

    std::unique_ptr<Disclosure> actionsBlock(int gi, int si)
    {
        auto A = [this, gi, si] () -> Act& { return S.plan.groups[(size_t) gi].cells[(size_t) si].act; };
        auto& a = A();
        const bool on = a.rotZ != 0 || a.tiltX || a.tiltY || a.stretch || a.jitter != 0;
        auto d = disclosure("act-" + juce::String(gi) + "-" + juce::String(si), "What happens within the group?", on, on ? "active" : "");
        auto r1 = grid(3);
        r1->add(field(T("Rotation (± degrees)"), num([A] () -> double& { return A().rotZ; })));
        r1->add(field(T("Tilt forward/back to (°)"), optNum([A] () -> std::optional<double>& { return A().tiltX; }, T("–"))));
        r1->add(field(T("Tilt sideways to (°)"), optNum([A] () -> std::optional<double>& { return A().tiltY; }, T("–"))));
        d->body.add(std::move(r1));
        auto r2 = grid(3);
        r2->add(field("Width to (factor)", optNum([A] () -> std::optional<double>& { return A().stretch; }, T("–"))));
        r2->add(field(T("Jitter (intensity)"), num([A] () -> double& { return A().jitter; })));
        r2->add(field("Jitter speed", num([A] () -> double& { return A().jitterSpeed; })));
        d->body.add(std::move(r2));
        d->body.add(hint(T("The rotation acts around the group's vertical axis and is added to the current angle. Tilt and width are target values; empty means unchanged. All rotations get fixed start values and run identically on every pass. The direction of rotation is schematic in the preview; check it once in the Animator.")));
        return d;
    }

    void gesturesStep(Flow& out)
    {
        auto& P = S.plan;
        const int si = juce::jlimit(0, juce::jmax(0, (int) P.sections.size() - 1), S.section);
        S.section = si;
        const auto starts = sectionStarts(P);
        out.add(head("Gestures", T("For each section, choose what each independent group does. Each gesture starts where the previous one ends.")));
        auto tabs = std::make_unique<WrapButtons>(4, 36);
        for (int i = 0; i < (int) P.sections.size(); ++i)
        {
            const auto t = P.sections[(size_t) i].name + "\t" + fmt(starts[(size_t) i], 1) + " s";
            const int w = textW(P.sections[(size_t) i].name, sans(13.5f, 500)) + 5 + textW(fmt(starts[(size_t) i], 1) + " s", mono(12.0f)) + 26;
            auto& b = tabs->add(t, w, [this, i] {
                S.section = i;
                S.t = sectionStarts(S.plan)[(size_t) i] + 0.001;
                H.planChanged(true);
            }, i == si, "tab");
            b.setTitle(P.sections[(size_t) i].name);
        }
        out.add(std::move(tabs));
        const auto& sec = P.sections[(size_t) si];
        for (int gi = 0; gi < (int) P.groups.size(); ++gi)
        {
            const auto& g = P.groups[(size_t) gi];
            if (g.relation.type != "frei")
            {
                const auto& L = P.groups[(size_t) juce::jlimit(0, (int) P.groups.size() - 1, g.relation.leader)];
                auto card = std::make_unique<Card>(groupColour(gi), g.name, true);
                juce::String s = "follows " + L.name + " " + stripRelSuffix(relationFor(g.relation.type).label);
                if (g.relation.type != "unisono" && g.relation.delay != 0) s += ", " + fmt(g.relation.delay, 2) + T(" s later");
                card->body.add(hint(s + "."));
                out.add(std::move(card));
                continue;
            }
            auto card = std::make_unique<Card>(groupColour(gi), "What does " + g.name + T(" in »") + sec.name + T("'?"));
            auto C = [this, gi, si] () -> Cell& { return S.plan.groups[(size_t) gi].cells[(size_t) si]; };
            StrOpts gOpts;
            for (auto& ge : gestures()) gOpts.push_back({ ge.key, ge.label });
            auto combo = strCombo(gOpts, [C] () -> juce::String& { return C().move.type; });
            combo->setTitle("Gesture");
            card->body.addFixed(std::move(combo), 34);
            const auto& cell = g.cells[(size_t) si];
            if (auto* gd = gestureFor(cell.move.type)) card->body.add(hint(gd->hint), 8);
            auto params = gestureParams(gi, si);
            if (! params->isEmpty()) card->body.add(std::move(params));
            if (cell.move.type != "ruhen")
            {
                auto r = grid(3);
                const std::vector<NumOption> onsets { { 0, "at start" }, { 0.25, T("after ¼") }, { 0.5, "at midpoint" }, { 0.75, T("after ¾") } };
                const std::vector<NumOption> spans { { 1, "until the end" }, { 0.75, T("¾ of the section") }, { 0.5, T("½ of the section") }, { 0.25, T("¼ of the section") } };
                r->add(field("Onset", numCombo(onsets, [C] () -> double& { return C().onset; })));
                r->add(field("Duration", numCombo(spans, [C] () -> double& { return C().span; })));
                StrOpts eOpts;
                for (auto& e : eases()) eOpts.push_back({ e.key, e.label });
                r->add(field("Shape", strCombo(eOpts, [C] () -> juce::String& { return C().ease; })));
                card->body.add(std::move(r));
            }
            card->body.add(actionsBlock(gi, si));
            out.add(std::move(card));
        }
        out.add(footerNav());
    }

    // -------------------------------------------------------------- 4 Beziehungen
    void relationsStep(Flow& out)
    {
        auto& P = S.plan;
        const int n = (int) P.groups.size();
        out.add(head("Relations", T("Contrapuntal relations between the groups. A derived group takes over the gestures and actions of its lead group, geometrically transformed and shifted in time.")));
        for (int gi = 0; gi < n; ++gi)
        {
            const auto& g = P.groups[(size_t) gi];
            const auto& r = g.relation;
            const bool derived = r.type != "frei";
            auto card = std::make_unique<Card>(groupColour(gi), T("How does ") + g.name + " relate to the other groups?", derived);
            auto G = [this, gi] () -> Group& { return S.plan.groups[(size_t) gi]; };
            auto row = grid(2);
            StrOpts relOpts;
            for (auto& rd : relations()) relOpts.push_back({ rd.key, rd.label });
            auto rc = strCombo(relOpts, [G] () -> juce::String& { return G().relation.type; }, [this, gi] {
                auto& gg = S.plan.groups[(size_t) gi];
                if (gg.relation.leader == gi || gg.relation.leader < 0 || gg.relation.leader >= (int) S.plan.groups.size())
                    gg.relation.leader = gi == 0 ? 1 : 0;
            });
            rc->setEnabled(n >= 2);
            row->add(field("Relation", std::move(rc)));
            if (derived)
            {
                juce::StringArray names;
                std::vector<int> idx;
                int sel = -1;
                for (int i = 0; i < n; ++i)
                {
                    if (i == gi) continue;
                    if (i == r.leader) sel = names.size();
                    names.add(P.groups[(size_t) i].name);
                    idx.push_back(i);
                }
                row->add(field("Lead group", makeCombo(names, sel, [this, gi, idx] (int k) {
                    if (k < 0) return;
                    S.plan.groups[(size_t) gi].relation.leader = idx[(size_t) k];
                    H.planChanged(true);
                })));
            }
            else
                row->addSpacer();
            card->body.add(std::move(row));
            if (derived && r.type != "unisono")
            {
                auto r2 = grid(2);
                r2->add(field("Offset (s)", num([G] () -> double& { return G().relation.delay; })));
                r2->addSpacer();
                card->body.add(std::move(r2));
            }
            if (derived) card->body.add(hint(relHint(r.type)));
            out.add(std::move(card));
        }
        out.add(hint(T("With an offset the following group waits at the transformed start position. Whatever falls past the end of the form because of the offset is truncated. Random drift is not mirrored but gets its own random seed.")));
        out.add(footerNav());
    }

    // -------------------------------------------------------------- 5 Prüfen & Export
    std::unique_ptr<Kpis> kpis(const Scene& sc)
    {
        size_t nM = 0, nA = 0;
        for (auto& t : sc.timelines)
        {
            nM += t.movement.size();
            nA += t.actions.size();
        }
        auto k = std::make_unique<Kpis>();
        k->items = { { juce::String((int) sc.timelines.size()), "Timelines" }, { juce::String((int) nM), "Movement clips" },
                     { juce::String((int) nA), "Action clips" } };
        return k;
    }
    std::unique_ptr<Flow> issues(const std::vector<Issue>& iss)
    {
        auto f = q(T("Review"));
        auto l = std::make_unique<IssuesList>();
        l->list = iss;
        f->add(std::move(l));
        return f;
    }
    std::unique_ptr<Flow> clips(const Scene& sc, const juce::StringArray& names)
    {
        auto f = q("Clips");
        static const char* const actNames[6] = { "–", "Rotate X", "Rotate Y", "Rotate Z", "Stretch", "Jitter" };
        for (size_t gi = 0; gi < sc.timelines.size(); ++gi)
        {
            const auto& tl = sc.timelines[gi];
            auto table = std::make_unique<ClipTable>();
            std::vector<std::pair<juce::int64, ClipTable::RowData>> rows;
            for (auto& c : tl.movement)
            {
                juce::String what = T(movementTypeNames[juce::jlimit(0, 9, c.movementType)]);
                if (c.palindrome) what += T(" · Palindrome");
                if (c.repetitions > 1) what += T(" · ") + juce::String(c.repetitions) + T("×");
                rows.push_back({ c.start, { "M", c.id, what, juce::String(c.start / 1000.0, 2), juce::String(c.length / 1000.0, 2) } });
            }
            for (auto& c : tl.actions)
            {
                juce::StringArray parts;
                for (auto& a : c.actions)
                {
                    juce::String s = T(actNames[juce::jlimit(0, 5, a.actionType)]);
                    if (a.actionType == 5) s += " " + formatNumber(a.value, 6);
                    else s += " " + (a.useStartValue ? fmt(a.startValue, 1) + T("→") : juce::String()) + fmt(a.value, 2);
                    parts.add(s);
                }
                rows.push_back({ c.start, { "A", c.id, parts.joinIntoString(", "), juce::String(c.start / 1000.0, 2), juce::String(c.length / 1000.0, 2) } });
            }
            std::stable_sort(rows.begin(), rows.end(), [] (auto& a, auto& b) { return a.first < b.first; });
            for (auto& r : rows) table->rows.push_back(r.second);
            const auto nm = (int) gi < names.size() ? names[(int) gi] : "Timeline " + juce::String((int) gi + 1);
            auto d = disclosure("clips-" + juce::String((int) gi) + (S.importMode ? "-i" : ""),
                                nm + T(" · ") + juce::String((int) tl.movement.size()) + " Movement, " + juce::String((int) tl.actions.size()) + " Action",
                                gi == 0, {}, groupColour((int) gi));
            d->body.add(std::move(table));
            f->add(std::move(d), 4);
        }
        return f;
    }

    void exportStep(Flow& out)
    {
        const auto& sc = S.scene;
        out.add(head(T("Review & Export"), T("The scene is checked with the same formulas the Animator uses during playback.")));
        out.add(kpis(sc));
        out.add(issues(S.issues()));
        {
            auto f = q("Load in the Animator");
            auto h = std::make_unique<HowTo>();
            const int n = (int) sc.timelines.size();
            h->add("In the AmbiEncoder, activate at least " + juce::String(n) + " group" + (n > 1 ? "s" : "") + ".");
            h->add(T("Open the Animator → `Import Scene…` → choose the exported XML."));
            h->add(T("For each timeline choose `Replace` (or `Insert at Cursor`) and the target group."));
            h->add(T("If the Animator asks about the scaling factor: `Rescale` adapts the coordinates to the project."));
            h->add(T("Start playback in REAPER; the Animator follows play, pause and loop."));
            f->add(std::move(h));
            out.add(std::move(f));
        }
        {
            auto r = std::make_unique<WrapButtons>(8, 36);
            r->add("Export XML", 150, [this] { H.exportXml(); }, false, "primary");
            r->add("Copy XML", 130, [this] { H.copyXml(); }, false, "");
            out.add(std::move(r));
        }
        out.add(clips(sc, S.groupNames()));
        {
            const auto xml = sceneToXmlString(sc);
            auto d = disclosure("xml", "View XML (" + juce::String(xml.getNumBytesAsUTF8() / 1024.0, 1) + " KB)", false);
            if (d->isOpen())
            {
                auto ed = std::make_unique<juce::TextEditor>();
                ed->setMultiLine(true, false);
                ed->setReadOnly(true);
                ed->setScrollbarsShown(true);
                ed->setFont(mono(11.5f));
                ed->setText(xml, false);
                d->body.addFixed(std::move(ed), 360);
            }
            else
            {
                // Inhalt erst beim Aufklappen erzeugen: Neuaufbau über den Panel-Zustand
                auto* sp = &panel;
                d->onToggle = [sp] (bool o) {
                    sp->openState["xml"] = o;
                    sp->requestRebuild(true);
                };
            }
            out.add(std::move(d));
        }
        out.add(footerNav());
    }

    void importStep(Flow& out)
    {
        const auto& sc = S.imported;
        out.add(std::make_unique<Banner>(S.importName, [this] { H.leaveImport(); }));
        out.add(head(T("Review scene"), T("Preview and review of an existing Animator file using the Animator's formulas.")));
        out.add(kpis(sc));
        out.add(hint("Scaling factor: `" + (sc.distanceScaler ? formatNumber(*sc.distanceScaler, 6) : juce::String("not specified"))
                     + T("`. Clips without a fixed start point begin in the preview at the end of the previous clip or at the origin; in the Animator their path depends on where the group currently is.")));
        out.add(issues(S.issues()));
        out.add(clips(sc, {}));
    }
};

// ------------------------------------------------------------------ Panel
StepPanel::StepPanel(AppState& s, PanelHost& h) : S(s), H(h)
{
    for (int i = 0; i < 5; ++i)
    {
        auto* b = steps.add(new StepButton(i, T(stepLabels[i])));
        b->onClick = [this, i] { H.gotoStep(i); };
        addAndMakeVisible(b);
    }
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(10);
    addAndMakeVisible(viewport);
    builder.reset(new PanelBuilder { S, H, *this });
    rebuildNow(false);
}

StepPanel::~StepPanel()
{
    viewport.setViewedComponent(nullptr, false);
    content.reset();
}

void StepPanel::requestRebuild(bool keepScroll)
{
    pendingKeepScroll = keepScroll;
    triggerAsyncUpdate();
}

void StepPanel::handleAsyncUpdate() { rebuildNow(pendingKeepScroll); }

void StepPanel::rebuildNow(bool keepScroll)
{
    cancelPendingUpdate();
    const auto pos = viewport.getViewPosition();
    liveUpdate = nullptr;
    auto c = std::make_unique<Flow>(20, juce::BorderSize<int>(20, 20, 48, 20));
    auto& b = *builder;
    if (S.importMode) b.importStep(*c);
    else
        switch (S.step)
        {
            case 0: b.form(*c); break;
            case 1: b.groupsStep(*c); break;
            case 2: b.gesturesStep(*c); break;
            case 3: b.relationsStep(*c); break;
            default: b.exportStep(*c); break;
        }
    viewport.setViewedComponent(nullptr, false);
    content = std::move(c);
    viewport.setViewedComponent(content.get(), false);
    for (int i = 0; i < steps.size(); ++i)
    {
        steps[i]->setToggleState(! S.importMode && i == S.step, juce::dontSendNotification);
    }
    updateBadges();
    relayout();
    if (keepScroll) viewport.setViewPosition(pos);
    else viewport.setViewPosition(0, 0);
}

void StepPanel::relayout()
{
    if (content == nullptr) return;
    const int w = viewport.getMaximumVisibleWidth() > 0 ? viewport.getWidth() - viewport.getScrollBarThickness() : getWidth();
    const int width = juce::jmax(200, w);
    content->setSize(width, content->heightFor(width));
    content->resized();
}

void StepPanel::updateBadges()
{
    auto iss = S.issues();
    int err = 0, warn = 0;
    for (auto& i : iss)
    {
        if (i.level == Issue::error) ++err;
        else if (i.level == Issue::warn) ++warn;
    }
    auto* b = steps[4];
    b->badge = err ? juce::String(err) : warn ? juce::String(warn) : juce::String();
    b->badgeWarn = err == 0;
    b->repaint();
    resized();
}

void StepPanel::updateLiveTexts()
{
    if (liveUpdate) liveUpdate();
}

void StepPanel::paint(juce::Graphics& g)
{
    g.fillAll(col::bg);
    const int sb = viewport.getY();
    g.setColour(col::divider);
    g.fillRect(0, sb - 1, getWidth(), 1);
}

void StepPanel::resized()
{
    // Schrittleiste umbrechend
    const int padX = 16, padY = 12, gap = 4, bh = 38;
    int x = padX, y = padY;
    for (auto* b : steps)
    {
        const int w = b->preferredWidth();
        if (x > padX && x + w > getWidth() - padX)
        {
            x = padX;
            y += bh + gap;
        }
        b->setBounds(x, y, w, bh);
        x += w + gap;
    }
    const int top = y + bh + padY + 1;
    const bool sizeChanged = viewport.getBounds() != juce::Rectangle<int>(0, top, getWidth(), getHeight() - top);
    viewport.setBounds(0, top, getWidth(), getHeight() - top);
    if (sizeChanged) relayout();
}
} // namespace ui
