#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "IRVisualizer.h"

class Reverb402AudioProcessorEditor : public juce::AudioProcessorEditor, public juce::ChangeListener
{
public:
    Reverb402AudioProcessorEditor (Reverb402AudioProcessor&);
    ~Reverb402AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

private:
    Reverb402AudioProcessor& audioProcessor;

    IRVisualizerContainer visualizadorIR;

    juce::TextButton btnPresetAnterior { "<" };
    juce::TextButton btnPresetSiguiente { ">" };
    juce::ComboBox comboPresets;

    std::unique_ptr<juce::AlertWindow> cwRelease;

    juce::Slider sliderPreDelay;
    juce::Slider sliderDecay;
    juce::Slider sliderHPF;
    juce::Slider sliderLPF;
    juce::Slider sliderMix;

    juce::Label lblPreDelay { {}, "Pre-Delay" };
    juce::Label lblDecay { {}, "Decay" };
    juce::Label lblHPF { {}, "High-Pass" };
    juce::Label lblLPF { {}, "Low-Pass" };
    juce::Label lblMix { {}, "Mix" };

    juce::ComboBox comboIR;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> attachmentPreDelay;
    std::unique_ptr<SliderAttachment> attachmentDecay;
    std::unique_ptr<SliderAttachment> attachmentHPF;
    std::unique_ptr<SliderAttachment> attachmentLPF;
    std::unique_ptr<SliderAttachment> attachmentMix;
    std::unique_ptr<ComboBoxAttachment> attachmentIR;

    void actualizarMenuPresets();
    void cambiarPresetRelativo (int direccion);
    void mostrarDialogoGuardarPreset();
    void ejecutarBorradoPreset();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reverb402AudioProcessorEditor)
};