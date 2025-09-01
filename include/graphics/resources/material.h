#pragma once

#include <filesystem>
#include <optional>

#include "graphics/resources/texture.h"

template <typename Material>
struct MaterialBuilder;

class UnlitMaterial;

template <>
class MaterialBuilder<UnlitMaterial> {
   public:
    MaterialBuilder() = default;

    MaterialBuilder(const MaterialBuilder&) = delete;
    MaterialBuilder& operator=(const MaterialBuilder&) = delete;

    MaterialBuilder(MaterialBuilder&&) = default;
    MaterialBuilder& operator=(MaterialBuilder&&) = default;

    MaterialBuilder& setAlbedoTextureData(
        std::optional<TextureData>&& texture) {
        albedoTexture = std::move(texture);
        return *this;
    }

   private:
    friend class UnlitMaterial;

    std::optional<TextureData> albedoTexture;
};

class EmptyMaterial;

template <>
class MaterialBuilder<EmptyMaterial> {};
