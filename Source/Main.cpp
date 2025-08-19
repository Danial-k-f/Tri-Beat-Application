#include <JuceHeader.h>
#include "MainComponent.h"
#include "OnboardingOverlay.h"  // your overlay header

class TriBeatApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return "Tri-Beat Prototype"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed()          override { return true; }

    // App-wide settings container
    juce::ApplicationProperties appProps;

    void initialise(const juce::String&) override
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName = "Tri-Beat";
        opts.filenameSuffix = "settings";
        opts.osxLibrarySubFolder = "Application Support";
        opts.storageFormat = juce::PropertiesFile::storeAsXML;

        appProps.setStorageParameters(opts);

        mainWindow.reset(new MainWindow("Tri-Beat", appProps));

        // --- First-run + hide flag logic ---
        auto* settings = appProps.getUserSettings();
        const bool firstRun = settings->getBoolValue("firstRun", true);
        const bool hideTour = settings->getBoolValue("hideOnboarding", false);

     
        if (firstRun || !hideTour)
        {
            settings->setValue("firstRun", false);
            settings->saveIfNeeded();

         
            juce::MessageManager::callAsync([this]
                {
                    if (mainWindow != nullptr)
                        mainWindow->showOnboarding();
                });
        }
    
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

private:
    // ---- Single MainWindow definition (keep ONLY this one) ----
    struct MainWindow : public juce::DocumentWindow
    {
        MainWindow(juce::String name, juce::ApplicationProperties& p)
            : DocumentWindow(name, juce::Colours::black, allButtons), props(p)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);

            // Create your MainComponent and pass the app properties into it
            auto* mc = new MainComponent();
            mc->setAppProperties(&props);              // <-- important
            setContentOwned(mc, true);

            

            centreWithSize(900, 600);
            setVisible(true);

            
            juce::MessageManager::callAsync([this]
                {
                    if (auto* c = getContentComponent())
                        c->grabKeyboardFocus();
                });

        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void showOnboarding()
        {
            if (auto* mc = dynamic_cast<MainComponent*>(getContentComponent()))
                mc->openOnboardingOverlay();           // opens overlay + handles flag
        }

        juce::ApplicationProperties& props;            // <--- stored here
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(TriBeatApplication)
