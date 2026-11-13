#include "Theme.h"
#include "BinaryData.h"

namespace ui
{
namespace
{
struct Faces
{
    juce::Typeface::Ptr regular, medium, semibold, monoRegular, monoMedium;
    Faces()
    {
        regular = juce::Typeface::createSystemTypefaceFor(ChoreoFonts::InterRegular_ttf, ChoreoFonts::InterRegular_ttfSize);
        medium = juce::Typeface::createSystemTypefaceFor(ChoreoFonts::InterMedium_ttf, ChoreoFonts::InterMedium_ttfSize);
        semibold = juce::Typeface::createSystemTypefaceFor(ChoreoFonts::InterSemiBold_ttf, ChoreoFonts::InterSemiBold_ttfSize);
        monoRegular = juce::Typeface::createSystemTypefaceFor(ChoreoFonts::JetBrainsMonoRegular_ttf, ChoreoFonts::JetBrainsMonoRegular_ttfSize);
        monoMedium = juce::Typeface::createSystemTypefaceFor(ChoreoFonts::JetBrainsMonoMedium_ttf, ChoreoFonts::JetBrainsMonoMedium_ttfSize);
    }
};
Faces& faces()
{
    static Faces f;
    return f;
}
} // namespace

juce::Font sans(float size, int weight)
{
    auto& f = faces();
    auto tf = weight >= 600 ? f.semibold : weight >= 500 ? f.medium : f.regular;
    return juce::Font(juce::FontOptions(tf).withHeight(size));
}

juce::Font mono(float size, int weight)
{
    auto& f = faces();
    return juce::Font(juce::FontOptions(weight >= 500 ? f.monoMedium : f.monoRegular).withHeight(size));
}

void setVariant(juce::Component& c, const juce::String& v) { c.getProperties().set("variant", v); }

static juce::String variantOf(const juce::Component& c) { return c.getProperties()["variant"].toString(); }

LookAndFeel::LookAndFeel()
{
    regular = faces().regular;
    medium = faces().medium;
    semibold = faces().semibold;

    setColour(juce::ResizableWindow::backgroundColourId, col::bg);
    setColour(juce::DocumentWindow::textColourId, col::text);
    setColour(juce::TextButton::buttonColourId, col::surface2);
    setColour(juce::TextButton::textColourOffId, col::text);
    setColour(juce::TextButton::textColourOnId, col::primaryInk);
    setColour(juce::ComboBox::backgroundColourId, col::surface2);
    setColour(juce::ComboBox::textColourId, col::text);
    setColour(juce::ComboBox::outlineColourId, col::border);
    setColour(juce::ComboBox::arrowColourId, col::textMuted);
    setColour(juce::ComboBox::focusedOutlineColourId, col::primary);
    setColour(juce::PopupMenu::backgroundColourId, col::surface2);
    setColour(juce::PopupMenu::textColourId, col::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, col::surface3);
    setColour(juce::PopupMenu::highlightedTextColourId, col::text);
    setColour(juce::TextEditor::backgroundColourId, col::surface2);
    setColour(juce::TextEditor::textColourId, col::text);
    setColour(juce::TextEditor::outlineColourId, col::border);
    setColour(juce::TextEditor::focusedOutlineColourId, col::primary);
    setColour(juce::TextEditor::highlightColourId, col::primary.withAlpha(0.35f));
    setColour(juce::TextEditor::highlightedTextColourId, col::text);
    setColour(juce::CaretComponent::caretColourId, col::primary);
    setColour(juce::Label::textColourId, col::text);
    setColour(juce::Label::textWhenEditingColourId, col::text);
    setColour(juce::ToggleButton::textColourId, col::text);
    setColour(juce::ToggleButton::tickColourId, col::primaryInk);
    setColour(juce::ScrollBar::thumbColourId, col::surface3);
    setColour(juce::Slider::thumbColourId, col::primary);
    setColour(juce::Slider::trackColourId, col::primary);
    setColour(juce::Slider::backgroundColourId, col::surface3);
    setColour(juce::AlertWindow::backgroundColourId, col::surface);
    setColour(juce::AlertWindow::textColourId, col::text);
    setColour(juce::AlertWindow::outlineColourId, col::border);
    setColour(juce::TooltipWindow::backgroundColourId, col::surface3);
    setColour(juce::TooltipWindow::textColourId, col::text);
    setColour(juce::TooltipWindow::outlineColourId, col::border);
    setColour(juce::CaretComponent::caretColourId, col::primary);

    auto scheme = getDarkColourScheme();
    scheme.setUIColour(ColourScheme::windowBackground, col::bg);
    scheme.setUIColour(ColourScheme::widgetBackground, col::surface2);
    scheme.setUIColour(ColourScheme::menuBackground, col::surface2);
    scheme.setUIColour(ColourScheme::outline, col::border);
    scheme.setUIColour(ColourScheme::defaultText, col::text);
    scheme.setUIColour(ColourScheme::defaultFill, col::primary);
    scheme.setUIColour(ColourScheme::highlightedText, col::text);
    scheme.setUIColour(ColourScheme::highlightedFill, col::surface3);
    scheme.setUIColour(ColourScheme::menuText, col::text);
    setColourScheme(scheme);
    // setColourScheme überschreibt einige Einzelfarben
    setColour(juce::TextButton::buttonColourId, col::surface2);
    setColour(juce::ComboBox::outlineColourId, col::border);
    setColour(juce::TextEditor::outlineColourId, col::border);
    setColour(juce::TextEditor::focusedOutlineColourId, col::primary);
    setColour(juce::TextEditor::backgroundColourId, col::surface2);
}

juce::Typeface::Ptr LookAndFeel::getTypefaceForFont(const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName() || f.getTypefaceName().isEmpty())
        return f.isBold() ? semibold : regular;
    return juce::LookAndFeel_V4::getTypefaceForFont(f);
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto v = variantOf(b);
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    if (v == "invisible")
    {
        if (b.hasKeyboardFocus(false))
        {
            g.setColour(col::primary.withAlpha(0.8f));
            g.drawRoundedRectangle(r.reduced(1.0f), 6.0f, 1.5f);
        }
        return;
    }
    const float rad = v == "seg" || v == "tab" ? 6.0f : 7.0f;
    const bool on = b.getToggleState();
    const bool enabled = b.isEnabled();
    juce::Colour fill = col::surface2, line = col::border;

    if (v == "primary") { fill = over ? col::primaryHover : col::primary; line = fill; }
    else if (v == "ghost") { fill = over ? col::surface2 : juce::Colours::transparentBlack; line = over ? col::border : juce::Colours::transparentBlack; }
    else if (v == "danger") { fill = over ? col::error.withAlpha(0.14f) : juce::Colours::transparentBlack; line = juce::Colours::transparentBlack; }
    else if (v == "seg") { fill = on ? col::primary : (over ? col::surface3 : col::surface2); line = on ? col::primary : col::border; }
    else if (v == "tab") { fill = on ? col::surface3 : (over ? col::surface2 : col::surface); line = on ? juce::Colour(0xff4a4e57) : col::border; }
    else if (v == "step") { fill = on ? col::surface2 : (over ? col::surface.brighter(0.03f) : juce::Colours::transparentBlack); line = on ? col::border : juce::Colours::transparentBlack; }
    else if (over) fill = col::surface3;

    if (down) fill = fill.darker(0.08f);
    if (! enabled) fill = fill.withMultipliedAlpha(0.5f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, rad);
    if (! line.isTransparent())
    {
        g.setColour(line);
        g.drawRoundedRectangle(r, rad, 1.0f);
    }
    if (b.hasKeyboardFocus(false) && b.isShowing() && v != "step")
    {
        g.setColour(col::primary.withAlpha(0.8f));
        g.drawRoundedRectangle(r.reduced(1.0f), rad, 1.5f);
    }
}

juce::Font LookAndFeel::getTextButtonFont(juce::TextButton& b, int)
{
    const auto v = variantOf(b);
    if (v == "primary" || (v == "seg" && b.getToggleState())) return sans(14.0f, 600);
    if (v == "tab") return sans(13.5f, 500);
    if (v == "small") return sans(12.5f, 500);
    return sans(14.0f, 500);
}

void LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const auto v = variantOf(b);
    juce::Colour c = col::text;
    if (v == "primary" || (v == "seg" && b.getToggleState())) c = col::primaryInk;
    else if (v == "danger") c = col::error;
    else if (v == "tab") c = b.getToggleState() ? col::text : col::textMuted;
    if (! b.isEnabled()) c = c.withMultipliedAlpha(0.45f);
    if (v == "invisible") return;
    if (v == "tab" && b.getButtonText().containsChar('\t'))
    {
        const auto name = b.getButtonText().upToFirstOccurrenceOf("\t", false, false);
        const auto t = b.getButtonText().fromFirstOccurrenceOf("\t", false, false);
        const auto f1 = getTextButtonFont(b, b.getHeight());
        const auto f2 = mono(12.0f);
        const int w1 = juce::GlyphArrangement::getStringWidthInt(f1, name), w2 = juce::GlyphArrangement::getStringWidthInt(f2, t);
        int x = juce::jmax(8, (b.getWidth() - (w1 + 5 + w2)) / 2);
        g.setColour(c);
        g.setFont(f1);
        g.drawText(name, x, 0, w1 + 1, b.getHeight(), juce::Justification::centredLeft, true);
        g.setColour(col::textFaint);
        g.setFont(f2);
        g.drawText(t, x + w1 + 5, 0, w2 + 1, b.getHeight(), juce::Justification::centredLeft, true);
        return;
    }
    g.setColour(c);
    g.setFont(getTextButtonFont(b, b.getHeight()));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(8, 2), juce::Justification::centred, 1, 0.9f);
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0, 0, (float) w, (float) h).reduced(0.5f);
    const bool small = variantOf(box) == "small";
    g.setColour(box.isEnabled() ? col::surface2 : col::surface);
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(box.hasKeyboardFocus(true) ? col::primary : (box.isMouseOver(true) ? juce::Colour(0xff4a4e57) : col::border));
    g.drawRoundedRectangle(r, 7.0f, 1.0f);
    // Pfeil
    const float ax = (float) w - (small ? 16.0f : 20.0f), ay = (float) h * 0.5f;
    juce::Path p;
    p.startNewSubPath(ax - 4.5f, ay - 2.0f);
    p.lineTo(ax, ay + 2.5f);
    p.lineTo(ax + 4.5f, ay - 2.0f);
    g.setColour(box.isEnabled() ? col::textMuted : col::textFaint);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font LookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return variantOf(box) == "small" ? sans(13.0f) : sans(14.0f);
}

void LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    const bool small = variantOf(box) == "small";
    label.setBounds(small ? 6 : 8, 1, box.getWidth() - (small ? 28 : 36), box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
    label.setMinimumHorizontalScale(0.85f);
}

juce::PopupMenu::Options LookAndFeel::getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label)
{
    return juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label).withStandardItemHeight(28);
}

void LookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h)
{
    g.fillAll(col::surface2);
    g.setColour(col::border);
    g.drawRect(0, 0, w, h);
}

void LookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                    bool isHighlighted, bool isTicked, bool, const juce::String& text, const juce::String&,
                                    const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(col::divider);
        g.fillRect(area.reduced(6, 0).withHeight(1).withY(area.getCentreY()));
        return;
    }
    auto r = area.reduced(3, 1);
    if (isHighlighted && isActive)
    {
        g.setColour(col::surface3);
        g.fillRoundedRectangle(r.toFloat(), 5.0f);
    }
    if (isTicked)
    {
        g.setColour(col::primary);
        g.fillEllipse((float) r.getX() + 7.0f, (float) r.getCentreY() - 3.0f, 6.0f, 6.0f);
    }
    g.setColour(isActive ? col::text : col::textFaint);
    g.setFont(getPopupMenuFont());
    g.drawFittedText(text, r.withTrimmedLeft(22).withTrimmedRight(8), juce::Justification::centredLeft, 1, 0.9f);
}

juce::Font LookAndFeel::getPopupMenuFont() { return sans(14.0f); }

void LookAndFeel::fillTextEditorBackground(juce::Graphics& g, int w, int h, juce::TextEditor& ed)
{
    g.setColour(ed.isEnabled() ? col::surface2 : col::surface);
    g.fillRoundedRectangle(juce::Rectangle<float>(0, 0, (float) w, (float) h).reduced(0.5f), 7.0f);
}

void LookAndFeel::drawTextEditorOutline(juce::Graphics& g, int w, int h, juce::TextEditor& ed)
{
    if (ed.isReadOnly() && ed.getProperties()["variant"].toString() == "plain") return;
    g.setColour(ed.hasKeyboardFocus(true) && ! ed.isReadOnly() ? col::primary : col::border);
    g.drawRoundedRectangle(juce::Rectangle<float>(0, 0, (float) w, (float) h).reduced(0.5f), 7.0f, 1.0f);
}

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const float s = 16.0f;
    const bool small = variantOf(b) == "small";
    auto box = juce::Rectangle<float>(1.0f, ((float) b.getHeight() - s) * 0.5f, s, s);
    const bool on = b.getToggleState();
    g.setColour(on ? col::primary : col::surface2);
    g.fillRoundedRectangle(box, 4.0f);
    g.setColour(on ? col::primary : (over ? juce::Colour(0xff4a4e57) : col::border));
    g.drawRoundedRectangle(box.reduced(0.5f), 4.0f, 1.0f);
    if (on)
    {
        juce::Path p;
        p.startNewSubPath(box.getX() + 4.0f, box.getCentreY());
        p.lineTo(box.getX() + 7.0f, box.getCentreY() + 3.0f);
        p.lineTo(box.getRight() - 3.5f, box.getY() + 4.5f);
        g.setColour(col::primaryInk);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    if (b.hasKeyboardFocus(false))
    {
        g.setColour(col::primary.withAlpha(0.6f));
        g.drawRoundedRectangle(box.expanded(2.0f), 5.0f, 1.2f);
    }
    g.setColour(b.isEnabled() ? col::text : col::textFaint);
    g.setFont(small ? sans(13.0f) : sans(14.0f));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().withTrimmedLeft((int) s + 9), juce::Justification::centredLeft, 2, 0.9f);
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                   juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cy = (float) y + (float) h * 0.5f;
    auto track = juce::Rectangle<float>((float) x, cy - 2.0f, (float) w, 4.0f);
    g.setColour(col::surface3);
    g.fillRoundedRectangle(track, 2.0f);
    g.setColour(col::primary);
    g.fillRoundedRectangle(track.withRight(pos), 2.0f);
    const float r = 7.0f;
    g.setColour(s.isMouseButtonDown() ? col::primaryHover : col::primary);
    g.fillEllipse(pos - r, cy - r, 2 * r, 2 * r);
    g.setColour(col::viewBg);
    g.drawEllipse(pos - r, cy - r, 2 * r, 2 * r, 1.5f);
}

void LookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                                int thumbSize, bool over, bool down)
{
    juce::Rectangle<float> thumb = vertical ? juce::Rectangle<float>((float) x + 2, (float) thumbStart, (float) w - 4, (float) thumbSize)
                                            : juce::Rectangle<float>((float) thumbStart, (float) y + 2, (float) thumbSize, (float) h - 4);
    g.setColour(down ? juce::Colour(0xff50545d) : over ? juce::Colour(0xff43464e) : col::surface3);
    g.fillRoundedRectangle(thumb, 3.0f);
}

void LookAndFeel::drawAlertBox(juce::Graphics& g, juce::AlertWindow& w, const juce::Rectangle<int>& textArea, juce::TextLayout& layout)
{
    g.fillAll(col::surface);
    g.setColour(col::border);
    g.drawRect(w.getLocalBounds());
    layout.draw(g, textArea.toFloat());
}

void LookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int w, int h)
{
    g.fillAll(col::surface3);
    g.setColour(col::border);
    g.drawRect(0, 0, w, h);
    g.setColour(col::text);
    g.setFont(sans(13.0f));
    g.drawFittedText(text, juce::Rectangle<int>(0, 0, w, h).reduced(6, 3), juce::Justification::centredLeft, 6);
}
void LookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    if (dynamic_cast<juce::ComboBox*>(label.getParentComponent()) == nullptr)
    {
        juce::LookAndFeel_V4::drawLabel(g, label);
        return;
    }
    if (label.isBeingEdited()) return;
    g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(label.isEnabled() ? 1.0f : 0.5f));
    g.setFont(getLabelFont(label));
    auto area = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
    g.drawText(label.getText(), area, juce::Justification::centredLeft, true);
}
} // namespace ui
