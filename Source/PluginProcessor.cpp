#include "PluginProcessor.h"
#include "PluginEditor.h"

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

std::vector<Preset> listaCompletaPresets;

const std::vector<Preset> presetsDeFabrica = {
    { "Default", 0.5f, 1.0f, 0.0f, 20.0f, 20000.0f, 0, false, {}},
    { "Testfull", 1.0f, 5.0f, 150.0f, 20.0f, 20000.0f, 0, false, {}},
};

Reverb402AudioProcessor::Reverb402AudioProcessor()
    : AudioProcessor (BusesProperties()
                    .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    listaParametros (*this, nullptr, "PARAMETROS", crearLayoutParametros())
{

    empaquetadorFormatos.registerBasicFormats();

    paramMix = listaParametros.getRawParameterValue("mix");
    paramDecay = listaParametros.getRawParameterValue("decay");
    paramHPF = listaParametros.getRawParameterValue("hpf");
    paramLPF = listaParametros.getRawParameterValue("lpf");
    paramPreDelay = listaParametros.getRawParameterValue("predelay");
    paramIRSelection = listaParametros.getRawParameterValue("ir_select");

    actualizarListaPresets();
}

Reverb402AudioProcessor::~Reverb402AudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout Reverb402AudioProcessor::crearLayoutParametros()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("mix", 1),
        "Mix",
        0.0f,
        1.0f,
        0.5f
    ));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("decay", 1),
        "Decay",
        0.1f,
        5.0f,
        1.0f
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("predelay", 1),
        "Pre-Delay",
        0.0f,
        150.0f,
        0.0f
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("hpf", 1), 
        "High-Pass", 
        juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.3f),
        20.0f
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("lpf", 1),
        "Low-Pass", 
        juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.3f),
        20000.0f
    ));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "ir_select",
        "Respuesta al Impulso",
        nombresIRs,
        0
    ));
    
    return layout;
}

void Reverb402AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    lineaPreDelay.prepare(spec);
    lineaPreDelay.setMaximumDelayInSamples (static_cast<int> (sampleRate));

    filtrosCorte.prepare(spec);
    *filtrosCorte.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 20.0f);
    *filtrosCorte.get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 20000.0f);

    juce::AudioBuffer<float> bufferVacio; 
    
    motorConvolucion.loadImpulseResponse (std::move (bufferVacio), sampleRate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);

    juce::dsp::ProcessSpec specConvolucion;
    specConvolucion.sampleRate = sampleRate;
    specConvolucion.numChannels = spec.numChannels;
    specConvolucion.maximumBlockSize = samplesPerBlock;
    motorConvolucion.prepare(specConvolucion);
    
    setLatencySamples(motorConvolucion.getLatency());
}

juce::File Reverb402AudioProcessor::obtenerArchivoIRFijo (int indice)
{
    juce::File archivoBinario = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

    juce::File carpetaProyecto = archivoBinario.getParentDirectory()
                                              .getParentDirectory()
                                              .getParentDirectory()
                                              .getParentDirectory()
                                              .getParentDirectory();
                                              
    juce::File carpetaIR = carpetaProyecto.getChildFile ("IR");

    if (! carpetaIR.isDirectory())
    {
        carpetaIR = archivoBinario.getParentDirectory().getChildFile ("IR");
    }

    switch (indice)
    {
        case 0: return carpetaIR.getChildFile ("Alumno_izquierda.wav");
        case 1: return carpetaIR.getChildFile ("Alumno_derecha.wav");
        case 2: return carpetaIR.getChildFile ("Alumno_wide.wav");
        case 3: return carpetaIR.getChildFile ("Alumno_wide_(prealigned).wav");
        case 4: return carpetaIR.getChildFile ("Profesor_izquierda.wav");
        case 5: return carpetaIR.getChildFile ("Profesor_derecha.wav");
        case 6: return carpetaIR.getChildFile ("Profesor_wide.wav");
        case 7: return carpetaIR.getChildFile ("Profesor_wide_(prealigned).wav");
        default: return carpetaIR.getChildFile ("Alumno_izquierda.wav");
    }
}

void Reverb402AudioProcessor::cargarArchivoIR (const juce::File& archivoAudio)
{
    std::unique_ptr<juce::AudioFormatReader> lector (empaquetadorFormatos.createReaderFor (archivoAudio));

    if (lector != nullptr)
    {
        fsIR = lector->sampleRate;
        const int canalesArchivo = static_cast<int>(lector->numChannels);
        const int muestrasArchivo = static_cast<int>(lector->lengthInSamples);

        irOriginal.clear();
        irOriginal.setSize (canalesArchivo, muestrasArchivo, false, true, true);

        lector->read (irOriginal.getArrayOfWritePointers(), canalesArchivo, 0, muestrasArchivo);

        if (muestrasArchivo > 0 && fsIR > 0.0)
        {
            float rmsTotalIR = irOriginal.getRMSLevel (0, 0, muestrasArchivo);
            if (rmsTotalIR > 0.0001f)
            {
                float duracionSeg = static_cast<float>(muestrasArchivo) / static_cast<float>(fsIR);
                float compensacionPorDuracion = std::sqrt (duracionSeg);

                factorCompensacionIR = (0.10f / rmsTotalIR) * compensacionPorDuracion;
                factorCompensacionIR = juce::jlimit (0.1f, 8.0f, factorCompensacionIR);
            }
            else
                factorCompensacionIR = 1.0f;
        }
        else
            factorCompensacionIR = 1.0f;

        juce::AudioBuffer<float> irTemporal;
        irTemporal.makeCopyOf (irOriginal);

        motorConvolucion.loadImpulseResponse(std::move (irTemporal), fsIR, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
    }
}

juce::AudioBuffer<float> Reverb402AudioProcessor::modificarDecayIR (const juce::AudioBuffer<float>& irOriginal, float factorDecay, double fsIR)
{
    const int numCanales = irOriginal.getNumChannels();
    const int muestrasOriginales = irOriginal.getNumSamples();
    double duracionReal = static_cast<double>(muestrasOriginales) / fsIR;
    double decaySeg = duracionReal * factorDecay;

    juce::AudioBuffer<float> irResultado;

    if (factorDecay == 1.0f)
        irResultado.makeCopyOf(irOriginal);
    else if (factorDecay <1.0f)
    { 
        irResultado.makeCopyOf(irOriginal);

        for (int muestra = 0; muestra < muestrasOriginales; ++muestra)
        {
            double t = static_cast<double>(muestra) / fsIR;
            float envolvente = std::exp(- (5.0f / static_cast<float>(decaySeg)) * static_cast<float>(t));

            for (int canal = 0; canal < numCanales; ++canal)
                irResultado.setSample (canal, muestra, irResultado.getSample (canal, muestra) * envolvente);
        }
    }
    else
    {
        int muestrasAtaque = static_cast<int>(0.060 * fsIR);
        if (muestrasAtaque > muestrasOriginales)
            muestrasAtaque = muestrasOriginales;
        
        int muestrasColaOriginal = muestrasOriginales - muestrasAtaque;
        int muestrasColaNuevas = static_cast<int>(static_cast<float>(muestrasColaOriginal) * factorDecay);
        int muestrasTotalesNuevas = muestrasAtaque + muestrasColaNuevas;

        irResultado.setSize (numCanales, muestrasTotalesNuevas, false, true, true);

        for (int canal = 0; canal < numCanales; ++canal)
            irResultado.copyFrom (canal, 0, irOriginal, canal, 0, muestrasAtaque);

        for (int i = 0; i < muestrasColaNuevas; ++i)
        {
            double posicionOriginalEnCola = (static_cast<double>(i) / (muestrasColaNuevas - 1)) * (muestrasColaOriginal - 1);
            int idxAbajo = static_cast<int>(std::floor (posicionOriginalEnCola));
            int idxArriba = std::min (idxAbajo + 1, muestrasColaOriginal -1);
            float t_interp = static_cast<float>(posicionOriginalEnCola - idxAbajo);

            for (int canal = 0; canal < numCanales; ++canal)
            {
                float muestraAbajo = irOriginal.getSample(canal, muestrasAtaque + idxAbajo);
                float muestraArriba = irOriginal.getSample(canal, muestrasAtaque + idxArriba);

                float muestraEstirada = muestraAbajo * (1.0f - t_interp) + muestraArriba * t_interp;

                int muestrasFade = static_cast<int>(0.040 * fsIR);
                if (i < muestrasFade && muestrasFade < muestrasColaNuevas)
                {
                    float rampa = static_cast<float>(i) / static_cast<float>(muestrasFade);
                    muestraEstirada *= rampa;
                }

                irResultado.setSample (canal, muestrasAtaque + i, muestraEstirada);
            }
        }
    }

    if (irResultado.getNumSamples() > 0 && fsIR > 0.0)
    {
        float rmsTotalIR = irResultado.getRMSLevel (0, 0, irResultado.getNumSamples());

        if (rmsTotalIR > 0.0001f)
        {
            float duracionResultadoSeg = static_cast<float>(irResultado.getNumSamples()) / static_cast<float>(fsIR);
            float compensacionPorDuracion = std::sqrt (duracionResultadoSeg);

            factorCompensacionIR = (0.10f / rmsTotalIR) * compensacionPorDuracion;
            factorCompensacionIR = juce::jlimit(0.1f, 8.0f, factorCompensacionIR);
        }
        else
            factorCompensacionIR = 1.0f; 
    }
    else
        factorCompensacionIR = 1.0f;
    
    return irResultado;
}

void Reverb402AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numMuestras = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numMuestras);

    if (totalNumInputChannels == 0 || numMuestras == 0)
        return;

    float valorMix = paramMix->load();
    float valorPreDelay = paramPreDelay->load();
    float valorHPF = paramHPF->load();
    float valorLPF = paramLPF->load();
    float valorDecay = paramDecay->load();
    int irElegida = static_cast<int>(paramIRSelection->load());

    if (irElegida != ultimaIrCargada)
    {
        ultimaIrCargada = irElegida;
        juce::File archivoElegido = obtenerArchivoIRFijo (irElegida);

        if (archivoElegido.existsAsFile())
        {
            cargarArchivoIR (archivoElegido);
            ultimoFactorDecay = -1.0f;
        }
    }

    if (valorDecay != ultimoFactorDecay && irOriginal.getNumSamples() > 0)
    {
        ultimoFactorDecay = valorDecay;

        hiloDeFondo.addJob ([this, valorDecay]()
        {
            juce::AudioBuffer<float> irTemporal = modificarDecayIR (irOriginal, valorDecay, fsIR);

            const juce::ScopedLock sl (cerrojoIR);
            irModificadaEnFondo.makeCopyOf (irTemporal);
            fsIRModificada = fsIR;
            hayNuevaIRLista.store (true);
        });
    }

    if (hayNuevaIRLista.load())
    {
        const juce::ScopedLock sl (cerrojoIR);

        if (irModificadaEnFondo.getNumSamples() > 0)
            motorConvolucion.loadImpulseResponse(std::move (irModificadaEnFondo), fsIR, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
    }
    hayNuevaIRLista.store (false);

    float muestrasPreDelay = (valorPreDelay / 1000.0f) * static_cast<float>(spec.sampleRate);
    lineaPreDelay.setDelay (muestrasPreDelay);

    if (valorHPF <= 20.0f)
        filtrosCorte.setBypassed<0> (true);
    else
    {
        filtrosCorte.setBypassed<0> (false);
        *filtrosCorte.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (spec.sampleRate, valorHPF);
    }

    if (valorLPF >= 20000.0f)
        filtrosCorte.setBypassed<1> (true);
    else
    {
        filtrosCorte.setBypassed<1> (false);
        *filtrosCorte.get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (spec.sampleRate, valorLPF);
    }

    juce::AudioBuffer<float> bufferWet;
    bufferWet.setSize (2, numMuestras, false, true, true);

    for (int canal = 0; canal < 2; ++canal)
    {
        int canalOrigen = (canal < totalNumInputChannels) ? canal : 0;
        bufferWet.copyFrom (canal, 0, buffer, canalOrigen, 0, numMuestras);
    }

    juce::dsp::AudioBlock<float> bloqueWet (bufferWet);
    juce::dsp::ProcessContextReplacing<float> contextoWet (bloqueWet);

    lineaPreDelay.process (contextoWet);

    {
        const juce::ScopedLock sl (cerrojoIR);
        if (motorConvolucion.getCurrentIRSize() > 0)
            motorConvolucion.process(contextoWet);
    }
    
    filtrosCorte.process (contextoWet);
    
    float radianesConstantes = valorMix * (juce::MathConstants<float>::halfPi);
    float gananciaDry = std::cos (radianesConstantes);
    float gananciaWet = std::sin (radianesConstantes);

    float compensacionInteligente = factorCompensacionIR; 

    for (int canal = 0; canal < totalNumOutputChannels; ++canal)
    {
        auto* datosSalida = buffer.getWritePointer (canal);
        
        int canalLecturaDry = (canal < totalNumInputChannels) ? canal : 0;
        auto* datosDry = buffer.getReadPointer (canalLecturaDry);
        
        auto* datosWet = bufferWet.getReadPointer (canal);

        for (int muestra = 0; muestra < numMuestras; ++muestra)
        {
            float senalWetNormalizada = datosWet[muestra] * compensacionInteligente;
            
            float senalFinal = (datosDry[muestra] * gananciaDry) + (senalWetNormalizada * gananciaWet);
            
            datosSalida[muestra] = juce::jlimit (-1.0f, 1.0f, senalFinal);
        }
    }
}

void Reverb402AudioProcessor::releaseResources()
{
    motorConvolucion.reset();
    filtrosCorte.reset();
    lineaPreDelay.reset();
}

int Reverb402AudioProcessor::getNumPrograms()
{
    return static_cast<int> (listaCompletaPresets.size());
}

int Reverb402AudioProcessor::getCurrentProgram()
{
    return programaActual;
}

void Reverb402AudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    
    programaActual = index;
    const auto& preset = listaCompletaPresets[index];

    if (auto* pMix = listaParametros.getParameter("mix"))
        pMix->setValueNotifyingHost (preset.mix);
    if (auto* pDecay = listaParametros.getParameter ("decay"))
        pDecay->setValueNotifyingHost (listaParametros.getParameterRange ("decay").convertTo0to1 (preset.decay));
    if (auto* pPre = listaParametros.getParameter ("predelay"))
        pPre->setValueNotifyingHost (listaParametros.getParameterRange ("predelay").convertTo0to1 (preset.preDelay));
    if (auto* pHPF = listaParametros.getParameter ("hpf"))
        pHPF->setValueNotifyingHost (listaParametros.getParameterRange ("hpf").convertTo0to1 (preset.hpf));
    if (auto* pLPF = listaParametros.getParameter ("lpf"))
        pLPF->setValueNotifyingHost (listaParametros.getParameterRange ("lpf").convertTo0to1 (preset.lpf));
    if (auto* pIR = listaParametros.getParameter ("ir_select"))
        pIR->setValueNotifyingHost (listaParametros.getParameterRange ("ir_select").convertTo0to1 (static_cast<float>(preset.irSelect)));

    updateHostDisplay();
}

const juce::String Reverb402AudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < getNumPrograms())
        return listaCompletaPresets[static_cast<size_t> (index)].nombre;
        
    return "Default";
}

void Reverb402AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

juce::File Reverb402AudioProcessor::obtenerCarpetaPresetsUsuario()
{
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    auto carpetaPlugin = appData.getChildFile ("UNTREF").getChildFile ("Reverb402");
    carpetaPlugin.createDirectory();
    return carpetaPlugin;
}

void Reverb402AudioProcessor::actualizarListaPresets()
{
    listaCompletaPresets.clear();

    for (const auto& p : presetsDeFabrica)
        listaCompletaPresets.push_back (p);
    
    auto carpetaUser = obtenerCarpetaPresetsUsuario();
    juce::Array<juce::File> archivosXML;
    carpetaUser.findChildFiles (archivosXML, juce::File::findFiles, false, "*.xml");

    for (auto& archivo : archivosXML)
    {
        std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (archivo));
        if (xml != nullptr && xml->hasTagName (listaParametros.state.getType()))
        {
            Preset pUser;

            pUser.nombre = archivo.getFileNameWithoutExtension();
            pUser.esDeUsuario = true;
            pUser.archivoOrigen = archivo;
            pUser.mix = xml->getDoubleAttribute ("mix", 0.5);
            pUser.decay = xml->getDoubleAttribute ("decay", 1.0);
            pUser.preDelay = xml->getDoubleAttribute ("predelay", 0.0);
            pUser.hpf = xml->getDoubleAttribute ("hpf", 20.0);
            pUser.lpf = xml->getDoubleAttribute ("lpf", 20000.0);
            pUser.irSelect = xml->getIntAttribute ("ir_select", 0);

            listaCompletaPresets.push_back (pUser);
        }
    }
}

void Reverb402AudioProcessor::guardarPresetRapido(const juce::String& nombrePreset)
{
    if (nombrePreset.isEmpty()) return;

    auto carpetaUser = obtenerCarpetaPresetsUsuario();
    auto archivoDestino = carpetaUser.getChildFile (nombrePreset).withFileExtension (".xml");

    std::unique_ptr<juce::XmlElement> xml (new juce::XmlElement (listaParametros.state.getType()));

    xml->setAttribute ("mix", paramMix->load());
    xml->setAttribute ("decay", paramDecay->load());
    xml->setAttribute ("predelay", paramPreDelay->load());
    xml->setAttribute ("hpf", paramHPF->load());
    xml->setAttribute ("lpf", paramLPF->load());
    xml->setAttribute ("ir_select", static_cast<int>(paramIRSelection->load()));

    xml->writeTo (archivoDestino);

    actualizarListaPresets();
}

void Reverb402AudioProcessor::eliminarPresetActual (int index)
{
    if (index < 0 || index >= static_cast<int> (listaCompletaPresets.size()))
        return;

    const auto& preset = listaCompletaPresets[static_cast<size_t> (index)];

    if (preset.esDeUsuario && preset.archivoOrigen.existsAsFile())
    {
        preset.archivoOrigen.deleteFile();
        
        actualizarListaPresets();
    }
}

bool Reverb402AudioProcessor::esPresetDeUsuario (int index)
{
    if (index >= 0 && index < static_cast<int> (listaCompletaPresets.size()))
        return listaCompletaPresets[static_cast<size_t> (index)].esDeUsuario;
        
    return false;
}

juce::AudioProcessorEditor* Reverb402AudioProcessor::createEditor()
{
    auto* editorGenerico = new juce::GenericAudioProcessorEditor (*this);
    
    class EditorConPresets : public juce::AudioProcessorEditor
    {
    public:
        EditorConPresets (Reverb402AudioProcessor& p, juce::AudioProcessorEditor* contenido)
            : AudioProcessorEditor (p), processor (p), editorInterno (contenido)
        {
            addAndMakeVisible (editorInterno.get());
            
            // Función auxiliar interna para actualizar el estado del botón Borrar
            auto chequearSiEsBorrable = [this] {
                // Solo es visible si el preset actual es del usuario
                btnBorrar.setVisible (processor.esPresetDeUsuario (idxLocal));
            };
            
            // Botón Anterior
            btnAnterior.setButtonText ("<- Preset");
            addAndMakeVisible (btnAnterior);
            btnAnterior.onClick = [this, chequearSiEsBorrable] {
                int totalPrograms = processor.getNumPrograms();
                if (totalPrograms > 0)
                {
                    idxLocal = (idxLocal - 1 + totalPrograms) % totalPrograms;
                    processor.setCurrentProgram (idxLocal);
                    lblNombre.setText (processor.getProgramName (idxLocal), juce::dontSendNotification);
                    chequearSiEsBorrable();
                }
            };
            
            // Botón Siguiente
            btnSiguiente.setButtonText ("Preset ->");
            addAndMakeVisible (btnSiguiente);
            btnSiguiente.onClick = [this, chequearSiEsBorrable] {
                int totalPrograms = processor.getNumPrograms();
                if (totalPrograms > 0)
                {
                    idxLocal = (idxLocal + 1) % totalPrograms;
                    processor.setCurrentProgram (idxLocal);
                    lblNombre.setText (processor.getProgramName (idxLocal), juce::dontSendNotification);
                    chequearSiEsBorrable();
                }
            };

            // Botón Guardar
            btnGuardar.setButtonText ("Guardar Nuevo");
            addAndMakeVisible (btnGuardar);
            btnGuardar.onClick = [this, chequearSiEsBorrable] {
                auto* aw = new juce::AlertWindow ("Guardar Preset",
                                                  "Introduce el nombre para tu preset de usuario:",
                                                  juce::MessageBoxIconType::NoIcon,
                                                  this);

                aw->addTextEditor ("nombrePreset", "Mi Preset Custom", "");
                aw->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey, 0, 0));
                aw->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey, 0, 0));

                aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw, chequearSiEsBorrable] (int resultado)
                {
                    if (resultado == 1)
                    {
                        if (auto* editorTexto = aw->getTextEditor ("nombrePreset"))
                        {
                            juce::String texto = editorTexto->getText();
                            if (texto.isNotEmpty())
                            {
                                processor.guardarPresetRapido (texto);
                                idxLocal = processor.getNumPrograms() - 1;
                                processor.setCurrentProgram (idxLocal);
                                lblNombre.setText (processor.getProgramName (idxLocal), juce::dontSendNotification);
                                chequearSiEsBorrable();
                            }
                        }
                    }
                    delete aw;
                }), true);
            };

            // Botón Borrar (Mapeado con estética roja de advertencia)
            btnBorrar.setButtonText ("Borrar");
            btnBorrar.setColour (juce::TextButton::buttonColourId, juce::Colours::darkred);
            addAndMakeVisible (btnBorrar);
            btnBorrar.onClick = [this, chequearSiEsBorrable] {
                // Cartel de confirmación para que no metan el dedo sin querer
                juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::WarningIcon,
                    "¿Borrar Preset?",
                    "¿Estás seguro de que querés eliminar \"" + processor.getProgramName(idxLocal) + "\"? Esta acción no se puede deshacer.",
                    "Sí, Borrar", "Cancelar", this,
                    juce::ModalCallbackFunction::create([this, chequearSiEsBorrable](int resultado) 
                    {
                        if (resultado == 1) // El usuario confirmó
                        {
                            processor.eliminarPresetActual (idxLocal);
                            
                            // Mandamos al usuario de vuelta al preset 0 (Default) de forma segura
                            idxLocal = 0;
                            processor.setCurrentProgram (idxLocal);
                            lblNombre.setText (processor.getProgramName (idxLocal), juce::dontSendNotification);
                            chequearSiEsBorrable();
                        }
                    }));
            };
            
            // Etiqueta de Nombre
            lblNombre.setJustificationType (juce::Justification::centred);
            lblNombre.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (0.5f));
            lblNombre.setColour (juce::Label::textColourId, juce::Colours::white);
            addAndMakeVisible (lblNombre);
            
            // Inicialización de la UI
            idxLocal = processor.getCurrentProgram();
            lblNombre.setText (processor.getProgramName (idxLocal), juce::dontSendNotification);
            chequearSiEsBorrable(); // Oculta el botón borrar al inicio si arranca en uno de fábrica
            
            setSize (editorInterno->getWidth(), editorInterno->getHeight() + 40);
        }
        
        void resized() override
        {
            auto zona = getLocalBounds();
            auto barraSuperior = zona.removeFromTop (40).reduced (5);
            
            btnAnterior.setBounds (barraSuperior.removeFromLeft (80));
            
            // Acomodamos los botones derechos en cascada desde la derecha
            btnGuardar.setBounds (barraSuperior.removeFromRight (110));
            btnBorrar.setBounds (barraSuperior.removeFromRight (70)); // <-- Espacio para el botón de borrar
            btnSiguiente.setBounds (barraSuperior.removeFromRight (80));
            
            lblNombre.setBounds (barraSuperior.reduced (5, 0));
            
            editorInterno->setBounds (zona);
            lblNombre.toFront (false);
        }
        
    private:
        Reverb402AudioProcessor& processor;
        std::unique_ptr<juce::AudioProcessorEditor> editorInterno;
        juce::TextButton btnAnterior, btnSiguiente, btnGuardar, btnBorrar;
        juce::Label lblNombre;
        int idxLocal = 0;
    };
    
    return new EditorConPresets (*this, editorGenerico);
}

void Reverb402AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto estado = listaParametros.copyState();
    std::unique_ptr<juce::XmlElement> xml (estado.createXml());
    copyXmlToBinary(*xml, destData);
}

void Reverb402AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlEstado (getXmlFromBinary (data, sizeInBytes));

    if (xmlEstado != nullptr)
    {
        if (xmlEstado->hasTagName (listaParametros.state.getType()))
            listaParametros.replaceState (juce::ValueTree::fromXml (*xmlEstado));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Reverb402AudioProcessor();
}