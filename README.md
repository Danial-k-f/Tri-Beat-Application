# Tri-Beat Application

## Overview

Tri-Beat is an interactive rhythm generation tool that allows you to
create layered **polyrhythms** and **polymeters** using polygonal
shapes. Each shape represents a rhythmic cycle. By stacking multiple
shapes, you can design complex grooves with accents, subdivisions, and
user-loaded drum samples.

------------------------------------------------------------------------
## Features

- **Layered Shapes:** Add/remove polygon layers; each layer can have its own number of sides.
- **Polyrhythm / Polymeter Modes:** Switch between evenly distributing N points over a 4/4 host bar (polyrhythm) and advancing per-beat in the layer’s own meter (polymeter).
- **Accents:** Click a vertex to set **Upbeat** (yellow), Shift+Click to set **Downbeat** (cyan). Unaccented vertices are grey; the orange dot is the playhead.
- **Samples:** Load your own audio for **Upbeat Sample** (kick), **Downbeat Sample** (snare), and **Subdivision Sample** (hi‑hat). If not loaded, a synthetic click is used.
- **Transport:** Play/Stop, 4/4 Metronome toggle, **Mute Subdivisions** toggle.
- **Zoom:** In/Out buttons in the shape panel.
- **Quick Tour:** Guided onboarding on first launch; press **F1** or use **Reset Tour** to see it again.
------------------------------------------------------------------------
## Requirements

- **Operating System:** Windows 10/11 (x64)
- **Build Toolchain (for source builds):**
  - **CMake** ≥ 3.21
  - **Visual Studio 2022** (Desktop development with C++) or Build Tools + MSVC v143
  - **Windows 10 SDK**
- **JUCE 8.0.x** (fetched automatically via CMake’s `FetchContent` by default)
- **Audio Formats:** WAV/AIFF recommended (MP3 decode requires appropriate codecs on the system)

> If you only want to run the app, see **Installation (Prebuilt)** below.
> ------------------------------------------------------------------------
## Quick Start Guide

### 1. Transport Controls

-   **Play / Stop** -- Start or stop audio playback.
-   **Metronome Toggle** -- Enables a 4/4 reference click track.
-   **Mute Subdivisions** -- Silences all non-accent beats, leaving only
    Upbeat and Downbeat sounds.

### 2. Shape Layers

-   **Add Shape** -- Creates a new polygon layer.
-   **Remove Shape** -- Deletes the currently selected layer.
-   **Layer Menu** -- Switch between different layers.

### 3. Polygon Editing

-   **+ / - Buttons** -- Change the number of polygon sides.\
    (Triangle = 3 sides, Pentagon = 5, etc.)
-   More sides = finer rhythmic subdivisions.

### 4. Accents

-   **Click on a vertex** → sets an **Upbeat** (Yellow).
-   **Shift+Click on a vertex** → sets a **Downbeat** (Cyan).
-   **Unaccented vertices** are Grey.
-   **Orange dot** = playhead.

### 5. Samples

You can load your own drum samples for different roles: - **Upbeat
Sample** → Kick - **Downbeat Sample** → Snare - **Subdivision Sample** →
Hi-hat

If no sample is loaded, a synthetic click will play instead.

### 6. Polyrhythm vs. Polymeter

-   **Polyrhythm** -- Shape's N points are distributed across the host
    bar (e.g., 3-in-4).
-   **Polymeter** -- Shape progresses in its own cycle, independent of
    the host bar.

### 7. Colour Coding

-   Grey = Subdivision
-   Yellow = Upbeat
-   Cyan = Downbeat
-   Orange = Playhead

### 8. Zoom Controls

-   **+ Button** → Zoom in on the Shape View.
-   **-- Button** → Zoom out.

### 9. Quick Tour & Help

-   On first launch, an onboarding tour introduces the main features.
-   You can reset the tour anytime with the **Reset Tour** button or
    press **F1**.

------------------------------------------------------------------------
## Installation (Prebuilt)

If you downloaded a packaged build from the repository or a GitHub Release:

1. Go to the project root’s **`Dist/`** directory.
2. Run the installer (e.g., `TriBeatInstaller.exe` or similar).
3. Launch **Tri-Beat** from Start Menu or installation folder.

------------------------------------------------------------------------
## Build from Source (Windows)

> The project uses CMake and fetches JUCE automatically (default).  
> Source files are under **`Source/`**.

### 1) Clone

```bash
git clone https://github.com/Danial-k-f/Tri-Beat-Application.git
cd Tri-Beat-Application
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
------------------------------------------------------------------------
## Repository Structure

Tri-Beat-Application/
├─ CMakeLists.txt
├─ Source/
│  ├─ Main.cpp
│  ├─ MainComponent.h / .cpp
│  ├─ OnboardingOverlay.h
│  └─ (other headers/sources)
├─ Dist/                  # optional: packaged installer(s), release artifacts
├─ icon.ico
├─ README.md
├─ LICENSE                # MIT License
├─Tri-Beat.jucer
├─Tri-Beat_User_Manual.pdf
└─ .gitignore

------------------------------------------------------------------------
## Support / Feedback

Issues and feature requests: open a ticket on the GitHub repository.

Quick Tour is available in-app via F1 or Reset Tour button in the bottom panel.

------------------------------------------------------------------------
## License
This project is licensed under the MIT License. See the LICENSE file.
This software was designed and developed by:

**Danial Kooshki**\
Audio Engineer, Programmer, and Musician

© 2025 Danial Kooshki. All rights reserved.

------------------------------------------------------------------------
