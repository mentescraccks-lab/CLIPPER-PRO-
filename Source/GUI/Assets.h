#pragma once
#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>

namespace Assets
{
    // Carga una imagen embebida por su nombre de archivo original (p. ej. "knob_output_body.png").
    inline juce::Image get (const juce::String& fileName)
    {
        const auto id = fileName.replaceCharacter ('.', '_');
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (id.toRawUTF8(), size))
            return juce::ImageCache::getFromMemory (data, size);
        jassertfalse; // recurso inexistente
        return {};
    }
}
