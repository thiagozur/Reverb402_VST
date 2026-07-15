#include "PluginProcessor.h"
#include "PluginEditor.h"

Reverb402AudioProcessorEditor::Reverb402AudioProcessorEditor (Reverb402AudioProcessor& p) : AudioProcessorEditor (&p), audioProcessor (p), visualizadorIR (p)
{
    addAndMakeVisible (visualizadorIR);

    addAndMakeVisible (btnPresetAnterior);
    btnPresetAnterior.onClick = [this] { cambiarPresetRelativo (-1); };

    addAndMakeVisible (btnPresetSiguiente);
    btnPresetSiguiente.onClick = [this] { cambiarPresetRelativo (1); };

    addAndMakeVisible (comboPresets);
    actualizarMenuPresets();

    comboPresets.onChange = [this] {
        int idSeleccionado = comboPresets.getSelectedId();

        if (idSeleccionado == 1000)
            mostrarDialogoGuardarPreset();

        else if (idSeleccionado == 1001)
            ejecutarBorradoPreset();

        else if (idSeleccionado > 0)
        {
            int indexPreset = idSeleccionado - 1;
            audioProcessor.setCurrentProgram (indexPreset);
            actualizarMenuPresets();
        }
    };

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

    sliderHPF.onDragEnd = [this] { visualizadorIR.actualizarGraficos(); };
    sliderLPF.onDragEnd = [this] { visualizadorIR.actualizarGraficos(); };
    sliderDecay.onDragEnd = [this] { visualizadorIR.actualizarGraficos(); };

    setSize (800, 600);

    audioProcessor.addChangeListener (this);
}

Reverb402AudioProcessorEditor::~Reverb402AudioProcessorEditor()
{
    audioProcessor.removeChangeListener (this);
}

void Reverb402AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF1E1E1E));
}

void Reverb402AudioProcessorEditor::resized ()
{
    auto bounds = getLocalBounds();

    auto areaPresets = bounds.removeFromTop (45).reduced (10, 8);

    int anchoFlecha = 30;

    btnPresetAnterior.setBounds (areaPresets.removeFromLeft (anchoFlecha));
    areaPresets.removeFromLeft (4);

    btnPresetSiguiente.setBounds (areaPresets.removeFromRight (anchoFlecha));
    areaPresets.removeFromRight (4);

    comboPresets.setBounds (areaPresets);

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

    visualizadorIR.actualizarGraficos();
}

void Reverb402AudioProcessorEditor::actualizarMenuPresets()
{
    comboPresets.clear (juce::dontSendNotification);

    int totalPresets = audioProcessor.getNumPrograms();
    int presetActual = audioProcessor.getCurrentProgram();

    for (int i = 0; i < totalPresets; ++i)
        comboPresets.addItem (audioProcessor.getProgramName (i), i + 1);

    comboPresets.addSeparator();

    comboPresets.addItem ("+ Guardar Preset", 1000);
    
    if (audioProcessor.esPresetDeUsuario (presetActual))
        comboPresets.addItem ("- Borrar Preset Actual", 1001);

    comboPresets.setSelectedId (presetActual + 1, juce::dontSendNotification);
}

void Reverb402AudioProcessorEditor::cambiarPresetRelativo (int direccion)
{
    int total = audioProcessor.getNumPrograms();

    if (total <= 0)
        return;
    
    int actual = audioProcessor.getCurrentProgram();
    int nuevoIdx = (actual + direccion + total) % total;

    audioProcessor.setCurrentProgram (nuevoIdx);
    actualizarMenuPresets();
}

void Reverb402AudioProcessorEditor::mostrarDialogoGuardarPreset()
{
    auto cuadroTexto = std::make_unique<juce::AlertWindow> ("Guardar Preset", "Introducir nombre:", juce::MessageBoxIconType::NoIcon, this);
    cuadroTexto->addTextEditor ("nombrePreset", "", "");
    cuadroTexto->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    cuadroTexto->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    cwRelease = std::move (cuadroTexto);

    cwRelease->enterModalState (true, juce::ModalCallbackFunction::create ([this, cw = cuadroTexto.get()] (int resultado)
    {
        auto cw = std::move (cwRelease);

        if (cw != nullptr && resultado == 1)
        {
            if (auto* editorTexto = cw->getTextEditor ("nombrePreset"))
            {
                juce::String nombre = editorTexto->getText().trim();
                
                if (nombre.isNotEmpty())
                {
                    audioProcessor.guardarPresetRapido (nombre);
                    audioProcessor.actualizarListaPresets();
                    int nuevoIdx = audioProcessor.obtenerIndicePresetPorNombre (nombre);
                    audioProcessor.setCurrentProgram (nuevoIdx);
                    
                    actualizarMenuPresets();
                }
            }
        }
        else
            actualizarMenuPresets();
    }));
}

void Reverb402AudioProcessorEditor::ejecutarBorradoPreset()
{
    int presetActual = audioProcessor.getCurrentProgram();
    if (audioProcessor.esPresetDeUsuario (presetActual))
    {  
        juce::String msg1 = "¿Estás seguro de que deseas eliminar ";
        juce::String nombrePresetBorrado = audioProcessor.getProgramName(presetActual);
        juce::String msg2 = " permamentemente?";
        juce::String msg = msg1 + nombrePresetBorrado + msg2;

        juce::AlertWindow::showOkCancelBox (
            juce::MessageBoxIconType::WarningIcon,
            "¿Borrar preset?",
            msg,
            "Sí, borrar",
            "Cancelar",
            this,
            juce::ModalCallbackFunction::create ([this, presetActual] (int resultado) 
            {
                if (resultado == 1) 
                {   
                    cambiarPresetRelativo (-1);
                    audioProcessor.eliminarPresetActual (presetActual);
                    actualizarMenuPresets();
                }
                else
                    actualizarMenuPresets();
            })
        );
    }
}

void Reverb402AudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &audioProcessor)
    {
        visualizadorIR.actualizarGraficos();
    }
}