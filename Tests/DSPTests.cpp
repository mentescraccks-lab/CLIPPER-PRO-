// Pruebas del DSP puro (sin JUCE). Tolerancias documentadas abajo.
// TOL_EXACT = 0.0 (el hard clip debe ser exacto); TOL_FP = 1e-6 (error de coma flotante float).
#include "../Source/DSP/ClipperDSP.h"
#include <cstdio>
#include <limits>
#include <vector>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf ("FALLO: %s\n", msg); ++failures; } else std::printf ("ok:    %s\n", msg); } while (0)

int main()
{
    using namespace clipper;
    const float TOL_FP = 1.0e-6f;
    const float T = dbToGain (-0.1f);

    { // 1. silencio
        bool ok = true;
        for (bool soft : { false, true }) ok &= (clipSample (0.0f, T, soft) == 0.0f);
        CHECK (ok, "el silencio permanece en silencio (hard y soft)");
    }
    { // 2. hard clip respeta el umbral
        bool ok = true;
        for (float x = -50.0f; x <= 50.0f; x += 0.013f) ok &= (std::abs (clipSample (x, T, false)) <= T);
        CHECK (ok, "HARD: |y| <= ceiling en barrido -50..50 (tolerancia exacta)");
        CHECK (clipSample (0.3f * T, T, false) == 0.3f * T, "HARD: por debajo del umbral pasa sin cambios");
    }
    { // 3. soft clip suave y acotado
        bool bounded = true, mono = true, odd = true;
        float prev = clipSample (-50.0f, T, true), maxStep = 0.0f;
        for (float x = -50.0f; x <= 50.0f; x += 0.01f)
        {
            const float y = clipSample (x, T, true);
            bounded &= (std::abs (y) <= T);
            mono &= (y >= prev - TOL_FP);
            maxStep = std::max (maxStep, std::abs (y - prev));
            odd &= (std::abs (clipSample (-x, T, true) + y) <= TOL_FP);
            prev = y;
        }
        CHECK (bounded, "SOFT: acotado por el ceiling");
        CHECK (mono, "SOFT: monótono");
        CHECK (odd, "SOFT: simétrico (función impar)");
        CHECK (maxStep <= 0.0101f, "SOFT: sin saltos (paso máximo <= pendiente 1 * 0.01)");
        CHECK (std::abs (clipSample (1.0e-3f, T, true) - 1.0e-3f) < 1.0e-6f, "SOFT: pendiente ~1 cerca de 0");
    }
    { // 4. drive cambia la señal / 5. output cambia el nivel
        const float x = 0.2f;
        const float y0 = clipSample (x * dbToGain (0.0f), T, true);
        const float y1 = clipSample (x * dbToGain (12.0f), T, true);
        CHECK (std::abs (y1) > std::abs (y0) + 0.1f, "INPUT DRIVE +12 dB cambia la señal procesada");
        CHECK (std::abs (y0 * dbToGain (-6.0f) - y0 * 0.5011872f) < 1.0e-6f, "OUTPUT -6 dB escala el nivel final");
    }
    { // 6. ceiling
        for (float cdb : { -3.0f, -1.0f, 0.0f })
        {
            const float t = dbToGain (cdb);
            bool ok = true;
            for (float x = -20.0f; x <= 20.0f; x += 0.05f) ok &= (std::abs (clipSample (x, t, false)) <= t) && (std::abs (clipSample (x, t, true)) <= t);
            char m[96]; std::snprintf (m, sizeof m, "CEILING %.1f dB respetado en ambos modos", cdb);
            CHECK (ok, m);
        }
    }
    { // 8. no finitos
        const float inf = std::numeric_limits<float>::infinity(), nan = std::numeric_limits<float>::quiet_NaN();
        bool ok = true;
        for (bool soft : { false, true })
            for (float x : { inf, -inf, nan }) ok &= std::isfinite (clipSample (x, T, soft));
        CHECK (ok, "NaN e infinitos producen salida finita");
    }
    { // 9. estéreo enlazado conserva la relación L/R
        float l = 2.0f, r = 1.0f;
        clipLinked (l, r, T, false);
        CHECK (std::abs (l - T) <= TOL_FP && std::abs (l / r - 2.0f) <= 1.0e-5f, "LINK: recorta el canal mayor y conserva la relación L/R");
        float a = 0.1f, b = -0.05f;
        const float ratio = clipLinked (a, b, T, false);
        CHECK (ratio == 1.0f && a == 0.1f && b == -0.05f, "LINK: bajo el umbral no modifica nada");
    }
    std::printf ("\n%s (%d fallos)\n", failures ? "PRUEBAS FALLIDAS" : "TODAS LAS PRUEBAS SUPERADAS", failures);
    return failures ? 1 : 0;
}
