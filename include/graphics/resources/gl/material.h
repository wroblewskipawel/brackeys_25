#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <ranges>
#include <vector>

#include "graphics/resources/gl/buffer/binding.h"
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/shader/uniform.h"
#include "graphics/resources/gl/texture/array.h"
#include "graphics/resources/material.h"
#include "graphics/resources/texture.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/material.h"

constexpr size_t materialPackBufferBinding = 0;

template <typename Material>
class MaterialPack;

template <typename Material>
class MaterialPackBuilder {
   public:
    MaterialPackBuilder() = default;

    MaterialPackBuilder(const MaterialPackBuilder&) = delete;
    MaterialPackBuilder& operator=(const MaterialPackBuilder&) = delete;

    MaterialPackBuilder(MaterialPackBuilder&&) = default;
    MaterialPackBuilder& operator=(MaterialPackBuilder&&) = default;

    ~MaterialPackBuilder() = default;

    using MaterialBuilderHandle = MaterialBuilderHandle<Material>;
    MaterialPackBuilder& addMaterial(
        const MaterialBuilderHandle& builderHandle) {
        materialHandles.emplace_back(builderHandle.copy());
        return *this;
    }

    MaterialPackBuilder& addMaterialMulti(
        const std::vector<MaterialBuilderHandle>& builderHandles) {
        for (const auto& materialHandles : builderHandles) {
            addMaterial(materialHandles);
        }
        return *this;
    }

    auto build() {
        if (materialHandles.size() == 0) {
            return MaterialPackHandle<Material>::getInvalid();
        }

        return registerMaterialPack(MaterialPack<Material>(materialHandles));
    }

   private:
    std::vector<MaterialBuilderHandle> materialHandles;
};

template <typename Material>
struct IsEmptyMaterialT : std::false_type {};

template <>
struct IsEmptyMaterialT<EmptyMaterial> : std::true_type {};

template <typename Material>
constexpr bool IsEmptyMaterialV = IsEmptyMaterialT<Material>::value;

template <typename Material>
concept EmptyMaterialType = IsEmptyMaterialV<Material>;

template <>
class MaterialPack<UnlitMaterial> {
   public:
    MaterialPack(
        const std::vector<MaterialBuilderHandle<UnlitMaterial>>& builderHandles)
        : albedoTextures{TextureArrayBuilder{}
                             .withTexture(std::views::transform(
                                 builderHandles,
                                 [](const auto& builderHandle)
                                     -> const TextureDataHandle& {
                                     return builderHandle.get()
                                         .get()
                                         .getAlbedoTexture();
                                 }))
                             .build()},
          materialUniforms{
              std140::UniformArrayBuilder<BufferType>{}
                  .pushMulti(std::views::transform(
                      albedoTextures.getLayerInfos(),
                      [arrayInfo = albedoTextures.getTextureInfo()](
                          const auto& layerInfo) {
                          return BufferType{
                              static_cast<float>(layerInfo.dimension.width) /
                                  static_cast<float>(arrayInfo.dimension.width),
                              static_cast<float>(layerInfo.dimension.height) /
                                  static_cast<float>(arrayInfo.dimension.height)};
                      }))
                  .build()} {}

    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) noexcept = default;

    ~MaterialPack() = default;

    void bind(const UniformLocations& uniformLocations) const noexcept {
        albedoTextures.bind(albedoTextureUnit);
        glUniform1i(uniformLocations.arrayMaterials.unlitMaterial.albedoSampler,
                    albedoTextureUnit);
        BindingState::bindBuffer<BufferBindings::Storage>(
            materialUniforms.getBuffer(), materialPackBufferBinding);
    };

   private:
    inline static constexpr GLuint albedoTextureUnit = 0;
    using BufferType = std140::Block<GLfloat, GLfloat>;

    TextureArray albedoTextures;
    std140::UniformArray<BufferType> materialUniforms;
};

template <>
class MaterialPack<EmptyMaterial> {
   public:
    MaterialPack(const std::vector<MaterialBuilderHandle<EmptyMaterial>>&) {}

    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    ~MaterialPack() = default;

    void bind(const UniformLocations& uniformLocations) const noexcept {};
};

template <typename Material>
inline void bindMaterialPack(const MaterialPackHandle<Material>& materialPack,
                             const UniformLocations& uniformLocations) {
    materialPack.get().get().bind(uniformLocations);
}
