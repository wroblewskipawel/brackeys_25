#pragma once

#include <glm/glm.hpp>
#include <ranges>
#include <vector>

#include "graphics/resources/gl/buffer/binding.h"
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/shader/uniform.h"
#include "graphics/resources/gl/texture/array.h"
#include "graphics/resources/material.h"
#include "graphics/resources/texture.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/material.h"

template <typename Material>
class Array;

template <>
class Array<UnlitMaterial> {
   public:
    Array(
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
                              static_cast<float>(layerInfo.width) /
                                  static_cast<float>(arrayInfo.width),
                              static_cast<float>(layerInfo.height) /
                                  static_cast<float>(arrayInfo.height)};
                      }))
                  .build()} {}

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    Array(Array&&) = default;
    Array& operator=(Array&&) noexcept = default;

    ~Array() = default;

    void bind(const UniformLocations& uniformLocations) const noexcept {
        albedoTextures.bind(albedoTextureUnit);
        glUniform1i(uniformLocations.arrayMaterials.unlitMaterial.albedoSampler,
                    albedoTextureUnit);
        BindingState::bindBuffer<BufferBindings::Storage>(
            materialUniforms.getBuffer(),
            materialPackBufferBinding);
    };

   private:
    inline static constexpr GLuint albedoTextureUnit = 0;
    using BufferType = std140::Block<GLfloat, GLfloat>;

    TextureArray albedoTextures;
    std140::UniformArray<BufferType> materialUniforms;
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
    friend class MaterialPackBuilder<Material>;

    MaterialPack(
        const std::vector<MaterialBuilderHandle<Material>>& materialHandles)
        : materialArray{materialHandles} {}

    Array<Material> materialArray;
};
