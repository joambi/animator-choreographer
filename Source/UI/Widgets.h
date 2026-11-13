#pragma once
#include "Theme.h"
#include <functional>
#include <vector>

namespace ui
{
// Bausteine mit Höhe abhängig von der Breite. Das Fragen-Panel wird daraus aufgebaut und bei
// Änderungen neu erzeugt (wie renderPanel der Web-App).
class Block : public juce::Component
{
public:
    virtual int heightFor(int width) = 0;
};

// Umbrechender Text, optional aus mehreren Läufen (Schrift/Farbe)
class TextBlock : public Block
{
public:
    TextBlock() = default;
    TextBlock(const juce::String& text, juce::Font font, juce::Colour colour);
    TextBlock& append(const juce::String& text, juce::Font font, juce::Colour colour);
    void setText(const juce::String& text, juce::Font font, juce::Colour colour);
    int heightFor(int width) override;
    void paint(juce::Graphics&) override;
    float lineSpacing = 2.0f;
    void setJustification(juce::Justification j) { as.setJustification(j); repaint(); }

private:
    juce::AttributedString as;
};

// Senkrechter Stapel
class Flow : public Block
{
public:
    Flow(int gap = 12, juce::BorderSize<int> padding = {});

    template <typename T>
    T& add(std::unique_ptr<T> c, int gapBefore = -1)
    {
        T& ref = *c;
        Block* blk = dynamic_cast<Block*>(c.get());
        jassert(blk != nullptr); // Nicht-Block-Komponenten gehören in addFixed()
        items.push_back({ std::move(c), [blk] (int w) { return blk != nullptr ? blk->heightFor(w) : 34; }, gapBefore });
        addAndMakeVisible(ref);
        return ref;
    }
    template <typename T>
    T& addFixed(std::unique_ptr<T> c, int height, int gapBefore = -1)
    {
        T& ref = *c;
        items.push_back({ std::move(c), [height] (int) { return height; }, gapBefore });
        addAndMakeVisible(ref);
        return ref;
    }
    void clear();
    bool isEmpty() const { return items.empty(); }

    int heightFor(int width) override { return layout(width, false); }
    void resized() override { layout(getWidth(), true); }

    juce::BorderSize<int> padding;
    int gap;

private:
    struct Item
    {
        std::unique_ptr<juce::Component> comp;
        std::function<int(int)> height;
        int gapBefore;
    };
    std::vector<Item> items;
    int layout(int width, bool apply);
};

// Nebeneinander, gleich breite Spalten (grid-2 / grid-3) oder feste Spaltenbreiten
class Row : public Block
{
public:
    explicit Row(int gap = 12) : gap(gap) {}
    // weight > 0: Anteil; weight < 0: feste Breite in px
    template <typename T>
    T& add(std::unique_ptr<T> c, float weight = 1.0f, int fixedHeight = -1)
    {
        T& ref = *c;
        Block* blk = dynamic_cast<Block*>(c.get());
        std::function<int(int)> h = blk ? std::function<int(int)>([blk] (int w) { return blk->heightFor(w); })
                                        : std::function<int(int)>([fixedHeight] (int) { return fixedHeight; });
        cols.push_back({ std::move(c), std::move(h), weight });
        addAndMakeVisible(ref);
        return ref;
    }
    void addSpacer(float weight = 1.0f);
    int heightFor(int width) override { return layout(width, false); }
    void resized() override { layout(getWidth(), true); }
    int gap;
    bool alignCentre = false;
    bool alignBottom = true;

private:
    struct Col
    {
        std::unique_ptr<juce::Component> comp;
        std::function<int(int)> height;
        float weight;
    };
    std::vector<Col> cols;
    std::vector<int> widths(int width) const;
    int layout(int width, bool apply);
};

// Beschriftetes Eingabeelement
class Field : public Block
{
public:
    Field(const juce::String& label, std::unique_ptr<juce::Component> control, int controlHeight = 34);
    int heightFor(int width) override;
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Component& control() { return *ctrl; }

private:
    TextBlock label;
    bool hasLabel;
    std::unique_ptr<juce::Component> ctrl;
    int controlHeight;
};

// Karte mit Farbmarke und Titel
class Card : public Block
{
public:
    Card(juce::Colour swatch, const juce::String& title, bool derived = false);
    Flow body { 12 };
    int heightFor(int width) override;
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    juce::Colour swatch;
    TextBlock titleBlock;
    bool derived;
};

// Aufklappbarer Bereich (details/summary)
class Disclosure : public Block, private juce::Button::Listener
{
public:
    Disclosure(const juce::String& summary, bool open, const juce::String& badge = {}, juce::Colour swatch = {});
    Flow body { 12 };
    int heightFor(int width) override;
    void resized() override;
    void paint(juce::Graphics&) override;
    bool isOpen() const { return open; }
    std::function<void(bool)> onToggle;

private:
    void buttonClicked(juce::Button*) override;
    juce::TextButton header;
    juce::String summary, badge;
    juce::Colour swatch;
    bool open;
};

// Fester Abstand oder Linie
class Spacer : public Block
{
public:
    explicit Spacer(int h) : h(h) {}
    int heightFor(int) override { return h; }

private:
    int h;
};

// Einfacher umbrechender Knopfverbund (Segment, Tabs)
class WrapButtons : public Block
{
public:
    explicit WrapButtons(int gap = 6, int buttonHeight = 34) : gap(gap), bh(buttonHeight) {}
    juce::TextButton& add(const juce::String& text, int minWidth, std::function<void()> onClick, bool on = false,
                          const juce::String& variant = "seg");
    template <typename T>
    T& addComponent(std::unique_ptr<T> c, int width)
    {
        T& ref = *c;
        addAndMakeVisible(ref);
        buttons.push_back({ std::move(c), width });
        return ref;
    }
    int heightFor(int width) override { return layout(width, false); }
    void resized() override { layout(getWidth(), true); }

private:
    struct B
    {
        std::unique_ptr<juce::Component> b;
        int w;
    };
    std::vector<B> buttons;
    int gap, bh;
    int layout(int width, bool apply);
};

// Hilfen für Eingaben
std::unique_ptr<juce::TextEditor> makeTextEditor(const juce::String& text, std::function<void(const juce::String&)> onChange, bool numeric = false);
std::unique_ptr<juce::ComboBox> makeCombo(const juce::StringArray& labels, int selectedIndex, std::function<void(int)> onChange);
std::unique_ptr<juce::TextButton> makeButton(const juce::String& text, const juce::String& variant, std::function<void()> onClick);
std::unique_ptr<juce::ToggleButton> makeToggle(const juce::String& text, bool on, std::function<void(bool)> onChange, bool small = false);

// Zahl aus Eingabe: Komma oder Punkt; leer → std::nullopt
std::optional<double> parseNumber(const juce::String&);
juce::String formatNumber(double v, int maxDecimals = 3);
juce::String fixed(double v, int decimals);
} // namespace ui
