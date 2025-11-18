#pragma once

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
    MaterialBuilder& operator=(MaterialBuilder&&) noexcept = default;

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

    [[nodiscard]] auto& getAlbedoTexture() const noexcept {
        return albedoTexture;
    }

    [[nodiscard]] auto getTextureDimensions() const noexcept {
        if (albedoTexture.isInvalid()) {
            return TextureDims{};
        }
        return albedoTexture.get().get().getInfo().dimension;
    }

   private:
    TextureDataHandle albedoTexture;
};

class EmptyMaterial;

template <>
class MaterialBuilder<EmptyMaterial> {};
