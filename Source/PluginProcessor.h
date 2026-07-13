#pragma once

#include <JuceHeader.h>

class Reverb402AudioProcessor : public juce::AudioProcessor
{
public:
    Reverb402AudioProcessor();
    ~Reverb402AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override { return "Reverb402"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::dsp::ProcessSpec spec;

    void cargarArchivoIR (const juce::File& archivoAudio);

    juce::AudioBuffer<float> modificarDecayIR (const juce::AudioBuffer<float>& irOriginal, float factorDecay, double fsIR);
    float factorCompensacionIR = 1.0f;

    juce::dsp::Convolution motorConvolucion;

    juce::AudioFormatManager empaquetadorFormatos;
    juce::AudioBuffer<float> irOriginal;
    double fsIR = 44100.0;

    float ultimoFactorDecay = -1.0f;

    juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>
    > filtrosCorte;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> lineaPreDelay;

    std::atomic<float>* paramMix = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramHPF = nullptr;
    std::atomic<float>* paramLPF = nullptr;
    std::atomic<float>* paramPreDelay = nullptr;
    std::atomic<float>* paramIRSelection = nullptr;

    juce::StringArray nombresIRs {
        "1 - Alumno Izquierda",
        "2 - Alumno Derecha",
        "3 - Alumno Wide",
        "4 - Alumno Wide (Prealigned)",
        "5 - Profesor Izquierda",
        "6 - Profesor Derecha",
        "7 - Profesor Wide",
        "8 - Profesor Wide (Prealigned)"
    };

    int ultimaIrCargada = -1;

    juce::File obtenerArchivoIRFijo (int indice);

    float gananciaNormalizacionActual = 1.0f;
    
    juce::AudioProcessorValueTreeState listaParametros;
    juce::AudioProcessorValueTreeState::ParameterLayout crearLayoutParametros();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reverb402AudioProcessor)
};