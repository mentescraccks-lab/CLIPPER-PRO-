#pragma once
// Comunicación hilo de audio -> GUI sin bloqueos. El audio escribe el máximo del bloque;
// la GUI lo lee con takeAndReset() (el valor queda retenido hasta que la GUI lo lee).
#include <atomic>

struct PeakHolder
{
    std::atomic<float> value { 0.0f };

    void updateMax (float v) noexcept
    {
        float cur = value.load (std::memory_order_relaxed);
        while (v > cur && ! value.compare_exchange_weak (cur, v, std::memory_order_relaxed)) {}
    }
    float takeAndReset() noexcept { return value.exchange (0.0f, std::memory_order_relaxed); }
};

struct ClipperMeters
{
    PeakHolder inputPeak;      // lineal, antes de INPUT DRIVE
    PeakHolder outputPeak;     // lineal, después de OUTPUT
    PeakHolder gainReduction;  // lineal >= 0: (1 - ratio mínimo) del bloque; 0 = sin clipping
    PeakHolder clipEvents;     // nº de muestras (a la tasa interna) que fueron recortadas en el bloque
};
