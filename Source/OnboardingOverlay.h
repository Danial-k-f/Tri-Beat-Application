#pragma once
#include <JuceHeader.h>
#include <functional>

class OnboardingOverlay : public juce::Component
{
public:
    using DoneCallback = std::function<void(bool dontShowAgain)>;

    explicit OnboardingOverlay(DoneCallback onDone)
        : done(std::move(onDone))
    {
        auto S = [](const char* cstr) -> juce::String
            {
                // Guard against nullptr; treat input as ANSI/ASCII.
                return juce::String(cstr != nullptr ? cstr : "");
            };

        setInterceptsMouseClicks(true, true);

        addAndMakeVisible(title);
        addAndMakeVisible(body);
        addAndMakeVisible(nextBtn);
        addAndMakeVisible(backBtn);
        addAndMakeVisible(skipBtn);
        addAndMakeVisible(dontShow);

        title.setJustificationType(juce::Justification::centred);
        title.setFont(juce::Font(20.0f, juce::Font::bold));
        body.setJustificationType(juce::Justification::centred);
        body.setFont(juce::Font(15.0f));

        nextBtn.setButtonText("Next");
        backBtn.setButtonText("Back");
        skipBtn.setButtonText("Skip");
        dontShow.setButtonText("Don't show this tour again");

        backBtn.onClick = [this] { if (page > 0) { --page; refresh(); } };
        nextBtn.onClick = [this] { if (page + 1 < (int)pages.size()) { ++page; refresh(); } else finish(); };
        skipBtn.onClick = [this] { finish(); };

        // Only ASCII here (no bullets, no smart quotes)
        pages = {
            { S("Welcome to Tri-Beat"),
              S("Create layered polyrhythms and polymeters with polygon shapes. "
                 "Each shape is a rhythmic cycle. Stack multiple shapes for complex grooves.") },

            { S("Transport"),
              S("Use Play / Stop to control audio. The global metronome runs in 4/4 and shapes align to this clock.") },

            { S("Adding & Removing Shapes"),
              S("Click Add Shape to create a new polygon layer. Click Remove Shape to delete the selected one. "
                 "Use the Layer menu to switch between layers.") },

            { S("Sides & Polygon Editing"),
              S("Use + / - to change the number of sides (triangle=3, pentagon=5, etc.). "
                 "More sides means finer subdivisions of the host bar.") },

            { S("Accents"),
              S("Click a vertex to set an Upbeat (yellow). Shift+Click sets a Downbeat (cyan). "
                 "Unaccented vertices are grey. The orange dot shows the playhead.") },

            { S("Samples"),
              S("Load your own audio for each role:\n"
                 "- Upbeat Sample -> kick\n"
                 "- Downbeat Sample -> snare\n"
                 "- Subdivision Sample -> hi-hat\n"
                 "If a role has no sample, a short click will play instead.") },

            { S("Mute Subdivisions"),
              S("Toggle Mute Subdivisions to silence all non-accent steps. Only Upbeat/Downbeat accents will sound.") },

            { S("Polyrhythm vs Polymeter"),
              S("Polyrhythm: the shape's N points are distributed across the host 4/4 bar (e.g., 3-in-4).\n"
                 "Polymeter: the shape advances once per beat in its own meter; accents occur on bar positions.") },

            { S("Colour Coding"),
              S("Grey = subdivision, Yellow = Upbeat, Cyan = Downbeat, Orange = playhead.") },

            { S("You're Ready!"),
              S("Experiment with multiple shapes, accents, and samples to design rich rhythmic textures. "
                 "Press Finish to start creating.") }
        };

        refresh();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.55f));
        auto r = getLocalBounds().reduced(juce::jmax(getWidth() / 6, 140));
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(r.toFloat(), 12.0f);
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(r.toFloat(), 12.0f, 1.5f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(juce::jmax(getWidth() / 6, 140));
        auto top = r.removeFromTop(60);
        title.setBounds(top);

        auto middle = r.removeFromTop(r.getHeight() - 60);
        body.setBounds(middle.reduced(20));

        auto bottom = r;

        
        auto left = bottom.removeFromLeft(120);
        backBtn.setBounds(left.reduced(10));

        
        auto right = bottom.removeFromRight(220); 
        auto half = right.reduced(10);
        auto skipArea = half.removeFromLeft(half.getWidth() / 2);
        skipBtn.setBounds(skipArea.reduced(5));
        nextBtn.setBounds(half.reduced(5));

        
        dontShow.setBounds(bottom.reduced(10));
    }

private:
    struct Page { juce::String title, text; };
    std::vector<Page> pages;
    int page = 0;

    juce::Label title, body;
    juce::TextButton nextBtn, backBtn, skipBtn;
    juce::ToggleButton dontShow;

    DoneCallback done;

    void refresh()
    {
        if (pages.empty())
        {
            title.setText({}, juce::dontSendNotification);
            body.setText({}, juce::dontSendNotification);
            nextBtn.setEnabled(false);
            backBtn.setEnabled(false);
            return;
        }

        title.setText(pages[(size_t)page].title, juce::dontSendNotification);
        body.setText(pages[(size_t)page].text, juce::dontSendNotification);
        backBtn.setEnabled(page > 0);
        nextBtn.setButtonText(page + 1 < (int)pages.size() ? "Next" : "Finish");
        repaint();
    }

    void finish()
    {
        if (done) done(dontShow.getToggleState());
        if (auto* p = getParentComponent()) p->removeChildComponent(this);
        delete this;
    }
};
