#pragma once
#include <JuceHeader.h>

class EscalaDb : public juce::Component
{
public:
    EscalaDb (float dBMin, float dBMax, int totalLuces, std::vector<float> valoresAMostrar);
    ~EscalaDb() override;

    void paint (juce::Graphics& g) override;

private:
    float dBMinimo;
    float dBMaximo;
    int totalLuces;

    juce::Font chivoMono;
    
    std::vector<float> valores;
};