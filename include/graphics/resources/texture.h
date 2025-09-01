#pragma once

#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <stb_image.h>

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

int numComponents(TextureFormat format) { return static_cast<int>(format); }

GLenum getFormat(TextureFormat format) {
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

GLenum getDataFormat(TextureFormat format) {
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
        const std::filesystem::path& filePath, TextureFormat format) {
        int width{0}, height{0}, components{0};
        int desiredChannels = numComponents(format);
        stbi_uc* imageData = stbi_load(filePath.string().c_str(), &width,
                                       &height, &components, desiredChannels);
        if (imageData) {
            auto texture =
                TextureData(imageData, width, height, components, format);
            stbi_image_free(imageData);
            return texture;
        } else {
            std::println(std::cerr,
                         "TextureData: Failed to load image from file {}",
                         filePath.string());
            return std::nullopt;
        }
    }

    static std::optional<TextureData> loadFromBuffer(const uint8_t* bytes,
                                                     size_t byteLength,
                                                     TextureFormat format) {
        int width{0}, height{0}, components{0};
        int desiredChannels = numComponents(format);
        stbi_uc* imageData = stbi_load_from_memory(
            bytes, byteLength, &width, &height, &components, desiredChannels);
        if (imageData) {
            auto texture =
                TextureData(imageData, width, height, components, format);
            stbi_image_free(imageData);
            return texture;
        } else {
            std::println(std::cerr,
                         "TextureData: Failed to load image from buffer");
            return std::nullopt;
        }
    }

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
    static Texture empty() {
        auto format = TextureFormat::RGBA;
        auto textureData = std::array<uint8_t, 4>{255, 255, 255, 255};
        auto textureInfo = TextureInfo{1, 1, format};
        auto samplerConfig = SamplerConfig{.wrapS = GL_CLAMP_TO_EDGE,
                                           .wrapT = GL_CLAMP_TO_EDGE,
                                           .minFilter = GL_NEAREST,
                                           .magFilter = GL_NEAREST};
        return Texture(textureData.data(), textureInfo, samplerConfig);
    }

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
