#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Componentes/IRVisualizer.h"
#include "Estilos/Estilo402.h"
#include "Estilos/EstiloKnobGain.h"
#include "Componentes/MedidorNivel.h"
#include "Componentes/EscalaDb.h"

class Reverb402Component : public juce::Component
{
public:
    Reverb402Component (Reverb402AudioProcessor&);
    ~Reverb402Component() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDoubleClick (const juce::MouseEvent& event) override
    {
        if (event.eventComponent == &sliderDecay || event.eventComponent == &sliderHPF || event.eventComponent == &sliderLPF)
            visualizadorIR.actualizarGraficos();
    }

private:
    Reverb402AudioProcessor& audioProcessor;

    Estilo402 estilo402;
    EstiloKnobGain estiloKnobGain;

    IRVisualizerContainer visualizadorIR;
    MedidorNivel medidorNivelInL;
    MedidorNivel medidorNivelInR;
    EscalaDb escalaDbIn { MedidorNivel::dBMin, MedidorNivel::dBMax, MedidorNivel::totalLuces, { 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -40.0f, -60.0f } };
    EscalaDb escalaDbOut { MedidorNivel::dBMin, MedidorNivel::dBMax, MedidorNivel::totalLuces, { 6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -40.0f, -60.0f } };
    MedidorNivel medidorNivelOutL;
    MedidorNivel medidorNivelOutR;

    juce::Font orbitron;
    juce::Font orbitronMed;
    juce::Image fondoSideCompleto;
    juce::Image fondoSide;

    juce::TextButton btnPresetAnterior { juce::String::fromUTF8 ("\xE2\x97\x80") };
    juce::TextButton btnPresetSiguiente { juce::String::fromUTF8 ("\xE2\x96\xB6") };
    juce::ComboBox comboPresets;

    std::unique_ptr<juce::AlertWindow> cwRelease;

    juce::Slider sliderPreDelay;
    juce::Slider sliderDecay;
    juce::Slider sliderHPF;
    juce::Slider sliderLPF;
    juce::Slider sliderMix;
    juce::Slider sliderInputGain;

    juce::Label lblPreDelay { {}, "Pre-Delay" };
    juce::Label lblDecay { {}, "Decay" };
    juce::Label lblHPF { {}, "High-Pass" };
    juce::Label lblLPF { {}, "Low-Pass" };
    juce::Label lblMix { {}, "Mix" };
    juce::Label lblInputGain { {}, "In Gain" };

    juce::ComboBox comboIR;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> attachmentPreDelay;
    std::unique_ptr<SliderAttachment> attachmentDecay;
    std::unique_ptr<SliderAttachment> attachmentHPF;
    std::unique_ptr<SliderAttachment> attachmentLPF;
    std::unique_ptr<SliderAttachment> attachmentMix;
    std::unique_ptr<SliderAttachment> attachmentInputGain;
    std::unique_ptr<ComboBoxAttachment> attachmentIR;

    void actualizarMenuPresets();
    void cambiarPresetRelativo (int direccion);
    void mostrarDialogoGuardarPreset();
    void ejecutarBorradoPreset();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reverb402Component)
};

class WrappedReverb402AudioProcessorEditor : public AudioProcessorEditor 
{
public:
    WrappedReverb402AudioProcessorEditor(Reverb402AudioProcessor&);
    void resized() override;

private:
    static constexpr int originalWidth { 992 };
    static constexpr int originalHeight { 700 };

    juce::ComponentBoundsConstrainer myConstrainer;

    Reverb402Component reverb402Component;

    ApplicationProperties applicationProperties;
};