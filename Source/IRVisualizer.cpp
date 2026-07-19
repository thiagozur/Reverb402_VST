#include "IRVisualizer.h"

static const int margenIzquierdo = 45;
static const int margenInferior = 20;
static const int grosorBorde = 8;

IRWaveformPlot::IRWaveformPlot (Reverb402AudioProcessor& p) : processor (p)
{

}

IRWaveformPlot::~IRWaveformPlot()
{
    stopTimer();
}

void IRWaveformPlot::actualizarOnda()
{
    int anchoComponente = getWidth();
    float altoComponente = getHeight();

    if (anchoComponente <= 0 || altoComponente <= 0)
        return;

    processor.obtenerCopiaIrActual (bufferLocal);
    caminoOnda.clear();

    int numSamples = bufferLocal.getNumSamples();

    if (numSamples <= 0)
        return;
    
    double sampleRate = processor.obtenerSampleRate();

    double tiempoFijoSegundos = 8.0;
    int muestrasEn8Segundos = static_cast<int>(tiempoFijoSegundos * sampleRate);
    
    const float* datosLectura = bufferLocal.getReadPointer(0);
    
    float areaAncho = static_cast<float>(anchoComponente - margenIzquierdo - 2.0f * grosorBorde);
    float areaAlto = static_cast<float>(altoComponente - margenInferior - 2.0f * grosorBorde);
    float centroY = areaAlto / 2.0f + grosorBorde;

    float muestrasPorPixel = static_cast<float>(muestrasEn8Segundos) / areaAncho;

    bool primerPunto = true;
    
    std::vector<juce::Point<float>> puntosInferiores;
    puntosInferiores.reserve (static_cast<size_t>(areaAncho));

    for (int pixelX = 0; pixelX < static_cast<int>(areaAncho); ++pixelX)
    {
        int inicioRango = static_cast<int>(pixelX * muestrasPorPixel);
        int finRango = static_cast<int>((pixelX + 1) * muestrasPorPixel);

        float maxPositivo = 0.0f;
        float minNegativo = 0.0f;

        if (datosLectura != nullptr && inicioRango < numSamples)
        {
            int limiteSuperior = juce::jmin (numSamples, finRango);
            for (int i = inicioRango; i < limiteSuperior; ++i)
            {
                float muestra = datosLectura[i];

                if (muestra > maxPositivo)
                    maxPositivo = muestra;

                if (muestra < minNegativo)
                    minNegativo = muestra;
            }
        }

        float factorDesvanecimiento = 1.0f;
        if (inicioRango >= numSamples)
            factorDesvanecimiento = 0.0f;
        else if (numSamples - inicioRango < 500)
            factorDesvanecimiento = static_cast<float>(numSamples - inicioRango) / 500.0f;

        maxPositivo *= factorDesvanecimiento;
        minNegativo *= factorDesvanecimiento;

        maxPositivo = juce::jmin (1.0f, maxPositivo);
        minNegativo = juce::jmax (-1.0f, minNegativo);

        float rangoDibujoY = (areaAlto * 0.85f) / 2.0f;

        float yArriba = centroY - (maxPositivo * rangoDibujoY);
        float yAbajo = centroY - (minNegativo * rangoDibujoY);

        float targetX = static_cast<float>(pixelX + margenIzquierdo + grosorBorde + 1);
        
        if (primerPunto)
        {
            caminoOnda.startNewSubPath (targetX, yArriba);
            primerPunto = false;
        }
        else
            caminoOnda.lineTo (targetX, yArriba);

        puntosInferiores.push_back ({ targetX, yAbajo });
    }

    for (auto it = puntosInferiores.rbegin(); it != puntosInferiores.rend(); ++it)
        caminoOnda.lineTo (it->x, it->y);

    caminoOnda.closeSubPath();

    repaint();
}

void IRWaveformPlot::timerCallback()
{
    actualizarOnda();
}

void IRWaveformPlot::visibilityChanged()
{
    if (isShowing())
        startTimerHz (25);
    else
        stopTimer();
}

void IRWaveformPlot::paint (juce::Graphics& g)
{
    float anchoBorde = static_cast<float>(getWidth());
    float altoBorde = static_cast<float>(getHeight());

    g.setColour (juce::Colour (0xFF111113));
    g.fillRoundedRectangle (0.0f, 0.0f, anchoBorde, altoBorde, 10.0f);

    float alto = altoBorde - 2.0f * grosorBorde;
    float ancho = anchoBorde - 2.0f * grosorBorde;

    g.setColour (juce::Colour (0xFFD1D1D1));
    g.fillRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);

    float areaAncho = ancho - margenIzquierdo;
    float areaAlto = alto - margenInferior;
    float centroY = areaAlto / 2.0f + grosorBorde;

    const juce::Font fuenteTicks ("Arial", 12.0f, juce::Font::bold);
    g.setFont (fuenteTicks);

    for (int i = 1; i < 8; ++i)
    {
        float x = margenIzquierdo + grosorBorde + (areaAncho / 8.0f) * i + 1;

        g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
        g.drawVerticalLine (static_cast<int>(x), grosorBorde, areaAlto + grosorBorde);

        g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
        g.drawText (juce::String (i) + " s", static_cast<int>(x) - 15, static_cast<int>(areaAlto + grosorBorde) + 3, 30, 15, juce::Justification::centred);
    }

    float rangoDibujoY = (areaAlto * 0.85f) / 2.0f;

    g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
    for (int i = 0; i < 5; ++i)
    {
        float amplitud = 1.0f - (static_cast<float>(i) / 2.0f);
        float y = centroY - ((amplitud) * rangoDibujoY);
        juce::String etiqueta (amplitud, 1);
        g.drawText (etiqueta, 5, static_cast<int>(y) - 6, margenIzquierdo + grosorBorde - 12, 12, juce::Justification::right);
    }

    g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
    g.drawHorizontalLine (static_cast<int>(centroY), margenIzquierdo + grosorBorde, ancho + grosorBorde);

    g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
    g.drawHorizontalLine (static_cast<int>(areaAlto + grosorBorde), margenIzquierdo + grosorBorde, ancho + grosorBorde);

    g.drawVerticalLine (margenIzquierdo + grosorBorde, grosorBorde, areaAlto + grosorBorde);

    g.setColour (juce::Colour (0xFF00A0D2));
    g.fillPath (caminoOnda);

    {
        juce::Graphics::ScopedSaveState saveState (g);
        
        juce::Path mascaraRedondeada;
        mascaraRedondeada.addRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);
        g.reduceClipRegion (mascaraRedondeada);

        float xInicio = grosorBorde;
        float xFin = grosorBorde + ancho;

        float alturaSombra = juce::jmin (40.0f, alto * 0.25f); 

        juce::ColourGradient sombraInterior;
        sombraInterior.isRadial = false;
        sombraInterior.point1 = juce::Point<float> (xInicio, grosorBorde);
        sombraInterior.point2 = juce::Point<float> (xInicio, grosorBorde + alturaSombra);

        sombraInterior.addColour (0.0f, juce::Colours::black.withAlpha (1.0f));  
        sombraInterior.addColour (0.7f, juce::Colours::black.withAlpha (0.3f)); 
        sombraInterior.addColour (1.0f, juce::Colours::transparentBlack);        
        
        g.setGradientFill (sombraInterior);
        g.fillRect (xInicio, static_cast<float>(grosorBorde), ancho, alturaSombra);

        float anchoSombraOclusion = juce::jmin (8.0f, ancho * 0.03f);
        juce::ColourGradient sombraInferior;
        sombraInferior.isRadial = false;
        sombraInferior.point1 = juce::Point<float> (xInicio, grosorBorde + alto);
        sombraInferior.point2 = juce::Point<float> (xInicio, grosorBorde + alto - anchoSombraOclusion);

        sombraInferior.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraInferior.addColour (1.0f, juce::Colours::transparentBlack);

        g.setGradientFill (sombraInferior);
        g.fillRect (xInicio, grosorBorde + alto - anchoSombraOclusion, ancho, anchoSombraOclusion);

        juce::ColourGradient sombraIzquierda;
        sombraIzquierda.isRadial = false;
        sombraIzquierda.point1 = juce::Point<float> (xInicio, grosorBorde);
        sombraIzquierda.point2 = juce::Point<float> (xInicio + anchoSombraOclusion, grosorBorde);
        sombraIzquierda.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraIzquierda.addColour (1.0f, juce::Colours::transparentBlack);
        
        g.setGradientFill (sombraIzquierda);
        g.fillRect (xInicio, static_cast<float> (grosorBorde), anchoSombraOclusion, alto);

        juce::ColourGradient sombraDerecha;
        sombraDerecha.isRadial = false;
        sombraDerecha.point1 = juce::Point<float> (xFin, grosorBorde);
        sombraDerecha.point2 = juce::Point<float> (xFin - anchoSombraOclusion, grosorBorde);
        sombraDerecha.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraDerecha.addColour (1.0f, juce::Colours::transparentBlack);
        
        g.setGradientFill (sombraDerecha);
        g.fillRect (xFin - anchoSombraOclusion, static_cast<float> (grosorBorde), anchoSombraOclusion, alto);

        float yLaterales = grosorBorde + (alto * 0.25f); 
        float puntoControlX = xInicio + (ancho / 2.0f);
        float puntoControlY = grosorBorde + (alto * 0.45f); 

        juce::Path caminoReflejo;
        caminoReflejo.startNewSubPath (xInicio, grosorBorde);
        caminoReflejo.lineTo (xInicio, yLaterales);
        caminoReflejo.quadraticTo (puntoControlX, puntoControlY, xFin, yLaterales);
        caminoReflejo.lineTo (xFin, grosorBorde);
        caminoReflejo.closeSubPath();

        juce::ColourGradient gradienteVidrio;
        gradienteVidrio.isRadial = false;
        gradienteVidrio.point1 = juce::Point<float> (xInicio, grosorBorde);
        gradienteVidrio.point2 = juce::Point<float> (xInicio, puntoControlY - 15);
        
        gradienteVidrio.addColour (0.0,  juce::Colours::white.withAlpha (0.12f));
        gradienteVidrio.addColour (0.7,  juce::Colours::white.withAlpha (0.35f));
        gradienteVidrio.addColour (1.0,  juce::Colours::transparentWhite);

        g.setGradientFill (gradienteVidrio);
        g.fillPath (caminoReflejo);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde), xInicio, xFin);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde + alto - 1.0f), xInicio, xFin);
    }
}

IRSpectrogramPlot::IRSpectrogramPlot (Reverb402AudioProcessor& p) : processor (p)
{
    descriptorFFT = std::make_unique<juce::dsp::FFT> (fftOrder);
    ventanaHann = std::make_unique<juce::dsp::WindowingFunction<float>> (fftSize, juce::dsp::WindowingFunction<float>::hann);
}

IRSpectrogramPlot::~IRSpectrogramPlot()
{
    stopTimer();
}

void IRSpectrogramPlot::resized()
{
    int ancho = getWidth();
    int alto = getHeight();

    if (ancho > margenIzquierdo && alto > margenInferior)
    {
        imagenEspectrograma = juce::Image (juce::Image::ARGB, ancho - margenIzquierdo, alto -  margenInferior, true);
        actualizarEspectrograma();
    }
}

void IRSpectrogramPlot::actualizarEspectrograma()
{
    int anchoComponente = getWidth() - 2.0f * grosorBorde;
    int altoComponente = getHeight() - 2.0f * grosorBorde;
    int areaAncho = anchoComponente - margenIzquierdo;
    int areaAlto = altoComponente - margenInferior;

    if (areaAncho <= 0 || areaAlto <= 0)
        return;
    
    processor.obtenerCopiaIrActual (bufferLocal);
    int numSamples = bufferLocal.getNumSamples();
    double sampleRate = processor.obtenerSampleRate();
    double nyquist = sampleRate / 2.0;

    imagenEspectrograma.clear (imagenEspectrograma.getBounds(), juce::Colours::transparentBlack);

    if (numSamples <= 0)
    {
        repaint();
        return;
    }

    double tiempoFijoSegundos = 8.0;
    int muestrasEn8Segundos = static_cast<int>(tiempoFijoSegundos * sampleRate);

    const float* datosLectura = bufferLocal.getReadPointer(0);
    float muestrasPorPixel = static_cast<float>(muestrasEn8Segundos) / static_cast<float>(areaAncho);

    std::array<float, fftSize * 2> datosFFT;

    {
        juce::Image::BitmapData mapaBits (imagenEspectrograma, juce::Image::BitmapData::writeOnly);

        juce::Colour colorBase = juce::Colour (0xFF4D9823);  

        for (int pixelX = 0; pixelX < areaAncho; ++pixelX)
        {
            int centroMuestra = static_cast<int>(pixelX * muestrasPorPixel);
            int inicioVentana = centroMuestra - (fftSize / 2);

            if (inicioVentana >= numSamples)
            {
                for (int targetY = 0; targetY < areaAlto; ++targetY)
                    mapaBits.setPixelColour (pixelX, targetY, juce::Colours::transparentBlack);
                continue;
            }

            float anchoFadeMuestras = fftSize * 3.5f;
            float factorDesvanecimiento = 1.0f;
            int muestrasRestantes = numSamples - centroMuestra;

            if (muestrasRestantes < anchoFadeMuestras)
            {
                factorDesvanecimiento = juce::jlimit (0.0f, 1.0f, static_cast<float>(muestrasRestantes) / anchoFadeMuestras);
                factorDesvanecimiento = std::sin (factorDesvanecimiento * juce::MathConstants<float>::halfPi);
            }

            std::fill (datosFFT.begin(), datosFFT.end(), 0.0f);

            for (int i = 0; i < fftSize; ++i)
            {
                int indiceMuestra = inicioVentana + i;
                if (indiceMuestra >= 0 && indiceMuestra < numSamples)
                    datosFFT[static_cast<size_t>(i)] = datosLectura[indiceMuestra];
            }

            ventanaHann->multiplyWithWindowingTable (datosFFT.data(), fftSize);
            descriptorFFT->performFrequencyOnlyForwardTransform (datosFFT.data());

            for (int targetY = 0; targetY < areaAlto; ++targetY)
            {
                float fraccionY = static_cast<float>(targetY) / static_cast<float>(areaAlto);
                float factorLog = std::pow (1.0f - fraccionY, 2.0f);

                float frecuenciaPixel = static_cast<float>(factorLog * nyquist);

                if (frecuenciaPixel < 20.0f || frecuenciaPixel > 20000.0f)
                {
                    mapaBits.setPixelColour (pixelX, targetY, juce::Colours::transparentBlack);
                    continue;
                }

                float indexBinFloat = factorLog * ((fftSize / 2) - 1);

                int binBajo = static_cast<int>(std::floor (indexBinFloat));
                int binAlto = juce::jmin ((fftSize / 2) - 1, binBajo + 1);
                float fraccionBin = indexBinFloat - static_cast<float>(binBajo);
                
                float magBaja = datosFFT[static_cast<size_t>(binBajo)];
                float magAlta = datosFFT[static_cast<size_t>(binAlto)];
                float magnitudInterpolada = magBaja + fraccionBin * (magAlta - magBaja);

                float db = 20.0f * std::log10 (magnitudInterpolada + 1e-5f);
                
                float floorVisual = processor.obtenerPisoRuidoActual() + 5.0f;
                float energiaNormalizada = juce::jmap (db, floorVisual, -5.0f, 0.0f, 1.0f);
                energiaNormalizada = juce::jlimit (0.0f, 1.0f, energiaNormalizada);

                float alpha = energiaNormalizada * factorDesvanecimiento;
                juce::Colour colorFinal = colorBase.withMultipliedAlpha (alpha);

                mapaBits.setPixelColour (pixelX, targetY, colorFinal);
            }
        }
    }

    repaint();
}

void IRSpectrogramPlot::timerCallback()
{
    actualizarEspectrograma();
}

void IRSpectrogramPlot::visibilityChanged()
{
    if (isShowing())
        startTimerHz (1);
    else
        stopTimer();
}

void IRSpectrogramPlot::paint (juce::Graphics& g)
{
    float anchoBorde = static_cast<float>(getWidth());
    float altoBorde = static_cast<float>(getHeight());

    g.setColour (juce::Colour (0xFF111113));
    g.fillRoundedRectangle (0.0f, 0.0f, anchoBorde, altoBorde, 10.0f);

    float alto = altoBorde - 2.0f * grosorBorde;
    float ancho = anchoBorde - 2.0f * grosorBorde;

    g.setColour (juce::Colour (0xFFD1D1D1));
    g.fillRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);

    float areaAncho = ancho - margenIzquierdo;
    float areaAlto = alto - margenInferior;

    const juce::Font fuenteTicks ("Arial", 12.0f, juce::Font::bold);
    g.setFont (fuenteTicks);

    double sampleRate = processor.obtenerSampleRate();
    double nyquist = sampleRate / 2.0;
    std::vector<float> frecuenciasGrilla = { 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 15000.0f };

    for (float f : frecuenciasGrilla)
    {
        if (f < nyquist)
        {
            float ratio = static_cast<float>(f / nyquist);
            float fraccionY = 1.0f - std::sqrt (ratio);
            float y = fraccionY * areaAlto + grosorBorde;

            if (y >= 0 && y < (areaAlto + grosorBorde))
            {
                g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
                g.drawHorizontalLine (static_cast<int>(y), static_cast<float>(margenIzquierdo + grosorBorde), ancho + grosorBorde);

                g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
                juce::String textoFreq = (f >= 1000.0f) ? juce::String (f / 1000.0f) + " kHz" : juce::String (static_cast<int>(f)) + " Hz";
                g.drawText (textoFreq, 5, static_cast<int>(y) - 6, margenIzquierdo + grosorBorde - 12, 12, juce::Justification::right);
            }
        }
    }

    for (int i = 1; i < 8; ++i)
    {
        float x = grosorBorde + margenIzquierdo + (areaAncho / 8.0f) * i + 1;
        
        g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
        g.drawVerticalLine (static_cast<int>(x), grosorBorde, areaAlto + grosorBorde);
        
        g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
        g.drawText (juce::String (i) + " s", static_cast<int>(x) - 15, static_cast<int>(areaAlto + grosorBorde) + 3, 30, 15, juce::Justification::centred);
    }

    g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
    g.drawVerticalLine (margenIzquierdo + grosorBorde, grosorBorde, areaAlto + grosorBorde);
    g.drawHorizontalLine (static_cast<int>(areaAlto + grosorBorde), static_cast<float>(margenIzquierdo + grosorBorde), ancho + grosorBorde);

    if (imagenEspectrograma.isValid())
        g.drawImageAt (imagenEspectrograma, margenIzquierdo + grosorBorde + 1, grosorBorde);

    {
        juce::Graphics::ScopedSaveState saveState (g);
        
        juce::Path mascaraRedondeada;
        mascaraRedondeada.addRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);
        g.reduceClipRegion (mascaraRedondeada);

        float xInicio = grosorBorde;
        float xFin = grosorBorde + ancho;

        float alturaSombra = juce::jmin (40.0f, alto * 0.25f); 

        juce::ColourGradient sombraInterior;
        sombraInterior.isRadial = false;
        sombraInterior.point1 = juce::Point<float> (xInicio, grosorBorde);
        sombraInterior.point2 = juce::Point<float> (xInicio, grosorBorde + alturaSombra);

        sombraInterior.addColour (0.0f, juce::Colours::black.withAlpha (1.0f));  
        sombraInterior.addColour (0.7f, juce::Colours::black.withAlpha (0.3f)); 
        sombraInterior.addColour (1.0f, juce::Colours::transparentBlack);        
        
        g.setGradientFill (sombraInterior);
        g.fillRect (xInicio, static_cast<float>(grosorBorde), ancho, alturaSombra);

        float anchoSombraOclusion = juce::jmin (8.0f, ancho * 0.03f);
        juce::ColourGradient sombraInferior;
        sombraInferior.isRadial = false;
        sombraInferior.point1 = juce::Point<float> (xInicio, grosorBorde + alto);
        sombraInferior.point2 = juce::Point<float> (xInicio, grosorBorde + alto - anchoSombraOclusion);

        sombraInferior.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraInferior.addColour (1.0f, juce::Colours::transparentBlack);

        g.setGradientFill (sombraInferior);
        g.fillRect (xInicio, grosorBorde + alto - anchoSombraOclusion, ancho, anchoSombraOclusion);

        juce::ColourGradient sombraIzquierda;
        sombraIzquierda.isRadial = false;
        sombraIzquierda.point1 = juce::Point<float> (xInicio, grosorBorde);
        sombraIzquierda.point2 = juce::Point<float> (xInicio + anchoSombraOclusion, grosorBorde);
        sombraIzquierda.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraIzquierda.addColour (1.0f, juce::Colours::transparentBlack);
        
        g.setGradientFill (sombraIzquierda);
        g.fillRect (xInicio, static_cast<float> (grosorBorde), anchoSombraOclusion, alto);

        juce::ColourGradient sombraDerecha;
        sombraDerecha.isRadial = false;
        sombraDerecha.point1 = juce::Point<float> (xFin, grosorBorde);
        sombraDerecha.point2 = juce::Point<float> (xFin - anchoSombraOclusion, grosorBorde);
        sombraDerecha.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraDerecha.addColour (1.0f, juce::Colours::transparentBlack);
        
        g.setGradientFill (sombraDerecha);
        g.fillRect (xFin - anchoSombraOclusion, static_cast<float> (grosorBorde), anchoSombraOclusion, alto);

        float yLaterales = grosorBorde + (alto * 0.25f); 
        float puntoControlX = xInicio + (ancho / 2.0f);
        float puntoControlY = grosorBorde + (alto * 0.45f); 

        juce::Path caminoReflejo;
        caminoReflejo.startNewSubPath (xInicio, grosorBorde);
        caminoReflejo.lineTo (xInicio, yLaterales);
        caminoReflejo.quadraticTo (puntoControlX, puntoControlY, xFin, yLaterales);
        caminoReflejo.lineTo (xFin, grosorBorde);
        caminoReflejo.closeSubPath();

        juce::ColourGradient gradienteVidrio;
        gradienteVidrio.isRadial = false;
        gradienteVidrio.point1 = juce::Point<float> (xInicio, grosorBorde);
        gradienteVidrio.point2 = juce::Point<float> (xInicio, puntoControlY - 15);
        
        gradienteVidrio.addColour (0.0,  juce::Colours::white.withAlpha (0.12f));
        gradienteVidrio.addColour (0.7,  juce::Colours::white.withAlpha (0.35f));
        gradienteVidrio.addColour (1.0,  juce::Colours::transparentWhite);

        g.setGradientFill (gradienteVidrio);
        g.fillPath (caminoReflejo);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde), xInicio, xFin);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde + alto - 1.0f), xInicio, xFin);
    }
}

FiltersPlot::FiltersPlot (Reverb402AudioProcessor& p) : processor (p)
{

}

FiltersPlot::~FiltersPlot()
{
    stopTimer();
}

void FiltersPlot::actualizarFiltros()
{
    int anchoDivisor = 3 * grosorBorde;

    float anchoComponente = static_cast<float>(getWidth());
    float altoComponente = static_cast<float>(getHeight());

    if (anchoComponente <= 0 || altoComponente <= 0)
        return;

    caminoHPF.clear();
    caminoLPF.clear();

    float areaAncho = static_cast<float>(anchoComponente - 2 * margenIzquierdo - 2 * grosorBorde - anchoDivisor);
    float areaAlto = static_cast<float>(altoComponente - margenInferior - 2 * grosorBorde);
    int anchoVisor = (static_cast<int>(areaAncho)) / 2;

    float yMaximo = static_cast<float>(grosorBorde) + areaAlto; 
    float centroY = (areaAlto / 2.0f) + static_cast<float>(grosorBorde);
    float rangoY = areaAlto / 2.0f;
    const float minDB = -24.0f; 

    const float freqMinVisor = 10.0f;
    const float freqMaxVisor = 40000.0f;
    const float logRango = std::log (freqMaxVisor / freqMinVisor);

    float hpfXInicio = static_cast<float>(grosorBorde + margenIzquierdo);
    float lpfXInicio = static_cast<float>(grosorBorde + anchoVisor + 2 * margenIzquierdo + anchoDivisor);
    
    if (auto* estadoHPF = processor.obtenerAPVTS().getRawParameterValue ("hpf"))
        freqHPFActual = estadoHPF->load();

    int xMinHPF = static_cast<int> ((std::log (20.0f / freqMinVisor) / logRango) * (anchoVisor - 1));
    int xMaxHPF = static_cast<int> ((std::log (20000.0f / freqMinVisor) / logRango) * (anchoVisor - 1));

    bool primerPuntoHPF = true;
    for (int x = xMinHPF; x <= xMaxHPF; ++x)
    {
        float pct = static_cast<float>(x) / (anchoVisor - 1);
        float freq = freqMinVisor * std::pow (freqMaxVisor / freqMinVisor, pct); 
        float magnitud = freq / std::sqrt (freq * freq + freqHPFActual * freqHPFActual);

        float db = (magnitud > 0.0001f) ? 20.0f * std::log10 (magnitud) : minDB;
        if (db < minDB)
            continue;

        float xPos = hpfXInicio + x;
        float yPos = centroY + (db / minDB) * rangoY;
        yPos = std::min (yPos, yMaximo);

        if (primerPuntoHPF)
        {
            caminoHPF.startNewSubPath (xPos, yPos);
            primerPuntoHPF = false;
        }
        else
            caminoHPF.lineTo (xPos, yPos);

    }

    if (auto* estadoLPF = processor.obtenerAPVTS().getRawParameterValue ("lpf"))
        freqLPFActual = estadoLPF->load();

    int xMinLPF = static_cast<int> ((std::log (20.0f / freqMinVisor) / logRango) * (anchoVisor - 1));
    int xMaxLPF = static_cast<int> ((std::log (20000.0f / freqMinVisor) / logRango) * (anchoVisor - 1));

    bool primerPuntoLPF = true;
    for (int x = xMinLPF; x <= xMaxLPF; ++x)
    {
        float pct = static_cast<float>(x) / (anchoVisor - 1);
        float freq = freqMinVisor * std::pow (freqMaxVisor / freqMinVisor, pct); 
        float magnitud = freqLPFActual / std::sqrt (freq * freq + freqLPFActual * freqLPFActual);

        float db = (magnitud > 0.0001f) ? 20.0f * std::log10 (magnitud) : minDB;
        if (db < minDB)
            continue;

        float xPos = lpfXInicio + x;
        float yPos = centroY + (db / minDB) * rangoY;
        yPos = std::min (yPos, yMaximo);

        if (primerPuntoLPF)
        {
            caminoLPF.startNewSubPath (xPos, yPos);
            primerPuntoLPF = false;
        }
        else
            caminoLPF.lineTo (xPos, yPos);
    }

    repaint();
}

void FiltersPlot::timerCallback()
{
    actualizarFiltros();
}

void FiltersPlot::visibilityChanged()
{
    if (isShowing())
        startTimerHz (25);
    else
        stopTimer();
}

void FiltersPlot::paint (juce::Graphics& g)
{
    float anchoDivisor = 3 * grosorBorde;

    float anchoBorde = static_cast<float>(getWidth());
    float altoBorde = static_cast<float>(getHeight());

    g.setColour (juce::Colour (0xFF111113));
    g.fillRoundedRectangle (0.0f, 0.0f, anchoBorde, altoBorde, 10.0f);

    float alto = altoBorde - 2.0f * grosorBorde;
    float ancho = anchoBorde - 2.0f * grosorBorde;

    float areaAlto = alto - margenInferior;
    float areaAncho = ancho - 2 * margenIzquierdo - anchoDivisor;

    g.setColour (juce::Colour (0xFFD1D1D1));
    g.fillRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);
    
    float anchoVisor = areaAncho / 2;

    g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
    g.fillRect (grosorBorde + margenIzquierdo + anchoVisor, static_cast<float>(grosorBorde), anchoDivisor, alto);

    float yPiso = static_cast<float>(grosorBorde) + areaAlto;
    float centroY = static_cast<float>(grosorBorde) + (areaAlto / 2.0f);
    float rangoY = areaAlto / 2.0f;
    const float minDB = -24.0f;

    float hpfXInicio = static_cast<float>(grosorBorde + margenIzquierdo);
    float lpfXInicio = static_cast<float>(grosorBorde + anchoVisor + 2 * margenIzquierdo + anchoDivisor);

    const float freqMinVisor = 10.0f;
    const float freqMaxVisor = 40000.0f;
    const float logRango = std::log (freqMaxVisor / freqMinVisor);

    const juce::Font fuenteTicks ("Arial", 12.0f, juce::Font::bold);
    g.setFont (fuenteTicks);

    std::vector<float> ticksDb = { 18.0f, 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -18.0f };
    int anchoTextoDb = static_cast<int>(hpfXInicio - 4);
    for (float db : ticksDb)
    {
        float yPos = centroY + (db / minDB) * rangoY;

        if (yPos >= static_cast<float>(grosorBorde) && yPos <= yPiso)
        {
            g.setColour (juce::Colour (0xFF121212).withAlpha (db == 0.0f ? 0.7f : 0.3f));
            g.drawHorizontalLine (yPos, hpfXInicio, hpfXInicio + anchoVisor);
            g.drawHorizontalLine (yPos, lpfXInicio, lpfXInicio + anchoVisor);

            g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
            juce::String textoDb = (db > 0 ? "+" : "") + juce::String (static_cast<int>(db)) + " dB";

            g.drawText (textoDb, 0, static_cast<int>(yPos - 6), anchoTextoDb, 12, juce::Justification::centredRight);
            int xInicioTextoLPF = static_cast<int>(lpfXInicio) - anchoTextoDb - 4;
            g.drawText (textoDb, xInicioTextoLPF, static_cast<int>(yPos - 6), anchoTextoDb, 12, juce::Justification::centredRight);
        }
    }

    std::vector<float> ticksFrecuencias = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    for (float f : ticksFrecuencias)
    {
        float pct = std::log (f / freqMinVisor) / logRango;

        float xPosHPF = hpfXInicio + (pct * (anchoVisor - 1));
        float xPosLPF = lpfXInicio + (pct * (anchoVisor - 1));

        g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
        g.drawVerticalLine (static_cast<int>(xPosHPF), grosorBorde, yPiso);
        g.drawVerticalLine (static_cast<int>(xPosLPF), grosorBorde, yPiso);

        g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
        juce::String textoF = (f >= 1000.0f) ? juce::String (f / 1000.0f, f >= 10000.0f ? 0 : 1) + "k" : juce::String (static_cast<int>(f));
        
        g.drawText (textoF, static_cast<int>(xPosHPF - 20), static_cast<int>(yPiso + 2), 40, 12, juce::Justification::horizontallyCentred);
        g.drawText (textoF, static_cast<int>(xPosLPF - 20), static_cast<int>(yPiso + 2), 40, 12, juce::Justification::horizontallyCentred);
    }

    g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
    
    g.drawHorizontalLine (static_cast<int>(yPiso), hpfXInicio, hpfXInicio + anchoVisor);
    g.drawHorizontalLine (static_cast<int>(yPiso), lpfXInicio, lpfXInicio + anchoVisor);

    g.drawVerticalLine (static_cast<int>(hpfXInicio), grosorBorde, yPiso);
    g.drawVerticalLine (static_cast<int>(hpfXInicio + anchoVisor), grosorBorde, yPiso);
    g.drawVerticalLine (static_cast<int>(lpfXInicio), grosorBorde, yPiso);
    g.drawVerticalLine (static_cast<int>(lpfXInicio + anchoVisor), grosorBorde, yPiso);

    g.setFont (fuenteTicks.withHeight (13.0f));
    g.setColour (juce::Colour (0xFF121212).withAlpha (0.7f));
    g.drawText ("HPF", static_cast<int>(hpfXInicio), static_cast<int>(grosorBorde + 24), static_cast<int>(anchoVisor), 14, juce::Justification::horizontallyCentred);
    g.drawText ("LPF", static_cast<int>(lpfXInicio), static_cast<int>(grosorBorde + 24), static_cast<int>(anchoVisor), 14, juce::Justification::horizontallyCentred);
    
    if (std::abs(freqHPFActual - 20.0f) < 0.1f)
        g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
    else
        g.setColour (juce::Colour (0xFFFC3172));
        
    g.strokePath (caminoHPF, juce::PathStrokeType (2.0f));

    if (std::abs(freqLPFActual - 20000.0f) < 0.1f)
        g.setColour (juce::Colour (0xFF121212).withAlpha (0.3f));
    else
        g.setColour (juce::Colour (0xFFFC3172));
        
    g.strokePath (caminoLPF, juce::PathStrokeType (2.0f));

    {
        juce::Graphics::ScopedSaveState saveState (g);
        
        juce::Path mascaraRedondeada;
        mascaraRedondeada.addRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);
        g.reduceClipRegion (mascaraRedondeada);

        float xInicio = grosorBorde;
        float xFin = grosorBorde + ancho;

        float alturaSombra = juce::jmin (40.0f, alto * 0.25f); 

        juce::ColourGradient sombraInterior;
        sombraInterior.isRadial = false;
        sombraInterior.point1 = juce::Point<float> (xInicio, grosorBorde);
        sombraInterior.point2 = juce::Point<float> (xInicio, grosorBorde + alturaSombra);

        sombraInterior.addColour (0.0f, juce::Colours::black.withAlpha (1.0f));  
        sombraInterior.addColour (0.7f, juce::Colours::black.withAlpha (0.3f)); 
        sombraInterior.addColour (1.0f, juce::Colours::transparentBlack);        
        
        g.setGradientFill (sombraInterior);
        g.fillRect (xInicio, static_cast<float>(grosorBorde), ancho, alturaSombra);

        float anchoSombraOclusion = juce::jmin (8.0f, ancho * 0.03f);
        juce::ColourGradient sombraInferior;
        sombraInferior.isRadial = false;
        sombraInferior.point1 = juce::Point<float> (xInicio, grosorBorde + alto);
        sombraInferior.point2 = juce::Point<float> (xInicio, grosorBorde + alto - anchoSombraOclusion);

        sombraInferior.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraInferior.addColour (1.0f, juce::Colours::transparentBlack);

        g.setGradientFill (sombraInferior);
        g.fillRect (xInicio, grosorBorde + alto - anchoSombraOclusion, ancho, anchoSombraOclusion);

        juce::ColourGradient sombraIzquierda;
        sombraIzquierda.isRadial = false;
        sombraIzquierda.point1 = juce::Point<float> (xInicio, grosorBorde);
        sombraIzquierda.point2 = juce::Point<float> (xInicio + anchoSombraOclusion, grosorBorde);
        sombraIzquierda.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraIzquierda.addColour (1.0f, juce::Colours::transparentBlack);
        
        g.setGradientFill (sombraIzquierda);
        g.fillRect (xInicio, static_cast<float> (grosorBorde), anchoSombraOclusion, alto);

        juce::ColourGradient sombraDerecha;
        sombraDerecha.isRadial = false;
        sombraDerecha.point1 = juce::Point<float> (xFin, grosorBorde);
        sombraDerecha.point2 = juce::Point<float> (xFin - anchoSombraOclusion, grosorBorde);
        sombraDerecha.addColour (0.0f, juce::Colours::black.withAlpha (0.35f));
        sombraDerecha.addColour (1.0f, juce::Colours::transparentBlack);
        
        g.setGradientFill (sombraDerecha);
        g.fillRect (xFin - anchoSombraOclusion, static_cast<float> (grosorBorde), anchoSombraOclusion, alto);

        float yLaterales = grosorBorde + (alto * 0.25f); 
        float puntoControlX = xInicio + (ancho / 2.0f);
        float puntoControlY = grosorBorde + (alto * 0.45f); 

        juce::Path caminoReflejo;
        caminoReflejo.startNewSubPath (xInicio, grosorBorde);
        caminoReflejo.lineTo (xInicio, yLaterales);
        caminoReflejo.quadraticTo (puntoControlX, puntoControlY, xFin, yLaterales);
        caminoReflejo.lineTo (xFin, grosorBorde);
        caminoReflejo.closeSubPath();

        juce::ColourGradient gradienteVidrio;
        gradienteVidrio.isRadial = false;
        gradienteVidrio.point1 = juce::Point<float> (xInicio, grosorBorde);
        gradienteVidrio.point2 = juce::Point<float> (xInicio, puntoControlY - 15);
        
        gradienteVidrio.addColour (0.0,  juce::Colours::white.withAlpha (0.12f));
        gradienteVidrio.addColour (0.7,  juce::Colours::white.withAlpha (0.35f));
        gradienteVidrio.addColour (1.0,  juce::Colours::transparentWhite);

        g.setGradientFill (gradienteVidrio);
        g.fillPath (caminoReflejo);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde), xInicio, xFin);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde + alto - 1.0f), xInicio, xFin);
    }
}
