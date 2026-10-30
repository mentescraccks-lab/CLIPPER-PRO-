#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace abdsp
{
//==============================================================================
// Biquad (RBJ cookbook) sin asignaciones de memoria: se puede recalcular en el hilo de audio.
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() noexcept { z1 = z2 = 0.0f; }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setRaw (double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept
    {
        const double inv = 1.0 / na0;
        b0 = (float) (nb0 * inv);
        b1 = (float) (nb1 * inv);
        b2 = (float) (nb2 * inv);
        a1 = (float) (na1 * inv);
        a2 = (float) (na2 * inv);
    }

    void setLowShelf (double fs, double f0, double gainDb, double slope = 0.8) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0);
        const double tsa = 2.0 * std::sqrt (A) * alpha;

        setRaw (A * ((A + 1) - (A - 1) * cw + tsa),
                2.0 * A * ((A - 1) - (A + 1) * cw),
                A * ((A + 1) - (A - 1) * cw - tsa),
                (A + 1) + (A - 1) * cw + tsa,
                -2.0 * ((A - 1) + (A + 1) * cw),
                (A + 1) + (A - 1) * cw - tsa);
    }

    void setHighShelf (double fs, double f0, double gainDb, double slope = 0.8) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0);
        const double tsa = 2.0 * std::sqrt (A) * alpha;

        setRaw (A * ((A + 1) + (A - 1) * cw + tsa),
                -2.0 * A * ((A - 1) + (A + 1) * cw),
                A * ((A + 1) + (A - 1) * cw - tsa),
                (A + 1) - (A - 1) * cw + tsa,
                2.0 * ((A - 1) - (A + 1) * cw),
                (A + 1) - (A - 1) * cw - tsa);
    }

    void setPeak (double fs, double f0, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * q);

        setRaw (1.0 + alpha * A, -2.0 * cw, 1.0 - alpha * A,
                1.0 + alpha / A, -2.0 * cw, 1.0 - alpha / A);
    }

    void setHighPass (double fs, double f0, double q = 0.7071) noexcept
    {
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * q);

        setRaw ((1.0 + cw) / 2.0, -(1.0 + cw), (1.0 + cw) / 2.0,
                1.0 + alpha, -2.0 * cw, 1.0 - alpha);
    }
};

//==============================================================================
// Bloqueador de DC de un polo (~10 Hz). Necesario porque Gold I genera armónicos pares.
struct DcBlocker
{
    float x1 = 0.0f, y1 = 0.0f, R = 0.999f;

    void prepare (double fs) noexcept
    {
        R = (float) (1.0 - (2.0 * juce::MathConstants<double>::pi * 10.0 / fs));
        x1 = y1 = 0.0f;
    }

    inline float process (float x) noexcept
    {
        const float y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};

//==============================================================================
// Línea de retardo simple (para compensar la latencia del oversampling en la mezcla "True Parallel")
struct SimpleDelay
{
    std::vector<float> buf;
    int pos = 0, size = 1, delay = 0;

    void prepare (int maxDelay)
    {
        size = maxDelay + 1;
        buf.assign ((size_t) size, 0.0f);
        pos = 0;
    }

    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
    void setDelay (int d) noexcept { delay = juce::jlimit (0, size - 1, d); }

    inline float process (float x) noexcept
    {
        buf[(size_t) pos] = x;
        int rp = pos - delay;
        if (rp < 0) rp += size;
        const float y = buf[(size_t) rp];
        if (++pos >= size) pos = 0;
        return y;
    }
};

//==============================================================================
// Curvas de clipping. u está normalizado al techo (|u| = 1  ->  Ceiling).
//   mode 0 = Modern : rodilla suave con tanh (transparente)
//   mode 1 = Classic: rodilla racional, más redondeada y "analógica"
//   mode 2 = Hard   : recorte duro en el techo (ignora la rodilla)
// t = umbral donde empieza la rodilla (0..1). t = 1 -> clipping duro.
inline float clipSample (float u, int mode, float t) noexcept
{
    const float a = std::abs (u);
    if (a <= t)
        return u;

    float y;
    const float w = 1.0f - t;

    if (mode == 2 || w < 1.0e-4f)
    {
        y = juce::jmin (a, 1.0f);
    }
    else
    {
        const float z = (a - t) / w;
        if (mode == 0)
            y = t + w * std::tanh (z);
        else
            y = t + w * (z / (1.0f + z));
    }

    return std::copysign (y, u);
}

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (juce::jmax (g, 1.0e-6f)); }

} // namespace abdsp
