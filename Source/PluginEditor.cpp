#include "PluginEditor.h"

namespace
{
    // Todo el diseño está hecho sobre un lienzo de referencia de 1670 x 941 px
    // (el tamaño de tu imagen) y se escala al tamaño real de la ventana.
    constexpr float refW = 1670.0f;
    constexpr float refH = 941.0f;

    struct KnobSpecData
    {
        const char* id;
        float cx, cy, R;       // centro y radio del slider (incluye marcas)
        float roCy, roW, roH;  // lectura numérica
        bool pct;
    };

    const KnobSpecData kSpecs[] =
    {
        { ParamIDs::inputGain, 170.0f,  610.0f, 90.0f, 502.0f, 92.0f, 32.0f, false },
        { ParamIDs::ceiling,   414.0f,  555.0f, 78.0f, 671.0f, 84.0f, 30.0f, false },
        { ParamIDs::knee,      596.0f,  555.0f, 78.0f, 671.0f, 84.0f, 30.0f, true  },
        { ParamIDs::gold1,     774.0f,  555.0f, 78.0f, 671.0f, 84.0f, 30.0f, true  },
        { ParamIDs::gold2,     948.0f,  555.0f, 78.0f, 671.0f, 84.0f, 30.0f, true  },
        { ParamIDs::alchemy,  1121.0f,  555.0f, 78.0f, 671.0f, 84.0f, 30.0f, true  },
        { ParamIDs::boxTone,  1293.0f,  555.0f, 78.0f, 671.0f, 84.0f, 30.0f, true  },
        { ParamIDs::mix,       507.0f,  795.0f, 40.0f, 848.0f, 76.0f, 26.0f, true  },
        { ParamIDs::ceilTrim, 1212.0f,  793.0f, 52.0f, 863.0f, 80.0f, 24.0f, false },
        { ParamIDs::output,   1493.0f,  780.0f, 68.0f, 863.0f, 76.0f, 26.0f, false },
        { ParamIDs::trimIn,   1457.0f,  392.0f, 50.0f, 461.0f, 80.0f, 26.0f, false },
        { ParamIDs::trimOut,  1571.0f,  392.0f, 50.0f, 461.0f, 80.0f, 26.0f, false },
    };

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float corner = 8.0f, bool inset = false)
    {
        g.setGradientFill (juce::ColourGradient (inset ? juce::Colour (0xff141415) : juce::Colour (0xff252527), r.getX(), r.getY(),
                                                 inset ? juce::Colour (0xff070708) : juce::Colour (0xff0d0d0e), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);

        // reflejos especulares verticales (solo paneles grandes)
        if (! inset && r.getWidth() > 150.0f)
        {
            juce::ColourGradient sheen (juce::Colours::transparentWhite, r.getX(), 0.0f,
                                        juce::Colours::transparentWhite, r.getRight(), 0.0f, false);
            sheen.addColour (0.18, juce::Colours::white.withAlpha (0.055f));
            sheen.addColour (0.30, juce::Colours::transparentWhite);
            sheen.addColour (0.70, gold::goldMain.withAlpha (0.045f));
            sheen.addColour (0.82, juce::Colours::transparentWhite);
            g.setGradientFill (sheen);
            g.fillRoundedRectangle (r, corner);
        }

        // metal cepillado: trazos horizontales aleatorios (semilla fija)
        juce::Random rnd ((int) (r.getX() * 31.0f + r.getY() * 17.0f + r.getWidth()));
        const int n = (int) (r.getWidth() * r.getHeight() / 450.0f);
        for (int i = 0; i < n; ++i)
        {
            const float x = r.getX() + 3.0f + rnd.nextFloat() * (r.getWidth() - 6.0f);
            const float y = r.getY() + 3.0f + rnd.nextFloat() * (r.getHeight() - 6.0f);
            const float x2 = juce::jmin (x + 15.0f + rnd.nextFloat() * 110.0f, r.getRight() - 3.0f);
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xffd9a441)).withAlpha (0.015f + rnd.nextFloat() * 0.04f));
            g.drawHorizontalLine ((int) y, x, x2);
        }

        // biseles
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, corner, 2.2f);
        g.setColour (gold::goldDeep);
        g.drawRoundedRectangle (r.reduced (2.5f), juce::jmax (1.0f, corner - 1.0f), 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.drawLine (r.getX() + corner, r.getY() + 1.2f, r.getRight() - corner, r.getY() + 1.2f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (r.getX() + corner, r.getBottom() - 3.5f, r.getRight() - corner, r.getBottom() - 3.5f, 1.0f);
    }

    void drawScrew (juce::Graphics& g, float x, float y)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a7a4a), x - 5.0f, y - 5.0f,
                                                 juce::Colour (0xff1a1608), x + 9.0f, y + 9.0f, true));
        g.fillEllipse (x - 13.0f, y - 13.0f, 26.0f, 26.0f);
        g.setColour (juce::Colours::black);
        g.drawEllipse (x - 13.0f, y - 13.0f, 26.0f, 26.0f, 2.0f);
        g.setColour (juce::Colour (0xff241c08));
        g.drawLine (x - 7.0f, y - 3.0f, x + 7.0f, y + 3.0f, 3.0f);
    }
}

//==============================================================================
AndresBeatsClipAudioProcessorEditor::AndresBeatsClipAudioProcessorEditor (AndresBeatsClipAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < kNumKnobs; ++i)
        setupKnob (i);

    // ---- High Air Shimmer (slider horizontal) ----
    shimmerSlider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " %"; };
    shimmerSlider.onValueChange = [this]
    {
        shimmerReadout.setText (shimmerSlider.getTextFromValue (shimmerSlider.getValue()), juce::dontSendNotification);
    };
    shimmerReadout.setJustificationType (juce::Justification::centred);
    shimmerReadout.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (shimmerSlider);
    addAndMakeVisible (shimmerReadout);
    shimmerAtt = std::make_unique<SliderAttachment> (proc.apvts, ParamIDs::shimmer, shimmerSlider);
    shimmerSlider.setDoubleClickReturnValue (true, 18.0);
    shimmerSlider.onValueChange();

    // ---- Botones ----
    for (auto* b : std::initializer_list<GoldButton*> { &os1, &os2, &os4, &modeModern, &modeClassic, &modeHard,
                                                         &linkBtn, &unityBtn, &powerBtn, &parallelBtn })
        addAndMakeVisible (*b);

    for (auto* b : std::initializer_list<GoldButton*> { &linkBtn, &unityBtn, &powerBtn, &parallelBtn })
        b->setClickingTogglesState (true);

    parallelBtn.onStateChange = [this] { refreshDimming(); };
    linkBtn.onStateChange     = [this] { refreshDimming(); };

    linkAtt     = std::make_unique<ButtonAttachment> (proc.apvts, ParamIDs::trimLink, linkBtn);
    unityAtt    = std::make_unique<ButtonAttachment> (proc.apvts, ParamIDs::unityGain, unityBtn);
    powerAtt    = std::make_unique<ButtonAttachment> (proc.apvts, ParamIDs::power, powerBtn);
    parallelAtt = std::make_unique<ButtonAttachment> (proc.apvts, ParamIDs::trueParallel, parallelBtn);

    // Oversampling (1x / 2x / 4x)
    osAtt = std::make_unique<juce::ParameterAttachment> (
        *proc.apvts.getParameter (ParamIDs::oversampling),
        [this] (float v)
        {
            const int i = (int) std::lround (v);
            os1.setToggleState (i == 0, juce::dontSendNotification);
            os2.setToggleState (i == 1, juce::dontSendNotification);
            os4.setToggleState (i == 2, juce::dontSendNotification);
        },
        nullptr);
    osAtt->sendInitialUpdate();
    os1.onClick = [this] { osAtt->setValueAsCompleteGesture (0.0f); };
    os2.onClick = [this] { osAtt->setValueAsCompleteGesture (1.0f); };
    os4.onClick = [this] { osAtt->setValueAsCompleteGesture (2.0f); };

    // Modo de clip (Modern / Classic / Hard)
    modeAtt = std::make_unique<juce::ParameterAttachment> (
        *proc.apvts.getParameter (ParamIDs::clipMode),
        [this] (float v)
        {
            const int i = (int) std::lround (v);
            modeModern.setToggleState (i == 0, juce::dontSendNotification);
            modeClassic.setToggleState (i == 1, juce::dontSendNotification);
            modeHard.setToggleState (i == 2, juce::dontSendNotification);
        },
        nullptr);
    modeAtt->sendInitialUpdate();
    modeModern.onClick  = [this] { modeAtt->setValueAsCompleteGesture (0.0f); };
    modeClassic.onClick = [this] { modeAtt->setValueAsCompleteGesture (1.0f); };
    modeHard.onClick    = [this] { modeAtt->setValueAsCompleteGesture (2.0f); };

    // ---- Medidores y pantalla ----
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (grMeterL);
    addAndMakeVisible (grMeterR);
    addAndMakeVisible (trimInMeter);
    addAndMakeVisible (trimOutMeter);
    addAndMakeVisible (scope);

    refreshDimming();

    setResizeLimits (836, 471, 2506, 1411);
    getConstrainer()->setFixedAspectRatio ((double) refW / (double) refH);
    setResizable (true, true);
    setSize (1114, 627);

    startTimerHz (30);
}

AndresBeatsClipAudioProcessorEditor::~AndresBeatsClipAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void AndresBeatsClipAudioProcessorEditor::setupKnob (int i)
{
    auto& k = knobs[(size_t) i];
    const auto& sp = kSpecs[i];

    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f, true);

    const bool pct = sp.pct;
    k.slider.textFromValueFunction = [pct] (double v) { return juce::String (v, 1) + (pct ? " %" : " dB"); };

    Knob* kp = &k;
    k.slider.onValueChange = [kp]
    {
        kp->readout.setText (kp->slider.getTextFromValue (kp->slider.getValue()), juce::dontSendNotification);
    };

    k.readout.setJustificationType (juce::Justification::centred);
    k.readout.setInterceptsMouseClicks (false, false);

    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.readout);

    k.att = std::make_unique<SliderAttachment> (proc.apvts, sp.id, k.slider);

    if (auto* par = proc.apvts.getParameter (sp.id))
        k.slider.setDoubleClickReturnValue (true, (double) par->convertFrom0to1 (par->getDefaultValue()));

    k.slider.onValueChange();
}

void AndresBeatsClipAudioProcessorEditor::refreshDimming()
{
    const float aMix = parallelBtn.getToggleState() ? 1.0f : 0.4f;
    knobs[kMix].slider.setAlpha (aMix);
    knobs[kMix].readout.setAlpha (aMix);

    const float aOut = linkBtn.getToggleState() ? 0.4f : 1.0f;
    knobs[kTrimOut].slider.setAlpha (aOut);
    knobs[kTrimOut].readout.setAlpha (aOut);
}

//==============================================================================
void AndresBeatsClipAudioProcessorEditor::timerCallback()
{
    auto toDb = [] (float lin) { return lin > 1.0e-5f ? 20.0f * std::log10 (lin) : -100.0f; };
    auto follow = [&toDb] (float& state, float lin, float fall)
    {
        const float db = toDb (lin);
        state = db > state ? db : juce::jmax (db, state - fall);
    };

    for (int ch = 0; ch < 2; ++ch)
    {
        follow (dbInput[ch],   proc.meterClipIn[ch].exchange (0.0f), 1.6f);
        follow (dbTrimIn[ch],  proc.meterRawIn[ch].exchange (0.0f),  1.6f);
        follow (dbTrimOut[ch], proc.meterOut[ch].exchange (0.0f),    1.6f);

        const float gr = proc.meterGR[ch].exchange (0.0f);
        grState[ch] = gr > grState[ch] ? gr : juce::jmax (gr, grState[ch] - 0.5f);
    }

    inputMeter.setLevels (dbInput[0], dbInput[1]);
    trimInMeter.setLevels (dbTrimIn[0], dbTrimIn[1]);
    trimOutMeter.setLevels (dbTrimOut[0], dbTrimOut[1]);
    grMeterL.setLevels (grState[0], grState[1]);
    grMeterR.setLevels (grState[0], grState[1]);

    scope.setGr (grState[0], grState[1]);
    scope.repaint();
}

//==============================================================================
void AndresBeatsClipAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImageAt (background, 0, 0);
    else
        g.fillAll (gold::bgDark);
}

void AndresBeatsClipAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / refW;

    auto R = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * s, y * s, w * s, h * s).toNearestInt();
    };

    for (int i = 0; i < kNumKnobs; ++i)
    {
        const auto& sp = kSpecs[i];
        auto& k = knobs[(size_t) i];
        k.slider.setBounds (R (sp.cx - sp.R, sp.cy - sp.R, sp.R * 2.0f, sp.R * 2.0f));
        k.readout.setBounds (R (sp.cx - sp.roW * 0.5f, sp.roCy - sp.roH * 0.5f, sp.roW, sp.roH));
    }

    shimmerSlider.setBounds (R (713.0f, 780.0f, 278.0f, 36.0f));
    shimmerReadout.setBounds (R (814.0f, 816.0f, 86.0f, 26.0f));

    os1.setBounds (R (652.0f, 85.0f, 72.0f, 44.0f));
    os2.setBounds (R (730.0f, 85.0f, 72.0f, 44.0f));
    os4.setBounds (R (808.0f, 85.0f, 72.0f, 44.0f));

    modeModern.setBounds  (R (972.0f  - 52.0f, 70.0f, 104.0f, 76.0f));
    modeClassic.setBounds (R (1097.0f - 52.0f, 70.0f, 104.0f, 76.0f));
    modeHard.setBounds    (R (1219.0f - 52.0f, 70.0f, 104.0f, 76.0f));

    linkBtn.setBounds  (R (1315.0f, 83.0f, 52.0f, 52.0f));
    unityBtn.setBounds (R (1419.0f, 82.0f, 52.0f, 52.0f));
    powerBtn.setBounds (R (1532.0f, 63.0f, 64.0f, 64.0f));
    parallelBtn.setBounds (R (365.0f, 764.0f, 34.0f, 52.0f));

    inputMeter.setBounds   (R (116.0f, 239.0f, 114.0f, 235.0f));
    grMeterL.setBounds     (R (356.0f, 245.0f, 116.0f, 192.0f));
    grMeterR.setBounds     (R (1217.0f, 245.0f, 120.0f, 192.0f));
    trimInMeter.setBounds  (R (1418.0f, 254.0f, 80.0f, 91.0f));
    trimOutMeter.setBounds (R (1533.0f, 254.0f, 80.0f, 91.0f));
    scope.setBounds        (R (522.0f, 192.0f, 648.0f, 258.0f));

    // fondo estático
    if (getWidth() > 0 && getHeight() > 0)
    {
        background = juce::Image (juce::Image::RGB, getWidth(), getHeight(), true);
        juce::Graphics bg (background);
        bg.addTransform (juce::AffineTransform::scale (s));
        drawStatic (bg);
    }
}

//==============================================================================
void AndresBeatsClipAudioProcessorEditor::drawStatic (juce::Graphics& g)
{
    auto txt = [&g] (const juce::String& t, float cx, float cy, float fh, juce::Colour col,
                     bool bold = false, float kern = 0.0f, bool left = false)
    {
        g.setColour (col);
        g.setFont (gold::makeFont (fh, bold, kern));
        const float x = left ? cx : cx - 150.0f;
        g.drawText (t, juce::Rectangle<float> (x, cy - fh, 300.0f, fh * 2.0f),
                    left ? juce::Justification::centredLeft : juce::Justification::centred, false);
    };

    const auto light = gold::textLight;
    const auto grey  = juce::Colour (0xffb7b0a0);

    // Fondo general con viñeta
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff141415), 0.0f, 0.0f, juce::Colour (0xff040404), 0.0f, refH, false));
    g.fillRect (0.0f, 0.0f, refW, refH);

    // ---------------- Cabecera ----------------
    drawPanel (g, { 10.0f, 30.0f, 1650.0f, 128.0f }, 10.0f);
    drawPanel (g, { 632.0f, 47.0f, 266.0f, 104.0f }, 6.0f, true);
    drawPanel (g, { 918.0f, 47.0f, 360.0f, 104.0f }, 6.0f, true);
    drawPanel (g, { 1292.0f, 47.0f, 98.0f, 104.0f }, 6.0f, true);
    drawPanel (g, { 1396.0f, 47.0f, 102.0f, 104.0f }, 6.0f, true);
    drawPanel (g, { 1504.0f, 47.0f, 140.0f, 104.0f }, 6.0f, true);

    g.setColour (juce::Colour (0xff030303));
    g.fillRoundedRectangle (644.0f, 78.0f, 242.0f, 62.0f, 6.0f);
    g.setColour (juce::Colour (0xff3a2b10));
    g.drawLine (932.0f, 75.0f, 1264.0f, 75.0f, 1.0f);

    // Logo (en tu nueva imagen la cabecera lleva solo el monograma y un separador)
    {
        juce::Path a;
        a.startNewSubPath (98.0f, 136.0f);
        a.lineTo (123.0f, 54.0f);
        a.lineTo (149.0f, 136.0f);
        a.startNewSubPath (109.0f, 110.0f);
        a.lineTo (138.0f, 110.0f);
        a.startNewSubPath (123.0f, 82.0f);
        a.lineTo (123.0f, 98.0f);
        g.setColour (gold::goldMain.withAlpha (0.22f));
        g.strokePath (a, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xffb9b4a8));
        g.strokePath (a, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff3a3a3d));
        g.strokePath (a, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (gold::goldDim);
    g.drawLine (192.0f, 62.0f, 192.0f, 128.0f, 1.5f);

    txt ("OVERSAMPLING", 765.0f, 60.0f, 15.0f, light);
    txt ("CLIP MODE", 1099.0f, 61.0f, 15.0f, light);
    txt ("TRIM LINK", 1342.0f, 65.0f, 15.0f, light);
    txt ("UNITY GAIN", 1447.0f, 65.0f, 15.0f, light);
    txt ("POWER", 1565.0f, 138.0f, 17.0f, light, true);

    // Tornillos
    const juce::Point<float> screws[] = { { 35.0f, 58.0f }, { 1633.0f, 58.0f }, { 28.0f, 138.0f }, { 1640.0f, 140.0f },
                                          { 32.0f, 182.0f }, { 1637.0f, 182.0f }, { 35.0f, 875.0f }, { 1633.0f, 875.0f } };
    for (const auto& pt : screws)
        drawScrew (g, pt.x, pt.y);

    // ---------------- Panel izquierdo (INPUT) ----------------
    drawPanel (g, { 10.0f, 165.0f, 288.0f, 735.0f }, 8.0f);
    drawPanel (g, { 66.0f, 172.0f, 204.0f, 582.0f }, 8.0f, true);
    g.setColour (juce::Colour (0xff020202));
    g.fillRoundedRectangle (90.0f, 212.0f, 164.0f, 316.0f, 6.0f);
    g.setColour (juce::Colour (0xff3a2b10));
    g.drawRoundedRectangle (90.0f, 212.0f, 164.0f, 316.0f, 6.0f, 1.0f);

    txt ("INPUT", 172.0f, 196.0f, 20.0f, gold::goldBright, true);
    txt ("L", 128.0f, 231.0f, 15.0f, grey);
    txt ("R", 216.0f, 231.0f, 15.0f, grey);
    txt ("-24", 103.0f, 689.0f, 16.0f, grey);
    txt ("+24", 236.0f, 689.0f, 16.0f, grey);
    txt ("INPUT GAIN", 170.0f, 716.0f, 19.0f, light, true);

    // ---------------- Reducción izquierda ----------------
    drawPanel (g, { 314.0f, 170.0f, 192.0f, 298.0f }, 8.0f);
    txt ("REDUCTION", 414.0f, 210.0f, 16.0f, light, true);
    txt ("L", 375.0f, 236.0f, 15.0f, grey);
    txt ("R", 449.0f, 236.0f, 15.0f, grey);

    // ---------------- Pantalla central ----------------
    drawPanel (g, { 510.0f, 172.0f, 668.0f, 292.0f }, 8.0f);
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (518.0f, 184.0f, 652.0f, 272.0f, 10.0f);
    g.setColour (gold::goldDim.withAlpha (0.6f));
    g.drawRoundedRectangle (518.0f, 184.0f, 652.0f, 272.0f, 10.0f, 1.5f);

    // ---------------- Reducción derecha ----------------
    drawPanel (g, { 1188.0f, 170.0f, 180.0f, 298.0f }, 8.0f);
    txt ("REDUCTION", 1274.0f, 210.0f, 16.0f, light, true);
    txt ("L", 1238.0f, 236.0f, 15.0f, grey);
    txt ("R", 1308.0f, 236.0f, 15.0f, grey);

    // ---------------- TRIM ----------------
    drawPanel (g, { 1402.0f, 180.0f, 226.0f, 306.0f }, 8.0f);
    txt ("TRIM", 1516.0f, 202.0f, 19.0f, light, true);
    txt ("IN", 1458.0f, 237.0f, 15.0f, grey);
    txt ("OUT", 1571.0f, 237.0f, 15.0f, grey);
    txt ("-12", 1428.0f, 437.0f, 11.0f, grey);
    txt ("+12", 1490.0f, 437.0f, 11.0f, grey);
    txt ("-12", 1542.0f, 437.0f, 11.0f, grey);
    txt ("+12", 1604.0f, 437.0f, 11.0f, grey);

    // ---------------- Fila de knobs principales ----------------
    drawPanel (g, { 312.0f, 474.0f, 1066.0f, 226.0f }, 8.0f);

    const char* titles[] = { "CEILING", "KNEE", "GOLD I", "GOLD II", "ALCHEMY", "BOX TONE" };
    const float tx[] = { 414.0f, 596.0f, 774.0f, 948.0f, 1121.0f, 1293.0f };
    for (int i = 0; i < 6; ++i)
        txt (titles[i], tx[i], 636.0f, 19.0f, light, true);

    struct EndLabel { const char* t; float x; };
    const EndLabel ends[] = {
        { "-12", 353.0f }, { "0", 473.0f }, { "SOFT", 533.0f }, { "HARD", 662.0f },
        { "0.0", 718.0f }, { "100", 831.0f }, { "0.0", 893.0f }, { "100", 1005.0f },
        { "0", 1069.0f }, { "100", 1177.0f }, { "0", 1241.0f }, { "100", 1350.0f } };
    for (const auto& e : ends)
        txt (e.t, e.x, 613.0f, 15.0f, grey);

    // ---------------- Fila inferior ----------------
    drawPanel (g, { 324.0f, 720.0f, 312.0f, 160.0f }, 8.0f);
    txt ("TRUE PARALLEL", 497.0f, 740.0f, 17.0f, gold::goldBright, true);
    txt ("DRY", 431.0f, 797.0f, 15.0f, grey);
    txt ("WET", 585.0f, 797.0f, 15.0f, grey);
    g.setColour (gold::goldDim);
    g.drawLine (453.0f, 797.0f, 471.0f, 797.0f, 1.2f);
    g.drawLine (543.0f, 797.0f, 563.0f, 797.0f, 1.2f);

    drawPanel (g, { 660.0f, 724.0f, 400.0f, 142.0f }, 8.0f);
    txt ("HIGH AIR SHIMMER", 856.0f, 753.0f, 17.0f, light, true);
    g.setColour (juce::Colour (0xff3a2b10));
    g.drawLine (685.0f, 766.0f, 1035.0f, 766.0f, 1.0f);
    txt ("0", 695.0f, 798.0f, 15.0f, grey);
    txt ("100", 1013.0f, 799.0f, 15.0f, grey);

    drawPanel (g, { 1090.0f, 716.0f, 246.0f, 168.0f }, 8.0f);
    txt ("CEIL TRIM", 1214.0f, 728.0f, 16.0f, light, true);
    txt ("-12", 1158.0f, 838.0f, 15.0f, grey);
    txt ("+12", 1267.0f, 838.0f, 15.0f, grey);

    drawPanel (g, { 1378.0f, 696.0f, 250.0f, 190.0f }, 8.0f);
    txt ("OUTPUT", 1493.0f, 706.0f, 17.0f, light, true);
    txt ("-24", 1436.0f, 837.0f, 15.0f, grey);
    txt ("+24", 1551.0f, 837.0f, 15.0f, grey);
}
