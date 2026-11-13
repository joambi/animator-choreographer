#pragma once
#include <JuceHeader.h>

namespace ui
{
namespace col
{
    const juce::Colour bg { 0xff15161a };
    const juce::Colour surface { 0xff1b1d22 };
    const juce::Colour surface2 { 0xff22252b };
    const juce::Colour surface3 { 0xff2a2d34 };
    const juce::Colour border { 0xff33363e };
    const juce::Colour divider { 0xff2a2d33 };
    const juce::Colour text { 0xffe7e4dc };
    const juce::Colour textMuted { 0xffa19d93 };
    const juce::Colour textFaint { 0xff6d6a63 };
    const juce::Colour primary { 0xffe8a33d };
    const juce::Colour primaryHover { 0xfff0b65c };
    const juce::Colour primaryInk { 0xff1a1408 };
    const juce::Colour error { 0xffe2726a };
    const juce::Colour warn { 0xffd9b45f };
    const juce::Colour ok { 0xff7cc48a };
    // Ansichten
    const juce::Colour viewBg { 0xff111215 };
    const juce::Colour grid { 0xff2b2e35 };
    const juce::Colour gridFaint { 0xff24272d };
    const juce::Colour label { 0xff6d6a63 };
    const juce::Colour labelMid { 0xff8a867d };
    const juce::Colour labelLight { 0xffc9c5bb };
    const juce::Colour listener { 0xff4a4d55 };
} // namespace col

juce::Font sans(float size, int weight = 400); // 400, 500, 600
juce::Font mono(float size, int weight = 400);

// Knopfvarianten über Komponenten-Eigenschaft "variant": "primary", "ghost", "danger", "seg", "tab"
void setVariant(juce::Component&, const juce::String& variant);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    void drawComboBox(juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox&, juce::Label&) override;

    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawPopupMenuBackground(juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;

    void fillTextEditorBackground(juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline(juce::Graphics&, int w, int h, juce::TextEditor&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius(juce::Slider&) override { return 7; }

    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                       int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return 10; }

    void drawAlertBox(juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
    juce::Font getAlertWindowTitleFont() override { return sans(17.0f, 600); }
    juce::Font getAlertWindowMessageFont() override { return sans(14.0f); }
    juce::Font getAlertWindowFont() override { return sans(14.0f); }

    void drawTooltip(juce::Graphics&, const juce::String& text, int w, int h) override;

private:
    juce::Typeface::Ptr regular, medium, semibold;
};
} // namespace ui
