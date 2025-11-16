#pragma once

#include <glad/glad.h>

#include "concepts/range.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/gl/texture/binding.h"
#include "graphics/resources/texture.h"

class TextureArrayBuilder;

class TextureArray {
   public:
    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    TextureArray(TextureArray&& other) noexcept
        : texture(other.texture), layerInfos(std::move(other.layerInfos)) {
        other.texture = 0;
    };
    TextureArray& operator=(TextureArray&& other) noexcept {
        if (this != &other) {
            texture = other.texture;
            layerInfos = std::move(other.layerInfos);
            other.texture = 0;
        }
        return *this;
    };

    ~TextureArray() { glDeleteTextures(1, &texture); };

    void bind(GLuint unitIndex) const noexcept {
        TextureUnitState::bindTexture<TextureBinding::Array>(texture,
                                                             unitIndex);
    }

    [[nodiscard]] auto getTextureInfo() const noexcept {
        return arrayInfo;
    }

    [[nodiscard]] auto& getLayerInfos() const noexcept {
        return layerInfos;
    }

   private:
    friend class TextureArrayBuilder;

    template <typename Layers>
        requires RefConstRange<Layers, TextureData>
    TextureArray(Layers&& layers, const TextureInfo& info,
                 const SamplerConfig& samplerConfig) : arrayInfo(info) {
        auto numLayers = std::ranges::distance(layers);
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &texture);
        glTextureStorage3D(texture, getMipLevels(info), getDataFormat(info),
                           static_cast<GLsizei>(info.width),
                           static_cast<GLsizei>(info.height),
                           static_cast<GLsizei>(numLayers));
        layerInfos.reserve(numLayers);
        for (auto [i, layer] : std::views::enumerate(layers)) {
            glTextureSubImage3D(texture, 0, 0, 0, static_cast<GLint>(i),
                                static_cast<GLsizei>(layer.info.width),
                                static_cast<GLsizei>(layer.info.height), 1,
                                getFormat(layer.info), GL_UNSIGNED_BYTE,
                                layer.imageData.data());
            layerInfos.emplace_back(layer.info);
        }
        applySamplerConfig(texture, samplerConfig);
    };

    GLuint texture;
    TextureInfo arrayInfo;
    std::vector<TextureInfo> layerInfos;
};

class TextureArrayBuilder {
   public:
    TextureArrayBuilder() = default;

    TextureArrayBuilder(const TextureArrayBuilder&) = delete;
    TextureArrayBuilder& operator=(const TextureArrayBuilder&) = delete;

    TextureArrayBuilder(TextureArrayBuilder&&) = delete;
    TextureArrayBuilder& operator=(TextureArrayBuilder&&) = delete;

    auto& withTexture(const TextureDataHandle& dataHandle) {
        auto& textureData = dataHandle.get().get();
        if (arrayInfo.isValid()) {
            arrayInfo = join(textureData.info, arrayInfo);
        } else {
            arrayInfo = textureData.info;
        }
        textureHandles.emplace_back(dataHandle.copy());
        return *this;
    }

    template <typename Range>
        requires RefConstRange<Range, TextureDataHandle>
    auto& withTexture(Range&& range) {
        for (const auto& textureHandle : range) {
            withTexture(textureHandle);
        }
        return *this;
    }

    auto& withSamplerConfig(const SamplerConfig& samplerConfig) {
        arraySamplerConfig = samplerConfig;
        return *this;
    }

    auto build() {
        return TextureArray(
            std::views::transform(textureHandles,
                                  [](const auto& textureHandle) {
                                      return textureHandle.get().get();
                                  }),
            arrayInfo, arraySamplerConfig);
    }

   private:
    TextureInfo arrayInfo;
    SamplerConfig arraySamplerConfig;

    std::vector<TextureDataHandle> textureHandles;
};
