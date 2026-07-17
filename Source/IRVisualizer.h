#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EstiloPushBtn.h"

class IRWaveformPlot : public juce::Component, public juce::Timer
{
public:
    IRWaveformPlot (Reverb402AudioProcessor& p);
    ~IRWaveformPlot() override;

    void paint (juce::Graphics& g) override;
    void actualizarOnda();
    void timerCallback() override;
    void visibilityChanged() override;

private:
    Reverb402AudioProcessor& processor;
    juce::AudioBuffer<float> bufferLocal;
    juce::Path caminoOnda;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRWaveformPlot)
};

class IRSpectrogramPlot : public juce::Component, public juce::Timer
{
public:
    IRSpectrogramPlot(Reverb402AudioProcessor& p);
    ~IRSpectrogramPlot() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void actualizarEspectrograma();
    void timerCallback() override;
    void visibilityChanged() override;

private:
    Reverb402AudioProcessor& processor;
    juce::AudioBuffer<float> bufferLocal;

    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    std::unique_ptr<juce::dsp::FFT> descriptorFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> ventanaHann;

    juce::Image imagenEspectrograma;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRSpectrogramPlot)
};

class IRVisualizerContainer : public juce::Component
{
public:
    IRVisualizerContainer (Reverb402AudioProcessor& p) : processor (p), waveformPlot (p), spectrogramPlot (p)
    {
        btnWaveform.setLookAndFeel (&estiloPushBtn);
        btnWaveform.setColour(EstiloPushBtn::colorLedActivoID, juce::Colour (0xFF00A0D2));

        btnSpectrogram.setLookAndFeel (&estiloPushBtn);
        btnSpectrogram.setColour(EstiloPushBtn::colorLedActivoID, juce::Colour (0xFF4D9823));

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

    ~IRVisualizerContainer() override
    {
        btnWaveform.setLookAndFeel (nullptr);
        btnSpectrogram.setLookAndFeel (nullptr);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::transparentBlack);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
    
        int margenLateral = 20;
        int margenInferior = 20;
        int altoBoton = 24;
        int espacioBotonesGrafico = 10;
        
        auto areaDisponible = bounds.reduced (margenLateral, 0);
        areaDisponible.removeFromBottom (margenInferior);
        
        areaDisponible.removeFromTop (8); 
        auto areaParaBotones = areaDisponible.removeFromTop (altoBoton);
        
        int anchoBoton = 110;
        int espacioEntreBotones = 6;
        
        btnWaveform.setBounds (areaParaBotones.getX(), areaParaBotones.getY(), anchoBoton, altoBoton);
        btnSpectrogram.setBounds (areaParaBotones.getX() + anchoBoton + espacioEntreBotones, areaParaBotones.getY(), anchoBoton, altoBoton);
        
        auto areaGrafico = areaDisponible.withTrimmedTop (espacioBotonesGrafico);
        
        waveformPlot.setBounds (areaGrafico);
        spectrogramPlot.setBounds (areaGrafico);
    }

    void actualizarGraficos()
    {
        waveformPlot.actualizarOnda();
        spectrogramPlot.actualizarEspectrograma();
    }

private:
    Reverb402AudioProcessor& processor;

    EstiloPushBtn estiloPushBtn;

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