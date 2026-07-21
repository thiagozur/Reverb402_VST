#pragma once

#include <JuceHeader.h>
#include "Estilo402.h"

class EstiloKnobGain : public Estilo402
{
public:
    EstiloKnobGain();
    ~EstiloKnobGain() override = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

private:
    juce::Font chivoMono;
};