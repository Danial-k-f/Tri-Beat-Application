#pragma once
#include <JuceHeader.h>
#include <juce_audio_formats/juce_audio_formats.h>


// --- MetronomeSource: 4/4  play/stop/seek ---
struct MetronomeSource : public juce::AudioSource
{
    // timing
    double  sampleRate = 48000.0;
    double  bpm = 120.0;
    double  samplesPerBeat = 48000.0 * 60.0 / 120.0;
    int     beatsPerBar = 4;

    // transport
    int64_t transportSamples = 0;
    int64_t lastBeatIndex = -1;
    bool    playing = true;

    // tiny click synth
    struct ClickSynth {
        double sr = 48000.0, freq = 1200.0, phase = 0.0, inc = 0.0, env = 0.0, decay = 0.995;
        float  gain = 1.0f;
        void prepare(double s) { sr = s; setFreq(freq); env = 0.0; }
        void setFreq(double f) { freq = f; inc = juce::MathConstants<double>::twoPi * freq / sr; }
        void trigger(float g) { env = 1.0; gain = g; }
        float next() {
            float s = (float)std::sin(phase) * (float)env * gain;
            phase += inc; if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            env *= decay; if (env < 1.0e-4) env = 0.0; return s;
        }
    } click;

    // tones
    double downHz = 1000.0, upHz = 1500.0;
    float  downGain = 1.0f, upGain = 0.9f;

    // API
    void setTempo(double b) { bpm = b; samplesPerBeat = sampleRate * 60.0 / juce::jmax(1.0, b); }
    void setBeatsPerBar(int n) { beatsPerBar = juce::jlimit(1, 64, n); }
    void setPlaying(bool b) { playing = b; }
    bool isPlaying() const { return playing; }
    void resetTransport() { transportSamples = 0; lastBeatIndex = -1; }

    // AudioSource
    void prepareToPlay(int /*block*/, double sr) override
    {
        sampleRate = sr; setTempo(bpm); click.prepare(sr); lastBeatIndex = -1;
    }

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        auto* buffer = info.buffer; if (!buffer) return;

        buffer->clear(info.startSample, info.numSamples);
        if (!playing) return;

        auto* L = buffer->getWritePointer(0, info.startSample);
        auto* R = (buffer->getNumChannels() > 1 ? buffer->getWritePointer(1, info.startSample) : nullptr);

        for (int i = 0;i < info.numSamples;++i)
        {
            const int64_t beatIdx = (samplesPerBeat > 0.0 ? (int64_t)std::floor((double)transportSamples / samplesPerBeat) : 0);
            if (beatIdx != lastBeatIndex)
            {
                lastBeatIndex = beatIdx;
                const int which = (int)(beatIdx % (int64_t)juce::jmax(1, beatsPerBar));
                const bool isDown = (which == 0);
                click.setFreq(isDown ? downHz : upHz);
                click.trigger(isDown ? downGain : upGain);
            }
            const float s = click.next();
            L[i] = s; if (R) R[i] = s;
            ++transportSamples;
        }
    }
};



struct ClickSynth
{
    void prepare (double sr)
    {
        sampleRate = sr;
        phase = 0.0;
        env = 0.0;
        setFrequency (clickFreqHz);
    }

    void setFrequency (double hz)
    {
        clickFreqHz = hz;
        phaseInc = juce::MathConstants<double>::twoPi * clickFreqHz / sampleRate;
    }

    void trigger (float gain = 1.0f)   
    {
        env = 1.0;
        hitGain = juce::jlimit (0.0f, 4.0f, gain);
    }

    inline float getNextSample()
    {
        float s = (float) std::sin (phase) * (float) env;
        phase += phaseInc;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;

        env *= envDecay;
        if (env < 1.0e-4) env = 0.0;

        return s * baseGain * hitGain;  
    }

    double sampleRate = 48000.0;
    double clickFreqHz = 1400.0;
    double phase = 0.0, phaseInc = 0.0;
    double env = 0.0, envDecay = 0.995;
    float  baseGain = 0.25f;      
    float  hitGain  = 1.0f;      
};



// =========== ClickAudioSource.h ===========
struct ClickAudioSource : public juce::AudioSource
{
    // ===== timing & mode =====
    double sampleRate = 48000.0, bpm = 120.0, samplesPerBeat = 48000.0 * 60.0 / 120.0;
    int64_t transportSamples = 0;
    bool    polyrhythm = true;
    int     subdivisions = 3;     
    int     beatsPerBar = 3;       
    int     hostBeatsPerBar = 4;   

    // ===== state =====
    int64_t stepSamps = 1, stepCountdown = 1;
    int     stepIndex = -1;
    int64_t lastBeatIndex = -1;
    bool    playing = true;

    // ===== accents =====
    int upIndex = -1, downIndex = -1;

    // ===== tiny click synth (fallback) =====
    struct ClickSynth {
        double sr = 48000.0, freq = 1200.0, phase = 0.0, inc = 0.0, env = 0.0, decay = 0.995;
        float  outGain = 1.0f;
        void prepare(double s) { sr = s; setFreq(freq); env = 0.0; }
        void setFreq(double f) { freq = f; inc = juce::MathConstants<double>::twoPi * freq / sr; }
        void trigger(float g) { env = 1.0; outGain = g; }
        float next() {
            float s = (float)std::sin(phase) * (float)env * outGain;
            phase += inc; if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
            env *= decay; if (env < 1.0e-4) env = 0.0; return s;
        }
    } click;

    // ===== one-shot drum =====
    struct OneShot
    {
        juce::AudioBuffer<float> data;
        int   pos = 0;
        float gain = 1.0f;
        bool  active = false;

        void setSample(juce::AudioBuffer<float>&& buf)
        {
            data = std::move(buf);
            pos = 0; active = false;
        }
        void clear() { data.setSize(0, 0); pos = 0; active = false; }

        void trigger(float g)
        {
            if (data.getNumSamples() > 0) { gain = g; pos = 0; active = true; }
        }
        float next()
        {
            if (!active || data.getNumSamples() == 0) return 0.0f;
            if (pos >= data.getNumSamples()) { active = false; return 0.0f; }
            float s = data.getSample(0, pos);
            if (data.getNumChannels() > 1) s = 0.5f * (s + data.getSample(1, pos));
            ++pos;
            return s * gain;
        }
        bool isActive() const { return active; }
    };

    OneShot kick, snare, hihat;
    bool    muteSubdivisions = false;

    // ===== tones =====
    double normalFreqHz = 1200.0, upFreqHz = 880.0, downFreqHz = 440.0;
    float  normalGain = 0.7f, upGain = 1.0f, downGain = 1.2f;

    // ===== API =====
    void  setTempo(double newBpm) { bpm = newBpm; samplesPerBeat = sampleRate * 60.0 / juce::jmax(1.0, newBpm); recalcStep(); }
    void  setPolyrhythm(bool b) { polyrhythm = b; recalcStep(); }
    void  setSubdivisions(int n) { subdivisions = juce::jlimit(1, 64, n); recalcStep(); }
    void  setBeatsPerBar(int n) { beatsPerBar = juce::jlimit(1, 64, n); }
    void  setHostBeatsPerBar(int n) { hostBeatsPerBar = juce::jlimit(1, 64, n); recalcStep(); }
    void  setUpbeatIndex(int idx) { upIndex = (idx >= 0 ? idx % juce::jmax(1, currentCount()) : -1); }
    void  setDownbeatIndex(int idx) { downIndex = (idx >= 0 ? idx % juce::jmax(1, currentCount()) : -1); }
    void  setUpbeatFreqHz(double f) { upFreqHz = f; }
    void  setDownbeatFreqHz(double f) { downFreqHz = f; }
    void  setNormalFreqHz(double f) { normalFreqHz = f; }
    void  setMuteSubdivisions(bool m) { muteSubdivisions = m; }

    void  setPlaying(bool b) { playing = b; }
    bool  isPlaying() const { return playing; }

    void restart() { transportSamples = 0; stepIndex = -1; lastBeatIndex = -1; recalcStep(); stepCountdown = 1; }
    void resetTransport() { restart(); }
    void setTransportSamples(int64_t s) { transportSamples = s; if (s == 0) { stepIndex = -1; lastBeatIndex = -1; recalcStep(); stepCountdown = 1; } }
    int64_t getTransportSamples() const { return transportSamples; }

    double getPhase01() const
    {
        if (sampleRate <= 0.0 || bpm <= 0.0) return 0.0;
        const double spb = sampleRate * (60.0 / bpm);
        const int    barN = polyrhythm ? hostBeatsPerBar : beatsPerBar; //Measure the bar length
        const double spbar = spb * (double)juce::jmax(1, barN);
        if (spbar <= 0.0) return 0.0;
        return std::fmod((double)transportSamples / spbar, 1.0);
    }
    double getVisualLapPhase01() const { return getPhase01(); }

    // ===== AudioSource =====
    void prepareToPlay(int /*block*/, double sr) override
    {
        sampleRate = sr; setTempo(bpm); click.prepare(sr);
        recalcStep(); stepIndex = -1; stepCountdown = 1; lastBeatIndex = -1;
    }
    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        auto* buffer = info.buffer; if (!buffer) return;
        const int numCh = buffer->getNumChannels();
        buffer->clear(info.startSample, info.numSamples);

        if (!playing) return;

        auto* L = buffer->getWritePointer(0, info.startSample);
        auto* R = (numCh > 1 ? buffer->getWritePointer(1, info.startSample) : nullptr);

        if (polyrhythm)
        {
            for (int i = 0; i < info.numSamples; ++i)
            {
                if (--stepCountdown <= 0)
                {

                  
                    stepCountdown += stepSamps;                     
                    stepIndex = (stepIndex + 1) % juce::jmax(1, subdivisions); 

                   
                    const bool isDown = (downIndex >= 0 && stepIndex == downIndex);
                    const bool isUp = (upIndex >= 0 && stepIndex == upIndex);
                    const bool isSub = (!isDown && !isUp);

                    bool usedSample = false;

                    if (isDown)
                    {
                        if (snare.data.getNumSamples() > 0) { snare.trigger(downGain); usedSample = true; }
                    }
                    else if (isUp)
                    {
                        if (kick.data.getNumSamples() > 0) { kick.trigger(upGain);    usedSample = true; }
                    }
                    else // subdivision
                    {
                        if (!muteSubdivisions)
                        {
                            if (hihat.data.getNumSamples() > 0) { hihat.trigger(normalGain); usedSample = true; }
                        }
                    }

                  
                    if (!usedSample)
                    {
                        if (!(isSub && muteSubdivisions))
                        {
                            double f = normalFreqHz; float g = normalGain;
                            if (isDown) { f = downFreqHz; g = downGain; }
                            else if (isUp) { f = upFreqHz; g = upGain; }
                            click.setFreq(f); click.trigger(g);
                        }
                    }

                }

                const float drums = kick.next() + snare.next() + hihat.next();
                const float tone = click.next();
                const float s = drums + tone;
                L[i] = s; if (R) R[i] = s;
                ++transportSamples;
            }
        }
        else
        {
            for (int i = 0; i < info.numSamples; ++i)
            {
                const int64_t beatIdx = (samplesPerBeat > 0.0 ? (int64_t)std::floor((double)transportSamples / samplesPerBeat) : 0);
                if (beatIdx != lastBeatIndex)
                {
                    lastBeatIndex = beatIdx;
                    const int whichInBar = (int)(beatIdx % (int64_t)juce::jmax(1, beatsPerBar));
                    const bool isDown = (downIndex >= 0 && whichInBar == downIndex);
                    const bool isUp = (upIndex >= 0 && whichInBar == upIndex);

                    if (!muteSubdivisions || isDown || isUp)
                    {
                        double f = normalFreqHz; float g = normalGain;
                        if (isDown) { f = downFreqHz; g = downGain; }
                        else if (isUp) { f = upFreqHz; g = upGain; }
                        click.setFreq(f); click.trigger(g);
                    }
                }

                const float s = kick.next() + snare.next() + hihat.next() + click.next();
                L[i] = s; if (R) R[i] = s;
                ++transportSamples;
            }
        }
    }

private:
    void recalcStep()
    {
        if (samplesPerBeat <= 0.0) { stepSamps = 1; stepCountdown = 1; return; }
        if (polyrhythm)
        {
            const double samplesPerBar = samplesPerBeat * (double)juce::jmax(1, hostBeatsPerBar);
            const double step = samplesPerBar / (double)juce::jmax(1, subdivisions);
            stepSamps = juce::jmax<int64_t>(1, (int64_t)std::llround(step));
        }
        else
        {
            const double step = samplesPerBeat; 
            stepSamps = juce::jmax<int64_t>(1, (int64_t)std::llround(step));
        }
        if (stepCountdown <= 0 || stepCountdown > stepSamps) stepCountdown = stepSamps;
    }
    int currentCount() const { return polyrhythm ? subdivisions : beatsPerBar; }
};
struct Layout {
    static constexpr int margin = 12;
    static constexpr int topH = 64;
    static constexpr int botH = 44;
    static constexpr int leftW = 220;
    static constexpr int gap = 8;
};
// === MainComponent:GUI + Timer ===
class MainComponent : public juce::Component,
    public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void setPlaying(bool b) { playing = b; }
    bool isPlaying() const { return playing; }
    //void openOnboardingOverlay(juce::ApplicationProperties& appProps);
    void setAppProperties(juce::ApplicationProperties* p) { appProps = p; }
    void showQuickTour(bool resetFlag = false);
    void openOnboardingOverlay();
    bool keyPressed(const juce::KeyPress& key) override;  
   
    // --- Uniform font for the three sample buttons ---
    struct UniformButtonLNF : public juce::LookAndFeel_V4
    {
        juce::Font getTextButtonFont(juce::TextButton& b, int buttonHeight) override
        {
            // One fixed size for all three buttons. Tweak 15–18 to taste.
            return juce::Font(juce::jlimit(3, 12, buttonHeight - 8));
        }

        void drawButtonText(juce::Graphics& g, juce::TextButton& b,
            bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
        {
            auto area = b.getLocalBounds();
            auto f = getTextButtonFont(b, b.getHeight());
            g.setFont(f);

            auto textCol = b.findColour(b.getToggleState()
                ? juce::TextButton::textColourOnId
                : juce::TextButton::textColourOffId);
            g.setColour(textCol);

            
            g.drawFittedText(b.getButtonText(), area, juce::Justification::centred, 1, 0.95f);
        }
    };

    UniformButtonLNF uniformBtnLNF;   // <-- member

    

private:

    //sound
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer  audioSourcePlayer; 
    ClickAudioSource         clickSource;

    bool   playing = true;

    // UI
    juce::Slider bpmSlider;
    juce::Label  bpmLabel;

    // Geometry
    juce::Point<float> tri[3];
    juce::Point<float> movingDot;
    void updateTriangle();
    void updateMovingDot(double t01);

    juce::TextEditor sidesEdit;
    juce::Label      sidesLabel;

    // --- polygon state ---
    int sides = 3; 
    std::vector<juce::Point<float>> verts; 
    void setSides(int n);
    void updatePolygon();              
    
    int accentIndex = -1;  

    void mouseDown(const juce::MouseEvent& e) override;
    juce::ComboBox accentNoteBox;   
    juce::Label    accentNoteLabel; 

    // --- Helpers ---
    static double midiNoteToHz(int midi)   
    {
        return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    }
    double accentPhase01 = -1.0;

    juce::TextButton sidesUp{ "+" };
    juce::TextButton sidesDown{ "-" };
    juce::Label      sidesValue;

    int    upIndexGUI = -1;
    int    downIndexGUI = -1;
    double upPhase01 = -1.0;
    double downPhase01 = -1.0;

    
    juce::ComboBox upNoteBox, downNoteBox;
    juce::Label    upNoteLabel, downNoteLabel;


    struct LayerState {
        int sides = 3;

        //Accents
        int    upIndex = -1;
        int    downIndex = -1;
        double upPhase01 = -1.0;   
        double downPhase01 = -1.0;

        
        std::vector<juce::Point<float>> verts;
    };

    std::vector<std::unique_ptr<ClickAudioSource>> layersAudio; 
    std::vector<LayerState>                       layers;       
    juce::MixerAudioSource                         mixer;      
    int activeLayer = 0;                                         

    // UI: layers
    juce::TextButton addLayerBtn{ "Add Shape" };
    juce::TextButton removeLayerBtn{ "Remove Shape" };
    juce::ComboBox   layerSelect;
    juce::Label      layerLabel;

    

    // multi-Layers
    void rebuildLayerVerts();               
    void setSidesForActive(int n);        
    void addNewLayer();                     
    void removeActiveLayer();               

    // Helpers
    ClickAudioSource* getActiveAudio();     
    LayerState& getActiveLayer(); 

    //Quick Tour
    void loadOneShotFromFile(ClickAudioSource::OneShot& dst, const juce::File& file);
    juce::ApplicationProperties* appProps = nullptr;
    juce::TextButton helpBtn{ "Quick Tour (F1)" };
    

	// Get the phase of the bar for a specific layer
    double getLayerBarPhase01(int layerIdx) const;

    // Metronome
    MetronomeSource metronome;
    bool metronomeEnabled = true;

	// Toggle Switches  
    juce::ToggleButton modeToggle{ "Polyrhythm per beat" };
    juce::ToggleButton metToggle{ "Metronome" };          

	// Buttons
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::ToggleButton muteSubsToggle{ "Mute subdivisions" };
    juce::TextButton loadKickBtn;
    juce::TextButton loadSnareBtn;
    juce::TextButton loadHihatBtn;
    juce::TextButton resetTourBtn{ "Reset Tour" };
    juce::TextButton zoomInBtn{ "+" };
    juce::TextButton zoomOutBtn{ "-" };
    float zoomFactor = 1.0f; 

    
	// File Chooser
    juce::AudioFormatManager formatManager;


    // --- Panels geometry ---
    juce::Rectangle<int> panelTop, panelLeft, panelCenter, panelBottom;

    // --- Panel styles ---
    float panelCorner = 10.0f;
    float panelStroke = 1.5f;
    juce::Colour panelFill = juce::Colours::white.withAlpha(0.06f);
    juce::Colour panelFrame = juce::Colours::white.withAlpha(0.18f);

    
    void drawPanelFrame(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title = {})
    {
        auto rf = r.toFloat();
        g.setColour(panelFill);
        g.fillRoundedRectangle(rf, panelCorner);

        g.setColour(panelFrame);
        g.drawRoundedRectangle(rf, panelCorner, panelStroke);

        if (title.isNotEmpty())
        {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(14.0f, juce::Font::bold));
            g.drawText(title, r.removeFromTop(22).reduced(8, 2),
                juce::Justification::left, false);
        }
    }


    juce::Component::SafePointer<class OnboardingOverlay> tourOverlay;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
