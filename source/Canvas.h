#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SpawnPoint.h"

//==============================================================================
class Canvas : public juce::Component
{
public:
    Canvas();
    ~Canvas() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void newSpawnPoint();

private:

    int maxSpawnPoints = 8;
    juce::OwnedArray<SpawnPoint> spawnPoints;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Canvas)
};
