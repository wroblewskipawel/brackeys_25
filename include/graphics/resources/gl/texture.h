#pragma once

#include <glad/glad.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <optional>

#include "graphics/resources/texture.h"
#include "graphics/storage/texture.h"

struct SamplerConfig {
    GLint wrapS = GL_REPEAT;
    GLint wrapT = GL_REPEAT;
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;
};

inline void applySamplerConfig(GLuint texture,
                               const SamplerConfig& config) noexcept {
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, config.minFilter);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, config.magFilter);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, config.wrapS);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, config.wrapT);
}

inline GLenum getFormat(const TextureInfo& info) noexcept {
    switch (info.format) {
        case TextureFormat::Grey:
            return GL_RED;
        case TextureFormat::GreyAlpha:
            return GL_RG;
        case TextureFormat::RGB:
            return GL_RGB;
        case TextureFormat::RGBA:
            return GL_RGBA;
        default:
            return GL_NONE;
    }
}

inline GLenum getDataFormat(const TextureInfo& info) noexcept {
    switch (info.format) {
        case TextureFormat::Grey:
            return GL_R8;
        case TextureFormat::GreyAlpha:
            return GL_RG8;
        case TextureFormat::RGB:
            return GL_RGB8;
        case TextureFormat::RGBA:
            return GL_RGBA8;
        default:
            return GL_NONE;
    }
}

inline GLsizei getMipLevels(const TextureInfo& info) noexcept { return 1; }

template <typename Texture>
Texture tryLoadFromDataHandle(const TextureDataHandle& textureDataHandle,
                              const SamplerConfig& samplerConfig) {
    if (textureDataHandle.isInvalid()) {
        std::println(std::cerr, "Texture: Mising texture data");
        return Texture::empty();
    }
    return Texture::load(textureDataHandle.get().get(), samplerConfig);
}

template <typename Texture>
Texture tryLoadFromData(const std::optional<TextureData>& textureData,
                        const SamplerConfig& samplerConfig) {
    if (!textureData.has_value()) {
        std::println(std::cerr, "Texture: Mising texture data");
        return Texture::empty();
    }
    return Texture::load(textureData.value(), samplerConfig);
}

template <typename Texture>
Texture tryLoadFromFile(const std::filesystem::path& path,
                        const SamplerConfig& samplerConfig) {
    auto textureData = TextureData::loadFromFile(path, TextureFormat::RGB);
    if (!textureData) {
        std::println(std::cerr, "Texture: Failed to load texture from {}",
                     path.string());
        return Texture::empty();
    }
    return Texture::load(textureData.value(), samplerConfig);
}

template <typename Texture>
Texture tryLoadFromBuffer(const uint8_t* buffer, size_t size,
                          const SamplerConfig& samplerConfig) {
    auto textureData =
        TextureData::loadFromBuffer(buffer, size, TextureFormat::RGB);
    if (!textureData) {
        std::println(std::cerr, "Texture: Failed to load texture from buffer");
        return Texture::empty();
    }
    return Texture::load(textureData.value(), samplerConfig);
}
