#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Assets.h"

// Perilla analógica: anillo de escala + arco naranja dibujado por código + cuerpo metálico girado.
// Interacción: arrastre vertical, rueda, doble clic = valor por defecto (lo gestiona el attachment).
class AnalogKnob : public juce::Slider
{
public:
    struct Spec
    {
        const char* ringFile;
        const char* bodyFile;
        float hitRadius;      // radio sensible al ratón (px de diseño)
        float arcRadius;      // radio del arco naranja
        float arcWidth;       // grosor del núcleo del arco
        float bakedAngleRad;  // ángulo en que el cuerpo trae pintada la marca (0 = arriba)
    };

    static constexpr float kSweep = juce::MathConstants<float>::pi * 0.75f; // ±135°

    explicit AnalogKnob (const Spec& s)
        : spec (s), ring (Assets::get (s.ringFile)), body (Assets::get (s.bodyFile))
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        juce::Slider::RotaryParameters p;
        p.startAngleRadians = -kSweep;
        p.endAngleRadians   =  kSweep;
        p.stopAtEnd = true;
        setRotaryParameters (p);
        setMouseDragSensitivity (260);
    }

    bool hitTest (int x, int y) override
    {
        const float dx = (float) x - (float) getWidth()  * 0.5f;
        const float dy = (float) y - (float) getHeight() * 0.5f;
        return dx * dx + dy * dy <= spec.hitRadius * spec.hitRadius;
    }

    void paint (juce::Graphics& g) override
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        const float cx = (float) getWidth() * 0.5f, cy = (float) getHeight() * 0.5f;
        const float angle = -kSweep + (float) valueToProportionOfLength (getValue()) * 2.0f * kSweep;

        g.drawImageAt (ring, 0, 0);

        if (angle > -kSweep + 0.02f)
        {
            juce::Path arc;
            arc.addCentredArc (cx, cy, spec.arcRadius, spec.arcRadius, 0.0f, -kSweep, angle, true);
            const auto orange = juce::Colour (0xffff9a1a);
            g.setColour (orange.withAlpha (0.10f)); g.strokePath (arc, juce::PathStrokeType (spec.arcWidth * 3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (orange.withAlpha (0.25f)); g.strokePath (arc, juce::PathStrokeType (spec.arcWidth * 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (orange.brighter (0.2f));   g.strokePath (arc, juce::PathStrokeType (spec.arcWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.drawImageTransformed (body, juce::AffineTransform::rotation (angle - spec.bakedAngleRad, cx, cy));
    }

private:
    Spec spec;
    juce::Image ring, body;
};
