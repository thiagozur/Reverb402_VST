#include "IRVisualizer.h"

static const int margenIzquierdo = 45;
static const int margenInferior = 20;
static const int grosorBorde = 6;

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

    if (numSamples <= 0 || anchoComponente <= 0)
        return;
    
    double sampleRate = processor.obtenerSampleRate();

    double tiempoFijoSegundos = 5.0;
    int muestrasEn5Segundos = static_cast<int>(tiempoFijoSegundos * sampleRate);
    
    const float* datosLectura = (numSamples > 0) ? bufferLocal.getReadPointer(0) : nullptr;
    
    float areaAncho = static_cast<float>(anchoComponente - margenIzquierdo - 2.0f * grosorBorde);
    float areaAlto = static_cast<float>(altoComponente - margenInferior - 2.0f * grosorBorde);
    float centroY = areaAlto / 2.0f + grosorBorde;

    float muestrasPorPixel = static_cast<float>(muestrasEn5Segundos) / areaAncho;

    for (int pixelX = 0; pixelX < static_cast<int>(areaAncho); ++pixelX)
    {
        int inicioRango = static_cast<int>(pixelX * muestrasPorPixel);
        int finRango = static_cast<int> ((pixelX + 1) * muestrasPorPixel);

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
        
        caminoOnda.startNewSubPath (targetX, yArriba);
        caminoOnda.lineTo(targetX, yAbajo);
    }

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

    g.setColour (juce::Colour (0xFF1F2024));
    g.fillRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);

    {
        juce::Graphics::ScopedSaveState saveState (g);
        
        juce::Path mascaraRedondeada;
        mascaraRedondeada.addRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);
        g.reduceClipRegion (mascaraRedondeada);

        float xInicio = grosorBorde;
        float xFin = grosorBorde + ancho;

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
        gradienteVidrio.point2 = juce::Point<float> (xInicio, puntoControlY - 20);
        
        gradienteVidrio.addColour (0.0,  juce::Colours::white.withAlpha (0.16f));
        gradienteVidrio.addColour (0.7,  juce::Colours::white.withAlpha (0.04f));
        gradienteVidrio.addColour (1.0,  juce::Colours::transparentWhite);

        g.setGradientFill (gradienteVidrio);
        g.fillPath (caminoReflejo);
        
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde), xInicio, xFin);
    }

    float areaAncho = ancho - margenIzquierdo;
    float areaAlto = alto - margenInferior;
    float centroY = areaAlto / 2.0f + grosorBorde;

    g.setFont (10.0f);

    for (int i = 1; i < 5; ++i)
    {
        float x = margenIzquierdo + grosorBorde + (areaAncho / 5.0f) * i;

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawVerticalLine (static_cast<int>(x), grosorBorde, areaAlto + grosorBorde);

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (juce::String (i) + "s", static_cast<int>(x) - 15, static_cast<int>(areaAlto + grosorBorde) + 3, 30, 15, juce::Justification::centred);
    }

    float rangoDibujoY = (areaAlto * 0.85f) / 2.0f;

    g.setColour (juce::Colours::white.withAlpha (0.4f));
    for (int i = 0; i < 5; ++i)
    {
        float amplitud = 1.0f - (static_cast<float>(i) / 2.0f);
        float y = centroY - ((amplitud) * rangoDibujoY);
        juce::String etiqueta (amplitud, 1);
        g.drawText (etiqueta, 5, static_cast<int>(y) - 6, margenIzquierdo + grosorBorde - 12, 12, juce::Justification::right);
    }

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (static_cast<int>(centroY), margenIzquierdo + grosorBorde, ancho + grosorBorde);

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (static_cast<int>(areaAlto + grosorBorde), margenIzquierdo + grosorBorde, ancho + grosorBorde);

    g.drawVerticalLine (margenIzquierdo + grosorBorde, grosorBorde, areaAlto + grosorBorde);

    g.setColour (juce::Colour (0xFF00E5FF)); 
    g.strokePath (caminoOnda, juce::PathStrokeType (1.2f));
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

    imagenEspectrograma.clear (imagenEspectrograma.getBounds(), juce::Colours::transparentBlack);

    if (numSamples <= 0)
    {
        repaint();
        return;
    }

    double tiempoFijoSegundos = 5.0;
    int muestrasEn5Segundos = static_cast<int>(tiempoFijoSegundos * sampleRate);

    const float* datosLectura = bufferLocal.getReadPointer(0);
    float muestrasPorPixel = static_cast<float>(muestrasEn5Segundos) / static_cast<float>(areaAncho);

    std::array<float, fftSize * 2> datosFFT;

    {
        juce::Image::BitmapData mapaBits (imagenEspectrograma, juce::Image::BitmapData::writeOnly);

        juce::Colour colorBase = juce::Colour (0xFF00E5FF);  

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

    int tamañoKernel = 5;
    juce::ImageConvolutionKernel kernel (tamañoKernel);
    kernel.createGaussianBlur(4.0f);

    juce::Image imagenDesenfoque (imagenEspectrograma.getFormat(), areaAncho, areaAlto, true);
    kernel.applyToImage (imagenDesenfoque, imagenEspectrograma, imagenEspectrograma.getBounds());
    imagenEspectrograma = imagenDesenfoque;

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

    g.setColour (juce::Colour (0xFF18181C));
    g.fillRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);

    {
        juce::Graphics::ScopedSaveState saveState (g);
        
        juce::Path mascaraRedondeada;
        mascaraRedondeada.addRoundedRectangle (grosorBorde, grosorBorde, ancho, alto, 10.0f);
        g.reduceClipRegion (mascaraRedondeada);

        float xInicio = grosorBorde;
        float xFin = grosorBorde + ancho;

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
        gradienteVidrio.point2 = juce::Point<float> (xInicio, puntoControlY - 20);
        
        gradienteVidrio.addColour (0.0,  juce::Colours::white.withAlpha (0.16f));
        gradienteVidrio.addColour (0.7,  juce::Colours::white.withAlpha (0.04f));
        gradienteVidrio.addColour (1.0,  juce::Colours::transparentWhite);

        g.setGradientFill (gradienteVidrio);
        g.fillPath (caminoReflejo);
        
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawHorizontalLine (static_cast<int>(grosorBorde), xInicio, xFin);
    }

    float areaAncho = ancho - margenIzquierdo;
    float areaAlto = alto - margenInferior;

    if (imagenEspectrograma.isValid())
        g.drawImageAt (imagenEspectrograma, margenIzquierdo + grosorBorde + 1, grosorBorde);

    g.setFont (10.0f);

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
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.drawHorizontalLine (static_cast<int>(y), static_cast<float>(margenIzquierdo + grosorBorde), ancho + grosorBorde);

                g.setColour (juce::Colours::white.withAlpha (0.4f));
                juce::String textoFreq = (f >= 1000.0f) ? juce::String (f / 1000.0f, 1) + " kHz" : juce::String (static_cast<int>(f)) + " Hz";
                g.drawText (textoFreq, 5, static_cast<int>(y) - 6, margenIzquierdo + grosorBorde - 12, 12, juce::Justification::right);
            }
        }
    }

    for (int i = 1; i < 5; ++i)
    {
        float x = grosorBorde + margenIzquierdo + (areaAncho / 5.0f) * i;
        
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawVerticalLine (static_cast<int>(x), grosorBorde, areaAlto + grosorBorde);
        
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (juce::String (i) + " s", static_cast<int>(x) - 15, static_cast<int>(areaAlto + grosorBorde) + 3, 30, 15, juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawVerticalLine (margenIzquierdo + grosorBorde, grosorBorde, areaAlto + grosorBorde);
    g.drawHorizontalLine (static_cast<int>(areaAlto + grosorBorde), static_cast<float>(margenIzquierdo + grosorBorde), ancho + grosorBorde);
}