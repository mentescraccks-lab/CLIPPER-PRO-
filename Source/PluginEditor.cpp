#include "PluginEditor.h"
#include "GUI/LayoutData.h"

using namespace juce;

namespace
{
    constexpr int kW = 1672, kH = 941;
    constexpr float kDeg = MathConstants<float>::pi / 180.0f;

    // Capas estáticas (chasis, tornillos, textos, logo, preset) compuestas una sola vez.
    Image buildComposite()
    {
        Image img (Image::ARGB, kW, kH, true);
        Graphics g (img);
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        for (const auto& l : LayoutData::staticLayers)
            g.drawImageAt (Assets::get (l.file), l.x, l.y);
        return img;
    }

    const AnalogKnob::Spec clipSpec  { "knob_clip_amount_scale_ring.png",  "knob_clip_amount_body.png",  138.0f, 148.0f, 5.0f, -145.0f * kDeg };
    const AnalogKnob::Spec driveSpec { "knob_input_drive_scale_ring.png",  "knob_input_drive_body.png",   86.0f,  85.0f, 4.0f, 0.0f };
    const AnalogKnob::Spec ceilSpec  { "knob_ceiling_scale_ring.png",      "knob_ceiling_body.png",       83.0f,  88.0f, 4.0f, 0.0f };
    const AnalogKnob::Spec outSpec   { "knob_output_scale_ring.png",       "knob_output_body.png",        83.0f,  87.0f, 4.0f, 0.0f };

    const LedMeter::Spec inSpec  { "meter_input_segments_ON_state.png",          25,  6.0f, 18.20f, true,  -1, -1 };
    const LedMeter::Spec outSpec2{ "meter_output_segments_ON_state.png",         25,  3.0f, 18.24f, true,  -1, -1 };
    const LedMeter::Spec grSpec  { "meter_gain_reduction_segments_ON_state.png", 29,  3.0f, 19.17f, false,  8, 20 };
}

ClipperAudioProcessorEditor::ClipperAudioProcessorEditor (ClipperAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      driveKnob (driveSpec), clipKnob (clipSpec), ceilKnob (ceilSpec), outKnob (outSpec),
      driveVal (driveKnob, 26.0f), clipVal (clipKnob, 34.0f), ceilVal (ceilKnob, 26.0f), outVal (outKnob, 26.0f),
      hardBtn ("hard"), softBtn ("soft"), os1 ("1x"), os2 ("2x"), os4 ("4x"), os8 ("8x"), os16 ("16x"),
      linkBtn ("link"), bypassBtn ("bypass"),
      inMeter (inSpec), outMeter (outSpec2), grMeter (grSpec)
{
    content.composite = buildComposite();
    addAndMakeVisible (content);

    // --- Perillas (el rectángulo de cada una coincide con su capa en el manifest) ---
    clipKnob.setBounds  (660, 162, 356, 356);
    driveKnob.setBounds ( 80, 387, 220, 220);
    ceilKnob.setBounds  (1102, 389, 216, 216);
    outKnob.setBounds   (1397, 389, 216, 216);
    for (auto* k : { &clipKnob, &driveKnob, &ceilKnob, &outKnob })
        content.addAndMakeVisible (k);

    clipVal.setBounds  ( 752, 553, 168, 63);
    driveVal.setBounds ( 125, 608, 129, 48);
    ceilVal.setBounds  (1146, 608, 126, 48);
    outVal.setBounds   (1441, 608, 126, 48);
    for (auto* v : { &clipVal, &driveVal, &ceilVal, &outVal })
        content.addAndMakeVisible (v);

    // --- MODE (HARD / SOFT) ---
    const auto ledOn  = Assets::get ("led_mode_hard.png");
    const auto ledOff = Assets::get ("led_mode_soft.png");
    hardBtn.setImages (ledOff, ledOn);
    softBtn.setImages (ledOff, ledOn);
    hardBtn.setBounds (381, 439, 88, 88);
    softBtn.setBounds (381, 516, 88, 88);
    content.addAndMakeVisible (hardBtn);
    content.addAndMakeVisible (softBtn);

    // --- OVERSAMPLING 1x..16x (botón encendido = imagen del 4x) ---
    struct OsDef { ImageStateButton* b; const char* off; const char* label; Rectangle<int> r; };
    const OsDef defs[] = {
        { &os1,  "btn_os_1x_empty.png",  "btn_label_os_1x.png",  {  82, 760, 86, 72 } },
        { &os2,  "btn_os_2x_empty.png",  "btn_label_os_2x.png",  { 183, 760, 86, 72 } },
        { &os4,  "btn_os_8x_empty.png",  "btn_label_os_4x.png",  { 283, 760, 88, 72 } },
        { &os8,  "btn_os_8x_empty.png",  "btn_label_os_8x.png",  { 387, 760, 86, 72 } },
        { &os16, "btn_os_16x_empty.png", "btn_label_os_16x.png", { 487, 760, 87, 72 } },
    };
    const auto osOn = Assets::get ("btn_os_4x_empty.png");
    for (const auto& d : defs)
    {
        d.b->setImages (Assets::get (d.off), osOn, Assets::get (d.label));
        d.b->setBounds (d.r);
        content.addAndMakeVisible (*d.b);
    }

    // --- LINK L/R y BYPASS ---
    const auto ledLit  = Assets::get ("led_link.png");
    const auto ledDark = Assets::get ("led_bypass.png");
    linkBtn.setup   (Assets::get ("btn_link.png"),   ledLit, ledDark, { 1324, 765, 97, 69 }, { 1300, 708, 50, 50 });
    bypassBtn.setup (Assets::get ("btn_bypass.png"), ledLit, ledDark, { 1493, 765, 97, 69 }, { 1469, 710, 46, 46 });
    content.addAndMakeVisible (linkBtn);
    content.addAndMakeVisible (bypassBtn);

    // --- Medidores ---
    inMeter.setBounds  (  92, 195, 470, 68);
    outMeter.setBounds (1118, 195, 464, 68);
    grMeter.setBounds  ( 660, 740, 562, 58);
    content.addAndMakeVisible (inMeter);
    content.addAndMakeVisible (outMeter);
    content.addAndMakeVisible (grMeter);

    // --- Vinculación con los parámetros reales (APVTS) ---
    auto& apvts = proc.apvts;
    driveAtt = std::make_unique<SliderAttachment> (apvts, ParamID::inputDrive, driveKnob);
    clipAtt  = std::make_unique<SliderAttachment> (apvts, ParamID::clipAmount, clipKnob);
    ceilAtt  = std::make_unique<SliderAttachment> (apvts, ParamID::ceiling,    ceilKnob);
    outAtt   = std::make_unique<SliderAttachment> (apvts, ParamID::output,     outKnob);
    linkAtt  = std::make_unique<ButtonAttachment> (apvts, ParamID::linkLR, linkBtn);
    bypassAtt = std::make_unique<ButtonAttachment> (apvts, ParamID::bypass, bypassBtn);
    for (auto* v : { &clipVal, &driveVal, &ceilVal, &outVal })
        v->refresh();

    modeAtt = std::make_unique<ParameterAttachment> (*apvts.getParameter (ParamID::mode),
        [this] (float v)
        {
            const int idx = roundToInt (v);
            hardBtn.setToggleState (idx == 0, dontSendNotification);
            softBtn.setToggleState (idx == 1, dontSendNotification);
        });
    modeAtt->sendInitialUpdate();
    hardBtn.onClick = [this] { modeAtt->setValueAsCompleteGesture (0.0f); };
    softBtn.onClick = [this] { modeAtt->setValueAsCompleteGesture (1.0f); };

    osAtt = std::make_unique<ParameterAttachment> (*apvts.getParameter (ParamID::oversampling),
        [this] (float v)
        {
            const int idx = roundToInt (v);
            ImageStateButton* all[] = { &os1, &os2, &os4, &os8, &os16 };
            for (int i = 0; i < 5; ++i)
                all[i]->setToggleState (i == idx, dontSendNotification);
        });
    osAtt->sendInitialUpdate();
    os1.onClick  = [this] { osAtt->setValueAsCompleteGesture (0.0f); };
    os2.onClick  = [this] { osAtt->setValueAsCompleteGesture (1.0f); };
    os4.onClick  = [this] { osAtt->setValueAsCompleteGesture (2.0f); };
    os8.onClick  = [this] { osAtt->setValueAsCompleteGesture (3.0f); };
    os16.onClick = [this] { osAtt->setValueAsCompleteGesture (4.0f); };

    // --- Ventana redimensionable con la proporción de la referencia ---
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setResizeLimits (836, 470, 2508, 1411);
    setSize (1254, 705);

    startTimerHz (30);
}

ClipperAudioProcessorEditor::~ClipperAudioProcessorEditor()
{
    stopTimer();
}

void ClipperAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void ClipperAudioProcessorEditor::resized()
{
    content.setBounds (0, 0, kW, kH);
    content.setTransform (AffineTransform::scale ((float) getWidth() / (float) kW));
}

void ClipperAudioProcessorEditor::timerCallback()
{
    auto toDb = [] (float lin) { return Decibels::gainToDecibels (lin, -100.0f); };

    // Ataque instantáneo, caída suave (~27 dB/s para entrada/salida, ~18 dB/s para GR).
    inDb  = jmax (toDb (proc.meters.inputPeak.takeAndReset()),  inDb  - 0.9f);
    outDb = jmax (toDb (proc.meters.outputPeak.takeAndReset()), outDb - 0.9f);

    const float red = proc.meters.gainReduction.takeAndReset();      // 1 - ratio mínimo
    const float grNow = red > 0.0f ? jmin (60.0f, -Decibels::gainToDecibels (jmax (1.0f - red, 1.0e-3f))) : 0.0f;
    grDb = jmax (grNow, grDb - 0.6f);

    // Escalas de los medidores: entrada/salida -24..+3 dB, gain reduction 0..-18 dB.
    inMeter.setLit  (roundToInt ((inDb  + 24.0f) / 27.0f * 25.0f));
    outMeter.setLit (roundToInt ((outDb + 24.0f) / 27.0f * 25.0f));
    grMeter.setLit  (roundToInt (grDb / 18.0f * 29.0f));
}
