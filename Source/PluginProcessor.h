#pragma once

#include <JuceHeader.h>

struct Preset
{
    juce::String nombre;
    float mix;
    float decay;
    float preDelay;
    float hpf;
    float lpf;
    int irSelect;
    bool esDeUsuario;
    juce::File archivoOrigen;
};

class Reverb402AudioProcessor : public juce::AudioProcessor, public juce::ChangeBroadcaster
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
    juce::File obtenerCarpetaPresetsUsuario();
    void actualizarListaPresets();
    void guardarPresetRapido(const juce::String& nombrePreset);
    void eliminarPresetActual (int index);
    bool esPresetDeUsuario (int index);
    int obtenerIndicePresetPorNombre (const juce::String& nombre);
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void obtenerCopiaIrActual (juce::AudioBuffer<float>& bufferDestino);
    float obtenerMagnitudFiltros (double frecuencia);
    juce::StringArray obtenerNombresIR() const
    {
        return {
            "1 - Alumno Derecha",
            "2 - Alumno Izquierda",
            "3 - Alumno Wide (prealigned)",
            "4 - Alumno Wide",
            "5 - Profesor Derecha",
            "6 - Profesor Izquierda",
            "7 - Profesor Wide (prealigned)",
            "8 - Profesor Wide"
        };
    }
    juce::AudioProcessorValueTreeState& obtenerAPVTS() { return listaParametros; }
    int getCantidadPresets() const { return static_cast<int>(listaCompletaPresets.size()); }

    std::vector<Preset> listaCompletaPresets;
    const std::vector<Preset> presetsDeFabrica = {
    { "Default", 0.5f, 1.0f, 0.0f, 20.0f, 20000.0f, 0, false, {}},
    { "Testfull", 1.0f, 5.0f, 150.0f, 20.0f, 20000.0f, 0, false, {}},
    };
    

private:
    juce::dsp::ProcessSpec spec;

    juce::AudioBuffer<float> cargarArchivoIRSeguro (const juce::File& archivoAudio, double& fsSalida, float& compensacionSalida);

    juce::AudioBuffer<float> modificarDecayIR (const juce::AudioBuffer<float>& irOriginal, float factorDecay, double fsIR);
    float factorCompensacionIR = 1.0f;

    juce::dsp::Convolution motorConvolucionHead { juce::dsp::Convolution::Latency { 64 } };
    juce::dsp::Convolution motorConvolucionTail { juce::dsp::Convolution::Latency { 4096 } };

    juce::dsp::DelayLine<float> lineaCompensacionHead { 8192 };

    juce::AudioBuffer<float> irHeadEnFondo, irTailEnFondo;
    juce::AudioBuffer<float> irCompletaModificada;
    juce::AudioBuffer<float> bufferTail;

    juce::AudioBuffer<float> bufferWet;

    juce::dsp::DelayLine<float> lineaCompensacionDry { 16384 };
    juce::AudioBuffer<float> bufferDryCompensado;

    static constexpr double duracionHeadMs = 180.0;
    int latenciaCompensacionMuestras = 0;
    void dividirIREnHeadYTail (const juce::AudioBuffer<float>& irCompleta, double fs, juce::AudioBuffer<float>& outHead, juce::AudioBuffer<float>& outTail);
    void cargarIREnMotores (juce::AudioBuffer<float>&& head, juce::AudioBuffer<float>&& tail, double fs);

    juce::AudioFormatManager empaquetadorFormatos;
    juce::AudioBuffer<float> irOriginal;
    double fsIR = 44100.0;

    float ultimoFactorDecay = -1.0f;

    using FiltroCorteChain = juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Filter<float>
    >;

    std::array<FiltroCorteChain, 2> filtrosCorte;

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
    int programaActual = 0;

    juce::File obtenerArchivoIRFijo (int indice);

    float gananciaNormalizacionActual = 1.0f;
    
    juce::AudioProcessorValueTreeState listaParametros;
    juce::AudioProcessorValueTreeState::ParameterLayout crearLayoutParametros();

    juce::ThreadPool hiloDeFondo { 1 };
    juce::CriticalSection cerrojoIR;
    std::atomic<bool> hayNuevaIRLista { false };

    std::atomic<bool> debeCargarNuevoArchivo { false };
    float gananciaTransicion = 1.0f;
    bool enTransicion = false;

    juce::AudioBuffer<float> irModificadaEnFondo;
    double fsIRModificada = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reverb402AudioProcessor)
};