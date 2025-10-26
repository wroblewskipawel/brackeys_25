#pragma once

#include <glad/glad.h>

#include "concepts/range.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/texture.h"

class TextureArrayBuilder;

class TextureArray {
   public:
    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    TextureArray(TextureArray&& other) noexcept
        : texture(other.texture), numLayers(other.numLayers) {
        other.texture = 0;
        other.numLayers = 0;
    };
    TextureArray& operator=(TextureArray&& other) noexcept {
        if (this != &other) {
            texture = other.texture;
            numLayers = other.numLayers;
            other.texture = 0;
            other.numLayers = 0;
        }
        return *this;
    };

    ~TextureArray() { glDeleteTextures(1, &texture); };

   private:
    friend class TextureArrayBuilder;

    template <typename Layers>
        requires RefConstRange<Layers, TextureData>
    TextureArray(Layers&& layers, const TextureInfo& info,
                 const SamplerConfig& SamplerConfig)
        : numLayers{std::static_cast<uint32_t>(std::ranges::distance(layers))} {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texture);
        glTextureStorage3D(texture, getMipLevels(info), getDataFormat(info),
                           static_cast<GLsizei>(info.width),
                           static_cast<GLsizei>(info.height),
                           static_cast<GLsizei>(numLayers));
        for (auto [i, layer] : std::views::enumerate(layers)) {
            glTextureSubImage3D(texture, 0, 0, 0, 0,
                                static_cast<GLsizei>(layer.info.width),
                                static_cast<GLsizei>(layer.info.height),
                                static_cast<GLsizei>(i), getFormat(layer),
                                GL_UNSIGNED_BYTE, layer.data.data());
        }
        applySamplerConfig(texture, samplerConfig);
    };

    GLuint texture;
    uint32_t numLayers;
};

class TextureArrayBuilder {
   public:
   private:
};
