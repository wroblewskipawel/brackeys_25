#pragma once

#include <cstdint>
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include <filesystem>
#include <iostream>
#include <magic_enum.hpp>
#include <optional>

enum class TextureFormat : uint8_t {
    Grey = 1,
    GreyAlpha = 2,
    RGB = 3,
    RGBA = 4,
};

inline int numComponents(TextureFormat format) {
    return static_cast<int>(format);
}

inline TextureFormat getMinFormat(TextureFormat first, TextureFormat second) {
    auto firstValue = magic_enum::enum_integer(first);
    auto secondValue = magic_enum::enum_integer(second);
    return firstValue < secondValue ? first : second;
}

struct TextureInfo {
    size_t width = 0;
    size_t height = 0;
    TextureFormat format = TextureFormat::Grey;

    friend bool operator==(const TextureInfo& lhs,
                           const TextureInfo& rhs) noexcept {
        return lhs.width == rhs.width && lhs.height == rhs.height &&
               lhs.format == rhs.format;
    }

    bool isValid() const noexcept { return width != 0 && height != 0; }

    friend auto join(const TextureInfo& lhs, const TextureInfo& rhs) noexcept {
        if (lhs.format != rhs.format) {
            std::println(std::cerr,
                         "TextureInfo::join: texture format mismatch");
            std::abort();
        }
        return TextureInfo{
            .width = std::max(lhs.width, rhs.width),
            .height = std::max(lhs.height, rhs.height),
            .format = lhs.format
        };
    }
};

class TextureData {
   public:
    static TextureInfo getFileInfo(const std::filesystem::path& filePath) {
        int width{0}, height{0}, components{0};
        stbi_info(filePath.string().c_str(), &width, &height, &components);
        return TextureInfo{
            .width = static_cast<size_t>(width),
            .height = static_cast<size_t>(height),
            .format = static_cast<TextureFormat>(components),
        };
    };

    static TextureInfo getBufferInfo(const uint8_t* bytes, size_t byteLength) {
        int width{0}, height{0}, components{0};
        stbi_info_from_memory(bytes, static_cast<int>(byteLength), &width, &height, &components);
        return TextureInfo{
            .width = static_cast<size_t>(width),
            .height = static_cast<size_t>(height),
            .format = static_cast<TextureFormat>(components),
        };
    };

    static std::optional<TextureData> loadFromFile(
        const std::filesystem::path& filePath, TextureFormat desiredFormat) {
        int width{0}, height{0}, components{0};
        auto imageInfo = getFileInfo(filePath);
        imageInfo.format = getMinFormat(imageInfo.format, desiredFormat);
        stbi_uc* imageData =
            stbi_load(filePath.string().c_str(), &width, &height, &components,
                      numComponents(imageInfo.format));
        if (imageData) {
            auto texture = TextureData(imageData, imageInfo);
            stbi_image_free(imageData);
            return texture;
        } else {
            std::println(std::cerr,
                         "TextureData: Failed to load image from file {}",
                         filePath.string());
            return std::nullopt;
        }
    }

    static std::optional<TextureData> loadFromBuffer(
        const uint8_t* bytes, size_t byteLength, TextureFormat desiredFormat) {
        int width{0}, height{0}, components{0};
        auto imageInfo = getBufferInfo(bytes, byteLength);
        imageInfo.format = getMinFormat(imageInfo.format, desiredFormat);
        stbi_uc* imageData =
            stbi_load_from_memory(bytes, static_cast<int>(byteLength), &width, &height,
                                  &components, numComponents(imageInfo.format));
        if (imageData) {
            auto texture = TextureData(imageData, imageInfo);
            stbi_image_free(imageData);
            return texture;
        } else {
            std::println(std::cerr,
                         "TextureData: Failed to load image from buffer");
            return std::nullopt;
        }
    }

   private:
    friend class TextureArray;
    friend class TextureArrayBuilder;

    TextureData(uint8_t* textureData, TextureInfo textureInfo)
        : imageData(textureData,
                    textureData + textureInfo.width * textureInfo.height *
                                      numComponents(textureInfo.format)),
          info{textureInfo} {};

    std::vector<uint8_t> imageData;
    TextureInfo info;
};
