#pragma once
#include "AppState.h"
#include "Widgets.h"

namespace ui
{
class TopView : public juce::Component
{
public:
    explicit TopView(AppState& s) : S(s) {}
    void paint(juce::Graphics&) override;

private:
    AppState& S;
};

class SideView : public juce::Component
{
public:
    explicit SideView(AppState& s) : S(s) {}
    void paint(juce::Graphics&) override;

private:
    AppState& S;
};

class Legend : public juce::Component
{
public:
    explicit Legend(AppState& s) : S(s) {}
    void paint(juce::Graphics&) override;

private:
    AppState& S;
};

class TimelineView : public juce::Component
{
public:
    explicit TimelineView(AppState& s) : S(s) {}
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override { scrub((float) e.position.x); }
    void mouseDrag(const juce::MouseEvent& e) override { scrub((float) e.position.x); }
    int preferredHeight() const;
    std::function<void(double)> onSeek;

private:
    void scrub(float x);
    AppState& S;
    float x0 = 96, x1 = 100;
};

// Kopfleiste über einer Ansicht (figcaption)
class Caption : public juce::Component
{
public:
    Caption(const juce::String& t, const juce::String& m) : title(t), muted(m) {}
    void paint(juce::Graphics&) override;

private:
    juce::String title, muted;
};

// Gerahmte Ansicht mit Titel
class ViewFrame : public juce::Component
{
public:
    ViewFrame(const juce::String& title, const juce::String& muted, juce::Component& content);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Caption caption;
    juce::Component& content;
};

class PlayButton : public juce::Button
{
public:
    PlayButton() : juce::Button("Play") {}
    bool playing = false;
    void paintButton(juce::Graphics&, bool over, bool down) override;
};

class Transport : public juce::Component
{
public:
    explicit Transport(AppState& s);
    void resized() override;
    void paint(juce::Graphics&) override;
    void refresh(); // Zeit und Regler nachführen
    std::function<void(bool)> onPlay;
    std::function<void(double)> onSeek;

private:
    AppState& S;
    PlayButton play;
    juce::Slider slider;
    juce::ComboBox speed;
    juce::ToggleButton loop { "Loop" };
    juce::String timeText;
};
} // namespace ui
