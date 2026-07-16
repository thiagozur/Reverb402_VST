#include "PluginProcessor.h"
#include "PluginEditor.h"

Reverb402AudioProcessorEditor::Reverb402AudioProcessorEditor (Reverb402AudioProcessor& p) : AudioProcessorEditor (&p), audioProcessor (p), visualizadorIR (p)
{
    setLookAndFeel (&estilo402);

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
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setTextValueSuffix (sufijo);
        addAndMakeVisible (slider);
        
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    configurarKnob (sliderPreDelay, lblPreDelay, " ms");
    configurarKnob (sliderDecay, lblDecay, "x");
    configurarKnob (sliderHPF, lblHPF, " Hz");
    configurarKnob (sliderLPF, lblLPF, " kHz");
    configurarKnob (sliderMix, lblMix, "%");

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

    auto actualizarTextoLabel = [] (juce::Slider& s, juce::Label& l, const juce::String& nombreParametro)
    {
        juce::String sufijo = s.getTextValueSuffix();
        double valor = s.getValue();

        juce::String valorTexto;

        if (nombreParametro == "High-Pass")
            valorTexto = juce::String (static_cast<int>(valor)) + sufijo;
        else if (nombreParametro == "Low-Pass")
            valorTexto = juce::String (valor / 1000.0, 1) + sufijo;
        else if (nombreParametro == "Mix")
            valorTexto = juce::String (static_cast<int>(valor * 100)) + sufijo;
        else if (nombreParametro == "Decay")
            valorTexto = juce::String (valor, 1) + sufijo;
        else if (nombreParametro == "Pre-Delay")
            valorTexto = juce::String (static_cast<int>(valor)) + sufijo;
        else
            valorTexto = juce::String (valor, 1) + sufijo;

        l.setText (nombreParametro + "\n" + valorTexto, juce::dontSendNotification);
    };

    sliderPreDelay.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderPreDelay, lblPreDelay, "Pre-Delay"); };
    sliderDecay.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderDecay, lblDecay, "Decay"); };
    sliderHPF.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderHPF, lblHPF, "High-Pass"); };
    sliderLPF.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderLPF, lblLPF, "Low-Pass"); };
    sliderMix.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderMix, lblMix, "Mix"); };

    comboPresets.onChange = [this] { visualizadorIR.actualizarGraficos(); };
    comboIR.onChange = [this] { visualizadorIR.actualizarGraficos(); };
    sliderDecay.onDragEnd = [this] { visualizadorIR.actualizarGraficos(); };
    sliderDecay.addMouseListener(this, false);
    sliderHPF.onDragEnd = [this] { visualizadorIR.actualizarGraficos(); };
    sliderHPF.addMouseListener(this, false);
    sliderLPF.onDragEnd = [this] { visualizadorIR.actualizarGraficos(); };
    sliderLPF.addMouseListener(this, false);

    actualizarTextoLabel (sliderPreDelay, lblPreDelay, "Pre-Delay");
    actualizarTextoLabel (sliderDecay, lblDecay, "Decay");
    actualizarTextoLabel (sliderHPF, lblHPF, "High-Pass");
    actualizarTextoLabel (sliderLPF, lblLPF, "Low-Pass");
    actualizarTextoLabel (sliderMix, lblMix, "Mix");

    setSize (800, 700);
}

Reverb402AudioProcessorEditor::~Reverb402AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void Reverb402AudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    auto areaSuperior = bounds.removeFromTop (350);
    auto areaBorde = bounds.removeFromTop (10);
    auto areaInferior = bounds;

    g.setColour (juce::Colour (0xFF1F2024)); 
    g.fillRect (areaSuperior);

    g.setColour (juce::Colour (0xFFFCFCFC)); 
    g.fillRect (areaBorde);

    g.setColour (juce::Colour (0xFFEAEAEA)); 
    g.fillRect (areaInferior);

    g.setColour (juce::Colour (0xFF2D2D34).withAlpha (0.3f));
    g.drawHorizontalLine (areaSuperior.getBottom(), 0.0f, static_cast<float>(getWidth()));
    g.setColour (juce::Colours::white);
    g.drawHorizontalLine (areaBorde.getBottom(), 0.0f, static_cast<float>(getWidth()));
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

    visualizadorIR.setBounds (bounds.removeFromTop (300).reduced (10, 8));

    bounds.removeFromTop(20);

    auto areaControles = bounds.reduced (10, 8);

    auto areaIR = areaControles.removeFromTop (45);
    int xComboCentrado = areaIR.getX() + (areaIR.getWidth() - 340) / 2;
    int yComboCentrado = areaIR.getY() + (areaIR.getHeight() - 35) / 2;
    comboIR.setBounds (xComboCentrado, yComboCentrado, 340, 35);

    int anchoDecayKnob = 200;
    int anchoKnobsChicos = (areaControles.getWidth() - anchoDecayKnob) / 4;

    std::vector<std::pair<juce::Slider*, juce::Label*>> leftKnobs = {
        {&sliderPreDelay, &lblPreDelay},
        {&sliderHPF, &lblHPF}
    };

    for (auto& k : leftKnobs)
    {
        auto areaColumna = areaControles.removeFromLeft (anchoKnobsChicos).reduced (6, 0);
        
        int anchoEfectivo = areaColumna.getWidth();

        float deltaX = (anchoDecayKnob - anchoEfectivo) / 2;
        areaColumna.removeFromTop (deltaX);
        
        auto areaKnob = areaColumna.removeFromTop (anchoEfectivo + 0.2f * anchoEfectivo);
        
        auto areaLabel = areaColumna.removeFromTop (35);

        k.first->setBounds (areaKnob);
        k.second->setBounds (areaLabel);
    }

    auto areaColumnaDecay = areaControles.removeFromLeft (anchoDecayKnob).reduced (6, 0);
    int anchoEfectivoDecay = areaColumnaDecay.getWidth();
    auto areaKnobDecay = areaColumnaDecay.removeFromTop (anchoEfectivoDecay + 0.2f * anchoEfectivoDecay);
    auto areaLabelDecay = areaColumnaDecay.removeFromTop (35);
    sliderDecay.setBounds (areaKnobDecay);
    lblDecay.setBounds (areaLabelDecay);
    
    std::vector<std::pair<juce::Slider*, juce::Label*>> rightKnobs = {
        {&sliderLPF, &lblLPF},
        {&sliderMix, &lblMix}
    };

    for (auto& k : rightKnobs)
    {
        auto areaColumna = areaControles.removeFromLeft (anchoKnobsChicos).reduced (6, 0);
        
        int anchoEfectivo = areaColumna.getWidth();

        float deltaX = (anchoDecayKnob - anchoEfectivo) / 2;
        areaColumna.removeFromTop (deltaX);
        
        auto areaKnob = areaColumna.removeFromTop (anchoEfectivo + 0.2f * anchoEfectivo);
        
        auto areaLabel = areaColumna.removeFromTop (35);

        k.first->setBounds (areaKnob);
        k.second->setBounds (areaLabel);
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