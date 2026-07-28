#include "PluginProcessor.h"
#include "PluginEditor.h"

Reverb402AudioProcessor::Reverb402AudioProcessor()
    : AudioProcessor (BusesProperties()
                    .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    listaParametros (*this, nullptr, "PARAMETROS", crearLayoutParametros())
{

    empaquetadorFormatos.registerBasicFormats();

    paramMix = listaParametros.getRawParameterValue ("mix");
    paramDecay = listaParametros.getRawParameterValue ("decay");
    paramHPF = listaParametros.getRawParameterValue ("hpf");
    paramLPF = listaParametros.getRawParameterValue ("lpf");
    paramPreDelay = listaParametros.getRawParameterValue ("predelay");
    paramIRSelection = listaParametros.getRawParameterValue ("ir_select");
    paramInputGain = listaParametros.getRawParameterValue ("inputGain");
    paramDuck = listaParametros.getRawParameterValue ("duck");
    paramAir = listaParametros.getRawParameterValue ("air");
    paramWarm = listaParametros.getRawParameterValue ("warm");

    actualizarListaPresets();

    estadoA = listaParametros.copyState();
    estadoB = listaParametros.copyState();
}

Reverb402AudioProcessor::~Reverb402AudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout Reverb402AudioProcessor::crearLayoutParametros()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("mix", 1),
        "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([](float val, int) { return juce::String (juce::roundToInt (val * 100.0f)) + " %"; })
    ));
    
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("decay", 1),
        "Decay",
        juce::NormalisableRange<float> (0.1f, 5.0f, 0.1f),
        1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([](float val, int) { return juce::String (val, 1) + "x"; })
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("predelay", 1),
        "Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 150.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([](float val, int) { return juce::String (juce::roundToInt (val)) + " ms"; })
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
    20000.0f,
    juce::AudioParameterFloatAttributes().withStringFromValueFunction ([](float val, int)
        {
            if (val >= 1000.0f)
                return juce::String (val / 1000.0f, 1) + " kHz";
            return juce::String (juce::roundToInt (val)) + " Hz";
        })
    ));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "ir_select",
        "Respuesta al Impulso",
        nombresIRs,
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID {"btnAB", 1},
        "Botón A/B",
        false
    ));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [](float value, int) { return juce::String (value, 1) + " dB"; }
        )
    ));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "duck", 1 },
        "Ducked Reverb",
        false
    ));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "air", 1 },
        "Air",
        false
    ));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "warm", 1 },
        "Warm",
        false
    ));
    
    return layout;
}

void Reverb402AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    rmsInL.reset (sampleRate, 0.5);
    rmsInR.reset (sampleRate, 0.5);
    rmsOutL.reset (sampleRate, 0.5);
    rmsOutR.reset (sampleRate, 0.5);

    rmsInL.setCurrentAndTargetValue (-100.0f);
    rmsInR.setCurrentAndTargetValue (-100.0f);
    rmsOutL.setCurrentAndTargetValue (-100.0f);
    rmsOutR.setCurrentAndTargetValue (-100.0f);

    lineaPreDelay.prepare (spec);
    lineaPreDelay.setMaximumDelayInSamples (static_cast<int>(sampleRate));

    for (int canal = 0; canal < 2; ++canal)  
    {
        filtrosCorte[canal].prepare (spec);
        *filtrosCorte[canal].get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 20.0f);
        *filtrosCorte[canal].get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 20000.0f);
    } 

    duckDetector.prepare (spec);
    duckDetector.setLevelCalculationType (juce::dsp::BallisticsFilterLevelCalculationType::RMS);
    duckDetector.setAttackTime (10.0f);
    duckDetector.setReleaseTime (200.0f);
    duckDetector.reset();

    filtroAirShelf.prepare (spec);
    *filtroAirShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain (5.5f));

    filtroAirHPF.prepare (spec);
    *filtroAirHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 6000.0f, 0.707f);

    filtroWarmShelf.prepare (spec);
    *filtroWarmShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, 350.0f, 0.707f, juce::Decibels::decibelsToGain (2.5f));
    
    filtroWarmLPF.prepare (spec);
    *filtroWarmLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 550.0f, 0.707f);

    juce::dsp::ProcessSpec specConvolucion;
    specConvolucion.sampleRate = sampleRate;
    specConvolucion.numChannels = spec.numChannels;
    specConvolucion.maximumBlockSize = samplesPerBlock;

    motorConvolucionHeadA.prepare (specConvolucion);
    motorConvolucionHeadB.prepare (specConvolucion);
    motorConvolucionTailA.prepare (specConvolucion);
    motorConvolucionTailB.prepare (specConvolucion);
    lineaCompensacionHead.prepare (specConvolucion);
    lineaCompensacionDry.prepare (specConvolucion);

    bufferTail.setSize (2, samplesPerBlock, false, true, true);
    bufferWet.setSize (2, samplesPerBlock, false, true, true);

    latenciaCompensacionMuestras = motorConvolucionTailA.getLatency() - motorConvolucionHeadA.getLatency();
    lineaCompensacionHead.setDelay (static_cast<float>(juce::jmax (0, latenciaCompensacionMuestras)));

    setLatencySamples (motorConvolucionTailA.getLatency());

    lineaCompensacionDry.setDelay (static_cast<float>(getLatencySamples()));
    bufferDryCompensado.setSize (2, samplesPerBlock, false, true, true);

    bufferAgudosAir.setSize (2, samplesPerBlock, false, true, true);

    bufferGananciaDuck.assign (static_cast<size_t>(samplesPerBlock), 1.0f);
}

juce::File Reverb402AudioProcessor::obtenerArchivoIRFijo (int indice)
{
    auto appData = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory);

    juce::File carpetaIR = appData.getChildFile("UNTREF").getChildFile("Reverb402").getChildFile("IR");

    if (! carpetaIR.isDirectory())
    {
        carpetaIR.createDirectory();
    }

    switch (indice)
    {
        case 0: return carpetaIR.getChildFile ("1_Alumno_izquierda.wav");
        case 1: return carpetaIR.getChildFile ("2_Alumno_derecha.wav");
        case 2: return carpetaIR.getChildFile ("3_Alumno_wide.wav");
        case 3: return carpetaIR.getChildFile ("4_Alumno_wide_(prealigned).wav");
        case 4: return carpetaIR.getChildFile ("5_Profesor_izquierda.wav");
        case 5: return carpetaIR.getChildFile ("6_Profesor_derecha.wav");
        case 6: return carpetaIR.getChildFile ("7_Profesor_wide.wav");
        case 7: return carpetaIR.getChildFile ("8_Profesor_wide_(prealigned).wav");
        default: return carpetaIR.getChildFile ("1_Alumno_izquierda.wav");
    }
}

void Reverb402AudioProcessor::dividirIREnHeadYTail (const juce::AudioBuffer<float>& irCompleta, double fs, juce::AudioBuffer<float>& outHead, juce::AudioBuffer<float>& outTail)
{
    const int numCanales = irCompleta.getNumChannels();
    const int totalMuestras = irCompleta.getNumSamples();

    int muestrasHead = static_cast<int>((duracionHeadMs/1000.0) * fs);
    muestrasHead = juce::jmin (muestrasHead, totalMuestras);
    int muestrasTail = totalMuestras - muestrasHead;

    outHead.setSize (numCanales, muestrasHead, false, true, true);
    for (int canal = 0; canal < numCanales; ++canal)
        outHead.copyFrom (canal, 0, irCompleta, canal, 0, muestrasHead);
    
    if (muestrasTail > 0)
    {
        outTail.setSize (numCanales, muestrasTail, false, true, true);
        for (int canal = 0; canal < numCanales; ++canal)
            outTail.copyFrom (canal, 0, irCompleta, canal, muestrasHead, muestrasTail);
    }
    else
        outTail.setSize (numCanales, 0, false, true, true);
}

float Reverb402AudioProcessor::estimarPisoDeRuidoDb (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return -60.0f;

    const int muestrasPorVentana = static_cast<int>(0.05 * sampleRate);
    if (muestrasPorVentana <= 0 || numSamples < muestrasPorVentana)
        return -60.0f;

    int inicioAnalisis = static_cast<int>(numSamples * 0.7);
    int muestrasAAnalizar = numSamples - inicioAnalisis;

    std::vector<float> rmsPorVentanaDb;

    for (int pos = inicioAnalisis; pos + muestrasPorVentana <= numSamples; pos += muestrasPorVentana)
    {
        float rms = buffer.getRMSLevel (0, pos, muestrasPorVentana);
        float db = 20.0f * std::log10 (rms + 1e-9f);
        rmsPorVentanaDb.push_back (db);
    }

    if (rmsPorVentanaDb.empty())
        return -60.0f;

    std::sort (rmsPorVentanaDb.begin(), rmsPorVentanaDb.end());
    float medianaDb = rmsPorVentanaDb[rmsPorVentanaDb.size() / 2];

    return medianaDb;
}

juce::AudioBuffer<float> Reverb402AudioProcessor::cargarArchivoIRSeguro (const juce::File& archivoAudio, double& fsSalida, float& compensacionSalida)
{
    juce::AudioBuffer<float> irResultado;
    std::unique_ptr<juce::AudioFormatReader> lector (empaquetadorFormatos.createReaderFor (archivoAudio));

    if (lector != nullptr)
    {
        fsSalida = lector->sampleRate;
        const int canalesArchivo = static_cast<int>(lector->numChannels);
        const int muestrasArchivo = static_cast<int>(lector->lengthInSamples);

        juce::AudioBuffer<float> bufferTemporalRaw (canalesArchivo, muestrasArchivo);
        lector->read (bufferTemporalRaw.getArrayOfWritePointers(), canalesArchivo, 0, muestrasArchivo);

        if (muestrasArchivo > 0 && fsSalida > 0.0)
        {
            const float* datosCanalAnalisis = bufferTemporalRaw.getReadPointer (0);

            int indPico = 0;
            float valorMaxAbs = 0.0f;
            for (int i = 0; i < muestrasArchivo; ++i)
            {
                float absVal = std::abs (datosCanalAnalisis[i]);
                if (absVal > valorMaxAbs)
                {
                    valorMaxAbs = absVal;
                    indPico = i;
                }
            }

            int muestrasRecortadasAlInicio = muestrasArchivo - indPico;
            if (muestrasRecortadasAlInicio > 0)
            {
                float pisoRuidoDb = estimarPisoDeRuidoDb(bufferTemporalRaw, fsSalida);
                pisoRuidoActual = juce::jlimit(-66.0f, -26.0f, pisoRuidoDb);

                float margenSeguridadDb = 5.0f;
                float dbCorteDeseado = pisoRuidoDb + margenSeguridadDb;
                float amplitudCorteAbsoluta = std::pow (10.0f, dbCorteDeseado / 20.0f);
                float factorUmbral = amplitudCorteAbsoluta / valorMaxAbs;
                factorUmbral = juce::jlimit (0.0005f, 0.05f, factorUmbral);
                float umbral = factorUmbral * valorMaxAbs;

                int ultimoIndiceSobreUmbral = -1;
                
                for (int i = 0; i < muestrasRecortadasAlInicio; ++i)
                {
                    float absVal = std::abs (datosCanalAnalisis[indPico + i]);
                    if (absVal > umbral)
                        ultimoIndiceSobreUmbral = i;
                }

                int muestrasFinalesARecortar = muestrasRecortadasAlInicio;

                if (ultimoIndiceSobreUmbral != -1)
                {
                    int margen = static_cast<int>(0.2 * fsSalida);
                    int corte = std::min (ultimoIndiceSobreUmbral + margen, muestrasRecortadasAlInicio);
                    muestrasFinalesARecortar = corte;
                }

                irResultado.setSize (canalesArchivo, muestrasFinalesARecortar, false, true, true);

                for (int canal = 0; canal < canalesArchivo; ++canal)
                    irResultado.copyFrom (canal, 0, bufferTemporalRaw, canal, indPico, muestrasFinalesARecortar);
            }
            else
                irResultado.makeCopyOf (bufferTemporalRaw);

            const int muestrasFinales = irResultado.getNumSamples();
            float rmsTotalIR = irResultado.getRMSLevel (0, 0, muestrasFinales);

            if (rmsTotalIR > 0.0001f)
            {
                float duracionSeg = static_cast<float>(muestrasFinales) / static_cast<float>(fsSalida);
                float compensacionPorDuracion = std::sqrt (duracionSeg);

                compensacionSalida = (0.10f / rmsTotalIR) * compensacionPorDuracion;
                compensacionSalida = juce::jlimit (0.1f, 8.0f, compensacionSalida);
            }
            else
                compensacionSalida = 1.0f;
        }
    }
    return irResultado;
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

    auto inputGainDb = paramInputGain->load();
    auto inputGainLineal = juce::Decibels::decibelsToGain (inputGainDb);

    buffer.applyGain (inputGainLineal);

    bool duck = paramDuck->load() > 0.5f;

    if ((int) bufferGananciaDuck.size() < numMuestras)
        bufferGananciaDuck.resize (static_cast<size_t>(numMuestras));

    {
        const float umbralLineal = juce::Decibels::decibelsToGain (-36.0f);
        const float maxDuckDb = -32.0f;

        for (int m = 0; m < numMuestras; ++m)
        {
            float instantMax = std::abs (buffer.getSample (0, m));
            if (totalNumInputChannels > 1)
                instantMax = juce::jmax (instantMax, std::abs (buffer.getSample (1, m)));

            float gananciaObjetivo = 1.0f;

            if (duck)
            {
                float nivelEntrada = duckDetector.processSample (0, instantMax);

                if (nivelEntrada > umbralLineal)
                {
                    float exceso = juce::jlimit (0.0f, 1.0f, (nivelEntrada - umbralLineal) / (1.0f - umbralLineal));
                    exceso = std::pow (exceso, 0.7f);

                    float atenuacionDb = exceso * maxDuckDb;
                    gananciaObjetivo = juce::Decibels::decibelsToGain (atenuacionDb);
                }
            }

            bufferGananciaDuck[static_cast<size_t>(m)] = gananciaObjetivo;
        }
    }

    rmsInL.skip (numMuestras);
    rmsInR.skip (numMuestras);

    {
        const auto valor = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, numMuestras));
        if (valor < rmsInL.getCurrentValue())
            rmsInL.setTargetValue (valor);
        else
            rmsInL.setCurrentAndTargetValue (valor);
    }
    
    {
        const auto valor = juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, numMuestras));
        if (valor < rmsInR.getCurrentValue())
            rmsInR.setTargetValue (valor);
        else
            rmsInR.setCurrentAndTargetValue (valor);
    }

    float valorMix = paramMix->load();
    float valorPreDelay = paramPreDelay->load();
    float valorHPF = paramHPF->load();
    float valorLPF = paramLPF->load();
    float valorDecay = paramDecay->load();
    int irElegida = static_cast<int>(paramIRSelection->load());
    bool airActivo = paramAir->load() > 0.5f;
    bool warmActivo = paramWarm->load() > 0.5f; 

    if (irElegida != ultimaIrCargada)
    {
        ultimaIrCargada = irElegida;
        enTransicion = true;
        debeCargarNuevoArchivo.store (true);
        ultimoFactorDecay = -1.0f;
    }

    if (airActivo != ultimoEstadoAir)
    {
        ultimoEstadoAir = airActivo;
        enTransicion = true;
        ultimoFactorDecay = -1.0f;
    }

    if (warmActivo != ultimoEstadoWarm)
    {
        ultimoEstadoWarm = warmActivo;
        enTransicion = true;
        ultimoFactorDecay = -1.0f;
    }

    if (debeCargarNuevoArchivo.load() || (valorDecay != ultimoFactorDecay && irOriginal.getNumSamples() > 0))
    {
        bool cargarNuevo = debeCargarNuevoArchivo.exchange (false);
        ultimoFactorDecay = valorDecay;
        enTransicion = true;

        hiloDeFondo.addJob ([this, irElegida, valorDecay, cargarNuevo, airActivo, warmActivo]()
        {
            if (cargarNuevo)
            {
                juce::File archivoElegido = obtenerArchivoIRFijo (irElegida);
                if (archivoElegido.existsAsFile())
                {
                    irOriginal = cargarArchivoIRSeguro (archivoElegido, fsIR, factorCompensacionIR);
                }
            }

            if (irOriginal.getNumSamples() > 0)
            {
                juce::AudioBuffer<float> irTemporal = modificarDecayIR (irOriginal, valorDecay, fsIR);

                if (airActivo)
                {
                    juce::dsp::AudioBlock<float> bloqueIR (irTemporal);
                    juce::dsp::ProcessContextReplacing<float> contextoIR (bloqueIR);
                    filtroAirShelf.process (contextoIR);
                }

                if (warmActivo)
                {
                    juce::dsp::AudioBlock<float> bloqueIR (irTemporal);
                    juce::dsp::ProcessContextReplacing<float> contextoIR (bloqueIR);
                    filtroWarmShelf.process (contextoIR);
                }

                juce::AudioBuffer<float> head, tail;
                dividirIREnHeadYTail (irTemporal, fsIR, head, tail);

                if (head.getNumSamples() > 0)
                    motorHeadInactivo().loadImpulseResponse (std::move (head), fsIR, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);

                if (tail.getNumSamples() > 0)
                    motorTailInactivo().loadImpulseResponse (std::move (tail), fsIR, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);

                const juce::ScopedLock sl (cerrojoIR);
                irCompletaModificada.makeCopyOf (irTemporal);
                fsIRModificada = fsIR;
                hayNuevaIRLista.store (true);
            }
        });
    }

    if (hayNuevaIRLista.load() && gananciaTransicion < 0.01f)
    {
        usarMotorB.store (! usarMotorB.load());
        hayNuevaIRLista.store (false);
        enTransicion = false;
    }
    
    float gananciaObjetivo = enTransicion ? 0.0f : 1.0f;

    float muestrasPreDelay = (valorPreDelay / 1000.0f) * static_cast<float>(spec.sampleRate);
    lineaPreDelay.setDelay (muestrasPreDelay);

    for (int canal = 0; canal < 2; ++canal)
    {
        if (valorHPF <= 20.0f)
            filtrosCorte[canal].setBypassed<0> (true);
        else
        {
            filtrosCorte[canal].setBypassed<0> (false);
            *filtrosCorte[canal].get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (spec.sampleRate, valorHPF);
        }

        if (valorLPF >= 20000.0f)
            filtrosCorte[canal].setBypassed<1> (true);
        else
        {
            filtrosCorte[canal].setBypassed<1> (false);
            *filtrosCorte[canal].get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (spec.sampleRate, valorLPF);
        }
    }

    bufferWet.setSize (2, numMuestras, false, true, true);

    for (int canal = 0; canal < 2; ++canal)
    {
        int canalOrigen = (canal < totalNumInputChannels) ? canal : 0;
        bufferWet.copyFrom (canal, 0, buffer, canalOrigen, 0, numMuestras);
    }

    juce::dsp::AudioBlock<float> bloqueWet (bufferWet);
    juce::dsp::ProcessContextReplacing<float> contextoWet (bloqueWet);

    lineaPreDelay.process (contextoWet);

    bufferTail.copyFrom (0, 0, bufferWet, 0, 0, numMuestras);
    bufferTail.copyFrom (1, 0, bufferWet, 1, 0, numMuestras);

    juce::dsp::AudioBlock<float> bloqueTail (bufferTail);
    juce::dsp::ProcessContextReplacing<float> contextoTail (bloqueTail);

    auto& motorHead = motorHeadActivo();
    auto& motorTail = motorTailActivo();

    if (motorHead.getCurrentIRSize() > 0)
        motorHead.process (contextoWet);

    if (motorTail.getCurrentIRSize() > 0)
        motorTail.process (contextoTail);

    lineaCompensacionHead.process (contextoWet);

    if (motorTail.getCurrentIRSize() > 0)
    {
        bufferWet.addFrom (0, 0, bufferTail, 0, 0, numMuestras);
        bufferWet.addFrom (1, 0, bufferTail, 1, 0, numMuestras);
    }   
    
    if (airActivo)
    {
        for (int canal = 0; canal < 2; ++canal)
            bufferAgudosAir.copyFrom (canal, 0, bufferWet, canal, 0, numMuestras);

        juce::dsp::AudioBlock<float> bloqueAgudos (bufferAgudosAir);
        juce::dsp::ProcessContextReplacing<float> contextoAgudos (bloqueAgudos);
        filtroAirHPF.process (contextoAgudos);

        const float driveAir = 2.0f;
        const float cantidadArmonicos = 0.15f; 

        for (int canal = 0; canal < 2; ++canal)
        {
            auto* datosWet = bufferWet.getWritePointer (canal);
            auto* datosAgudos = bufferAgudosAir.getReadPointer (canal);

            for (int i = 0; i < numMuestras; ++i)
            {
                float entrada = datosAgudos[i] * driveAir;
                float saturada = std::tanh (entrada);

                datosWet[i] += saturada * cantidadArmonicos;
            }
        }
    }

    if (warmActivo)
    {
        bufferGravesWarm.setSize (2, numMuestras, false, true, true);
        for (int canal = 0; canal < 2; ++canal)
            bufferGravesWarm.copyFrom (canal, 0, bufferWet, canal, 0, numMuestras);

        juce::dsp::AudioBlock<float> bloqueGraves (bufferGravesWarm);
        juce::dsp::ProcessContextReplacing<float> contextoGraves (bloqueGraves);
        filtroWarmLPF.process (contextoGraves);

        const float driveWarm = 1.5f;
        const float cantidadArmonicosWarm = 0.12f;

        for (int canal = 0; canal < 2; ++canal)
        {
            auto* datosWet = bufferWet.getWritePointer (canal);
            auto* datosGraves = bufferGravesWarm.getReadPointer (canal);

            for (int i = 0; i < numMuestras; ++i)
            {
                float x = datosGraves[i] * driveWarm;
                
                float saturadaPares = x + (0.2f * x * x) - (0.15f * x * x * x);

                datosWet[i] += (saturadaPares - x) * cantidadArmonicosWarm;
            }
        }
    }

    for (int canal = 0; canal < 2; ++canal)
    {
        auto bloqueCanalUnico = bloqueWet.getSingleChannelBlock (canal);
        juce::dsp::ProcessContextReplacing<float> contextoCanal (bloqueCanalUnico);
        filtrosCorte[canal].process (contextoCanal);
    }

    bufferDryCompensado.setSize (2, numMuestras, false, true, true);
    for (int canal = 0; canal < 2; ++canal)
    {
        int canalOrigen = (canal < totalNumInputChannels) ? canal : 0;
        bufferDryCompensado.copyFrom (canal, 0, buffer, canalOrigen, 0, numMuestras);
    }

    juce::dsp::AudioBlock<float> bloqueDry (bufferDryCompensado);
    juce::dsp::ProcessContextReplacing<float> contextoDry (bloqueDry);
    lineaCompensacionDry.process (contextoDry);
        
    float radianesConstantes = valorMix * (juce::MathConstants<float>::halfPi);
    float gananciaDry = std::cos (radianesConstantes);
    float gananciaWet = std::sin (radianesConstantes);
    float compensacionInteligente = factorCompensacionIR; 
    float gananciaRampa = gananciaTransicion;

    for (int muestra = 0; muestra < numMuestras; ++muestra)
    {
        gananciaRampa += (gananciaObjetivo - gananciaRampa) * 0.02f;

        duckGainSmoothing += (bufferGananciaDuck[static_cast<size_t>(muestra)] - duckGainSmoothing) * 0.015f;

        for (int canal = 0; canal < totalNumOutputChannels; ++canal)
        {
            auto* datosSalida = buffer.getWritePointer (canal);
            auto* datosDry = bufferDryCompensado.getReadPointer (canal);
            auto* datosWet = bufferWet.getReadPointer (canal);

            float senalWetNormalizada = datosWet[muestra] * compensacionInteligente * gananciaRampa * duckGainSmoothing;
            
            float senalFinal = (datosDry[muestra] * gananciaDry) + (senalWetNormalizada * gananciaWet);
            
            datosSalida[muestra] = juce::jlimit (-1.0f, 1.0f, senalFinal);
        }
        gananciaTransicion = gananciaRampa;
    }

    rmsOutL.skip (numMuestras);
    rmsOutR.skip (numMuestras);

    {
        const auto valor = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, numMuestras));
        if (valor < rmsOutL.getCurrentValue())
            rmsOutL.setTargetValue (valor);
        else
            rmsOutL.setCurrentAndTargetValue (valor);
    }
    
    if (totalNumOutputChannels > 1)
    {
        const auto valor = juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, numMuestras));
        if (valor < rmsOutR.getCurrentValue())
            rmsOutR.setTargetValue (valor);
        else
            rmsOutR.setCurrentAndTargetValue (valor);
    }   
}

void Reverb402AudioProcessor::obtenerCopiaIrActual (juce::AudioBuffer<float>& bufferDestino)
{
    const juce::ScopedLock sl (cerrojoIR);

    if (irCompletaModificada.getNumSamples() > 0)
    {
        bufferDestino.setSize(irCompletaModificada.getNumChannels(), irCompletaModificada.getNumSamples(), false, true, true);
        for (int canal = 0; canal < irCompletaModificada.getNumChannels(); ++canal)
            bufferDestino.copyFrom (canal, 0, irCompletaModificada.getReadPointer (canal), irCompletaModificada.getNumSamples());
        
        double sampleRate = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        float valorHPF = paramHPF->load();
        float valorLPF = paramLPF->load();

        juce::dsp::ProcessSpec specCopia;
        specCopia.sampleRate = sampleRate;
        specCopia.maximumBlockSize = static_cast<juce::uint32>(bufferDestino.getNumSamples());
        specCopia.numChannels = static_cast<juce::uint32>(bufferDestino.getNumChannels());

        for (int canal = 0; canal < bufferDestino.getNumChannels(); ++canal)
        {
            if (valorHPF > 20.0f)
            {
                juce::dsp::IIR::Filter<float> filtroHPF;
                filtroHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, valorHPF);
                filtroHPF.prepare (specCopia);

                juce::dsp::AudioBlock<float> block (bufferDestino);
                juce::dsp::AudioBlock<float> singleChannelBlock = block.getSingleChannelBlock (canal);
                juce::dsp::ProcessContextReplacing<float> context (singleChannelBlock);
                
                filtroHPF.process (context);
            }

            if (valorLPF < 20000.0f)
            {
                juce::dsp::IIR::Filter<float> filtroLPF;
                filtroLPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, valorLPF);
                filtroLPF.prepare (specCopia);

                juce::dsp::AudioBlock<float> block (bufferDestino);
                juce::dsp::AudioBlock<float> singleChannelBlock = block.getSingleChannelBlock (canal);
                juce::dsp::ProcessContextReplacing<float> context (singleChannelBlock);
                
                filtroLPF.process (context);
            }
        }
    }
    else
    {
        bufferDestino.setSize (1, 1);
        bufferDestino.clear();
    }
}

float Reverb402AudioProcessor::obtenerMagnitudFiltros (double frecuencia)
{
    float magnitudTotal = 1.0f;
    double sampleRate = spec.sampleRate;
    
    if (sampleRate <= 0.0)
        return magnitudTotal;
    
    auto& cadenaCanalCero = filtrosCorte[0];

    if (! cadenaCanalCero.isBypassed<0>())
    {
        auto coefsHPF = cadenaCanalCero.get<0>().coefficients;
        if (coefsHPF != nullptr)
            magnitudTotal *= coefsHPF->getMagnitudeForFrequency (frecuencia, sampleRate);
    }

    if (! cadenaCanalCero.isBypassed<1>())
    {
        auto coefsLPF = cadenaCanalCero.get<1>().coefficients;
        if (coefsLPF != nullptr)
            magnitudTotal *= coefsLPF->getMagnitudeForFrequency (frecuencia, sampleRate);
    }

    return magnitudTotal;
}

float Reverb402AudioProcessor::obtenerRMSIn(const int canal) const
{
    if (canal == 0)
        return rmsInL.getCurrentValue();
    else if (canal == 1)
        return rmsInR.getCurrentValue();
    else
        return 0.0f;
}

float Reverb402AudioProcessor::obtenerRMSOut(const int canal) const
{
    if (canal == 0)
        return rmsOutL.getCurrentValue();
    else if (canal == 1)
        return rmsOutR.getCurrentValue();
    else
        return 0.0f;
}

void Reverb402AudioProcessor::releaseResources()
{
    motorConvolucionHeadA.reset();
    motorConvolucionHeadB.reset();
    motorConvolucionTailA.reset();
    motorConvolucionTailB.reset();
    lineaPreDelay.reset();
    duckDetector.reset();

    for (int canal = 0; canal < 2; ++canal)
        filtrosCorte[canal].reset();
}

int Reverb402AudioProcessor::getNumPrograms()
{
    return static_cast<int>(listaCompletaPresets.size());
}

int Reverb402AudioProcessor::getCurrentProgram()
{
    return modoBActivo ? programaActualB : programaActualA;
}

void Reverb402AudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    
    if (modoBActivo)
        programaActualB = index;
    else
        programaActualA = index;
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
    if (auto* pDuck = listaParametros.getParameter ("duck"))
        pDuck->setValueNotifyingHost (preset.duck ? 1.0f : 0.0f);
    if (auto pAir = listaParametros.getParameter ("air"))
        pAir->setValueNotifyingHost (preset.air ? 1.0f : 0.0f);
    if (auto pWarm = listaParametros.getParameter ("warm"))
        pWarm->setValueNotifyingHost (preset.warm ? 1.0f : 0.0f);

    updateHostDisplay();
}

const juce::String Reverb402AudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < getNumPrograms())
        return listaCompletaPresets[static_cast<size_t>(index)].nombre;
        
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
        if (xml != nullptr && xml->hasTagName ("Reverb402Preset"))
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
            pUser.duck = xml->getBoolAttribute ("duck", 0);
            pUser.air = xml->getBoolAttribute ("air", 0);
            pUser.warm = xml->getBoolAttribute ("warm", 0);

            listaCompletaPresets.push_back (pUser);
        }
    }
}

void Reverb402AudioProcessor::guardarPresetRapido(const juce::String& nombrePreset)
{
    if (nombrePreset.isEmpty()) return;

    auto carpetaUser = obtenerCarpetaPresetsUsuario();
    auto archivoDestino = carpetaUser.getChildFile (nombrePreset).withFileExtension (".xml");

    std::unique_ptr<juce::XmlElement> xml (new juce::XmlElement ("Reverb402Preset"));

    xml->setAttribute ("mix", paramMix->load());
    xml->setAttribute ("decay", paramDecay->load());
    xml->setAttribute ("predelay", paramPreDelay->load());
    xml->setAttribute ("hpf", paramHPF->load());
    xml->setAttribute ("lpf", paramLPF->load());
    xml->setAttribute ("ir_select", static_cast<int>(paramIRSelection->load()));
    xml->setAttribute ("duck", paramDuck->load() > 0.5f);
    xml->setAttribute ("air", paramAir->load() > 0.5f);
    xml->setAttribute ("warm", paramWarm->load() > 0.5f);

    xml->writeTo (archivoDestino);

    actualizarListaPresets();
}

void Reverb402AudioProcessor::eliminarPresetActual (int index)
{
    if (index < 0 || index >= static_cast<int>(listaCompletaPresets.size()))
        return;

    const auto& preset = listaCompletaPresets[static_cast<size_t>(index)];

    if (preset.esDeUsuario && preset.archivoOrigen.existsAsFile())
    {
        preset.archivoOrigen.deleteFile();
        
        actualizarListaPresets();
    }
}

bool Reverb402AudioProcessor::esPresetDeUsuario (int index)
{
    if (index >= 0 && index < static_cast<int>(listaCompletaPresets.size()))
        return listaCompletaPresets[static_cast<size_t>(index)].esDeUsuario;
        
    return false;
}

int Reverb402AudioProcessor::obtenerIndicePresetPorNombre (const juce::String& nombre)
{
    for (int i = 0; i < static_cast<int>(listaCompletaPresets.size()); ++i)
    {
        if (listaCompletaPresets[i].nombre == nombre)
            return i;
    }
    return 0;
}

juce::AudioProcessorEditor* Reverb402AudioProcessor::createEditor()
{
    return new WrappedReverb402AudioProcessorEditor (*this);
}

void Reverb402AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    sincronizarSlotActivo();

    juce::ValueTree estadoAConNombre ("EstadoA");
    estadoAConNombre.copyPropertiesAndChildrenFrom (estadoA, nullptr);

    juce::ValueTree estadoBConNombre ("EstadoB");
    estadoBConNombre.copyPropertiesAndChildrenFrom (estadoB, nullptr);

    juce::ValueTree estadoCompleto ("Reverb402EstadoCompleto");
    estadoCompleto.setProperty ("modoBActivo", modoBActivo, nullptr);
    estadoCompleto.setProperty ("programaActualA", programaActualA, nullptr);
    estadoCompleto.setProperty ("programaActualB", programaActualB, nullptr);
    estadoCompleto.appendChild (estadoAConNombre, nullptr);
    estadoCompleto.appendChild (estadoBConNombre, nullptr);

    std::unique_ptr<juce::XmlElement> xml (estadoCompleto.createXml());
    copyXmlToBinary (*xml, destData);
}

void Reverb402AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlEstado (getXmlFromBinary (data, sizeInBytes));

    if (xmlEstado == nullptr)
        return;

    if (xmlEstado->hasTagName (listaParametros.state.getType()))
    {
        listaParametros.replaceState (juce::ValueTree::fromXml (*xmlEstado));
        estadoA = listaParametros.copyState();
        estadoB = listaParametros.copyState();
        modoBActivo = false;
        programaActualA = 0;
        programaActualB = 0;
        return;
    }

    if (! xmlEstado->hasTagName ("Reverb402EstadoCompleto"))
        return;

    juce::ValueTree estadoCompleto = juce::ValueTree::fromXml (*xmlEstado);

    modoBActivo = static_cast<bool>(estadoCompleto.getProperty ("modoBActivo", false));
    programaActualA = static_cast<int> (estadoCompleto.getProperty ("programaActualA", 0));
    programaActualB = static_cast<int> (estadoCompleto.getProperty ("programaActualB", 0));

    auto hijoEstadoA = estadoCompleto.getChildWithName ("EstadoA");
    auto hijoEstadoB = estadoCompleto.getChildWithName ("EstadoB");

    if (hijoEstadoA.isValid())
    {
        estadoA = juce::ValueTree (listaParametros.state.getType());
        estadoA.copyPropertiesAndChildrenFrom (hijoEstadoA, nullptr);
    }

    if (hijoEstadoB.isValid())
    {
        estadoB = juce::ValueTree (listaParametros.state.getType());
        estadoB.copyPropertiesAndChildrenFrom (hijoEstadoB, nullptr);
    }

    listaParametros.replaceState (modoBActivo ? estadoB : estadoA);

    if (auto* param = listaParametros.getParameter ("btnAB"))
        param->setValueNotifyingHost (modoBActivo ? 1.0f : 0.0f);
}

void Reverb402AudioProcessor::conmutarEstadoAB (bool usarEstadoB)
{
    if (modoBActivo == usarEstadoB)
        return;

    juce::ValueTree estadoActual = listaParametros.copyState();

    if (modoBActivo)
        estadoB = estadoActual;
    else
        estadoA = estadoActual;

    modoBActivo = usarEstadoB;

    listaParametros.replaceState (modoBActivo ? estadoB : estadoA);

    if (auto* param = listaParametros.getParameter ("btnAB"))
        param->setValueNotifyingHost (modoBActivo ? 1.0f : 0.0f);
}

void Reverb402AudioProcessor::sincronizarSlotActivo()
{
    if (modoBActivo)
        estadoB = listaParametros.copyState();
    else
        estadoA = listaParametros.copyState();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Reverb402AudioProcessor();
}