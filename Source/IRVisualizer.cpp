#include "IRVisualizer.h"

static const int margenIzquierdo = 55;
static const int margenInferior = 20;

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
    
    float areaAncho = static_cast<float>(anchoComponente);
    float areaAlto = static_cast<float>(altoComponente - margenInferior);
    float centroY = areaAlto / 2.0f;

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

        float targetX = static_cast<float>(pixelX);
        
        caminoOnda.startNewSubPath (targetX, yArriba);
        caminoOnda.lineTo(targetX, yAbajo);
    }

    repaint();
}

void IRWaveformPlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF121214));

    float ancho = static_cast<float>(getWidth());
    float alto = static_cast<float>(getHeight());
    float areaAlto = alto - margenInferior;

    g.setColour (juce::Colour (0xFF18181C));
    g.fillRect (0.0f, 0.0f, ancho, areaAlto);

    g.setFont (10.0f);

    for (int i = 1; i < 5; ++i)
    {
        float x = (ancho / 5.0f) * i;

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawVerticalLine (static_cast<int>(x), 0.0f, areaAlto);

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (juce::String (i) + "s", static_cast<int> (x) - 15, static_cast<int> (areaAlto) + 3, 30, 15, juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (static_cast<int> (areaAlto / 2.0f), 0.0f, ancho);

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawVerticalLine (0.0f, 0.0f, areaAlto);

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
    int anchoComponente = getWidth();
    int altoComponente = getHeight();
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
                
                float energiaNormalizada = juce::jmap (db, -70.0f, -5.0f, 0.0f, 1.0f);
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

void IRSpectrogramPlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF121214));

    float ancho = static_cast<float>(getWidth());
    float alto = static_cast<float>(getHeight());
    float areaAncho = ancho - margenIzquierdo;
    float areaAlto = alto - margenInferior;

    g.setColour (juce::Colour (0xFF18181C));
    g.fillRect (static_cast<float>(margenIzquierdo), 0.0f, areaAncho, areaAlto);

    if (imagenEspectrograma.isValid())
        g.drawImageAt (imagenEspectrograma, margenIzquierdo, 0);

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
            float y = fraccionY * areaAlto;

            if (y >= 0 && y < areaAlto)
            {
                g.setColour (juce::Colours::white.withAlpha (0.05f));
                g.drawHorizontalLine (static_cast<int>(y), static_cast<float>(margenIzquierdo), ancho);

                g.setColour (juce::Colours::white.withAlpha (0.4f));
                juce::String textoFreq = (f >= 1000.0f) ? juce::String (f / 1000.0f, 1) + "k" : juce::String (static_cast<int>(f));
                g.drawText (textoFreq, 5, static_cast<int>(y) - 6, margenIzquierdo - 12, 12, juce::Justification::right);
            }
        }
    }

    for (int i = 1; i < 8; ++i)
    {
        float x = margenIzquierdo + (areaAncho / 8.0f) * i;
        
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawVerticalLine (static_cast<int>(x), 0.0f, areaAlto);
        
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (juce::String (i) + "s", static_cast<int>(x) - 15, static_cast<int>(areaAlto) + 3, 30, 15, juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawVerticalLine (margenIzquierdo, 0.0f, areaAlto);
    g.drawHorizontalLine (static_cast<int>(areaAlto), static_cast<float>(margenIzquierdo), ancho);
}