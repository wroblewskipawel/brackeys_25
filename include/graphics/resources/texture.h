#pragma once

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>
#include <filesystem>
#include <iostream>
#include <optional>
#include <array>

enum class TextureFormat : GLenum {
    Grey = 1,
    GreyAlpha = 2,
    RGB = 3,
    RGBA = 4,
};

int numComponents(TextureFormat format) { return static_cast<int>(format); }

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
