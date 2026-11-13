#include "StageViews.h"

namespace ui
{
using namespace choreo;

static void viewBackground(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(col::viewBg);
    g.fillRoundedRectangle(r.reduced(0.5f), 9.0f);
    g.setColour(col::divider);
    g.drawRoundedRectangle(r.reduced(0.5f), 9.0f, 1.0f);
}

static void strokeDashed(juce::Graphics& g, const juce::Path& p, float width)
{
    juce::Path d;
    const float dash[] = { 3.0f, 4.0f };
    juce::PathStrokeType(width).createDashedStroke(d, p, dash, 2);
    g.fillPath(d);
}

static juce::Colour alpha(juce::Colour c, int a) { return c.withAlpha((juce::uint8) a); }

// ------------------------------------------------------------------ Draufsicht
void TopView::paint(juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    viewBackground(g, getLocalBounds().toFloat());
    if (S.prep.empty() && S.current().timelines.empty()) return;
    const double R = S.roomRadius();
    double ext = R * 1.18;
    for (auto& pts : S.paths)
        for (auto& p : pts) ext = juce::jmax(ext, std::hypot(p.x, p.y) * 1.05);
    const float cx = w / 2, cy = h / 2;
    const double sc = (juce::jmin(w, h) / 2 - 22) / ext;
    auto P = [&] (V3 p) { return juce::Point<float>((float) (cx + p.x * sc), (float) (cy - p.y * sc)); };

    // Raster
    g.setColour(col::grid);
    const double fr[3] = { 0.35, 0.65, 1.0 };
    for (int i = 0; i < 3; ++i)
    {
        const float rr = (float) (R * fr[i] * sc);
        juce::Path c;
        c.addEllipse(cx - rr, cy - rr, rr * 2, rr * 2);
        if (i == 2) g.strokePath(c, juce::PathStrokeType(1.0f));
        else strokeDashed(g, c, 1.0f);
    }
    g.setColour(col::gridFaint);
    const float Rs = (float) (R * sc);
    g.drawLine(cx - Rs, cy, cx + Rs, cy, 1.0f);
    g.drawLine(cx, cy - Rs, cx, cy + Rs, 1.0f);
    g.setColour(col::label);
    g.setFont(sans(11.0f));
    g.drawText("front", juce::Rectangle<float>(cx - 60, 4, 120, 16), juce::Justification::centred);
    g.drawText("back", juce::Rectangle<float>(cx - 60, h - 20, 120, 16), juce::Justification::centred);
    g.drawText("left", juce::Rectangle<float>(10, cy - 18, 80, 16), juce::Justification::centredLeft);
    g.drawText("right", juce::Rectangle<float>(w - 90, cy - 18, 80, 16), juce::Justification::centredRight);
    // Hörposition
    juce::Path tri;
    tri.addTriangle(cx, cy - 7, cx + 5, cy + 5, cx - 5, cy + 5);
    g.setColour(col::listener);
    g.fillPath(tri);

    const int N = S.paths.empty() ? 0 : (int) S.paths[0].size() - 1;
    const int idx = (int) jsRound((S.t / S.duration) * N);
    const int tail = (int) jsRound((2.0 / S.duration) * N);
    for (size_t gi = 0; gi < S.prep.size(); ++gi)
    {
        if (! S.nonEmpty((int) gi)) continue;
        const auto c = groupColour((int) gi);
        const auto& pts = S.paths[gi];
        juce::Path full;
        for (size_t i = 0; i < pts.size(); ++i)
            i ? full.lineTo(P(pts[i])) : full.startNewSubPath(P(pts[i]));
        g.setColour(alpha(c, 0x38));
        g.strokePath(full, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path tp;
        bool first = true;
        for (int i = juce::jmax(0, idx - tail); i <= juce::jmin(N, idx); ++i)
        {
            if (first) { tp.startNewSubPath(P(pts[(size_t) i])); first = false; }
            else tp.lineTo(P(pts[(size_t) i]));
        }
        g.setColour(alpha(c, 0xcc));
        g.strokePath(tp, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    for (size_t gi = 0; gi < S.prep.size(); ++gi)
    {
        if (! S.nonEmpty((int) gi)) continue;
        const auto c = groupColour((int) gi);
        const auto smp = S.sample((int) gi);
        const auto sats = satellites(smp, R, (int) gi, S.t);
        const auto gp = P(smp.pos);
        g.setColour(alpha(c, 0x88));
        for (auto& s : sats) g.drawLine(juce::Line<float>(gp, P(s)), 1.0f);
        for (size_t k = 0; k < sats.size(); ++k)
        {
            const auto sp = P(sats[k]);
            g.setColour(k == 0 ? juce::Colour(0xfff4efe4) : c);
            g.fillEllipse(sp.x - 2.6f, sp.y - 2.6f, 5.2f, 5.2f);
        }
        const float rad = (float) (6 + juce::jlimit(-2.0, 5.0, smp.pos.z * 1.6));
        juce::Rectangle<float> dot(gp.x - rad, gp.y - rad, rad * 2, rad * 2);
        g.setColour(c);
        g.fillEllipse(dot);
        g.setColour(col::viewBg);
        g.drawEllipse(dot, 2.0f);
        g.setFont(mono(10.0f, 500));
        g.drawText(juce::String((int) gi + 1), dot.expanded(4).translated(0, 0.5f), juce::Justification::centred);
    }
}

// ------------------------------------------------------------------ Seitenansicht
void SideView::paint(juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    viewBackground(g, getLocalBounds().toFloat());
    const double R = S.roomRadius();
    double ext = R * 1.15;
    for (auto& pts : S.paths)
        for (auto& p : pts) ext = std::max({ ext, std::abs(p.y) * 1.05, std::abs(p.z) * 1.05 });
    const float cx = w / 2, cy = h * 0.62f;
    const double sc = juce::jmin((w / 2 - 14) / ext, (h * 0.55) / ext);
    auto P = [&] (V3 p) { return juce::Point<float>((float) (cx + p.y * sc), (float) (cy - p.z * sc)); };
    g.setColour(col::grid);
    g.drawLine(10, cy, w - 10, cy, 1.0f);
    juce::Path arc;
    const float rr = (float) (R * sc);
    arc.addCentredArc(cx, cy, rr, rr, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    strokeDashed(g, arc, 1.0f);
    g.setColour(col::label);
    g.setFont(sans(11.0f));
    g.drawText("back", juce::Rectangle<float>(10, cy + 5, 80, 14), juce::Justification::topLeft);
    g.drawText("front", juce::Rectangle<float>(w - 90, cy + 5, 80, 14), juce::Justification::topRight);
    g.drawText(U("ear level"), juce::Rectangle<float>(cx - 40, cy + 5, 80, 14), juce::Justification::centredTop);
    g.setColour(col::listener);
    g.fillEllipse(cx - 3.5f, cy - 3.5f, 7, 7);
    for (size_t gi = 0; gi < S.prep.size(); ++gi)
    {
        if (! S.nonEmpty((int) gi)) continue;
        const auto c = groupColour((int) gi);
        juce::Path p;
        const auto& pts = S.paths[gi];
        for (size_t i = 0; i < pts.size(); ++i)
            i ? p.lineTo(P(pts[i])) : p.startNewSubPath(P(pts[i]));
        g.setColour(alpha(c, 0x30));
        g.strokePath(p, juce::PathStrokeType(1.0f));
        const auto q = P(S.sample((int) gi).pos);
        g.setColour(c);
        g.fillEllipse(q.x - 5, q.y - 5, 10, 10);
    }
}

// ------------------------------------------------------------------ Legende
void Legend::paint(juce::Graphics& g)
{
    const auto names = S.groupNames();
    int y = 0;
    const int rowH = 20;
    for (size_t gi = 0; gi < S.prep.size(); ++gi)
    {
        if (! S.nonEmpty((int) gi)) continue;
        if (y + rowH > getHeight()) break;
        const auto p = S.sample((int) gi).pos;
        const double d = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        const double az = std::fmod(std::atan2(p.x, p.y) * 180.0 / juce::MathConstants<double>::pi + 360.0, 360.0);
        const double el = d > 1e-6 ? std::asin(p.z / d) * 180.0 / juce::MathConstants<double>::pi : 0;
        auto ps = fmt(az, 0).paddedLeft(' ', 3) + U("° ") + fmt(el, 0) + U("° ") + fmt(d, 2);
        g.setColour(groupColour((int) gi));
        g.fillRoundedRectangle(6.0f, (float) y + 4, 12.0f, 12.0f, 3.0f);
        g.setFont(mono(11.0f));
        const int pw = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), ps) + 2;
        g.setColour(col::textMuted);
        g.drawText(ps, getWidth() - 6 - pw, y, pw, rowH, juce::Justification::centredRight);
        g.setColour(col::text);
        g.setFont(sans(12.5f));
        g.drawText(names[(int) gi], 26, y, getWidth() - 26 - pw - 14, rowH, juce::Justification::centredLeft, true);
        y += rowH + 3;
    }
}

// ------------------------------------------------------------------ Timeline
int TimelineView::preferredHeight() const { return 22 + (int) S.current().timelines.size() * 26 + 8; }

void TimelineView::scrub(float x)
{
    const double f = juce::jlimit(0.0, 1.0, (double) (x - x0) / (double) (x1 - x0));
    if (onSeek) onSeek(f * S.duration);
}

void TimelineView::paint(juce::Graphics& g)
{
    const auto& sc = S.current();
    const int rowH = 26, top = 22, labelW = 96;
    const float w = (float) getWidth(), h = (float) getHeight();
    juce::Path clipShape;
    clipShape.addRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 9.0f);
    g.saveState();
    g.reduceClipRegion(clipShape);
    g.fillAll(col::viewBg);
    x0 = (float) labelW;
    x1 = w - 10;
    const double D = S.duration * 1000.0;
    auto X = [&] (double ms) { return (float) (x0 + (ms / D) * (x1 - x0)); };
    g.setFont(sans(11.0f));
    if (! S.importMode)
    {
        const auto st = sectionStarts(S.plan);
        for (size_t i = 0; i < S.plan.sections.size(); ++i)
        {
            const auto& s = S.plan.sections[i];
            const float a = X(st[i] * 1000), b = X((st[i] + s.dur) * 1000);
            g.setColour(juce::Colour(i % 2 ? 0xff16171b : 0xff1a1c21));
            g.fillRect(a, 0.0f, b - a, h);
            g.setColour(((int) i == S.section && S.step == 2) ? col::text : col::labelMid);
            g.drawText(s.name, juce::Rectangle<float>(a + 6, 0, juce::jmax(0.0f, b - a - 8), (float) top), juce::Justification::centredLeft, false);
            g.setColour(col::grid);
            g.drawLine(a + 0.5f, 0, a + 0.5f, h, 1.0f);
        }
    }
    else
    {
        const int step = (int) juce::jmax(1.0, jsRound(S.duration / 10));
        for (int s = 0; s <= S.duration; s += step)
        {
            const float x = X(s * 1000.0);
            g.setColour(col::labelMid);
            g.drawText(juce::String(s) + " s", juce::Rectangle<float>(x + 3, 0, 60, (float) top), juce::Justification::centredLeft);
            g.setColour(col::gridFaint);
            g.drawLine(x + 0.5f, (float) top - 4, x + 0.5f, h, 1.0f);
        }
    }
    const auto names = S.groupNames();
    for (size_t gi = 0; gi < sc.timelines.size(); ++gi)
    {
        const auto& tl = sc.timelines[gi];
        const float y = (float) (top + (int) gi * rowH);
        g.setColour(groupColour((int) gi));
        g.fillRect(6.0f, y + rowH / 2.0f - 4, 8.0f, 8.0f);
        g.setColour(col::labelLight);
        g.setFont(sans(11.0f));
        g.drawText(names[(int) gi], juce::Rectangle<float>(20, y, (float) labelW - 26, (float) rowH), juce::Justification::centredLeft, true);
        auto lane = [&] (const auto& list, float yy, float hh) {
            for (auto& c : list)
            {
                const float a = X((double) c.start), b = juce::jmax(a + 1.5f, X((double) (c.start + c.length)));
                g.setColour(juce::Colour(c.colour | 0xff000000u).withAlpha(c.muted ? 0.3f : 0.9f));
                g.fillRect(a + 0.5f, yy, juce::jmax(0.5f, b - a - 1), hh);
            }
        };
        lane(tl.movement, y + 3, 11);
        lane(tl.actions, y + 16, 6);
    }
    const float px = X(S.t * 1000.0);
    g.setColour(col::primary);
    g.drawLine(px, 0, px, h, 1.5f);
    g.restoreState();
    g.setColour(col::divider);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 9.0f, 1.0f);
}

// ------------------------------------------------------------------ Rahmen
void Caption::paint(juce::Graphics& g)
{
    g.setFont(sans(12.5f, 500));
    g.setColour(col::text);
    const int tw = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), title);
    g.drawText(title, 0, 0, tw + 2, getHeight(), juce::Justification::centredLeft);
    g.setFont(sans(12.5f));
    g.setColour(col::textMuted);
    g.drawText(U(" · ") + muted, tw + 2, 0, getWidth() - tw - 2, getHeight(), juce::Justification::centredLeft, true);
}

ViewFrame::ViewFrame(const juce::String& t, const juce::String& m, juce::Component& c) : caption(t, m), content(c)
{
    addAndMakeVisible(caption);
    addAndMakeVisible(content);
}
void ViewFrame::paint(juce::Graphics&) {}
void ViewFrame::resized()
{
    caption.setBounds(0, 0, getWidth(), 18);
    content.setBounds(0, 26, getWidth(), juce::jmax(0, getHeight() - 26));
}

// ------------------------------------------------------------------ Transport
void PlayButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(down ? col::surface3 : (over ? col::surface3 : col::surface2));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(col::border);
    g.drawRoundedRectangle(r, 6.0f, 1.0f);
    // 24er-Raster wie das SVG, 18 px gross
    const float s = 18.0f / 24.0f, ox = r.getCentreX() - 9, oy = r.getCentreY() - 9;
    juce::Path p;
    if (playing)
    {
        p.addRectangle(ox + 7 * s, oy + 5 * s, 4 * s, 14 * s);
        p.addRectangle(ox + 13 * s, oy + 5 * s, 4 * s, 14 * s);
    }
    else
        p.addTriangle(ox + 8 * s, oy + 5 * s, ox + 8 * s, oy + 19 * s, ox + 19 * s, oy + 12 * s);
    g.setColour(col::text);
    g.fillPath(p);
    if (hasKeyboardFocus(false))
    {
        g.setColour(col::primary);
        g.drawRoundedRectangle(r.expanded(1.0f), 7.0f, 1.5f);
    }
}

Transport::Transport(AppState& s) : S(s)
{
    play.setTitle("Play");
    play.onClick = [this] { if (onPlay) onPlay(! S.playing); };
    addAndMakeVisible(play);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(0, 1000, 1);
    slider.setTitle("Time position");
    slider.onValueChange = [this] {
        if (onSeek) onSeek(slider.getValue() / 1000.0 * S.duration);
    };
    addAndMakeVisible(slider);
    speed.addItem(U("0.5×"), 1);
    speed.addItem(U("1×"), 2);
    speed.addItem(U("2×"), 3);
    speed.setSelectedId(2, juce::dontSendNotification);
    setVariant(speed, "small");
    speed.setTitle("Playback speed");
    speed.onChange = [this] { S.speed = speed.getSelectedId() == 1 ? 0.5 : speed.getSelectedId() == 3 ? 2.0 : 1.0; };
    addAndMakeVisible(speed);
    setVariant(loop, "small");
    loop.setToggleState(true, juce::dontSendNotification);
    loop.onClick = [this] { S.loop = loop.getToggleState(); };
    addAndMakeVisible(loop);
}

void Transport::resized()
{
    auto r = getLocalBounds();
    play.setBounds(r.removeFromLeft(36).withSizeKeepingCentre(36, 36));
    r.removeFromLeft(12);
    loop.setBounds(r.removeFromRight(64).withSizeKeepingCentre(64, 30));
    r.removeFromRight(12);
    speed.setBounds(r.removeFromRight(78).withSizeKeepingCentre(78, 30));
    r.removeFromRight(12);
    r.removeFromRight(104); // Zeitangabe
    r.removeFromRight(12);
    slider.setBounds(r);
}

void Transport::paint(juce::Graphics& g)
{
    g.setFont(mono(12.0f));
    g.setColour(col::textMuted);
    const int x = speed.getX() - 12 - 104;
    g.drawText(timeText, x, 0, 104, getHeight(), juce::Justification::centredRight);
}

void Transport::refresh()
{
    play.playing = S.playing;
    play.setTitle(S.playing ? "Stop" : "Play");
    play.repaint();
    slider.setValue(jsRound(S.t / S.duration * 1000.0), juce::dontSendNotification);
    timeText = juce::String(S.t, 1) + " / " + juce::String(S.duration, 1) + " s";
    repaint(speed.getX() - 130, 0, 130, getHeight());
}
} // namespace ui
