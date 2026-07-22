#pragma once

#include <JuceHeader.h>
#include "Estilo402.h"

class EstiloClickBtn : public Estilo402
{
public:
    EstiloClickBtn() = default;
    ~EstiloClickBtn() override = default;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};