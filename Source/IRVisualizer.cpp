#include "IRVisualizer.h"

void IRWaveformPlot::actualizarOnda()
{
    int anchoComponente = getWidth();

    if (anchoComponente <= 0)
        return;

    processor.obtenerCopiaIrActual (bufferLocal);

    caminoOnda.clear();

    int numSamples = bufferLocal.getNumSamples();

    if (numSamples <= 0 || anchoComponente <= 0)
        return;
    
    const float* datosLectura = bufferLocal.getReadPointer(0);
    float altoComponente = static_cast<float>(getHeight());
    float centroY = altoComponente / 2.0f;

    float muestrasPorPixel = static_cast<float>(numSamples) / static_cast<float>(anchoComponente);

    caminoOnda.startNewSubPath(0.0f, centroY);

    for (int pixelX = 0; pixelX < anchoComponente; ++pixelX)
    {
        int inicioRango = static_cast<int>(pixelX * muestrasPorPixel);
        int finRango = juce::jmin (numSamples, static_cast<int>((pixelX + 1) * muestrasPorPixel));

        float maxPositivo = 0.0f;
        float minNegativo = 0.0f;


        for (int i = inicioRango; i < finRango; ++i)
        {
            float muestra = datosLectura[i];

            if (muestra > maxPositivo)
                maxPositivo = muestra;

            if (muestra < minNegativo)
                minNegativo = muestra;
        }

        maxPositivo = juce::jmin (1.0f, maxPositivo);
        minNegativo = juce::jmax (-1.0f, minNegativo);

        float rangoDibujoY = (altoComponente * 0.8f) / 2.0f;

        float yArriba = centroY - (maxPositivo * rangoDibujoY);
        float yAbajo = centroY - (minNegativo * rangoDibujoY);
        
        caminoOnda.startNewSubPath (static_cast<float>(pixelX), yArriba);
        caminoOnda.lineTo(static_cast<float>(pixelX), yAbajo);
    }

    repaint();
}

void IRWaveformPlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF151515));
    g.setColour (juce::Colours::darkgrey.withAlpha (0.2f));
    float centroY = getHeight() / 2.0f;
    g.drawHorizontalLine (static_cast<int>(centroY), 0.0f, static_cast<float>(getWidth()));

    if (! caminoOnda.isEmpty())
    {
        g.setColour (juce::Colours::cyan.withAlpha (0.8f));
        g.strokePath (caminoOnda, juce::PathStrokeType (1.5f));
    }
    else
    {
        g.setColour (juce::Colours::grey);
        g.setFont (14.0f);
        g.drawText (juce::String (juce::CharPointer_UTF8 ("Cargando respuesta al impulso...")), getLocalBounds(), juce::Justification::centred);
    }
}