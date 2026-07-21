#include "EstiloKnobGain.h"

EstiloKnobGain::EstiloKnobGain()
{
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00E5FF));

    auto chivoMonoTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::ChivoMonoSemiBold_ttf, BinaryData::ChivoMonoSemiBold_ttfSize);
    chivoMono = juce::Font (chivoMonoTypeface);
    chivoMono.setHeight (18.0f);
}

void EstiloKnobGain::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (x, y, width, height).reduced (10.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreY = bounds.getCentreY();
    auto centreX = bounds.getCentreX();

    const int numTicks = 9;
    const float tickRadiusOuter = radius + 2.0f;
    const float tickRadiusInner = radius - 4.0f;

    g.setColour (juce::Colours::white.withAlpha (0.6f));

    for (int i = 0; i < numTicks; ++i)
    {
        float angle = rotaryStartAngle + (i / static_cast<float>(numTicks - 1)) * (rotaryEndAngle - rotaryStartAngle);

        auto outerPoint = juce::Point<float> (centreX + tickRadiusOuter * std::sin (angle), centreY - tickRadiusOuter * std::cos (angle));
        auto innerPoint = juce::Point<float> (centreX + tickRadiusInner * std::sin (angle), centreY - tickRadiusInner * std::cos (angle));

        g.drawLine (juce::Line<float> (innerPoint, outerPoint), 1.5f);
    }

    g.setFont (chivoMono);

    auto minPoint = juce::Point<float> (centreX + (radius + 12.0f) * std::sin (rotaryStartAngle), centreY - (radius + 12.0f) * std::cos (rotaryStartAngle));
    g.setColour (juce::Colours::black);
    g.drawText ("-12", juce::Rectangle<float> (22.0f, 12.0f).withCentre (minPoint).translated (0.0f, -1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("-12", juce::Rectangle<float> (22.0f, 12.0f).withCentre (minPoint).translated (0.0f, 1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
    g.drawText ("-12", juce::Rectangle<float> (22.0f, 12.0f).withCentre (minPoint), juce::Justification::centred, false);
    
    auto maxPoint = juce::Point<float> (centreX + (radius + 12.0f) * std::sin (rotaryEndAngle), centreY - (radius + 12.0f) * std::cos (rotaryEndAngle));
    g.setColour (juce::Colours::black);
    g.drawText ("+12", juce::Rectangle<float> (22.0f, 12.0f).withCentre (maxPoint).translated (0.0f, -1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("+12", juce::Rectangle<float> (22.0f, 12.0f).withCentre (maxPoint).translated (0.0f, 1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
    g.drawText ("+12", juce::Rectangle<float> (22.0f, 12.0f).withCentre (maxPoint), juce::Justification::centred, false);

    float zeroAngle = rotaryStartAngle + 0.5f * (rotaryEndAngle - rotaryStartAngle);
    auto zeroPoint = juce::Point<float> (centreX + (radius + 12.0f) * std::sin (zeroAngle), centreY - (radius + 12.0f) * std::cos (zeroAngle));
    g.setColour (juce::Colours::black);
    g.drawText ("0", juce::Rectangle<float> (16.0f, 12.0f).withCentre (zeroPoint).translated (0.0f, -1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("0", juce::Rectangle<float> (16.0f, 12.0f).withCentre (zeroPoint).translated (0.0f, 1.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
    g.drawText ("0", juce::Rectangle<float> (16.0f, 12.0f).withCentre (zeroPoint), juce::Justification::centred, false);

    auto knobRadius = radius - 8.0f;
    auto knobBounds = juce::Rectangle<float> (centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    juce::ColourGradient shadeOclusion (juce::Colours::black.withAlpha (0.35f), { centreX, centreY }, juce::Colours::transparentBlack, { centreX + knobRadius + 6.0f, centreY }, true);
    g.setGradientFill (shadeOclusion);
    g.fillEllipse (centreX - (knobRadius + 4.0f), centreY - (knobRadius + 4.0f), (knobRadius + 4.0f) * 2.0f, (knobRadius + 4.0f) * 2.0f);

    float shadowOffsetY = 2.5f;
    juce::ColourGradient shadeInferior (juce::Colours::black.withAlpha (0.45f), { centreX, centreY + shadowOffsetY }, juce::Colours::transparentBlack, { centreX + knobRadius + 8.0f, centreY + shadowOffsetY }, true);
    g.setGradientFill (shadeInferior);
    g.fillEllipse (centreX - (knobRadius + 5.0f), centreY + shadowOffsetY - (knobRadius + 5.0f), (knobRadius + 5.0f) * 2.0f, (knobRadius + 5.0f) * 2.0f);

    g.setColour (juce::Colour (0xff282828));
    g.fillEllipse (knobBounds);

    juce::ColourGradient borderGrad (juce::Colours::white.withAlpha (0.45f), centreX, centreY - knobRadius, juce::Colours::black.withAlpha (0.60f), centreX, centreY + knobRadius, false);
    g.setGradientFill (borderGrad);
    g.drawEllipse (knobBounds, 1.2f);

    float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
    juce::Path cursor;
    auto cursorLength = knobRadius * 0.55f;
    auto cursorThickness = 2.0f;
    auto overhang = 2.0f;

    cursor.addRectangle (-cursorThickness * 0.5f, -knobRadius - overhang, cursorThickness, cursorLength + overhang);
    cursor.applyTransform (juce::AffineTransform::rotation (currentAngle).translated (centreX, centreY));

    g.setColour (juce::Colours::white);
    g.fillPath (cursor);
}