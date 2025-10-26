#pragma once

#include "graphics/resources/gl/texture.h"

class TextureBindless {
   public:
    static TextureBindless empty() {
        auto format = TextureFormat::RGBA;
        auto textureData = std::array<uint8_t, 4>{255, 255, 255, 255};
        auto textureInfo = TextureInfo{1, 1, format};
        auto samplerConfig = SamplerConfig{.wrapS = GL_CLAMP_TO_EDGE,
                                           .wrapT = GL_CLAMP_TO_EDGE,
                                           .minFilter = GL_NEAREST,
                                           .magFilter = GL_NEAREST};
        return TextureBindless(textureData.data(), textureInfo, samplerConfig);
    }

    static TextureBindless load(const TextureData& textureData,
                                const SamplerConfig& samplerConfig) {
        return TextureBindless(textureData.imageData.data(), textureData.info,
                               samplerConfig);
    }

    TextureBindless(const TextureBindless&) = delete;
    TextureBindless& operator=(const TextureBindless&) = delete;

    TextureBindless(TextureBindless&& other) noexcept
        : texture(other.texture), bindlessHandle(other.bindlessHandle) {
        other.texture = 0;
        other.bindlessHandle = 0;
    };
    TextureBindless& operator=(TextureBindless&& other) noexcept {
        if (this != &other) {
            glDeleteTextures(1, &texture);
            texture = other.texture;
            bindlessHandle = other.bindlessHandle;
            other.texture = 0;
            other.bindlessHandle = 0;
        }
        return *this;
    };

    GLuint64 getBindlessHandle() const { return bindlessHandle; }
    void setResident() const { glMakeTextureHandleResidentARB(bindlessHandle); }
    void setNotResident() const {
        glMakeTextureHandleNonResidentARB(bindlessHandle);
    }

    ~TextureBindless() { glDeleteTextures(1, &texture); }

   private:
    TextureBindless(const uint8_t* textureData, const TextureInfo& info,
                    const SamplerConfig& samplerConfig) {
        glCreateTextures(GL_TEXTURE_2D, 1, &texture);
        glTextureStorage2D(texture, 1, getDataFormat(info.format),
                           static_cast<GLsizei>(info.width),
                           static_cast<GLsizei>(info.height));
        glTextureSubImage2D(texture, 0, 0, 0, static_cast<GLsizei>(info.width),
                            static_cast<GLsizei>(info.height),
                            getFormat(info.format), GL_UNSIGNED_BYTE,
                            textureData);

        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER,
                            samplerConfig.minFilter);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER,
                            samplerConfig.magFilter);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, samplerConfig.wrapS);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, samplerConfig.wrapT);
        bindlessHandle = glGetTextureHandleARB(texture);
    }

    GLuint texture;
    GLuint64 bindlessHandle;
};
