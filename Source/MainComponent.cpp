#include "MainComponent.h"
#include <cmath>
#include "OnboardingOverlay.h"

// ================= ctor/dtor =================
MainComponent::MainComponent()

{

    // UI
    addAndMakeVisible(bpmSlider);
    bpmSlider.setRange(30.0, 300.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.onValueChange = [this]
        {
            const double b = bpmSlider.getValue();
            metronome.setTempo(b);
            for (auto& src : layersAudio) src->setTempo(b);
        };


    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.attachToComponent(&bpmSlider, true);

    
    addAndMakeVisible(modeToggle);
    modeToggle.setToggleState(true, juce::dontSendNotification); // Polyrhythm
    modeToggle.onClick = [this]
        {
            const bool isPoly = modeToggle.getToggleState();
            for (auto& src : layersAudio)
            {
                src->setPolyrhythm(isPoly);
                // sides 
                const int n = getActiveLayer().sides;
                if (isPoly) src->setSubdivisions(n);
                else        src->setBeatsPerBar(n);
            }
            repaint();
        };

    addAndMakeVisible(metToggle);
    metToggle.setToggleState(true, juce::dontSendNotification);
    metToggle.onClick = [this]
        {
            const bool on = metToggle.getToggleState();
            if (on)
            {
                metronome.resetTransport(); 
                metronome.setPlaying(true);  
            }
            else
            {
                metronome.setPlaying(false); 
                
            }
        };



    // ==== Up-beat note ====
    addAndMakeVisible(upNoteBox);
    addAndMakeVisible(upNoteLabel);
    upNoteLabel.setText("Up Note", juce::dontSendNotification);
    upNoteLabel.attachToComponent(&upNoteBox, true);

    // ==== Down-beat note ====
    addAndMakeVisible(downNoteBox);
    addAndMakeVisible(downNoteLabel);
    downNoteLabel.setText("Down Note", juce::dontSendNotification);
    downNoteLabel.attachToComponent(&downNoteBox, true);

    sidesUp.onClick = [this] { setSidesForActive(getActiveLayer().sides + 1); };
    sidesDown.onClick = [this] { setSidesForActive(getActiveLayer().sides - 1); };


    upNoteBox.onChange = [this]
        {
            int id = upNoteBox.getSelectedId(); if (id <= 0) return;
            int midi = 60 + (id - 1);
            layersAudio[(size_t)activeLayer]->setUpbeatFreqHz(440.0 * std::pow(2.0, (midi - 69) / 12.0));
        };

    downNoteBox.onChange = [this]
        {
            int id = downNoteBox.getSelectedId(); if (id <= 0) return;
            int midi = 60 + (id - 1);
            layersAudio[(size_t)activeLayer]->setDownbeatFreqHz(440.0 * std::pow(2.0, (midi - 69) / 12.0));
        };


   
    struct Note { const char* name; int midi; };
    static const Note kNotes[] = {
        {"C4",60},{"C#4/Db4",61},{"D4",62},{"D#4/Eb4",63},{"E4",64},{"F4",65},
        {"F#4/Gb4",66},{"G4",67},{"G#4/Ab4",68},{"A4",69},{"A#4/Bb4",70},{"B4",71},{"C5",72}
    };
    auto fillNoteBox = [&](juce::ComboBox& box)
        {
            int id = 1;
            for (auto& n : kNotes) box.addItem(n.name, id++);
            box.setSelectedId(10, juce::dontSendNotification); 
        };

    fillNoteBox(upNoteBox);
    fillNoteBox(downNoteBox);

    


    // --- Sides UI 
    addAndMakeVisible(sidesUp);
    addAndMakeVisible(sidesDown);
    addAndMakeVisible(sidesLabel);
    addAndMakeVisible(sidesValue);

    sidesLabel.setText("Sides", juce::dontSendNotification);
    sidesValue.setJustificationType(juce::Justification::centred);
    sidesValue.setInterceptsMouseClicks(false, false); 

    sidesUp.setTooltip("Increase sides");
    sidesDown.setTooltip("Decrease sides");

    // ==== Play / Stop ====
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);

    playButton.onClick = [this]
        {
           
            metronome.resetTransport();
            metronome.setPlaying(metToggle.getToggleState());

            
            for (auto& src : layersAudio)
            {
                src->resetTransport();
                src->setPlaying(true);
            }
        };

    stopButton.onClick = [this]
        {
            
            for (auto& src : layersAudio)
            {
                src->setPlaying(false);
                src->resetTransport();
            }
            
            metronome.setPlaying(false);
            metronome.resetTransport();
        };

    addAndMakeVisible(muteSubsToggle);
    muteSubsToggle.setTooltip(" For hearing only Up/Down accent ");

    muteSubsToggle.onClick = [this]
        {
            const bool m = muteSubsToggle.getToggleState();
            for (auto& src : layersAudio)
                src->setMuteSubdivisions(m);
        };


    addAndMakeVisible(loadKickBtn);
    addAndMakeVisible(loadSnareBtn);
    addAndMakeVisible(loadHihatBtn);

    // Final, user-facing labels
    loadKickBtn.setButtonText("Upbeat Sample");
    loadSnareBtn.setButtonText("Downbeat Sample");
    loadHihatBtn.setButtonText("Subdivision Sample");

    // Apply uniform font LookAndFeel to these three buttons
    loadKickBtn.setLookAndFeel(&uniformBtnLNF);
    loadSnareBtn.setLookAndFeel(&uniformBtnLNF);
    loadHihatBtn.setLookAndFeel(&uniformBtnLNF);

    
    auto makeLoadHandler = [this](ClickAudioSource::OneShot& target)
        {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Choose a sample...", juce::File(), "*.wav;*.aiff;*.mp3");

            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, &target](const juce::FileChooser& fc)
                {
                    auto f = fc.getResult();
                    if (f.existsAsFile())
                        loadOneShotFromFile(target, f);
                });
        };

    loadKickBtn.onClick = [this, makeLoadHandler]() { if (!layersAudio.empty()) makeLoadHandler(layersAudio[activeLayer]->kick);  };
    loadSnareBtn.onClick = [this, makeLoadHandler]() { if (!layersAudio.empty()) makeLoadHandler(layersAudio[activeLayer]->snare); };
    loadHihatBtn.onClick = [this, makeLoadHandler]() { if (!layersAudio.empty()) makeLoadHandler(layersAudio[activeLayer]->hihat); };


    addAndMakeVisible(helpBtn);
    helpBtn.onClick = [this] { showQuickTour(false); };
    setWantsKeyboardFocus(true); 
    addAndMakeVisible(resetTourBtn);
    resetTourBtn.onClick = [this] { showQuickTour(true); };

    addAndMakeVisible(zoomInBtn);
    addAndMakeVisible(zoomOutBtn);

    zoomInBtn.setTooltip("Zoom In");
    zoomOutBtn.setTooltip("Zoom Out");

    
    zoomInBtn.onClick = [this]
        {
            zoomFactor = juce::jmin(2.5f, zoomFactor * 1.2f); 
            rebuildLayerVerts();
            repaint();
        };

    zoomOutBtn.onClick = [this]
        {
            zoomFactor = juce::jmax(0.5f, zoomFactor / 1.2f); 
            rebuildLayerVerts();
            repaint();
        };

    
    sidesValue.setText(juce::String(sides), juce::dontSendNotification);


    setSize(800, 450);

    // ======================= Safe boot ???? ??? =======================
    {
        audioSourcePlayer.setSource(nullptr);
        deviceManager.removeAudioCallback(&audioSourcePlayer);
        deviceManager.closeAudioDevice();

        auto& types = deviceManager.getAvailableDeviceTypes();
        juce::String chosenType;

        if (types.size() > 0)
        {
            for (auto* t : types)
                if (t && t->getTypeName().isNotEmpty()
                    && t->getTypeName().containsIgnoreCase("Windows")) // WASAPI
                {
                    chosenType = t->getTypeName(); break;
                }

            if (chosenType.isEmpty())
                for (auto* t : types)
                    if (t && t->getTypeName().isNotEmpty())
                    {
                        chosenType = t->getTypeName(); break;
                    }
        }

        if (chosenType.isNotEmpty())
            deviceManager.setCurrentAudioDeviceType(chosenType, true);
        else
            DBG("[Audio] No device types found!");

        auto err = deviceManager.initialise(0, 2, nullptr, true, {}, nullptr);
        if (err.isNotEmpty())
            DBG("[Audio] init error: " << err);

        if (deviceManager.getCurrentAudioDevice() == nullptr)
        {
            DBG("[Audio] Fallback to DirectSound / Windows Audio");

            deviceManager.setCurrentAudioDeviceType("DirectSound", true);
            err = deviceManager.initialise(0, 2, nullptr, true, {}, nullptr);

            if (deviceManager.getCurrentAudioDevice() == nullptr)
            {
                deviceManager.setCurrentAudioDeviceType("Windows Audio", true);
                err = deviceManager.initialise(0, 2, nullptr, true, {}, nullptr);
            }
        }

        

        if (auto* dev = deviceManager.getCurrentAudioDevice())
            DBG("[Audio] Device: " << dev->getName() << "  SR=" << dev->getCurrentSampleRate());
    }
    //==================================================================
    
    metronome.setTempo(bpmSlider.getValue());
    metronome.setBeatsPerBar(4);
    mixer.addInputSource(&metronome, false); 

    layers.emplace_back();
    layersAudio.emplace_back(std::make_unique<ClickAudioSource>());

    layers[0].sides = 3;
    layersAudio[0]->setTempo(bpmSlider.getValue());

    
    const bool isPoly = true; 
    layersAudio[0]->setPolyrhythm(isPoly);
    if (isPoly)   layersAudio[0]->setSubdivisions(layers[0].sides);
    else          layersAudio[0]->setBeatsPerBar(layers[0].sides);

    // Up/Down Beat Freq
    layersAudio[0]->setUpbeatFreqHz(440.0); 
    layersAudio[0]->setDownbeatFreqHz(220.0); 

    // mixer
    mixer.addInputSource(layersAudio[0].get(), false);


    
    audioSourcePlayer.setSource(&mixer);
    deviceManager.addAudioCallback(&audioSourcePlayer);


    formatManager.registerBasicFormats();



    // ===== 3) UI Layers =====
    addAndMakeVisible(addLayerBtn);
    addAndMakeVisible(removeLayerBtn);
    addAndMakeVisible(layerSelect);
    addAndMakeVisible(layerLabel);
    layerLabel.setText("Layer", juce::dontSendNotification);
    layerLabel.attachToComponent(&layerSelect, true);

    auto refreshLayerCombo = [this]
        {
            layerSelect.clear();
            for (int i = 0; i < (int)layers.size(); ++i)
                layerSelect.addItem("Layer " + juce::String(i + 1), i + 1);
            layerSelect.setSelectedId(activeLayer + 1, juce::dontSendNotification);
        };

    addLayerBtn.onClick = [this, refreshLayerCombo]
        {
            addNewLayer();
            refreshLayerCombo();
            repaint();
        };

        removeLayerBtn.onClick = [this, refreshLayerCombo]
            {
                removeActiveLayer();
                refreshLayerCombo();
                repaint();
            };

            layerSelect.onChange = [this]
                {
                    const int id = layerSelect.getSelectedId();
                    if (id > 0 && id - 1 < (int)layers.size())
                        activeLayer = id - 1;
                    repaint();
                };

         
            refreshLayerCombo();
            rebuildLayerVerts();

    // ==================================================================


    startTimerHz(60);

}

MainComponent::~MainComponent()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);

    // detach L&F from buttons before destruction
    loadKickBtn.setLookAndFeel(nullptr);
    loadSnareBtn.setLookAndFeel(nullptr);
    loadHihatBtn.setLookAndFeel(nullptr);
    // resetTourBtn.setLookAndFeel(nullptr); // if applied

    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);

}

// ================= GUI =================

void MainComponent::paint(juce::Graphics& g)
{
    
    g.fillAll(juce::Colours::black);

    
    const auto panelFill = juce::Colour::fromRGB(26, 28, 34);    
    const auto panelFillA = panelFill.withAlpha(0.85f);           
    const auto panelBorder = juce::Colour::fromRGB(230, 230, 235); 
    const float corner = 10.0f;
    const float stroke = 2.0f;

    

    // Top Panel
    g.setColour(panelFillA);
    g.fillRoundedRectangle(panelTop.toFloat(), corner);
    g.setColour(panelBorder.withAlpha(0.9f));
    g.drawRoundedRectangle(panelTop.toFloat(), corner, stroke);

    // Left Panel
    g.setColour(panelFillA);
    g.fillRoundedRectangle(panelLeft.toFloat(), corner);
    g.setColour(panelBorder.withAlpha(0.9f));
    g.drawRoundedRectangle(panelLeft.toFloat(), corner, stroke);

    // Shape Panel
    g.setColour(panelFillA);
    g.fillRoundedRectangle(panelCenter.toFloat(), corner);
    g.setColour(panelBorder.withAlpha(0.9f));
    g.drawRoundedRectangle(panelCenter.toFloat(), corner, stroke);

    // Transport/Load Panel
    g.setColour(panelFillA);
    g.fillRoundedRectangle(panelBottom.toFloat(), corner);
    g.setColour(panelBorder.withAlpha(0.9f));
    g.drawRoundedRectangle(panelBottom.toFloat(), corner, stroke);


    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(panelCenter.reduced(2)); 

        


        const int layerCount = (int)layers.size();
        const int audioCount = (int)layersAudio.size();

        for (int li = 0; li < layerCount; ++li)
        {
            const auto& L = layers[(size_t)li];
            const int N = (int)L.verts.size();
            if (N < 3) continue;
            if (li >= audioCount) continue;

            // 
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            juce::Path p; p.startNewSubPath(L.verts[0]);
            for (int i = 1; i < N; ++i) p.lineTo(L.verts[(size_t)i]);
            p.closeSubPath();
            g.strokePath(p, juce::PathStrokeType(2.0f));

            // 
            g.setColour(juce::Colours::darkgrey);
            for (const auto& v : L.verts) g.fillEllipse(v.x - 3, v.y - 3, 6, 6);

            // 
            const float R1 = 12.0f, R2 = 16.0f, T = 2.0f;
            if (L.upIndex >= 0 && L.upIndex < N)
            {
                const auto v = L.verts[(size_t)L.upIndex];
                g.setColour(juce::Colours::yellow);
                g.drawEllipse(v.x - R1, v.y - R1, 2 * R1, 2 * R1, T);
            }
            if (L.downIndex >= 0 && L.downIndex < N)
            {
                const auto v = L.verts[(size_t)L.downIndex];
                g.setColour(juce::Colours::cyan);
                g.drawEllipse(v.x - R2, v.y - R2, 2 * R2, 2 * R2, T);
            }

            // 
            const double tLap = layersAudio[(size_t)li]->getPhase01(); 
            if (L.sides > 0)
            {
                const double seg = tLap * (double)L.sides;
                const int    i0 = (int)std::floor(seg) % L.sides;
                const int    i1 = (i0 + 1) % L.sides;
                const float  frac = (float)(seg - std::floor(seg));

                const auto a = L.verts[(size_t)i0];
                const auto b = L.verts[(size_t)i1];
                const auto P = a + (b - a) * frac;

                g.setColour(juce::Colours::orange);
                g.fillEllipse(P.x - 5, P.y - 5, 10, 10);
            }
        }
    }
    // (ASCII)
    g.setColour(juce::Colours::antiquewhite);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("PolyRhythm - Multiple Concentric Shapes",
        getLocalBounds().reduced(10, 20),
        juce::Justification::centredTop , false);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(10);

    
    const int topH = 88;               
    const int bottomH = 64;              
    const int leftW = 260;              
    const int gap = 8;                
    const int inset = 10;               

    // Panels
    panelTop = area.removeFromTop(topH);
    area.removeFromTop(gap);

    panelBottom = area.removeFromBottom(bottomH);
    area.removeFromBottom(gap);

    panelLeft = area.removeFromLeft(leftW);
    area.removeFromLeft(gap);

    panelCenter = area; 

    // ---------- Layout: Top panel ----------
    {
        auto top = panelTop.reduced(inset);
        const int ctlH = juce::jlimit(22, 32, top.getHeight() - 20);

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.flexWrap = juce::FlexBox::Wrap::wrap;
        fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
        fb.alignItems = juce::FlexBox::AlignItems::center;

        fb.items.add(juce::FlexItem(bpmSlider).withMinWidth(160).withMaxWidth(450).withHeight(ctlH).withFlex(2.0f));
        fb.items.add(juce::FlexItem(modeToggle).withMinWidth(160).withMaxWidth(260).withHeight(ctlH).withFlex(1.0f));
        fb.items.add(juce::FlexItem(metToggle).withMinWidth(120).withMaxWidth(200).withHeight(ctlH));
        fb.items.add(juce::FlexItem(muteSubsToggle).withMinWidth(160).withMaxWidth(240).withHeight(ctlH));

        fb.performLayout(top);
    }

    // ---------- Layout: Left panel ----------
    {
        auto left = panelLeft.reduced(inset);
        const int rowH = 28;
        const int gapY = 6;

        int y = left.getY();

        // Layer selection
        layerLabel.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + 2;
        layerSelect.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + gapY;

        // Add/Remove
        auto row = juce::Rectangle<int>(left.getX(), y, left.getWidth(), rowH);
        addLayerBtn.setBounds(row.removeFromLeft((row.getWidth() / 2) - 4));
        removeLayerBtn.setBounds(row.reduced(8, 0));
        y += rowH + gapY;

        // sides
        sidesLabel.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + 2;
        auto srow = juce::Rectangle<int>(left.getX(), y, left.getWidth(), rowH);
        sidesDown.setBounds(srow.removeFromLeft(36));
        sidesValue.setBounds(srow.removeFromLeft(50).reduced(6, 0));
        sidesUp.setBounds(srow.removeFromLeft(36));
        
        y += rowH + gapY;

        // notes
        upNoteLabel.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + 2;
        upNoteBox.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + gapY;

        downNoteLabel.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + 2;
        downNoteBox.setBounds(left.getX(), y, left.getWidth(), rowH); y += rowH + gapY;

  

    }
    // ---------- Layout: Shape panel (panelCenter) ----------
    {
        auto shape = panelCenter.reduced(inset);

       
        const int btnW = 32;
        const int btnH = 28;
        const int pad = 6;

       
        juce::Rectangle<int> row(shape.getRight() - (2 * btnW + pad) - pad,
            shape.getY() + pad,
            2 * btnW + pad,
            btnH);

        zoomInBtn.setBounds(row.removeFromLeft(btnW).reduced(0)); // [+]
        row.removeFromLeft(pad);
        zoomOutBtn.setBounds(row.removeFromLeft(btnW).reduced(0)); // [-]
    }

    // ---------- Layout: Bottom panel ----------
    {
        auto bottom = panelBottom.reduced(inset);

        const int btnW = 110;   // was 100; make them wider so "Subdivision Sample" fits
        const int btnH = 28;
        const int gapX = 8;

        auto row = juce::Rectangle<int>(bottom.getX(),
            bottom.getCentreY() - btnH / 2,
            bottom.getWidth(), btnH);

        // Play / Stop (left)
        auto leftRow = row.removeFromLeft(2 * btnW + gapX);
        playButton.setBounds(leftRow.removeFromLeft(btnW));
        leftRow.removeFromLeft(gapX);
        stopButton.setBounds(leftRow.removeFromLeft(btnW));

        // Sample loaders (right)
        auto rightRow = row.removeFromRight(4 * btnW + 3 * gapX);
        loadKickBtn.setBounds(rightRow.removeFromLeft(btnW));
        rightRow.removeFromLeft(gapX);
        loadSnareBtn.setBounds(rightRow.removeFromLeft(btnW));
        rightRow.removeFromLeft(gapX);
        loadHihatBtn.setBounds(rightRow.removeFromLeft(btnW));
        rightRow.removeFromLeft(gapX);
        // Reset Tour
        resetTourBtn.setBounds(rightRow.removeFromLeft(btnW));

        if (tourOverlay != nullptr)
            tourOverlay->setBounds(getLocalBounds());
    }

    // ---------- Geometry for Shape View ----------
    
    rebuildLayerVerts();
}

ClickAudioSource* MainComponent::getActiveAudio()
{
    jassert(!layersAudio.empty());
    return layersAudio[(size_t)activeLayer].get();
}

MainComponent::LayerState& MainComponent::getActiveLayer()
{
    jassert(!layers.empty());
    return layers[(size_t)activeLayer];
}

void MainComponent::addNewLayer()
{
    // 
    const int newSides = 5;

    // 1) 
    LayerState st;
    st.sides = newSides;
    layers.emplace_back(std::move(st));

    // 2) 
    auto src = std::make_unique<ClickAudioSource>();
    //src->setBeatsPerBar(newSides);
    src->setUpbeatFreqHz(440.0);
    src->setDownbeatFreqHz(220.0);

    src->setTempo(bpmSlider.getValue());
    const bool isPoly = modeToggle.getToggleState();
    src->setPolyrhythm(isPoly);
    if (isPoly) src->setSubdivisions(newSides);
    else        src->setBeatsPerBar(newSides);
    src->setMuteSubdivisions(muteSubsToggle.getToggleState());
    src->setHostBeatsPerBar(4); 


    // 3) 
    if (!layersAudio.empty())
        src->setTransportSamples(layersAudio[0]->getTransportSamples());
   


    // 4) 
    mixer.addInputSource(src.get(), false);
    layersAudio.emplace_back(std::move(src));

    // 5) 
    activeLayer = (int)layers.size() - 1;

    rebuildLayerVerts();
}

void MainComponent::removeActiveLayer()
{
    if (layers.size() <= 1)
        return; 

    // 
    mixer.removeInputSource(layersAudio[(size_t)activeLayer].get());

    // 
    layersAudio.erase(layersAudio.begin() + activeLayer);
    layers.erase(layers.begin() + activeLayer);

    //  activeLayer
    if (activeLayer >= (int)layers.size())
        activeLayer = (int)layers.size() - 1;

    rebuildLayerVerts();
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    if (layers.empty()) return;
    auto& L = getActiveLayer();
    auto* A = getActiveAudio();
    if (L.verts.size() < 3) return;

    const auto pos = e.position;
    int best = -1; float bestDist2 = std::numeric_limits<float>::max();

    for (int i = 0; i < (int)L.verts.size(); ++i)
    {
        const float dx = pos.x - L.verts[(size_t)i].x;
        const float dy = pos.y - L.verts[(size_t)i].y;
        const float d2 = dx * dx + dy * dy;
        if (d2 < bestDist2) { bestDist2 = d2; best = i; }
    }

    const float threshold2 = 14.0f * 14.0f;
    if (best >= 0 && bestDist2 <= threshold2)
    {
        if (e.mods.isShiftDown())
        {
            L.downIndex = best;
            L.downPhase01 = (double)best / (double)L.sides;
            A->setDownbeatIndex(best);
        }
        else
        {
            L.upIndex = best;
            L.upPhase01 = (double)best / (double)L.sides;
            A->setUpbeatIndex(best);
        }
        repaint();
    }
}

void MainComponent::timerCallback()
{

   // const double barPhase = clickSource.getBarPhase01();
   // updateMovingDot(barPhase);
    repaint();
}
void MainComponent::setSides(int n)
{
    n = juce::jlimit(3, 16, n);
    if (n == sides) return;

    bool hadUp = (upPhase01 >= 0.0);
    bool hadDown = (downPhase01 >= 0.0);

    sides = n;
    sidesValue.setText(juce::String(sides), juce::dontSendNotification);

    if (hadUp)
    {
        int mapped = (int)std::round(upPhase01 * (double)sides);
        if (mapped == sides) mapped = 0;
        upIndexGUI = juce::jlimit(0, sides - 1, mapped);
    }
    else upIndexGUI = -1;

    if (hadDown)
    {
        int mapped = (int)std::round(downPhase01 * (double)sides);
        if (mapped == sides) mapped = 0;
        downIndexGUI = juce::jlimit(0, sides - 1, mapped);
    }
    else downIndexGUI = -1;

    clickSource.setBeatsPerBar(sides);
    clickSource.setUpbeatIndex(upIndexGUI);
    clickSource.setDownbeatIndex(downIndexGUI);

    updatePolygon();
    repaint();
}
void MainComponent::setSidesForActive(int n)
{
    n = juce::jlimit(3, 16, n);
    auto& L = getActiveLayer();
    auto* A = getActiveAudio();

    if (n == L.sides) return;   

   
    if (L.upPhase01 >= 0.0) {
        int mapped = (int)std::round(L.upPhase01 * n);
        if (mapped == n) mapped = 0;
        L.upIndex = juce::jlimit(0, n - 1, mapped);
        A->setUpbeatIndex(L.upIndex);
    }
    if (L.downPhase01 >= 0.0) {
        int mapped = (int)std::round(L.downPhase01 * n);
        if (mapped == n) mapped = 0;
        L.downIndex = juce::jlimit(0, n - 1, mapped);
        A->setDownbeatIndex(L.downIndex);
    }

    L.sides = n;

    
    const bool isPoly = modeToggle.getToggleState();
    if (isPoly) A->setSubdivisions(n);
    else        A->setBeatsPerBar(n);

    if (layersAudio[activeLayer]->isPlaying())
    {
        layersAudio[activeLayer]->restart();
    }

    sidesValue.setText(juce::String(n), juce::dontSendNotification);

    rebuildLayerVerts();
    repaint();
}
void MainComponent::rebuildLayerVerts()
{
  
    auto drawArea = panelCenter.reduced(16).toFloat(); 
    auto c = drawArea.getCentre();
    const float baseR = std::min(drawArea.getWidth(), drawArea.getHeight()) * 0.28f * zoomFactor;
    const float stepR = 18.0f * zoomFactor;



    for (int li = 0; li < (int)layers.size(); ++li)
    {
        auto& L = layers[(size_t)li];
        L.verts.clear();
        const int N = juce::jmax(3, L.sides);

        const float R = baseR + li * stepR;
        const float start = -juce::MathConstants<float>::halfPi; 

        L.verts.reserve((size_t)N);
        for (int k = 0; k < N; ++k)
        {
            const float ang = start + juce::MathConstants<float>::twoPi * (float)k / (float)N;
            L.verts.push_back(c + juce::Point<float>(std::cos(ang), std::sin(ang)) * R);
        }
    }
}

double MainComponent::getLayerBarPhase01(int li) const
{
  
    return layersAudio[(size_t)li]->getPhase01();
}

void MainComponent::loadOneShotFromFile(ClickAudioSource::OneShot& dst, const juce::File& file)
{
    if (!file.existsAsFile()) return;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return;

    const int numSamples = (int)reader->lengthInSamples;
    const int numCh = juce::jlimit(1, (int)reader->numChannels, 2);

    juce::AudioBuffer<float> temp(numCh, numSamples);
    reader->read(&temp, 0, numSamples, 0, true, true);

    dst.setSample(std::move(temp));
}

void MainComponent::updatePolygon()
{
    auto drawArea = getLocalBounds().reduced(50, 70).toFloat();
    const auto c = drawArea.getCentre();
    const float radius = std::min(drawArea.getWidth(), drawArea.getHeight()) * 0.38f;

    verts.clear();
    verts.reserve((size_t)sides);

    const float startRot = -juce::MathConstants<float>::halfPi; 

    for (int k = 0; k < sides; ++k)
    {
        const float angle = startRot + juce::MathConstants<float>::twoPi * (float)k / (float)sides;
        verts.push_back(c + juce::Point<float>(std::cos(angle), std::sin(angle)) * radius);
    }

    
    if (!verts.empty())
        movingDot = verts[0];
}
void MainComponent::openOnboardingOverlay()
{
    // If an overlay is already open, bring it to front instead of spawning another
    if (tourOverlay != nullptr)
    {
        tourOverlay->setBounds(getLocalBounds());
        tourOverlay->toFront(true);
        return;
    }

    auto* overlay = new OnboardingOverlay(
        [this](bool dontShow)
        {
            if (appProps != nullptr)
            {
                auto* settings = appProps->getUserSettings();
                settings->setValue("hideOnboarding", dontShow);
                settings->saveIfNeeded();
            }
            // When overlay finishes (and self-deletes), clear our pointer
            tourOverlay = nullptr;
        });

    tourOverlay = overlay;                 // keep a safe handle
    addAndMakeVisible(overlay);

    // If we’re already sized, place it now; otherwise do it on the next message loop tick.
    if (getWidth() > 0 && getHeight() > 0)
        overlay->setBounds(getLocalBounds());
    else
        juce::MessageManager::callAsync([this]
            {
                if (tourOverlay != nullptr)
                    tourOverlay->setBounds(getLocalBounds());
            });

    overlay->toFront(true);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::F1Key)
    {
        showQuickTour(true);  // reset flag and open tour
        return true;
    }
    return false;
}

void MainComponent::showQuickTour(bool resetFlag)
{
    if (appProps && resetFlag)
    {
        auto* s = appProps->getUserSettings();
        s->setValue("hideOnboarding", false);  
        s->saveIfNeeded();
    }
    openOnboardingOverlay();
}
// ================ geometry =================
void MainComponent::updateTriangle()
{
    auto drawArea = getLocalBounds().reduced(50, 70).toFloat();
    const auto c = drawArea.getCentre();
    const float radius = std::min(drawArea.getWidth(), drawArea.getHeight()) * 0.38f;

    tri[0] = { c.x,                 c.y - radius };
    tri[1] = { c.x - radius * 0.866f, c.y + radius * 0.5f };
    tri[2] = { c.x + radius * 0.866f, c.y + radius * 0.5f };

    movingDot = tri[0];
}

void MainComponent::updateMovingDot(double t01)
{
    if (verts.size() < 3) return;

    const int N = sides;
    const double seg = t01 * (double)N;
    const int i0 = (int)std::floor(seg) % N;
    const int i1 = (i0 + 1) % N;
    const float frac = (float)(seg - std::floor(seg));

    auto a = verts[(size_t)i0];
    auto b = verts[(size_t)i1];
    movingDot = a + (b - a) * frac;
}
