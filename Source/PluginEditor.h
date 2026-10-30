#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "PluginProcessor.h"
#include "GoldLookAndFeel.h"

//==============================================================================
// Botón multiuso dibujado a mano (segmentos de oversampling, selector de modo, link, unity, power, switch)
class GoldButton : public juce::Button
{
public:
    enum class Kind { Segment, Mode, Link, Unity, Power, Switch };

    GoldButton (Kind k, const juce::String& text) : juce::Button (text), kind (k) {}

    void paintButton (juce::Graphics& g, bool over, bool /*down*/) override
    {
        const auto b = getLocalBounds().toFloat();
        const bool on = getToggleState();
        const auto c = b.getCentre();
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.0f;

        switch (kind)
        {
            case Kind::Segment:
            {
                const auto rc = b.reduced (1.0f);
                if (on)
                {
                    g.setGradientFill (juce::ColourGradient (gold::goldBright, rc.getX(), rc.getY(),
                                                             gold::goldMain.darker (0.3f), rc.getX(), rc.getBottom(), false));
                    g.fillRoundedRectangle (rc, 6.0f);
                }
                else
                {
                    g.setGradientFill (juce::ColourGradient (juce::Colour (over ? 0xff333336 : 0xff2a2a2d), rc.getX(), rc.getY(),
                                                             juce::Colour (0xff111112), rc.getX(), rc.getBottom(), false));
                    g.fillRoundedRectangle (rc, 6.0f);
                }
                g.setColour (on ? gold::goldBright : juce::Colour (0xff4a3a18));
                g.drawRoundedRectangle (rc, 6.0f, 1.5f);
                g.setColour (on ? juce::Colour (0xff1a1204) : gold::textLight);
                g.setFont (gold::makeFont (rc.getHeight() * 0.42f, true));
                g.drawText (getButtonText(), rc, juce::Justification::centred, false);
                break;
            }

            case Kind::Mode:
            {
                const float cx = b.getCentreX(), cy = b.getHeight() * 0.38f, kr = b.getHeight() * 0.2f;
                g.setColour (on ? gold::goldMain : juce::Colour (0xff5a5a5c));
                g.drawLine (cx - kr * 2.7f, cy, cx - kr * 1.5f, cy, 1.5f);
                g.drawLine (cx + kr * 1.5f, cy, cx + kr * 2.7f, cy, 1.5f);

                if (on)
                {
                    g.setColour (gold::goldMain.withAlpha (0.25f));
                    g.fillEllipse (cx - kr * 1.6f, cy - kr * 1.6f, kr * 3.2f, kr * 3.2f);
                }

                if (on)
                    g.setGradientFill (juce::ColourGradient (gold::goldBright, cx - kr * 0.5f, cy - kr * 0.5f,
                                                             gold::goldMain.darker (0.5f), cx + kr, cy + kr, true));
                else
                    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff85878b), cx - kr * 0.5f, cy - kr * 0.5f,
                                                             juce::Colour (0xff17171a), cx + kr, cy + kr, true));
                g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
                g.setColour (juce::Colours::black);
                g.drawEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f, 1.5f);

                g.setColour (on ? gold::goldBright : gold::textLight);
                g.setFont (gold::makeFont (b.getHeight() * 0.19f, true));
                g.drawText (getButtonText(), b.withTop (b.getHeight() * 0.66f), juce::Justification::centred, false);
                break;
            }

            case Kind::Link:
            {
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3d41), c.x, c.y - r, juce::Colour (0xff0e0e10), c.x, c.y + r, false));
                g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
                g.setColour (on ? gold::goldMain : juce::Colour (0xff55555a));
                g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);

                const auto stroke = juce::PathStrokeType (juce::jmax (2.0f, r * 0.14f));
                g.setColour (on ? gold::goldBright : juce::Colour (0xff9a9a9a));
                auto link = [&] (float ox, float oy)
                {
                    juce::Path p;
                    p.addRoundedRectangle (-r * 0.55f, -r * 0.26f, r * 1.1f, r * 0.52f, r * 0.26f);
                    g.strokePath (p, stroke, juce::AffineTransform::rotation (-0.785f).translated (c.x + ox, c.y + oy));
                };
                link (-r * 0.2f,  r * 0.2f);
                link ( r * 0.2f, -r * 0.2f);
                break;
            }

            case Kind::Unity:
            {
                g.setColour (juce::Colour (0xff0f0f11));
                g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
                g.setColour (on ? gold::goldMain : juce::Colour (0xff4a4a4e));
                g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);

                const float kr = r * 0.68f;
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb4b6ba), c.x - kr * 0.4f, c.y - kr * 0.5f,
                                                         juce::Colour (0xff202023), c.x + kr, c.y + kr, true));
                g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
                g.setColour (on ? gold::goldBright : juce::Colour (0xff2a2a2d));
                g.drawLine (c.x, c.y - kr * 0.2f, c.x, c.y - kr * 0.95f, 2.5f);
                break;
            }

            case Kind::Power:
            {
                if (on)
                {
                    g.setColour (gold::goldMain.withAlpha (0.25f));
                    g.fillEllipse (c.x - r - 4.0f, c.y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
                }
                g.setGradientFill (juce::ColourGradient (juce::Colour (on ? 0xff7a500c : 0xff2a2a2d), c.x, c.y - r,
                                                         juce::Colour (on ? 0xff1b1206 : 0xff0e0e10), c.x, c.y + r, false));
                g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
                g.setColour (on ? gold::goldMain : gold::goldDim);
                g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 3.0f);

                const float sr = r * 0.5f;
                juce::Path arc;
                arc.addCentredArc (c.x, c.y, sr, sr, 0.0f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
                arc.startNewSubPath (c.x, c.y - sr * 1.15f);
                arc.lineTo (c.x, c.y - sr * 0.1f);
                g.setColour (on ? gold::goldBright : juce::Colour (0xff6a5326));
                g.strokePath (arc, juce::PathStrokeType (juce::jmax (2.5f, r * 0.1f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            }

            case Kind::Switch:
            {
                const auto slot = juce::Rectangle<float> (b.getCentreX() - b.getWidth() * 0.22f, b.getY() + 2.0f,
                                                          b.getWidth() * 0.44f, b.getHeight() - 4.0f);
                g.setColour (juce::Colour (0xff060607));
                g.fillRoundedRectangle (slot, slot.getWidth() * 0.5f);
                g.setColour (juce::Colour (0xff4a4a4e));
                g.drawRoundedRectangle (slot, slot.getWidth() * 0.5f, 1.2f);

                const float kr = slot.getWidth() * 0.9f;
                const float ky = on ? slot.getY() + kr : slot.getBottom() - kr;
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc2c4c8), c.x - kr * 0.4f, ky - kr * 0.5f,
                                                         juce::Colour (0xff2a2a2d), c.x + kr, ky + kr, true));
                g.fillEllipse (c.x - kr, ky - kr, kr * 2.0f, kr * 2.0f);
                g.setColour (on ? gold::goldMain : juce::Colour (0xff222224));
                g.drawEllipse (c.x - kr, ky - kr, kr * 2.0f, kr * 2.0f, 1.6f);
                break;
            }
        }
    }

private:
    Kind kind;
};

//==============================================================================
// Medidor de LEDs vertical (L y R en los extremos, escala numérica en el centro)
class LevelMeter : public juce::Component
{
public:
    enum class Style { Input, Reduction, Trim };

    LevelMeter (std::vector<float> rungsDb, Style s, int segsPerRungIn, float barFracIn)
        : rungs (std::move (rungsDb)), style (s), segsPerRung (segsPerRungIn), barFrac (barFracIn)
    {
        setInterceptsMouseClicks (false, false);
        level[0] = level[1] = (s == Style::Reduction ? 0.0f : -100.0f);
    }

    void setLevels (float l, float r)
    {
        level[0] = l;
        level[1] = r;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const int n = (int) rungs.size();
        const int segs = n * segsPerRung;
        const float segH = b.getHeight() / (float) segs;
        const float barW = b.getWidth() * barFrac;

        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = ch == 0 ? b.getX() : b.getRight() - barW;
            const float lvl = level[ch];

            for (int j = 0; j < segs; ++j)
            {
                const float pos = ((float) j + 0.5f) / (float) segs;
                bool lit;
                if (style == Style::Reduction)
                    lit = lvl > 0.05f && pos <= posFromTop (-lvl);
                else
                    lit = pos >= posFromTop (lvl);

                const auto seg = juce::Rectangle<float> (x, b.getY() + (float) j * segH + 1.0f, barW, segH - 2.0f);
                const auto col = lit ? segColour (pos) : juce::Colour (0xff2b2008).withAlpha (0.75f);

                if (lit)
                {
                    g.setColour (col.withAlpha (0.22f));
                    g.fillRoundedRectangle (seg.expanded (1.5f), 2.0f);
                }
                g.setColour (col);
                g.fillRoundedRectangle (seg, 1.5f);
            }
        }

        const float fh = juce::jmin (b.getHeight() / (float) n * 0.62f, 15.0f);
        g.setFont (gold::makeFont (fh));
        g.setColour (juce::Colour (0xffd9d2c2).withAlpha (0.9f));
        for (int i = 0; i < n; ++i)
        {
            const float y = b.getY() + ((float) i + 0.5f) / (float) n * b.getHeight();
            const int v = (int) rungs[(size_t) i];
            const juce::String t = v > 0 ? "+" + juce::String (v) : juce::String (v);
            g.drawText (t, juce::Rectangle<float> (b.getX() + barW, y - fh, b.getWidth() - 2.0f * barW, fh * 2.0f),
                        juce::Justification::centred, false);
        }
    }

private:
    float posFromTop (float db) const
    {
        const int n = (int) rungs.size();
        if (db >= rungs.front()) return 0.0f;
        if (db <= rungs.back())  return 1.0f;
        for (int k = 0; k < n - 1; ++k)
            if (db <= rungs[(size_t) k] && db >= rungs[(size_t) k + 1])
            {
                const float f = (rungs[(size_t) k] - db) / (rungs[(size_t) k] - rungs[(size_t) k + 1]);
                return ((float) k + 0.5f + f) / (float) n;
            }
        return 1.0f;
    }

    float dbFromPos (float pos) const
    {
        const int n = (int) rungs.size();
        const float t = juce::jlimit (0.0f, (float) (n - 1) - 0.001f, pos * (float) n - 0.5f);
        const int k = (int) t;
        const float f = t - (float) k;
        return rungs[(size_t) k] - f * (rungs[(size_t) k] - rungs[(size_t) k + 1]);
    }

    juce::Colour segColour (float pos) const
    {
        const float db = dbFromPos (pos);
        switch (style)
        {
            case Style::Input:
                if (db > 0.0f)   return juce::Colour (0xffff4d1a);
                if (db > -6.0f)  return juce::Colour (0xffffa81e);
                if (db > -18.0f) return juce::Colour (0xffffd23c);
                return juce::Colour (0xff9ccc2a).interpolatedWith (juce::Colour (0xff5faa28), juce::jlimit (0.0f, 1.0f, (-db - 18.0f) / 30.0f));
            case Style::Reduction:
                return gold::goldBright.interpolatedWith (juce::Colour (0xfff08a10), pos);
            case Style::Trim:
                return db >= 0.0f ? juce::Colour (0xffff8a1a) : gold::goldMain;
        }
        return gold::goldMain;
    }

    std::vector<float> rungs;
    Style style;
    int segsPerRung;
    float barFrac;
    float level[2];
};

//==============================================================================
// Pantalla central: historial de forma de onda, LED de clipping y lectura de Gain Reduction
class ScopeDisplay : public juce::Component
{
public:
    explicit ScopeDisplay (AndresBeatsClipAudioProcessor& p) : proc (p)
    {
        setInterceptsMouseClicks (false, false);
    }

    void setGr (float l, float r) { gr[0] = l; gr[1] = r; }

    void paint (juce::Graphics& g) override
    {
        const auto full = getLocalBounds().toFloat();
        g.addTransform (juce::AffineTransform::scale (full.getWidth() / 648.0f, full.getHeight() / 258.0f));

        const juce::Rectangle<float> b (0.0f, 0.0f, 648.0f, 258.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff140f09), 0.0f, 0.0f, juce::Colour (0xff000000), 0.0f, 258.0f, false));
        g.fillRoundedRectangle (b, 8.0f);

        // resplandor cálido de fondo
        {
            juce::ColourGradient glow (gold::goldMain.withAlpha (0.10f), 324.0f, 133.0f,
                                       juce::Colours::transparentBlack, 0.0f, 133.0f, true);
            g.setGradientFill (glow);
            g.fillRoundedRectangle (b, 8.0f);
        }

        // LED de clip
        const bool clipNow = (juce::Time::getMillisecondCounter() - proc.lastClipMs.load()) < 160;
        if (clipNow)
        {
            g.setColour (juce::Colour (0xffff2a2a).withAlpha (0.3f));
            g.fillEllipse (38.0f - 14.0f, 38.0f - 14.0f, 28.0f, 28.0f);
        }
        g.setColour (clipNow ? juce::Colour (0xffff3030) : juce::Colour (0xff4a0c0c));
        g.fillEllipse (38.0f - 8.0f, 38.0f - 8.0f, 16.0f, 16.0f);

        g.setColour (gold::textLight);
        g.setFont (gold::makeFont (16.0f));
        g.drawText ("CLIP ACTIVITY", juce::Rectangle<float> (56.0f, 26.0f, 200.0f, 24.0f), juce::Justification::centredLeft, false);

        // Gain reduction
        g.setColour (gold::goldBright);
        g.setFont (gold::makeFont (15.0f));
        g.drawText ("GAIN REDUCTION", juce::Rectangle<float> (490.0f, 27.0f, 150.0f, 22.0f), juce::Justification::centredLeft, false);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float y = 53.0f + (float) ch * 23.0f;
            g.setColour (gold::goldMain);
            g.setFont (gold::makeFont (15.0f));
            g.drawText (ch == 0 ? "L" : "R", juce::Rectangle<float> (490.0f, y, 20.0f, 24.0f), juce::Justification::centredLeft, false);
            g.setColour (gold::textLight);
            g.setFont (gold::makeFont (16.0f, true));
            const juce::String v = gr[ch] > 0.05f ? "-" + juce::String (gr[ch], 1) + " dB" : "0.0 dB";
            g.drawText (v, juce::Rectangle<float> (512.0f, y, 120.0f, 24.0f), juce::Justification::centredLeft, false);
        }

        // Forma de onda
        const float x0 = 34.0f, wPlot = 578.0f, cy = 133.0f, halfH = 44.0f;
        const int N = AndresBeatsClipAudioProcessor::scopeSize;
        const int w = proc.scopeWrite.load (std::memory_order_acquire);

        // línea de referencia superior (tenue) y marca lateral
        g.setColour (gold::goldMain.withAlpha (0.20f));
        g.drawLine (x0, 70.0f, x0 + wPlot, 70.0f, 1.0f);
        g.drawLine (x0, 66.0f, x0, 74.0f, 1.0f);

        auto ampOf = [] (float lin)
        {
            const float db = juce::jlimit (-60.0f, 0.0f, abdsp::gainToDb (lin));
            return juce::jmap (db, -60.0f, 0.0f, 0.0f, 1.0f);
        };

        juce::Path post, preTop;
        for (int k = 0; k < N; ++k)
        {
            const size_t idx = (size_t) ((w + k) % N);
            const float x = x0 + (float) k * wPlot / (float) (N - 1);
            const float aPost = ampOf (proc.scopePost[idx].load (std::memory_order_relaxed)) * halfH;
            const float aPre  = ampOf (proc.scopePre[idx].load (std::memory_order_relaxed)) * halfH;

            if (k == 0) { post.startNewSubPath (x, cy - aPost); preTop.startNewSubPath (x, cy - aPre); }
            else        { post.lineTo (x, cy - aPost);          preTop.lineTo (x, cy - aPre); }
        }
        for (int k = N - 1; k >= 0; --k)
        {
            const size_t idx = (size_t) ((w + k) % N);
            const float x = x0 + (float) k * wPlot / (float) (N - 1);
            const float aPost = ampOf (proc.scopePost[idx].load (std::memory_order_relaxed)) * halfH;
            post.lineTo (x, cy + aPost);
        }
        post.closeSubPath();

        // brillo detrás de la onda
        g.setColour (gold::goldMain.withAlpha (0.12f));
        g.strokePath (post, juce::PathStrokeType (6.0f));

        // nivel previo (antes de clipear) como contorno tenue
        g.setColour (gold::goldMain.withAlpha (0.35f));
        g.strokePath (preTop, juce::PathStrokeType (1.0f));

        g.setGradientFill (juce::ColourGradient (gold::goldBright.withAlpha (0.95f), 0.0f, cy,
                                                 gold::goldDim.withAlpha (0.65f), 0.0f, cy - halfH, false));
        g.fillPath (post);

        // marcas rojas donde hubo clipping
        g.setColour (juce::Colour (0xffff2a2a).withAlpha (0.55f));
        for (int k = 0; k < N; ++k)
        {
            const size_t idx = (size_t) ((w + k) % N);
            if (proc.scopeClip[idx].load (std::memory_order_relaxed))
            {
                const float x = x0 + (float) k * wPlot / (float) (N - 1);
                g.drawLine (x, cy - halfH - 4.0f, x, cy + halfH + 4.0f, 1.2f);
            }
        }

        // línea base brillante y escala
        g.setColour (gold::goldMain.withAlpha (0.25f));
        g.drawLine (x0, 189.0f, x0 + wPlot, 189.0f, 5.0f);
        g.setColour (gold::goldBright);
        g.drawLine (x0, 189.0f, x0 + wPlot, 189.0f, 1.6f);

        g.setColour (juce::Colour (0xffc9c2b2));
        g.setFont (gold::makeFont (15.0f));
        const char* labels[] = { "-60", "-48", "-36", "-24", "-12", "-6", "0", "dBFS" };
        const float xs[] = { 39.0f, 106.0f, 174.0f, 243.0f, 311.0f, 382.0f, 438.0f, 530.0f };
        for (int i = 0; i < 8; ++i)
            g.drawText (labels[i], juce::Rectangle<float> (xs[i] - 30.0f, 204.0f, 60.0f, 24.0f), juce::Justification::centred, false);

        // reflejo de cristal arriba y viñeta
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.06f), 0.0f, 0.0f,
                                                 juce::Colours::transparentWhite, 0.0f, 90.0f, false));
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (juce::Colour (0xff3a2b10));
        g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.5f);
    }

private:
    AndresBeatsClipAudioProcessor& proc;
    float gr[2] { 0.0f, 0.0f };
};

//==============================================================================
class AndresBeatsClipAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit AndresBeatsClipAudioProcessorEditor (AndresBeatsClipAudioProcessor&);
    ~AndresBeatsClipAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label readout;
        std::unique_ptr<SliderAttachment> att;
    };

    enum KnobId { kInput = 0, kCeiling, kKnee, kGold1, kGold2, kAlchemy, kBox, kMix, kCeilTrim, kOutput, kTrimIn, kTrimOut, kNumKnobs };

    void timerCallback() override;
    void setupKnob (int i);
    void drawStatic (juce::Graphics& g);
    void refreshDimming();

    AndresBeatsClipAudioProcessor& proc;
    GoldLookAndFeel lnf;

    std::array<Knob, kNumKnobs> knobs;

    juce::Slider shimmerSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Label shimmerReadout;
    std::unique_ptr<SliderAttachment> shimmerAtt;

    GoldButton os1 { GoldButton::Kind::Segment, "1x" }, os2 { GoldButton::Kind::Segment, "2x" }, os4 { GoldButton::Kind::Segment, "4x" };
    GoldButton modeModern { GoldButton::Kind::Mode, "MODERN" }, modeClassic { GoldButton::Kind::Mode, "CLASSIC" }, modeHard { GoldButton::Kind::Mode, "HARD" };
    GoldButton linkBtn { GoldButton::Kind::Link, "" }, unityBtn { GoldButton::Kind::Unity, "" };
    GoldButton powerBtn { GoldButton::Kind::Power, "" }, parallelBtn { GoldButton::Kind::Switch, "" };

    std::unique_ptr<ButtonAttachment> linkAtt, unityAtt, powerAtt, parallelAtt;
    std::unique_ptr<juce::ParameterAttachment> osAtt, modeAtt;

    LevelMeter inputMeter  { { 6, 3, 0, -3, -6, -9, -12, -18, -24, -30, -36, -48 }, LevelMeter::Style::Input, 2, 0.11f };
    LevelMeter grMeterL    { { 0, -3, -6, -9, -12, -15, -18, -21 }, LevelMeter::Style::Reduction, 3, 0.13f };
    LevelMeter grMeterR    { { 0, -3, -6, -9, -12, -15, -18, -21 }, LevelMeter::Style::Reduction, 3, 0.13f };
    LevelMeter trimInMeter { { 12, 6, 0, -6, -12 }, LevelMeter::Style::Trim, 3, 0.15f };
    LevelMeter trimOutMeter{ { 12, 6, 0, -6, -12 }, LevelMeter::Style::Trim, 3, 0.15f };
    ScopeDisplay scope;

    float dbInput[2] { -100.0f, -100.0f }, dbTrimIn[2] { -100.0f, -100.0f }, dbTrimOut[2] { -100.0f, -100.0f }, grState[2] { 0.0f, 0.0f };

    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AndresBeatsClipAudioProcessorEditor)
};
