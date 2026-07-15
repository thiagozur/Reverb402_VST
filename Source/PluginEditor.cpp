#include "PluginProcessor.h"
#include "PluginEditor.h"

Reverb402AudioProcessorEditor::Reverb402AudioProcessorEditor (Reverb402AudioProcessor& p) : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible (visualizadorIR);
    addAndMakeVisible (panelPresets);

    addAndMakeVisible (btnAnterior);
    btnAnterior.onClick = [this] {
        int idx = audioProcessor.getCurrentProgram();
        int total = audioProcessor.getNumPrograms();
        if (total > 0)
        {
            idx = (idx - 1 + total) % total;
            audioProcessor.setCurrentProgram (idx);
            lblNombrePreset.setText (audioProcessor.getProgramName (idx), juce::dontSendNotification);
            actualizarEstadoBotonBorrar();
        }
    };

    addAndMakeVisible (btnSiguiente);
    btnSiguiente.onClick = [this] {
        int idx = audioProcessor.getCurrentProgram();
        int total = audioProcessor.getNumPrograms();
        if (total > 0)
        {
            idx = (idx + 1) % total;
            audioProcessor.setCurrentProgram (idx);
            lblNombrePreset.setText (audioProcessor.getProgramName (idx), juce::dontSendNotification);
            actualizarEstadoBotonBorrar();
        }
    };

    addAndMakeVisible (btnGuardar);
    btnGuardar.onClick = [this] {
        auto* alertWindow = new juce::AlertWindow ("Guardar Preset", "Introduzca un nombre:", juce::MessageBoxIconType::NoIcon, this);
        alertWindow->addTextEditor ("nombrePreset", "Preset Usuario", "");
        alertWindow->addButton ("Guardar", 1);
        alertWindow->addButton ("Cancelar", 0);
        alertWindow->enterModalState (true, juce::ModalCallbackFunction::create ([this, alertWindow] (int resultado) {
            if (resultado == 1)
            {
                if (auto* editorTexto = alertWindow->getTextEditor ("nombrePreset"))
                {
                    juce::String texto = editorTexto->getText();
                    if (texto.isNotEmpty())
                    {
                        audioProcessor.guardarPresetRapido (texto);
                        int nuevoIdx = audioProcessor.obtenerIndicePresetPorNombre (texto);
                        audioProcessor.setCurrentProgram (nuevoIdx);
                        lblNombrePreset.setText (audioProcessor.getProgramName (nuevoIdx), juce::dontSendNotification);
                        actualizarEstadoBotonBorrar();
                    }
                }
            }
            delete alertWindow;
        }), true);
    };

    btnBorrar.setColour (juce::TextButton::buttonColourId, juce::Colours::darkred);
    addAndMakeVisible (btnBorrar);
    btnBorrar.onClick = [this] {
        int idx = audioProcessor.getCurrentProgram();
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::WarningIcon, "¿Borrar?", "¿Estás seguro?", "Sí", "Cancelar", this,
            juce::ModalCallbackFunction::create ([this, idx] (int resultado) {
                if (resultado == 1) {
                    audioProcessor.eliminarPresetActual (idx);
                    audioProcessor.setCurrentProgram (0);
                    lblNombrePreset.setText (audioProcessor.getProgramName (0), juce::dontSendNotification);
                    actualizarEstadoBotonBorrar();
                }
            }));
    };

    lblNombrePreset.setJustificationType (juce::Justification::centred);
    lblNombrePreset.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (0.4f));
    addAndMakeVisible (lblNombrePreset);

    lblNombrePreset.setText (audioProcessor.getProgramName (audioProcessor.getCurrentProgram()), juce::dontSendNotification);
    actualizarEstadoBotonBorrar();

    auto configurarKnob = [this] (juce::Slider& slider, juce::Label& label, const juce::String& sufijo) {
        slider.setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        slider.setTextValueSuffix (sufijo);
        addAndMakeVisible (slider);
        
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    configurarKnob (sliderPreDelay, lblPreDelay, "");
    configurarKnob (sliderDecay, lblDecay, "");
    configurarKnob (sliderHPF, lblHPF, " Hz");
    configurarKnob (sliderLPF, lblLPF, "");
    configurarKnob (sliderMix, lblMix, "");

    addAndMakeVisible (comboIR);
    auto nombresIR = audioProcessor.obtenerNombresIR();

    for (int i = 0; i < nombresIR.size(); ++i)
        comboIR.addItem (nombresIR[i], i + 1);

    int idInicial = static_cast<int>(audioProcessor.obtenerAPVTS().getRawParameterValue("ir_select")->load()) + 1;
    comboIR.setSelectedId(idInicial, juce::dontSendNotification);

    attachmentPreDelay = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "predelay", sliderPreDelay);
    attachmentDecay = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "decay", sliderDecay);
    attachmentHPF = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "hpf", sliderHPF);
    attachmentLPF = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "lpf", sliderLPF);
    attachmentMix = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "mix", sliderMix);
    attachmentIR = std::make_unique<ComboBoxAttachment> (audioProcessor.obtenerAPVTS(), "ir_select", comboIR);

    setSize (800, 600);
}

Reverb402AudioProcessorEditor::~Reverb402AudioProcessorEditor() {}

void Reverb402AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF1E1E1E));
}

void Reverb402AudioProcessorEditor::resized ()
{
    auto bounds = getLocalBounds();

    auto areaPresetsCompleta = bounds.removeFromTop (60);
    
    panelPresets.setBounds (areaPresetsCompleta.reduced (10, 5));

    int yWidgets = areaPresetsCompleta.getY() + 22;
    int altoWidgets = 24;

    btnAnterior.setBounds  (20,  yWidgets, 40, altoWidgets);
    btnSiguiente.setBounds (65,  yWidgets, 40, altoWidgets);

    int xDerecha = areaPresetsCompleta.getWidth() - 20;
    btnGuardar.setBounds   (xDerecha - 90, yWidgets, 90, altoWidgets);
    btnBorrar.setBounds    (xDerecha - 90 - 10 - 70, yWidgets, 70, altoWidgets);

    int xLabel = 115;
    int anchoLabel = (xDerecha - 90 - 10 - 70 - 10) - xLabel;
    lblNombrePreset.setBounds (xLabel, yWidgets, anchoLabel, altoWidgets);

    visualizadorIR.setBounds (bounds.removeFromTop (250).reduced (10, 5));

    auto areaControles = bounds.reduced (10, 5);

    auto areaIR = areaControles.removeFromLeft (160);
    int xComboCentrado = areaIR.getX() + (areaIR.getWidth() - 140) / 2;
    int yComboCentrado = areaIR.getY() + (areaIR.getHeight() - 32) / 2;
    comboIR.setBounds (xComboCentrado, yComboCentrado, 140, 32);

    int anchoKnob = areaControles.getWidth() / 5;
    
    std::vector<std::pair<juce::Slider*, juce::Label*>> knobs = {
        {&sliderPreDelay, &lblPreDelay},
        {&sliderDecay, &lblDecay},
        {&sliderHPF, &lblHPF},
        {&sliderLPF, &lblLPF},
        {&sliderMix, &lblMix}
    };

    for (auto& k : knobs)
    {
        auto areaIndividual = areaControles.removeFromLeft (anchoKnob).reduced (4, 0);
        k.second->setBounds (areaIndividual.removeFromTop (20));
        k.first->setBounds (areaIndividual);
    }
}

void Reverb402AudioProcessorEditor::actualizarEstadoBotonBorrar()
{
    int idx = audioProcessor.getCurrentProgram();
    btnBorrar.setEnabled (audioProcessor.esPresetDeUsuario (idx));
}