#include "MedidorNivel.h"

Luz::Luz(const juce::Colour& c) : color (c)
{

}

Luz::~Luz()
{

}

void Luz::setState(const bool state)
{
    encendida = state;
}

void Luz::paint (juce::Graphics& g)
{
    const auto delta = 4.0f;
    const auto bounds = getLocalBounds().toFloat().reduced (delta);
    const auto lado = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto boundsLuz = juce::Rectangle<float> (lado, lado).withCentre (bounds.getCentre());

    if (encendida)
    {
        g.setColour (color);
    }
    else
        g.setColour (juce::Colours::black);

    g.fillEllipse (boundsLuz);

    g.setColour (juce::Colours::black);
    g.drawEllipse (boundsLuz, 1.0f);

    if (encendida)
    {
        juce::ColourGradient brillo;
        brillo.point1 = boundsLuz.getCentre();
        brillo.addColour (0.0, color.withAlpha (0.3f));
        brillo.addColour (1.0, color.withLightness (1.5f).withAlpha (0.0f));
        brillo.isRadial = true;

        g.setGradientFill (brillo);
        g.fillEllipse (boundsLuz.expanded (delta));
    }
}

MedidorNivel::MedidorNivel (std::function<float()>&& funcionValores) : obtenerValores(std::move (funcionValores))
{
    startTimerHz(24);
}

MedidorNivel::~MedidorNivel()
{
    stopTimer();
}

void MedidorNivel::resized ()
{
    const auto bounds = getLocalBounds().toFloat();

    escala.point1 = bounds.getBottomLeft();
    escala.point2 = bounds.getTopLeft();
    escala.addColour (0.0, juce::Colour (0xFF00A0D2));
    escala.addColour (0.5, juce::Colour (0xFF00E5FF));
    escala.addColour (0.8, juce::Colour (0xFFD6F4FF));
    escala.addColour (1.0, juce::Colour (0xFFFFFFFF));
    escala.isRadial = false;

    auto boundsLuces = getLocalBounds();
    const auto alturaLuz = boundsLuces.getHeight() / totalLuces;
    luces.clear();

    for (int i = 0; i < totalLuces; ++i)
    {
        auto luz = std::make_unique<Luz> (escala.getColourAtPosition (static_cast<double>(i) / totalLuces));
        addAndMakeVisible (luz.get());
        luz->setBounds (boundsLuces.removeFromBottom (alturaLuz));
        luces.push_back (std::move (luz));
    }
}

void MedidorNivel::timerCallback()
{
    const auto level = juce::jmap (obtenerValores(), dBMin, dBMax, 0.0f, 1.0f);

    for (int i = 0; i < totalLuces; ++i)
        luces[i]->setState (static_cast<float>(i + 1) / totalLuces <= level);

    repaint();
}