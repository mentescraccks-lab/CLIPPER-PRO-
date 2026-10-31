#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Assets.h"

// Botón con imagen para estado apagado / encendido y una capa opcional encima (etiqueta).
class ImageStateButton : public juce::Button
{
public:
    explicit ImageStateButton (const juce::String& name) : juce::Button (name) {}

    void setImages (const juce::Image& offImage, const juce::Image& onImage, const juce::Image& overlayImage = {})
    {
        off = offImage; on = onImage; overlay = overlayImage;
    }

    void paintButton (juce::Graphics& g, bool isOver, bool isDown) override
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        const auto r = getLocalBounds().toFloat();
        g.drawImage (getToggleState() ? on : off, r, juce::RectanglePlacement::stretchToFit);
        if (overlay.isValid())
            g.drawImage (overlay, r, juce::RectanglePlacement::stretchToFit);
        if (isDown)      { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (r.reduced (3.0f), 6.0f); }
        else if (isOver) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (r.reduced (3.0f), 6.0f); }
    }

private:
    juce::Image off, on, overlay;
};

// Botón grande oscuro con un LED aparte que muestra su estado (LINK L/R, BYPASS).
class LedPushButton : public juce::Button
{
public:
    explicit LedPushButton (const juce::String& name) : juce::Button (name) { setClickingTogglesState (true); }

    // Rectángulos en coordenadas de diseño; el componente ocupa la unión de ambos.
    void setup (const juce::Image& buttonImage, const juce::Image& ledOnImage, const juce::Image& ledOffImage,
                juce::Rectangle<int> buttonRect, juce::Rectangle<int> ledRect)
    {
        btn = buttonImage; ledOn = ledOnImage; ledOff = ledOffImage;
        const auto all = buttonRect.getUnion (ledRect);
        origin = all.getPosition();
        btnLocal = buttonRect.translated (-origin.x, -origin.y);
        ledLocal = ledRect.translated (-origin.x, -origin.y);
        setBounds (all);
    }

    void paintButton (juce::Graphics& g, bool isOver, bool isDown) override
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (btn, btnLocal.toFloat(), juce::RectanglePlacement::stretchToFit);
        g.drawImage (getToggleState() ? ledOn : ledOff, ledLocal.toFloat(), juce::RectanglePlacement::stretchToFit);
        if (isDown)      { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (btnLocal.toFloat().reduced (4.0f), 6.0f); }
        else if (isOver) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (btnLocal.toFloat().reduced (4.0f), 6.0f); }
    }

    bool hitTest (int x, int y) override { return btnLocal.contains (x, y) || ledLocal.contains (x, y); }

private:
    juce::Image btn, ledOn, ledOff;
    juce::Point<int> origin;
    juce::Rectangle<int> btnLocal, ledLocal;
};

// Campo numérico naranja sobre la "pastilla". Doble clic para escribir un valor.
class ValueReadout : public juce::Label
{
public:
    ValueReadout (juce::Slider& s, float fontHeight) : slider (s)
    {
        setJustificationType (juce::Justification::centred);
        setFont (juce::Font (juce::FontOptions (fontHeight, juce::Font::bold)));
        setColour (juce::Label::textColourId, juce::Colour (0xffff9d1c));
        setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff1a1208));
        setColour (juce::Label::textWhenEditingColourId, juce::Colour (0xffffb347));
        setColour (juce::Label::outlineWhenEditingColourId, juce::Colour (0xffff9d1c));
        setEditable (false, true, false);

        slider.onValueChange = [this] { refresh(); };
        onTextChange = [this]
        {
            const auto digits = getText().retainCharacters ("-+.,0123456789").replaceCharacter (',', '.');
            if (digits.containsAnyOf ("0123456789"))
                slider.setValue (digits.getDoubleValue(), juce::sendNotificationSync);
            refresh();
        };
    }

    void refresh()
    {
        auto v = slider.getValue();
        if (std::abs (v) < 0.05) v = 0.0; // evita "-0.0"
        setText (juce::String (v, 1) + " dB", juce::dontSendNotification);
    }

private:
    juce::Slider& slider;
};
