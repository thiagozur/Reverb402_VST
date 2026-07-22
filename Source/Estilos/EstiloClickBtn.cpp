#include "EstiloClickBtn.h"

void EstiloClickBtn::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    bool estaActivo = button.getToggleState();
    bool estaPresionado = estaActivo || shouldDrawButtonAsDown;

    g.setColour (juce::Colour (0xFF0D0D0D));
    g.fillRoundedRectangle (bounds, 3.0f);

    juce::ColourGradient gradienteFrame (juce::Colours::white.withAlpha (0.45f), bounds.getX(), bounds.getY(), juce::Colours::black.withAlpha (0.85f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (gradienteFrame);
    g.drawRoundedRectangle (bounds, 3.0f, 2.0f);

    auto tapaBounds = bounds.reduced (2.0f);
    float desplazamientoY = estaPresionado ? 0.5f : -2.5f;
    tapaBounds = tapaBounds.translated (0.0f, desplazamientoY);

    float largoSombra = estaPresionado ? 1.5f : 3.0f;
    float opacidadSombra = estaPresionado ? 0.6f : 0.85f;
    juce::ColourGradient shadeTapa (juce::Colours::black.withAlpha (opacidadSombra), tapaBounds.getCentreX(), tapaBounds.getBottom(), juce::Colours::transparentBlack, tapaBounds.getCentreX(), tapaBounds.getBottom() + largoSombra, false);
    g.setGradientFill (shadeTapa);
    g.fillRoundedRectangle (tapaBounds.expanded (0.5f, 0.0f).translated (0.0f, 1.0f), 2.5f);

    juce::Colour colorTapa = juce::Colour (0xFF1E1E1E); 
    g.setColour (colorTapa);
    g.fillRoundedRectangle (tapaBounds, 2.5f);

    float brilloSuperior = estaPresionado ? 0.35f : 0.45f;
    juce::ColourGradient shadeBisel (juce::Colours::white.withAlpha (brilloSuperior), tapaBounds.getX(), tapaBounds.getY(), juce::Colours::black.withAlpha (0.65f), tapaBounds.getX(), tapaBounds.getBottom(), false);
    g.setGradientFill (shadeBisel);
    g.drawRoundedRectangle (tapaBounds, 2.5f, 1.2f);
}