#include "Estilo402.h"

Estilo402::Estilo402()
    {
        setColour (juce::Slider::thumbColourId, juce::Colour (0xFF00A0D2));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00A0D2));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF2A2A30));

        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF111113));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF535660));
        setColour (juce::ComboBox::focusedOutlineColourId, juce::Colour (0xFF00A0D2));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (0xFF00A0D2));
        setColour (juce::ComboBox::textColourId, juce::Colours::white.withAlpha (0.9f));

        setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111113));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF00A0D2));
        setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.8f));
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);

        setColour (juce::Label::textColourId, juce::Colour (0xFF1A1A1E));
    }

    void Estilo402::drawLabel (juce::Graphics& g, juce::Label& label)
    {
        g.fillAll (label.findColour (juce::Label::backgroundColourId));

        if (! label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const juce::Font font ("Arial", 18.0f, juce::Font::bold);

            g.setFont (font);
            g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));

            auto area = label.getLocalBounds().toFloat();
            g.drawFittedText (label.getText(), area.getSmallestIntegerContainer(), juce::Justification::centred, 2);
        }
    }

    void Estilo402::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto cornerSize = 4.0f;

        auto baseColour = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId) : button.findColour (juce::TextButton::buttonColourId);

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker (0.4f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter (0.15f);

        if (! button.isEnabled())
            baseColour = baseColour.withAlpha (0.35f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds.reduced (2.0f), cornerSize);

        auto outlineColour = button.isEnabled() ? (shouldDrawButtonAsHighlighted ? juce::Colour (0xFF535660).brighter (0.15f) : juce::Colour (0xFF535660)) : juce::Colour (0xFF535660).withAlpha (0.2f);

        g.setColour (outlineColour);
        g.drawRoundedRectangle (bounds.reduced (2.0f), cornerSize, 1.0f);
    }

    void Estilo402::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto area = button.getLocalBounds().toFloat();
        
        const juce::Font font ("Arial", 18.0f, juce::Font::bold);
        g.setFont (font);

        auto alpha = button.isEnabled() ? 1.0f : 0.5f;
        g.setColour (button.findColour (juce::ComboBox::textColourId).withMultipliedAlpha (alpha));

        g.drawFittedText (button.getButtonText(), area.getSmallestIntegerContainer(), juce::Justification::centred, 2);
    }

    void Estilo402::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
    {
        auto bounds = juce::Rectangle<float> (x, y, width, height).reduced (6.0f);
        auto radio = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centroX = bounds.getCentreX();
        auto centroY = bounds.getCentreY();

        auto anguloActual = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto grosorLinea = 3.5f;

        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        juce::Path fondoCamino;
        fondoCamino.addCentredArc (centroX, centroY, radio - grosorLinea, radio - grosorLinea, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.strokePath (fondoCamino, juce::PathStrokeType (grosorLinea, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
            juce::Path valorCamino;
            valorCamino.addCentredArc (centroX, centroY, radio - grosorLinea, radio - grosorLinea, 0.0f, rotaryStartAngle, anguloActual, true);
            g.strokePath (valorCamino, juce::PathStrokeType (grosorLinea, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        float radioDial = radio - grosorLinea * 2.5f;

        float radioSombra = radioDial + 0.2f * radioDial;

        float sombraCentroX = centroX;
        float sombraCentroY = centroY + 0.2f * radioDial;

        juce::ColourGradient gradienteSombra (juce::Colours::black.withAlpha (0.65f), sombraCentroX, sombraCentroY, juce::Colours::transparentBlack, sombraCentroX + radioSombra, sombraCentroY, true);

        g.setGradientFill (gradienteSombra);
        g.fillEllipse (sombraCentroX - radioSombra, sombraCentroY - radioSombra, radioSombra * 2.0f, radioSombra * 2.0f);

        juce::Colour colorLuzSuperior = juce::Colour (0xFF535660);
        juce::Colour colorSombraInferior = juce::Colour (0xFF111113);

        juce::ColourGradient gradienteBisel (colorLuzSuperior, centroX, centroY - radioDial, colorSombraInferior, centroX, centroY + radioDial, false);
        g.setGradientFill (gradienteBisel);
        g.fillEllipse (centroX - radioDial, centroY - radioDial, radioDial * 2.0f, radioDial * 2.0f);

        float radioCuerpoInterno = radioDial - 5.0f;

        g.setColour (juce::Colour (0xFF1F2024));
        g.fillEllipse (centroX - radioCuerpoInterno, centroY - radioCuerpoInterno, radioCuerpoInterno * 2.0f, radioCuerpoInterno * 2.0f);

        if (slider.isEnabled())
        {
            juce::Path aguja;

            float largoAguja = radioCuerpoInterno - 7.5f;
            float inicioAguja = radioCuerpoInterno * 0.25f;

            float x1 = centroX + inicioAguja * std::sin (anguloActual);
            float y1 = centroY - inicioAguja * std::cos (anguloActual);
            float x2 = centroX + largoAguja * std::sin (anguloActual);
            float y2 = centroY - largoAguja * std::cos (anguloActual);

            aguja.startNewSubPath (x1, y1);
            aguja.lineTo (x2, y2);

            g.setColour (juce::Colours::white);
            g.strokePath (aguja, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    void Estilo402::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
    {
        auto cornerSize = 4.0f;
        auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds.reduced (2.0f), cornerSize);

        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds.reduced (2.0f), cornerSize, 1.0f);

        juce::Path arrow;
        float arrowW = 8.0f;
        float arrowH = 4.0f;
        float centerX = width - 18.0f;
        float centerY = height / 2.0f;

        arrow.startNewSubPath (centerX - arrowW / 2.0f, centerY - arrowH / 2.0f);
        arrow.lineTo (centerX + arrowW / 2.0f, centerY - arrowH /2.0f);
        arrow.lineTo (centerX, centerY + arrowH / 2.0f);
        arrow.closeSubPath();

        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 1.0f : 0.3f));
        g.fillPath (arrow);
    }