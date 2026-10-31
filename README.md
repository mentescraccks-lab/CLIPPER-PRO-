# CLIPPER (Fase 2: motor DSP)

Plugin VST3 para Windows x64 (JUCE 8.0.4 + CMake). Estado: DSP implementado (hard/soft clip, oversampling 1x-16x, medidores, suavizado). Las pruebas del DSP puro se ejecutan con ctest; el resto está pendiente de compilar en GitHub Actions.
Interfaz: editor genérico provisional (la interfaz analógica llega en la Fase 3).

## Compilar con GitHub Actions
1. Sube todo el contenido de esta carpeta a la raíz del repositorio (rama `main`).
2. Pestaña **Actions** → **Windows VST3 build** → **Run workflow**.
3. Al terminar, descarga el artefacto `CLIPPER-vst3-win-x64` (contiene `CLIPPER.vst3`).
4. Copia `CLIPPER.vst3` a `C:\Program Files\Common Files\VST3\` y reescanea plugins en tu DAW.

## Compilar en local (Visual Studio 2022)
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
Resultado: `build/CLIPPER_artefacts/Release/VST3/CLIPPER.vst3`
