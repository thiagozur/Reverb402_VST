#include "PluginProcessor.h"
#include "PluginEditor.h"

Reverb402Component::Reverb402Component (Reverb402AudioProcessor& p) : audioProcessor (p), visualizadorIR (p), medidorNivelInL ([&]() { return audioProcessor.obtenerRMSIn (0); }), medidorNivelInR ([&]() { return audioProcessor.obtenerRMSIn (1); }), medidorNivelOutL ([&]() { return audioProcessor.obtenerRMSOut (0); }), medidorNivelOutR ([&]() { return audioProcessor.obtenerRMSOut (1); })
{
    setLookAndFeel (&estilo402);

    auto orbitronTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::OrbitronBlack_ttf, BinaryData::OrbitronBlack_ttfSize);
    orbitron = juce::Font (orbitronTypeface);
    orbitron.setHeight (30.0f);

    auto orbitronMedTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::OrbitronMedium_ttf, BinaryData::OrbitronMedium_ttfSize);
    orbitronMed = juce::Font (orbitronMedTypeface);
    orbitronMed.setHeight (20.0f);
    
    fondoSideCompleto = juce::ImageCache::getFromMemory (BinaryData::aluminio_jpg, BinaryData::aluminio_jpgSize);

    addAndMakeVisible (visualizadorIR);
    visualizadorIR.onABToggled = [this] { actualizarMenuPresets();  };

    addAndMakeVisible (escalaDbIn);
    addAndMakeVisible (medidorNivelInL);
    addAndMakeVisible (medidorNivelInR);
    addAndMakeVisible (escalaDbOut);
    addAndMakeVisible (medidorNivelOutL);
    addAndMakeVisible (medidorNivelOutR);

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

    sliderInputGain.setLookAndFeel (&estiloKnobGain);
    configurarKnob (sliderInputGain, lblInputGain, " dB");

    addAndMakeVisible (comboIR);
    auto nombresIR = audioProcessor.obtenerNombresIR();

    for (int i = 0; i < nombresIR.size(); ++i)
        comboIR.addItem (nombresIR[i], i + 1);

    attachmentPreDelay = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "predelay", sliderPreDelay);
    attachmentDecay = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "decay", sliderDecay);
    attachmentHPF = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "hpf", sliderHPF);
    attachmentLPF = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "lpf", sliderLPF);
    attachmentMix = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "mix", sliderMix);
    attachmentInputGain = std::make_unique<SliderAttachment> (audioProcessor.obtenerAPVTS(), "inputGain", sliderInputGain);
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
        else if (nombreParametro == "Input Gain")
            valorTexto = juce::String (valor, 1) + sufijo;
        else
            valorTexto = juce::String (valor, 1) + sufijo;

        l.setText (nombreParametro + "\n" + valorTexto, juce::dontSendNotification);
    };

    sliderPreDelay.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderPreDelay, lblPreDelay, "Pre-Delay"); };
    sliderDecay.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderDecay, lblDecay, "Decay"); };
    sliderHPF.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderHPF, lblHPF, "High-Pass"); };
    sliderLPF.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderLPF, lblLPF, "Low-Pass"); };
    sliderMix.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderMix, lblMix, "Mix"); };
    sliderInputGain.onValueChange = [this, actualizarTextoLabel] { actualizarTextoLabel (sliderInputGain, lblInputGain, "Input Gain"); };

    actualizarTextoLabel (sliderPreDelay, lblPreDelay, "Pre-Delay");
    actualizarTextoLabel (sliderDecay, lblDecay, "Decay");
    actualizarTextoLabel (sliderHPF, lblHPF, "High-Pass");
    actualizarTextoLabel (sliderLPF, lblLPF, "Low-Pass");
    actualizarTextoLabel (sliderMix, lblMix, "Mix");
    actualizarTextoLabel (sliderInputGain, lblInputGain, "Input Gain");

    auto configurarBtn = [this](juce::TextButton& b)
    {
        b.setButtonText ("");
        b.setClickingTogglesState (true);
        b.setComponentID ("ClickButton");
        b.setLookAndFeel (&estiloClickBtn);
        addAndMakeVisible (b);
    };

    configurarBtn (btnDuck);

    attachmentDuck = std::make_unique<ButtonAttachment> (audioProcessor.obtenerAPVTS(), "duck", btnDuck);
}

Reverb402Component::~Reverb402Component()
{
    setLookAndFeel (nullptr);
    sliderInputGain.setLookAndFeel (nullptr);
}

void Reverb402Component::paint (juce::Graphics& g)
{
    auto frameBounds = getLocalBounds();
    auto areaMenuPresets = frameBounds.removeFromTop (45);
    auto leftSidebarBounds = frameBounds.removeFromLeft(90);
    auto bounds = frameBounds.removeFromLeft(812);
    auto rightSidebarBounds = frameBounds;

    if (fondoSide.isValid())
    {
        g.drawImageAt (fondoSide, leftSidebarBounds.getX(), leftSidebarBounds.getY());
        g.drawImageAt (fondoSide, rightSidebarBounds.getX(), rightSidebarBounds.getY());
    }
    
    auto areaSuperior = bounds.removeFromTop (305);
    auto areaAristaIzq = areaSuperior.removeFromLeft (8).toFloat();
    auto areaAristaDer = areaSuperior.removeFromRight (8).toFloat();
    auto areaBorde = bounds.removeFromTop (10);
    auto areaInferior = bounds;

    juce::Path aristaIzq;
    aristaIzq.startNewSubPath (areaBorde.toFloat().getBottomLeft());
    aristaIzq.lineTo (areaAristaIzq.getTopLeft());
    aristaIzq.lineTo (areaAristaIzq.getTopRight());
    aristaIzq.lineTo (areaAristaIzq.getBottomRight());
    aristaIzq.closeSubPath();

    juce::Path aristaDer;
    aristaDer.startNewSubPath (areaBorde.toFloat().getBottomRight());
    aristaDer.lineTo (areaAristaDer.getTopRight());
    aristaDer.lineTo (areaAristaDer.getTopLeft());
    aristaDer.lineTo (areaAristaDer.getBottomLeft());
    aristaDer.closeSubPath();

    g.setColour (juce::Colour (0xFF111113)); 
    g.fillRect (areaMenuPresets);

    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawHorizontalLine (areaMenuPresets.getBottom() - 1, areaMenuPresets.getX(),  areaMenuPresets.getWidth());

    juce::ColourGradient shadeSuperior;
    shadeSuperior.isRadial = false;
    shadeSuperior.point1 = areaSuperior.toFloat().getTopLeft();
    shadeSuperior.point2 = areaSuperior.toFloat().getBottomLeft();
    shadeSuperior.addColour (0.0f, juce::Colour (0xFF1F2024).darker (0.4));
    shadeSuperior.addColour (0.05f, juce::Colour (0xFF1F2024));
    shadeSuperior.addColour (0.95f, juce::Colour (0xFF1F2024));
    shadeSuperior.addColour (1.0f, juce::Colour (0xFF1F2024).darker (0.4));
    
    g.setGradientFill(shadeSuperior);
    g.fillRect (areaSuperior);

    juce::ColourGradient shadeSuperiorHorizontal;
    shadeSuperiorHorizontal.isRadial = false;
    shadeSuperiorHorizontal.point1 = areaSuperior.toFloat().getTopLeft();
    shadeSuperiorHorizontal.point2 = areaSuperior.toFloat().getTopRight();
    shadeSuperiorHorizontal.addColour (0.0f, juce::Colour (0xFF1F2024).darker (1));
    shadeSuperiorHorizontal.addColour (0.03f, juce::Colour (0xFF1F2024).withAlpha (0.0f));
    shadeSuperiorHorizontal.addColour (0.97f, juce::Colour (0xFF1F2024).withAlpha (0.0f));
    shadeSuperiorHorizontal.addColour (1.0f, juce::Colour (0xFF1F2024).darker (1));

    g.setGradientFill(shadeSuperiorHorizontal);
    g.fillRect (areaSuperior);

    juce::ColourGradient shadeBorde;
    shadeBorde.isRadial = false;
    shadeBorde.point1 = areaBorde.toFloat().getTopLeft();
    shadeBorde.point2 = areaBorde.toFloat().getBottomLeft();
    shadeBorde.addColour (0.0f, juce::Colour (0xFFFCFCFC).darker (0.6));
    shadeBorde.addColour (0.8f, juce::Colour (0xFFFCFCFC));
    shadeBorde.addColour (1.0f, juce::Colour (0xFFFCFCFC));

    g.setGradientFill(shadeBorde);
    g.fillRect (areaBorde);

    juce::ColourGradient shadeBordeHorizontal;
    shadeBordeHorizontal.isRadial = false;
    shadeBordeHorizontal.point1 = areaBorde.toFloat().getTopLeft();
    shadeBordeHorizontal.point2 = areaBorde.toFloat().getTopRight();
    shadeBordeHorizontal.addColour (0.0f, juce::Colour (0xFFFCFCFC).darker (4));
    shadeBordeHorizontal.addColour (0.05f, juce::Colour (0xFFFCFCFC).withAlpha (0.0f));
    shadeBordeHorizontal.addColour (0.95f, juce::Colour (0xFFFCFCFC).withAlpha (0.0f));
    shadeBordeHorizontal.addColour (1.0f, juce::Colour (0xFFFCFCFC).darker (4));

    g.setGradientFill (shadeBordeHorizontal);
    g.fillRect (areaBorde);

    juce::ColourGradient shadeBordeInv;
    shadeBordeInv.isRadial = false;
    shadeBordeInv.point1 = areaBorde.toFloat().getBottomLeft();
    shadeBordeInv.point2 = areaBorde.toFloat().getTopLeft();
    shadeBordeInv.addColour (0.0f, juce::Colour (0xFFFCFCFC).withAlpha (0.8f));
    shadeBordeInv.addColour (0.8f, juce::Colour (0xFFFCFCFC).withAlpha (0.0f));

    g.setGradientFill (shadeBordeInv);
    g.fillRect (areaBorde);

    juce::ColourGradient shadeAristaIzq;
    shadeAristaIzq.isRadial = false;
    shadeAristaIzq.point1 = areaAristaIzq.getTopRight();
    shadeAristaIzq.point2 = areaAristaIzq.getTopLeft();
    shadeAristaIzq.addColour (0.0f, juce::Colour (0xFF282B2A).darker (0.9));
    shadeAristaIzq.addColour (0.6f, juce::Colour (0xFF282B2A).darker (0.2));
    shadeAristaIzq.addColour (1.0f, juce::Colour (0xFF282B2A).brighter (0.05));

    g.setGradientFill (shadeAristaIzq);
    g.fillPath (aristaIzq);

    juce::ColourGradient shadeAristaDer;
    shadeAristaDer.isRadial = false;
    shadeAristaDer.point1 = areaAristaDer.getTopLeft();
    shadeAristaDer.point2 = areaAristaDer.getTopRight();
    shadeAristaDer.addColour (0.0f, juce::Colour (0xFF282B2A).darker (0.9));
    shadeAristaDer.addColour (0.6f, juce::Colour (0xFF282B2A).darker (0.2));
    shadeAristaDer.addColour (1.0f, juce::Colour (0xFF282B2A).brighter (0.05));

    g.setGradientFill (shadeAristaDer);
    g.fillPath (aristaDer);

    juce::ColourGradient shadeInferior;
    shadeInferior.isRadial = false;
    shadeInferior.point1 = areaInferior.toFloat().getTopLeft();
    shadeInferior.point2 = areaInferior.toFloat().getTopRight();
    shadeInferior.addColour (0.0f, juce::Colour (0xFFEAEAEA).darker (0.2));
    shadeInferior.addColour (0.01f, juce::Colour (0xFFEAEAEA));
    shadeInferior.addColour (0.99f, juce::Colour (0xFFEAEAEA));
    shadeInferior.addColour (1.0f, juce::Colour (0xFFEAEAEA).darker (0.2));

    g.setGradientFill(shadeInferior); 
    g.fillRect (areaInferior);

    g.setFont (orbitron);
    g.setColour (juce::Colour (0xFF1F2024));

    int logoXPos = areaInferior.getX() + 20;
    int logoYPos = areaInferior.getY() + 20;
    int logoWidth = 300;
    int logoHeight = 40;

    g.drawText ("Reverb402", logoXPos, logoYPos, logoWidth, logoHeight, juce::Justification::topLeft);
    
    auto inMeterBounds = leftSidebarBounds.removeFromBottom (340);
    auto inMeterLabelBounds = inMeterBounds.removeFromTop (30).toFloat();
    
    g.setColour (juce::Colours::black);
    g.drawText ("IN", inMeterLabelBounds.translated (0.0f, -1.0f), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("IN", inMeterLabelBounds.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
    g.drawText ("IN", inMeterLabelBounds, juce::Justification::centred);

    auto outMeterBounds = rightSidebarBounds.removeFromBottom (340);
    auto outMeterLabelBounds = outMeterBounds.removeFromTop (30).toFloat();
    
    g.setColour (juce::Colours::black);
    g.drawText ("OUT", outMeterLabelBounds.translated (0.0f, -1.0f), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("OUT", outMeterLabelBounds.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
    g.drawText ("OUT", outMeterLabelBounds, juce::Justification::centred);

    auto inputSpacerBounds = leftSidebarBounds.removeFromTop (87);
    auto inputGainLabelBounds = leftSidebarBounds.removeFromTop (10);

    g.setFont (orbitronMed);
    g.setColour (juce::Colours::black);
    g.drawText ("In Gain", inputGainLabelBounds.translated (0.0f, -1.0f), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("In Gain", inputGainLabelBounds.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
    g.drawText ("In Gain", inputGainLabelBounds, juce::Justification::centred);
}

void Reverb402Component::resized ()
{
    auto frameBounds = getLocalBounds();
    auto areaPresets = frameBounds.removeFromTop (45).reduced (10, 8);
    auto leftSidebarBounds = frameBounds.removeFromLeft (90);
    auto bounds = frameBounds.removeFromLeft (812);
    auto rightSidebarBounds = frameBounds;

    if (fondoSideCompleto.isValid())
    {
        juce::Rectangle<int> zonaCorte (0, 0, leftSidebarBounds.getWidth(), leftSidebarBounds.getHeight());
        fondoSide = fondoSideCompleto.getClippedImage (zonaCorte);
    }

    int anchoEscala = 25;

    auto inMeterBounds = leftSidebarBounds.removeFromBottom (340).reduced (12, 10);
    auto inMeterLabelBounds = inMeterBounds.removeFromTop (30);
    auto escalaInBounds = inMeterBounds.removeFromLeft (anchoEscala);
    escalaDbIn.setBounds (escalaInBounds);
    inMeterBounds.reduce(2, 0);

    auto inputSpacerBounds = leftSidebarBounds.removeFromTop (87);
    auto inputGainLabelBounds = leftSidebarBounds.removeFromTop (10);
    auto sliderInputGainBounds = leftSidebarBounds.removeFromTop (121).reduced (5);
    sliderInputGain.setBounds (sliderInputGainBounds);

    auto outMeterBounds = rightSidebarBounds.removeFromBottom (340).reduced (12, 10);
    auto outMeterLabelBounds = outMeterBounds.removeFromTop (30);
    auto escalaOutBounds = outMeterBounds.removeFromRight (anchoEscala);
    escalaDbOut.setBounds (escalaOutBounds);
    outMeterBounds.reduce(2, 0);

    auto outputSpacerBounds = rightSidebarBounds.removeFromTop (80);
    auto btnDuckBounds = rightSidebarBounds.removeFromTop (25).reduced (20.0f, 0.0f);
    btnDuck.setBounds (btnDuckBounds);

    auto inMeterBoundsL = inMeterBounds.removeFromLeft (inMeterBounds.getWidth() / 2.0f);
    auto inMeterBoundsR = inMeterBounds;
    medidorNivelInL.setBounds (inMeterBoundsL);
    medidorNivelInR.setBounds (inMeterBoundsR);

    auto outMeterBoundsL = outMeterBounds.removeFromLeft (outMeterBounds.getWidth() / 2.0f);
    auto outMeterBoundsR = outMeterBounds;
    medidorNivelOutL.setBounds (outMeterBoundsL);
    medidorNivelOutR.setBounds (outMeterBoundsR);

    int anchoFlecha = 30;

    btnPresetAnterior.setBounds (areaPresets.removeFromLeft (anchoFlecha));
    areaPresets.removeFromLeft (4);

    btnPresetSiguiente.setBounds (areaPresets.removeFromRight (anchoFlecha));
    areaPresets.removeFromRight (4);

    comboPresets.setBounds (areaPresets);

    auto areaAristaIzq = bounds.removeFromLeft (6);
    auto areaAristaDer = bounds.removeFromRight (6);

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

void Reverb402Component::actualizarMenuPresets()
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

void Reverb402Component::cambiarPresetRelativo (int direccion)
{
    int total = audioProcessor.getNumPrograms();

    if (total <= 0)
        return;
    
    int actual = audioProcessor.getCurrentProgram();
    int nuevoIdx = (actual + direccion + total) % total;

    audioProcessor.setCurrentProgram (nuevoIdx);
    actualizarMenuPresets();
}

void Reverb402Component::mostrarDialogoGuardarPreset()
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

void Reverb402Component::ejecutarBorradoPreset()
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

WrappedReverb402AudioProcessorEditor::WrappedReverb402AudioProcessorEditor (Reverb402AudioProcessor& p) : AudioProcessorEditor (p), reverb402Component (p)
{
    addAndMakeVisible (reverb402Component);

    PropertiesFile::Options options;
    options.applicationName = ProjectInfo::projectName;
    options.commonToAllUsers = true;
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support"; 
    applicationProperties.setStorageParameters (options);

    myConstrainer.setFixedAspectRatio (static_cast<double>(originalWidth) / static_cast<double>(originalHeight));
    myConstrainer.setSizeLimits (688, 602, originalWidth, originalHeight);
    setConstrainer (&myConstrainer);

    setResizable (true, true);

    auto sizeRatio { 1.0 };

    if (auto* properties = applicationProperties.getCommonSettings(true))
    {
        sizeRatio = properties->getDoubleValue("sizeRatio", 1.0);
    }

    setSize (static_cast<int>(originalWidth * sizeRatio), static_cast<int>(originalHeight * sizeRatio));
}

void WrappedReverb402AudioProcessorEditor::resized()
{
    const auto scaleFactor = static_cast<float>(getWidth()) / originalWidth;
    if (auto* properties = applicationProperties.getCommonSettings (true))
    {
        properties->setValue("sizeRatio", scaleFactor);
    }

    reverb402Component.setTransform (AffineTransform::scale(scaleFactor));
    reverb402Component.setBounds (0.0f, 0.0f, originalWidth, originalHeight);
}