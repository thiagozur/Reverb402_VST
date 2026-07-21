#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class Luz : public juce::Component
{
public:
    Luz(const juce::Colour& c);
    ~Luz() override;

    void setState (const bool state);

    void paint (juce::Graphics& g) override;

private:
    bool encendida = false;
    juce::Colour color;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Luz)
};

class MedidorNivel : public juce::Component, public juce::Timer
{
public:
    MedidorNivel (std::function<float()>&& funcionValores);
    ~MedidorNivel() override;

    static constexpr float dBMin = -60.0f;
    static constexpr float dBMax = 6.0f;
    static constexpr int totalLuces = 10;

    void resized () override;

    void timerCallback() override;

private:
    std::function<float()> obtenerValores;
    std::vector<std::unique_ptr<Luz>> luces;
    juce::ColourGradient escala;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MedidorNivel)
};