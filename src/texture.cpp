#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

std::optional<TextureData> TextureData::loadFromFile(
    const std::filesystem::path& filePath, TextureFormat format) {
    int width{0}, height{0}, components{0};
    int desiredChannels = numComponents(format);
    stbi_uc* imageData = stbi_load(filePath.string().c_str(), &width, &height,
                                   &components, desiredChannels);
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

std::optional<TextureData> TextureData::loadFromBuffer(const uint8_t* bytes,
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
};

Texture Texture::empty() {
    auto format = TextureFormat::RGBA;
    auto textureData = std::array<uint8_t, 4>{255, 255, 255, 255};
    auto textureInfo = TextureInfo{1, 1, format};
    auto samplerConfig = SamplerConfig{.wrapS = GL_CLAMP_TO_EDGE,
                                       .wrapT = GL_CLAMP_TO_EDGE,
                                       .minFilter = GL_NEAREST,
                                       .magFilter = GL_NEAREST};
    return Texture(textureData.data(), textureInfo, samplerConfig);
}

Texture::Texture(const uint8_t* textureData, const TextureInfo& info,
                 const SamplerConfig& samplerConfig) {
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, getDataFormat(info.format),
                       static_cast<GLsizei>(info.width),
                       static_cast<GLsizei>(info.height));
    glTextureSubImage2D(texture, 0, 0, 0, static_cast<GLsizei>(info.width),
                        static_cast<GLsizei>(info.height),
                        getFormat(info.format), GL_UNSIGNED_BYTE, textureData);

    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER,
                        samplerConfig.minFilter);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER,
                        samplerConfig.magFilter);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, samplerConfig.wrapS);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, samplerConfig.wrapT);
    bindlessHandle = glGetTextureHandleARB(texture);
}

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
