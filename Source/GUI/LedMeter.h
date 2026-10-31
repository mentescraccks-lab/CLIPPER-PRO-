#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Assets.h"

// Medidor LED segmentado. Cada segmento se recorta de la imagen de la capa y se enciende o apaga
// según el nivel real recibido (setLit). No contiene animaciones ni valores ficticios.
class LedMeter : public juce::Component
{
public:
    struct Spec
    {
        const char* imageFile;
        int   count;       // nº de segmentos
        float startX;      // x del primer segmento dentro de la imagen
        float pitch;       // separación entre segmentos
        bool  darkenOff;   // apagado = oscurecer el segmento encendido de la imagen
        int   litSrcMax;   // si >= 0: índice máximo del segmento de origen para los encendidos
        int   offSrcIndex; // si >= 0: segmento de origen usado para los apagados
    };

    explicit LedMeter (const Spec& s) : spec (s), img (Assets::get (s.imageFile))
    {
        setInterceptsMouseClicks (false, false);
    }

    void setLit (int n)
    {
        n = juce::jlimit (0, spec.count, n);
        if (n != lit) { lit = n; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        const int h = getHeight();
        for (int i = 0; i < spec.count; ++i)
        {
            const bool on = i < lit;
            const int srcIdx = on ? (spec.litSrcMax >= 0 ? juce::jmin (i, spec.litSrcMax) : i)
                                  : (spec.offSrcIndex >= 0 ? spec.offSrcIndex : i);
            const float srcX = spec.startX + (float) srcIdx * spec.pitch;
            const float dstX = spec.startX + (float) i * spec.pitch;
            const int x0 = juce::roundToInt (dstX);
            const int x1 = juce::roundToInt (dstX + spec.pitch);
            const juce::Rectangle<int> dst (x0, 0, x1 - x0, h);

            g.saveState();
            g.reduceClipRegion (dst);
            g.drawImageAt (img, juce::roundToInt (dstX - srcX), 0);
            if (! on && spec.darkenOff)
            {
                g.setColour (juce::Colours::black.withAlpha (0.80f));
                g.fillRect (dst);
            }
            g.restoreState();
        }
    }

private:
    Spec spec;
    juce::Image img;
    int lit = 0;
};
