#pragma once

#include <vector>

#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/texture/array.h"
#include "graphics/resources/material.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/material.h"

template <typename Material>
class Array;

template <>
class Array<UnlitMaterial> {
   public:
    using BufferType = std140::Block<>;

    Array(
        const std::vector<MaterialBuilderHandle<UnlitMaterial>>& builderHandles)
        : albedoTextures{
              TextureArrayBuilder{}
                  .withTexture(std::views::transform(
                      builderHandles,
                      [](const auto& builderHandle)
                          -> const TextureDataHandle& {
                          return builderHandle.get().get().getAlbedoTexture();
                      }))
                  .build()} {}

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    Array(Array&&) = default;
    Array& operator=(Array&&) = default;

    ~Array() = default;

    void bind(const UniformLocations& uniformLocations) const noexcept {
        albedoTextures.bind(albedoTextureUnit);
        glUniform1i(
            uniformLocations.arrayMaterials.unlitMaterial.albedoSampler,
            albedoTextureUnit);
    };

   private:
    const GLuint albedoTextureUnit = 0;

    TextureArray albedoTextures;
};

template <>
class Array<EmptyMaterial> {
   public:
    Array(const std::vector<MaterialBuilderHandle<EmptyMaterial>>&) {}

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    Array(Array&&) = default;
    Array& operator=(Array&&) = default;

    ~Array() = default;

    void bind(const UniformLocations& uniformLocations) const noexcept {};

    using BufferType = std140::Block<>;
};

template <>
struct IsEmptyMaterialT<Array<EmptyMaterial>> : std::true_type {};

template <typename Material>
struct ArrayTextures;

template <typename Material>
class MaterialPack<Array<Material>> {
   public:
    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    static void bind(const MaterialPackHandle<Array<Material>>& materialPack,
                     const UniformLocations& uniformLocations) {
        materialPack.get().get().materialArray.bind(uniformLocations);
    }

   private:
    using BufferType = typename Array<Material>::BufferType;
    friend class MaterialPackBuilder<Material>;

    MaterialPack(
        const std::vector<MaterialBuilderHandle<Material>>& materialHandles)
        : materialArray{materialHandles} {}

    Array<Material> materialArray;
};
