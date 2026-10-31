#pragma once
// Funciones DSP puras (sin dependencia de JUCE) para poder probarlas de forma aislada.
//
// Ecuaciones (T = ceiling lineal > 0, x = muestra de entrada del clipper):
//   HARD:  y = clamp(x, -T, +T)                  -> |y| <= T exactamente.
//   SOFT:  y = T * tanh(x / T)                   -> continua, monótona, impar,
//                                                   pendiente 1 en x=0, |y| < T siempre.
// Cadena de señal:
//   x -> * 10^((InputDrive + ClipAmount)/20) -> [oversampling] -> clip(T=Ceiling) -> [downsampling]
//     -> * 10^(Output/20)
// Es un clipper, no un limitador: no hay look-ahead ni envolvente; actúa muestra a muestra.
#include <algorithm>
#include <cmath>

namespace clipper
{
    inline float dbToGain (float dB) noexcept { return std::pow (10.0f, dB * 0.05f); }

    inline float sanitize (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }

    inline float hardClip (float x, float t) noexcept { return std::clamp (x, -t, t); }

    inline float softClip (float x, float t) noexcept { return t * std::tanh (x / t); }

    inline float clipSample (float x, float t, bool soft) noexcept
    {
        t = std::max (t, 1.0e-6f);
        x = sanitize (x);
        return soft ? softClip (x, t) : hardClip (x, t);
    }

    // Clipping estéreo enlazado: se recorta la muestra de mayor magnitud y se aplica la misma
    // reducción a ambos canales (conserva la imagen estéreo). Devuelve el factor aplicado (0..1].
    inline float clipLinked (float& l, float& r, float t, bool soft) noexcept
    {
        l = sanitize (l);
        r = sanitize (r);
        const float m = std::max (std::abs (l), std::abs (r));
        if (m < 1.0e-9f)
            return 1.0f;
        const float ratio = clipSample (m, t, soft) / m;
        l *= ratio;
        r *= ratio;
        return ratio;
    }
}
