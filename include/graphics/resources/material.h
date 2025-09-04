#pragma once

#include <filesystem>
#include <optional>

#include "graphics/resources/texture.h"
#include "graphics/storage/texture.h"

template <typename Material>
struct MaterialBuilder;

class UnlitMaterial;

template <>
class MaterialBuilder<UnlitMaterial> {
   public:
    MaterialBuilder() noexcept
        : albedoTexture(TextureDataHandle::getInvalid()) {

          };

    MaterialBuilder(const MaterialBuilder&) = delete;
    MaterialBuilder& operator=(const MaterialBuilder&) = delete;

    MaterialBuilder(MaterialBuilder&&) = default;
    MaterialBuilder& operator=(MaterialBuilder&&) = default;

    MaterialBuilder& setAlbedoTextureData(TextureDataHandle&& textureHandle) {
        albedoTexture = std::move(textureHandle);
        return *this;
    }

    MaterialBuilder& setAlbedoTextureData(
        std::optional<TextureData>&& texture) {
        if (texture.has_value()) {
            albedoTexture = registerTextureData(std::move(*texture));
        } else {
            albedoTexture = TextureDataHandle::getInvalid();
        }
        return *this;
    }

   private:
    friend class UnlitMaterial;

    TextureDataHandle albedoTexture;
};

class EmptyMaterial;

template <>
class MaterialBuilder<EmptyMaterial> {};
