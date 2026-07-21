#include "EstiloPushBtn.h"

void EstiloPushBtn::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto radioCurvatura = 3.0f;
    bool estaActivo = button.getToggleState();

    auto colorLed = button.findColour (colorLedActivoID, true);
    if (colorLed == juce::Colour())
        colorLed = juce::Colour (0xFFB0E298);

    if (estaActivo)
    {
        g.setColour (juce::Colour (0xFF101012));
        g.fillRoundedRectangle (bounds, radioCurvatura);

        auto areaHundida = bounds.withTrimmedTop (2.0f);

        g.setColour (juce::Colour (0xFF191A1D));
        g.fillRoundedRectangle (areaHundida, radioCurvatura);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (areaHundida, radioCurvatura, 1.0f);

        g.setColour (colorLed);
        g.fillRect (areaHundida.getX() + 5, areaHundida.getY() + 2, areaHundida.getWidth() - 10, 2.0f);
        
        g.setColour (colorLed.withAlpha (0.15f));
        g.fillRect (areaHundida.getX() + 5, areaHundida.getY() + 4, areaHundida.getWidth() - 10, 3.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xFF141416));
        g.fillRoundedRectangle (bounds.withTrimmedTop (bounds.getHeight() - 4.0f), radioCurvatura);

        auto cuerpoBoton = bounds.withTrimmedBottom (3.0f);
        
        g.setColour (juce::Colour (0xFF2D2F35));
        g.fillRoundedRectangle (cuerpoBoton, radioCurvatura);

        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawHorizontalLine (0, cuerpoBoton.getX(), cuerpoBoton.getRight());

        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawHorizontalLine (static_cast<int>(cuerpoBoton.getHeight() - 1), cuerpoBoton.getX(), cuerpoBoton.getRight());

        g.setColour (juce::Colour (0xFF18181B));
        g.drawRoundedRectangle (cuerpoBoton, radioCurvatura, 1.0f);
    }
}

void EstiloPushBtn::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds();
    bool estaActivo = button.getToggleState();

    g.setColour (estaActivo ? juce::Colour (0xFFEAEAEA) : juce::Colour (0xFF8E9095));
    g.setFont (juce::Font ("Arial", 11.0f, juce::Font::bold));
    
    int desplazamientoY = estaActivo ? 2 : -1;

    g.drawText (button.getButtonText(), bounds.getX(), bounds.getY() + desplazamientoY, bounds.getWidth(), bounds.getHeight(), juce::Justification::centred);
}