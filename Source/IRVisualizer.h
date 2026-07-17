#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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
        g.fillAll (juce::Colours::transparentBlack);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto areaBotones = bounds.removeFromTop (30);
        btnWaveform.setBounds (areaBotones.removeFromLeft (120).reduced (2));
        btnSpectrogram.setBounds (areaBotones.removeFromLeft (120).reduced (2));

        auto areaGrafico = bounds.reduced (20);
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