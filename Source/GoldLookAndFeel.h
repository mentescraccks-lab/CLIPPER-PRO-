#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace gold
{
    const juce::Colour goldMain   { 0xfff2b01e };
    const juce::Colour goldBright { 0xffffd56b };
    const juce::Colour goldDim    { 0xff8a5f10 };
    const juce::Colour goldDeep   { 0xff3a2a0c };
    const juce::Colour bgDark     { 0xff0c0c0d };
    const juce::Colour panelTop   { 0xff202022 };
    const juce::Colour panelBot   { 0xff0f0f10 };
    const juce::Colour textLight  { 0xffe9e4d8 };

    inline juce::Font makeFont (float height, bool bold = false, float kerning = 0.0f)
    {
        auto opts = juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain);
        if (kerning > 0.0f)
            opts = opts.withKerningFactor (kerning);
        return juce::Font (opts);
    }
}

class GoldLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GoldLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId, gold::goldMain);
    }

    //==========================================================================
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider&) override
    {
        const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float size = juce::jmin (b.getWidth(), b.getHeight());
        const auto c = b.getCentre();

        const float outerR = size * 0.5f - 1.0f;
        const float ringR  = outerR * 0.84f;
        const float knobR  = ringR * 0.84f;
        const float angle  = startA + pos * (endA - startA);
        const float twoPi  = juce::MathConstants<float>::twoPi;

        auto pt = [&c] (float r, float a) { return juce::Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };

        // marcas finas alrededor (61 marcas, mayores cada 10)
        const int nTicks = 61;
        for (int i = 0; i < nTicks; ++i)
        {
            const float a = startA + (float) i / (float) (nTicks - 1) * (endA - startA);
            const bool major = (i % 10) == 0;
            const bool mid = (i % 5) == 0;
            g.setColour (gold::goldMain.withAlpha (major ? 0.95f : (mid ? 0.7f : 0.45f)));
            g.drawLine (juce::Line<float> (pt (outerR * (major ? 0.88f : (mid ? 0.92f : 0.95f)), a), pt (outerR, a)),
                        major ? 1.8f : (mid ? 1.2f : 0.9f));
        }

        // anillo dorado completo
        const float ringW = juce::jmax (3.0f, ringR * 0.11f);
        juce::Path track;
        track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startA, endA, true);
        g.setColour (juce::Colour (0xff7a520e));
        g.strokePath (track, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xffb98418).withAlpha (0.7f));
        g.strokePath (track, juce::PathStrokeType (ringW * 0.45f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // arco de valor con brillo
        juce::Path val;
        val.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startA, juce::jmax (startA + 0.001f, angle), true);
        g.setColour (gold::goldMain.withAlpha (0.25f));
        g.strokePath (val, juce::PathStrokeType (ringW * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (gold::goldMain);
        g.strokePath (val, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (gold::goldBright.withAlpha (0.8f));
        g.strokePath (val, juce::PathStrokeType (ringW * 0.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // cuerpo metálico cepillado (reflejos anisotrópicos tipo cónico)
        g.saveState();
        juce::Path circ;
        circ.addEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);
        g.reduceClipRegion (circ, juce::AffineTransform());

        const int wedges = 72;
        for (int i = 0; i < wedges; ++i)
        {
            const float a0 = (float) i * twoPi / (float) wedges;
            const float a1 = (float) (i + 1) * twoPi / (float) wedges + 0.02f;
            const float am = 0.5f * (a0 + a1);
            float l = 0.5f + 0.5f * std::cos (2.0f * (am - 0.7f));
            l *= l;
            const float br = 0.10f + 0.52f * l;
            g.setColour (juce::Colour::fromFloatRGBA (br * 0.97f, br, br * 1.03f, 1.0f));

            juce::Path wedge;
            wedge.startNewSubPath (c);
            wedge.lineTo (pt (knobR * 1.1f, a0));
            wedge.lineTo (pt (knobR * 1.1f, a1));
            wedge.closeSubPath();
            g.fillPath (wedge);
        }

        juce::ColourGradient vg (juce::Colours::transparentBlack, c.x, c.y,
                                 juce::Colours::black.withAlpha (0.6f), c.x + knobR, c.y, true);
        vg.addColour (0.62, juce::Colours::transparentBlack);
        g.setGradientFill (vg);
        g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);

        // casquete central con anillos concéntricos
        const float capR = knobR * 0.62f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3b3f).withAlpha (0.85f), c.x - capR * 0.5f, c.y - capR * 0.6f,
                                                 juce::Colour (0xff09090a).withAlpha (0.9f), c.x + capR, c.y + capR, true));
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        for (float r = capR * 0.2f; r < capR; r += capR * 0.14f)
            g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.7f);
        g.restoreState();

        // moleteado del borde
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        for (int i = 0; i < 72; ++i)
        {
            const float a = (float) i * twoPi / 72.0f;
            g.drawLine (juce::Line<float> (pt (knobR * 0.91f, a), pt (knobR * 0.99f, a)), 1.0f);
        }

        // borde
        g.setColour (juce::Colours::black.withAlpha (0.95f));
        g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 2.2f);
        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawEllipse (c.x - knobR + 2.0f, c.y - knobR + 2.0f, knobR * 2.0f - 4.0f, knobR * 2.0f - 4.0f, 1.0f);

        // indicador
        g.setColour (gold::goldMain.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (pt (knobR * 0.40f, angle), pt (knobR * 0.96f, angle)), juce::jmax (4.0f, knobR * 0.12f));
        g.setColour (gold::goldBright);
        g.drawLine (juce::Line<float> (pt (knobR * 0.40f, angle), pt (knobR * 0.96f, angle)), juce::jmax (2.0f, knobR * 0.06f));
    }

    //==========================================================================
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                           float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cy = (float) y + (float) h * 0.5f;
        const float grooveH = juce::jmax (4.0f, (float) h * 0.14f);
        auto groove = juce::Rectangle<float> ((float) x, cy - grooveH * 0.5f, (float) w, grooveH);

        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (groove, grooveH * 0.5f);
        g.setColour (juce::Colour (0xff3a3a3d));
        g.drawRoundedRectangle (groove.expanded (0.5f), grooveH * 0.5f, 1.0f);

        auto filled = groove.withRight (sliderPos);
        g.setColour (gold::goldDim);
        g.fillRoundedRectangle (filled, grooveH * 0.5f);

        const float tr = juce::jmin ((float) h * 0.5f, 18.0f);
        juce::ColourGradient grad (juce::Colour (0xff9a9ca0), sliderPos - tr * 0.4f, cy - tr * 0.5f,
                                   juce::Colour (0xff18181a), sliderPos + tr * 0.8f, cy + tr * 0.9f, true);
        g.setGradientFill (grad);
        g.fillEllipse (sliderPos - tr, cy - tr, tr * 2.0f, tr * 2.0f);
        g.setColour (gold::goldMain);
        g.drawEllipse (sliderPos - tr, cy - tr, tr * 2.0f, tr * 2.0f, 1.6f);
        g.setColour (gold::goldBright);
        g.drawLine (sliderPos, cy - tr * 0.55f, sliderPos, cy + tr * 0.55f, 1.6f);
    }

    int getSliderThumbRadius (juce::Slider&) override { return 14; }

    //==========================================================================
    void drawLabel (juce::Graphics& g, juce::Label& l) override
    {
        const auto b = l.getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff09090a));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (juce::Colour (0xff4f3a12));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

        g.setColour (gold::textLight);
        g.setFont (gold::makeFont (b.getHeight() * 0.58f, true));
        g.drawText (l.getText(), b, juce::Justification::centred, false);
    }
};
