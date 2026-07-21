#pragma once

#include <JuceHeader.h>
#include "Estilo402.h"

class EstiloPushBtn : public Estilo402
{
public:
    EstiloPushBtn() = default;
    ~EstiloPushBtn() override = default;

    enum ColourIDs
    {
        colorLedActivoID = 0x10005001
    };

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};