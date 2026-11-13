#include "Widgets.h"

namespace ui
{
// ------------------------------------------------------------------ TextBlock
TextBlock::TextBlock(const juce::String& text, juce::Font font, juce::Colour colour) { setText(text, font, colour); }

void TextBlock::setText(const juce::String& text, juce::Font font, juce::Colour colour)
{
    as.clear();
    as.setWordWrap(juce::AttributedString::byWord);
    append(text, font, colour);
}

TextBlock& TextBlock::append(const juce::String& text, juce::Font font, juce::Colour colour)
{
    as.setLineSpacing(lineSpacing);
    as.append(text, font, colour);
    repaint();
    return *this;
}

int TextBlock::heightFor(int width)
{
    if (as.getText().isEmpty()) return 0;
    juce::TextLayout tl;
    as.setLineSpacing(lineSpacing);
    tl.createLayout(as, (float) juce::jmax(10, width));
    return (int) std::ceil(tl.getHeight()) + 1;
}

void TextBlock::paint(juce::Graphics& g)
{
    juce::TextLayout tl;
    tl.createLayout(as, (float) juce::jmax(10, getWidth()));
    tl.draw(g, getLocalBounds().toFloat());
}

// ------------------------------------------------------------------ Flow
Flow::Flow(int gap_, juce::BorderSize<int> pad) : padding(pad), gap(gap_) {}

void Flow::clear()
{
    removeAllChildren();
    items.clear();
}

int Flow::layout(int width, bool apply)
{
    const int inner = width - padding.getLeftAndRight();
    int y = padding.getTop();
    bool first = true;
    for (auto& it : items)
    {
        if (! it.comp->isVisible() && apply) { /* sichtbar bleibt sichtbar */ }
        const int h = it.height(inner);
        if (! first) y += it.gapBefore >= 0 ? it.gapBefore : gap;
        else if (it.gapBefore > 0) y += it.gapBefore;
        if (apply) it.comp->setBounds(padding.getLeft(), y, inner, h);
        y += h;
        first = false;
    }
    return y + padding.getBottom();
}

// ------------------------------------------------------------------ Row
void Row::addSpacer(float weight)
{
    cols.push_back({ std::make_unique<juce::Component>(), [] (int) { return 0; }, weight });
}

std::vector<int> Row::widths(int width) const
{
    std::vector<int> w(cols.size(), 0);
    int fixedTotal = 0;
    float weights = 0;
    for (auto& c : cols)
    {
        if (c.weight < 0) fixedTotal += (int) -c.weight;
        else weights += c.weight;
    }
    const int avail = juce::jmax(0, width - fixedTotal - gap * (int) juce::jmax<size_t>(0, cols.size() - 1));
    for (size_t i = 0; i < cols.size(); ++i)
        w[i] = cols[i].weight < 0 ? (int) -cols[i].weight : (weights > 0 ? (int) std::floor(avail * cols[i].weight / weights) : 0);
    return w;
}

int Row::layout(int width, bool apply)
{
    auto w = widths(width);
    int h = 0;
    for (size_t i = 0; i < cols.size(); ++i) h = juce::jmax(h, cols[i].height(w[i]));
    if (apply)
    {
        int x = 0;
        for (size_t i = 0; i < cols.size(); ++i)
        {
            const int ch = cols[i].height(w[i]);
            const bool isBlock = dynamic_cast<Block*>(cols[i].comp.get()) != nullptr;
            const int y = alignCentre ? (h - ch) / 2 : (alignBottom ? h - ch : 0);
            cols[i].comp->setBounds(x, y, w[i], isBlock || alignCentre || alignBottom ? ch : h);
            x += w[i] + gap;
        }
    }
    return h;
}

// ------------------------------------------------------------------ Field
Field::Field(const juce::String& l, std::unique_ptr<juce::Component> c, int ch) : hasLabel(l.isNotEmpty()), ctrl(std::move(c)), controlHeight(ch)
{
    label.lineSpacing = 0.0f;
    label.setText(l, sans(12.5f), col::textMuted);
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);
    addAndMakeVisible(*ctrl);
}

int Field::heightFor(int width) { return (hasLabel ? label.heightFor(width) + 4 : 0) + controlHeight; }

void Field::resized()
{
    const int lh = hasLabel ? label.heightFor(getWidth()) : 0;
    label.setBounds(0, 0, getWidth(), lh);
    ctrl->setBounds(0, getHeight() - controlHeight, getWidth(), controlHeight);
}

void Field::paint(juce::Graphics&) {}

// ------------------------------------------------------------------ Card
Card::Card(juce::Colour s, const juce::String& title, bool d) : swatch(s), derived(d)
{
    titleBlock.setText(title, sans(14.0f, 600), col::text);
    addAndMakeVisible(titleBlock);
    addAndMakeVisible(body);
}

int Card::heightFor(int width)
{
    const int inner = width - 32 - 20;
    int h = 16 + juce::jmax(18, titleBlock.heightFor(inner));
    const int bh = body.heightFor(width - 32);
    if (bh > 0) h += 12 + bh;
    return h + 16;
}

void Card::resized()
{
    const int th = juce::jmax(18, titleBlock.heightFor(getWidth() - 52));
    titleBlock.setBounds(36, 16, getWidth() - 52, th);
    const int bh = body.heightFor(getWidth() - 32);
    body.setBounds(16, 16 + th + 12, getWidth() - 32, bh);
}

void Card::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    if (! derived)
    {
        g.setColour(col::surface);
        g.fillRoundedRectangle(r, 9.0f);
    }
    g.setColour(col::border);
    if (derived)
    {
        juce::Path p;
        p.addRoundedRectangle(r, 9.0f);
        juce::Path dashed;
        const float dash[] = { 5.0f, 4.0f };
        juce::PathStrokeType(1.0f).createDashedStroke(dashed, p, dash, 2);
        g.fillPath(dashed);
    }
    else
        g.drawRoundedRectangle(r, 9.0f, 1.0f);
    g.setColour(swatch);
    g.fillRoundedRectangle(16.0f, 20.0f, 12.0f, 12.0f, 3.0f);
}

// ------------------------------------------------------------------ Disclosure
Disclosure::Disclosure(const juce::String& s, bool o, const juce::String& b, juce::Colour sw) : summary(s), badge(b), swatch(sw), open(o)
{
    header.setButtonText({});
    setVariant(header, "invisible");
    header.onStateChange = [this] { repaint(); };
    header.addListener(this);
    header.setWantsKeyboardFocus(true);
    header.setTitle(summary);
    addAndMakeVisible(header);
    addChildComponent(body);
    body.setVisible(open);
}

void Disclosure::buttonClicked(juce::Button*)
{
    open = ! open;
    body.setVisible(open);
    if (onToggle) onToggle(open);
}

int Disclosure::heightFor(int width) { return 36 + (open ? 12 + body.heightFor(width) : 0); }

void Disclosure::resized()
{
    header.setBounds(0, 0, getWidth(), 36);
    body.setBounds(0, 48, getWidth(), open ? body.heightFor(getWidth()) : 0);
}

void Disclosure::paint(juce::Graphics& g)
{
    g.setColour(col::divider);
    g.fillRect(0, 0, getWidth(), 1);
    // Pfeil
    juce::Path p;
    const float cx = 5.0f, cy = 24.0f;
    if (open) { p.startNewSubPath(cx - 4, cy - 2); p.lineTo(cx, cy + 2.5f); p.lineTo(cx + 4, cy - 2); }
    else { p.startNewSubPath(cx - 2, cy - 4); p.lineTo(cx + 2.5f, cy); p.lineTo(cx - 2, cy + 4); }
    g.setColour(col::textMuted);
    g.strokePath(p, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    int x = 18;
    if (! swatch.isTransparent())
    {
        g.setColour(swatch);
        g.fillRoundedRectangle((float) x, 18.0f, 12.0f, 12.0f, 3.0f);
        x += 20;
    }
    g.setFont(sans(14.0f));
    g.setColour(header.isOver() ? col::text : col::textMuted);
    const int tw = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(), summary);
    const int maxW = getWidth() - x - (badge.isNotEmpty() ? 50 : 0);
    g.drawFittedText(summary, x, 12, juce::jmin(tw + 2, maxW), 24, juce::Justification::centredLeft, 1, 0.8f);
    if (badge.isNotEmpty())
    {
        g.setFont(sans(12.5f));
        g.setColour(col::primary);
        g.drawText(badge, x + juce::jmin(tw + 2, maxW) + 8, 12, 60, 24, juce::Justification::centredLeft);
    }
}

// ------------------------------------------------------------------ WrapButtons
juce::TextButton& WrapButtons::add(const juce::String& text, int minWidth, std::function<void()> onClick, bool on, const juce::String& variant)
{
    auto b = std::make_unique<juce::TextButton>(text);
    setVariant(*b, variant);
    b->setToggleState(on, juce::dontSendNotification);
    b->onClick = std::move(onClick);
    auto& ref = *b;
    addAndMakeVisible(ref);
    const int tw = juce::GlyphArrangement::getStringWidthInt(sans(14.0f, 600), text.upToFirstOccurrenceOf("\n", false, false)) + 26;
    buttons.push_back({ std::move(b), juce::jmax(minWidth, tw) });
    return ref;
}

int WrapButtons::layout(int width, bool apply)
{
    int x = 0, y = 0;
    for (auto& b : buttons)
    {
        if (x > 0 && x + b.w > width) { x = 0; y += bh + gap; }
        if (apply) b.b->setBounds(x, y, juce::jmin(b.w, width), bh);
        x += b.w + gap;
    }
    return buttons.empty() ? 0 : y + bh;
}

// ------------------------------------------------------------------ Eingaben
std::unique_ptr<juce::TextEditor> makeTextEditor(const juce::String& text, std::function<void(const juce::String&)> onChange, bool numeric)
{
    auto ed = std::make_unique<juce::TextEditor>();
    ed->setFont(numeric ? mono(14.0f) : sans(14.0f));
    ed->setIndents(10, 0);
    ed->setJustification(juce::Justification::centredLeft);
    ed->setText(text, false);
    ed->setSelectAllWhenFocused(numeric);
    if (numeric) ed->setInputRestrictions(16, "0123456789.,-");
    auto* raw = ed.get();
    ed->onTextChange = [raw, onChange] { onChange(raw->getText()); };
    ed->onReturnKey = [raw] { raw->giveAwayKeyboardFocus(); };
    ed->onEscapeKey = [raw] { raw->giveAwayKeyboardFocus(); };
    return ed;
}

std::unique_ptr<juce::ComboBox> makeCombo(const juce::StringArray& labels, int selectedIndex, std::function<void(int)> onChange)
{
    auto cb = std::make_unique<juce::ComboBox>();
    for (int i = 0; i < labels.size(); ++i) cb->addItem(labels[i], i + 1);
    cb->setSelectedId(selectedIndex + 1, juce::dontSendNotification);
    auto* raw = cb.get();
    cb->onChange = [raw, onChange] { onChange(raw->getSelectedId() - 1); };
    return cb;
}

std::unique_ptr<juce::TextButton> makeButton(const juce::String& text, const juce::String& variant, std::function<void()> onClick)
{
    auto b = std::make_unique<juce::TextButton>(text);
    setVariant(*b, variant);
    b->onClick = std::move(onClick);
    return b;
}

std::unique_ptr<juce::ToggleButton> makeToggle(const juce::String& text, bool on, std::function<void(bool)> onChange, bool small)
{
    auto t = std::make_unique<juce::ToggleButton>(text);
    if (small) setVariant(*t, "small");
    t->setToggleState(on, juce::dontSendNotification);
    auto* raw = t.get();
    t->onClick = [raw, onChange] { onChange(raw->getToggleState()); };
    return t;
}

std::optional<double> parseNumber(const juce::String& s)
{
    auto t = s.trim().replaceCharacter(',', '.');
    if (t.isEmpty()) return std::nullopt;
    if (! t.containsAnyOf("0123456789")) return std::nullopt;
    const double v = t.getDoubleValue();
    return std::isfinite(v) ? std::optional<double>(v) : std::nullopt;
}

juce::String formatNumber(double v, int maxDecimals)
{
    auto s = juce::String(v, maxDecimals);
    if (s.containsChar('.'))
    {
        while (s.endsWithChar('0')) s = s.dropLastCharacters(1);
        if (s.endsWithChar('.')) s = s.dropLastCharacters(1);
    }
    if (s == "-0") s = "0";
    return s;
}

juce::String fixed(double v, int decimals)
{
    const double f = std::pow(10.0, decimals);
    return juce::String(std::floor(v * f + 0.5) / f, decimals);
}
} // namespace ui
