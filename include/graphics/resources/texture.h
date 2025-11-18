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

struct TextureDims {
    size_t width = 0;
    size_t height = 0;

    friend bool operator==(const TextureDims& lhs,
                           const TextureDims& rhs) noexcept {
        return lhs.width == rhs.width && lhs.height == rhs.height;
    }

    [[nodiscard]] bool isValid() const noexcept {
        return width != 0 && height != 0;
    }

    friend class std::hash<TextureDims>;
};

namespace std {
template <>
struct hash<TextureDims> {
    std::size_t operator()(const TextureDims& dims) const noexcept {
        std::size_t h1 = std::hash<size_t>{}(dims.width);
        std::size_t h2 = std::hash<size_t>{}(dims.height);
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

struct TextureInfo {
    TextureDims dimension;
    TextureFormat format = TextureFormat::Grey;

    friend bool operator==(const TextureInfo& lhs,
                           const TextureInfo& rhs) noexcept {
        return lhs.dimension == rhs.dimension && lhs.format == rhs.format;
    }

    [[nodiscard]] bool isValid() const noexcept { return dimension.isValid(); }

    [[nodiscard]] auto getRequredByteSize() const noexcept {
        return dimension.width * dimension.height * numComponents(format);
    }

    friend class std::hash<TextureInfo>;
};

namespace std {
template <>
struct hash<TextureInfo> {
    std::size_t operator()(const TextureInfo& info) const noexcept {
        std::size_t h1 = std::hash<TextureDims>{}(info.dimension);
        std::size_t h2 = std::hash<uint8_t>{}(static_cast<uint8_t>(info.format));
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

class TextureData {
   public:
    static TextureInfo getFileInfo(const std::filesystem::path& filePath) {
        int width{0}, height{0}, components{0};
        stbi_info(filePath.string().c_str(), &width, &height, &components);
        return TextureInfo{
            .dimension =
                TextureDims{
                    .width = static_cast<size_t>(width),
                    .height = static_cast<size_t>(height),
                },
            .format = static_cast<TextureFormat>(components),
        };
    };

    static TextureInfo getBufferInfo(const uint8_t* bytes, size_t byteLength) {
        int width{0}, height{0}, components{0};
        stbi_info_from_memory(bytes, static_cast<int>(byteLength), &width,
                              &height, &components);
        return TextureInfo{
            .dimension =
                TextureDims{
                    .width = static_cast<size_t>(width),
                    .height = static_cast<size_t>(height),
                },
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
        stbi_uc* imageData = stbi_load_from_memory(
            bytes, static_cast<int>(byteLength), &width, &height, &components,
            numComponents(imageInfo.format));
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

    [[nodiscard]] auto& getInfo() const noexcept {
        return info;
    }

   private:
    friend class TextureArray;
    friend class TextureArrayBuilder;

    TextureData(uint8_t* textureData, TextureInfo textureInfo)
        : imageData(textureData,
                    textureData + textureInfo.getRequredByteSize()),
          info{textureInfo} {};

    std::vector<uint8_t> imageData;
    TextureInfo info;
};
