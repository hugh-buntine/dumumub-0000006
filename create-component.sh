#!/bin/bash

# Script to create JUCE component template files
# Usage: ./create-component.sh ComponentName

if [ -z "$1" ]; then
    echo "Usage: ./create-component.sh ComponentName"
    echo "Example: ./create-component.sh MyButton"
    exit 1
fi

COMPONENT_NAME=$1
HEADER_FILE="source/${COMPONENT_NAME}.h"
CPP_FILE="source/${COMPONENT_NAME}.cpp"

# Check if files already exist
if [ -f "$HEADER_FILE" ] || [ -f "$CPP_FILE" ]; then
    echo "Error: Files already exist!"
    [ -f "$HEADER_FILE" ] && echo "  - $HEADER_FILE"
    [ -f "$CPP_FILE" ] && echo "  - $CPP_FILE"
    exit 1
fi

# Create header file
cat > "$HEADER_FILE" << 'EOF'
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
class COMPONENT_NAME : public juce::Component
{
public:
    COMPONENT_NAME();
    ~COMPONENT_NAME() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (COMPONENT_NAME)
};
EOF

# Create cpp file
cat > "$CPP_FILE" << 'EOF'
#include "COMPONENT_NAME.h"

//==============================================================================
COMPONENT_NAME::COMPONENT_NAME()
{
    // Constructor
}

COMPONENT_NAME::~COMPONENT_NAME()
{
    // Destructor
}

//==============================================================================
void COMPONENT_NAME::paint (juce::Graphics& g)
{
    // Fill background with off white
    g.fillAll (juce::Colour (255, 255, 242));
    
    // Draw black outline
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds(), 1);
    
    // Draw component name in center
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("COMPONENT_NAME", getLocalBounds(), juce::Justification::centred, 1);
}

void COMPONENT_NAME::resized()
{
    // Layout child components here
}
EOF

# Replace COMPONENT_NAME placeholder with actual name
sed -i '' "s/COMPONENT_NAME/$COMPONENT_NAME/g" "$HEADER_FILE"
sed -i '' "s/COMPONENT_NAME/$COMPONENT_NAME/g" "$CPP_FILE"

echo "✅ Created JUCE component files:"
echo "   - $HEADER_FILE"
echo "   - $CPP_FILE"
