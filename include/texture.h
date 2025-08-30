#pragma once

#include <glad/glad.h>
#include <filesystem>
#include <iostream>
#include <optional>
#include <array>

struct SamplerConfig {
    GLint wrapS = GL_REPEAT;
    GLint wrapT = GL_REPEAT;
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;
};

enum class TextureFormat : GLenum {
    Grey = 1,
    GreyAlpha = 2,
    RGB = 3,
    RGBA = 4,
};

inline int numComponents(TextureFormat format) { return static_cast<int>(format); }

inline GLenum getFormat(TextureFormat format) {
    switch (format) {
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

inline GLenum getDataFormat(TextureFormat format) {
    switch (format) {
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

struct TextureInfo {
    size_t width;
    size_t height;
    TextureFormat format;
};

class Texture;

class TextureData {
   public:
    static std::optional<TextureData> loadFromFile(
        const std::filesystem::path& filePath, TextureFormat format);

    static std::optional<TextureData> loadFromBuffer(const uint8_t* bytes,
                                                     size_t byteLength,
                                                     TextureFormat format);

   private:
    friend class Texture;

    TextureData(uint8_t* textureData, int width, int height, int components,
                TextureFormat format)
        : imageData(textureData, textureData + width * height * components),
          info{static_cast<size_t>(width), static_cast<size_t>(height),
               format} {};

    std::vector<uint8_t> imageData;
    TextureInfo info;
};

class Texture {
   public:
    static Texture empty();

    static Texture load(const TextureData& textureData,
                        const SamplerConfig& samplerConfig) {
        return Texture(textureData.imageData.data(), textureData.info,
                       samplerConfig);
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept
        : texture(other.texture), bindlessHandle(other.bindlessHandle) {
        other.texture = 0;
        other.bindlessHandle = 0;
    };
    Texture& operator=(Texture&& other) noexcept {
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
    void setResident() { glMakeTextureHandleResidentARB(bindlessHandle); }
    void setNotResident() { glMakeTextureHandleNonResidentARB(bindlessHandle); }

    ~Texture() { glDeleteTextures(1, &texture); }

   private:
    Texture(const uint8_t* textureData, const TextureInfo& info,
            const SamplerConfig& samplerConfig);

    GLuint texture;
    GLuint64 bindlessHandle;
};

Texture tryLoadFromFile(const std::filesystem::path& path,
                        const SamplerConfig& samplerConfig);

Texture tryLoadFromBuffer(const uint8_t* buffer, size_t size,
                          const SamplerConfig& samplerConfig);
