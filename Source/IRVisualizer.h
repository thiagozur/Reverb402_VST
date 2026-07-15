#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class IRWaveformPlot : public juce::Component
{
public:
    IRWaveformPlot (Reverb402AudioProcessor& p) : processor (p)
    {
        actualizarOnda();
    }

    void paint (juce::Graphics& g) override;
    
    void actualizarOnda();

private:
    Reverb402AudioProcessor& processor;
    juce::AudioBuffer<float> bufferLocal;
    juce::Path caminoOnda;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRWaveformPlot)
};

class IRSpectrogramPlot : public juce::Component {
public:
    IRSpectrogramPlot() {}

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xFF151515));

        g.setColour (juce::Colours::orange);
        g.setFont (14.0f);
        g.drawText (juce::String (juce::CharPointer_UTF8 (u8"Aquí se dibujará el Espectrograma (Frecuencia vs Tiempo)")), getLocalBounds(), juce::Justification::centred);
    }
};

class IRVisualizerContainer : public juce::Component
{
public:
    IRVisualizerContainer (Reverb402AudioProcessor& p) : processor (p), waveformPlot (p)
    {
        btnWaveform.setButtonText ("Forma de Onda");
        btnWaveform.setRadioGroupId (1234);
        btnWaveform.setClickingTogglesState (true);
        btnWaveform.setToggleState (true, juce::dontSendNotification);
        btnWaveform.onClick = [this] { mostrarPestana (true); };
        addAndMakeVisible (btnWaveform);

        btnSpectrogram.setButtonText ("Espectro");
        btnSpectrogram.setRadioGroupId (1234);
        btnSpectrogram.setClickingTogglesState (true);
        btnSpectrogram.onClick = [this] {mostrarPestana (false); };
        addAndMakeVisible (btnSpectrogram);

        addAndMakeVisible (waveformPlot);
        addChildComponent (spectrogramPlot);
        
        actualizarGraficos();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xFF222222));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto areaBotones = bounds.removeFromTop (30);
        btnWaveform.setBounds (areaBotones.removeFromLeft (120).reduced (2));
        btnSpectrogram.setBounds (areaBotones.removeFromLeft (120).reduced (2));

        auto areaGrafico = bounds.reduced (4);
        waveformPlot.setBounds (areaGrafico);
        spectrogramPlot.setBounds (areaGrafico);
    }

    void actualizarGraficos()
    {
        waveformPlot.actualizarOnda();
    }

private:
    Reverb402AudioProcessor& processor;

    juce::TextButton btnWaveform;
    juce::TextButton btnSpectrogram;

    IRWaveformPlot waveformPlot;
    IRSpectrogramPlot spectrogramPlot;

    void mostrarPestana (bool verWaveform)
    {
        waveformPlot.setVisible (verWaveform);
        spectrogramPlot.setVisible (!verWaveform);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRVisualizerContainer)
};