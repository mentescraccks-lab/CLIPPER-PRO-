# CLIPPER (Fase 1: base compilable)

Plugin VST3 para Windows x64 (JUCE 8.0.4 + CMake). Estado: **solo implementado, NO compilado ni probado**.
La Fase 1 contiene los 8 parámetros (APVTS), guardado de estado y un editor genérico; el audio pasa sin procesar.

## Compilar con GitHub Actions
1. Sube todo el contenido de esta carpeta a la raíz del repositorio (rama `main`).
2. Pestaña **Actions** → **Windows VST3 build** → **Run workflow**.
3. Al terminar, descarga el artefacto `CLIPPER-vst3-win-x64` (contiene `CLIPPER.vst3`).
4. Copia `CLIPPER.vst3` a `C:\Program Files\Common Files\VST3\` y reescanea plugins en tu DAW.

## Compilar en local (Visual Studio 2022)
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
Resultado: `build/CLIPPER_artefacts/Release/VST3/CLIPPER.vst3`
