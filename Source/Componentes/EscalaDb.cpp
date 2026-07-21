#include "EscalaDb.h"

EscalaDb::EscalaDb (float dBMin, float dBMax, int totalLuces, std::vector<float> valoresAMostrar) : dBMinimo (dBMin), dBMaximo (dBMax), totalLuces (totalLuces), valores (std::move (valoresAMostrar))
{
    auto chivoMonoTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::ChivoMonoSemiBold_ttf, BinaryData::ChivoMonoSemiBold_ttfSize);
    chivoMono = juce::Font (chivoMonoTypeface);
    chivoMono.setHeight (18.0f);
}

EscalaDb::~EscalaDb()
{

}

void EscalaDb::paint (juce::Graphics& g)
{
    auto alturaComponente = static_cast<float>(getHeight());
    auto alturaLuz = alturaComponente / static_cast<float>(totalLuces);

    g.setFont (chivoMono);

    std::vector<float> valoresOrdenados = valores;
    std::sort (valoresOrdenados.begin(), valoresOrdenados.end());

    std::set<int> indicesYaUsados;

    for (float db : valoresOrdenados)
    {
        float level = juce::jmap (db, dBMinimo, dBMaximo, 0.0f, 1.0f);

        int indiceLuz = juce::jlimit (0, totalLuces - 1, static_cast<int>(level * totalLuces));
        if (indicesYaUsados.count (indiceLuz) > 0)
            continue;
        indicesYaUsados.insert(indiceLuz);

        float y = alturaComponente - (indiceLuz + 0.5f) * alturaLuz;

        juce::String texto = (db > 0 ? "+" : "") + juce::String (static_cast<int>(db));

        g.setColour (juce::Colours::black);
        g.drawText (texto, 0, static_cast<int>(y) - 7, getWidth(), 12, juce::Justification::centred);

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawText (texto, 0, static_cast<int>(y) - 5, getWidth(), 12, juce::Justification::centred);

        g.setColour (juce::Colour (0xFFEAEAEA).darker (0.4));
        g.drawText (texto, 0, static_cast<int>(y) - 6, getWidth(), 12, juce::Justification::centred);
    }
}