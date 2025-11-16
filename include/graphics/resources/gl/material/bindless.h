#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include "graphics/resources/gl/buffer/binding.h"
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/shader/uniform.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/gl/texture/bindless.h"
#include "graphics/resources/material.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/material.h"

template <typename Material>
class Bindless;

template <>
class Bindless<UnlitMaterial> {
   public:
    using BufferType = std140::Block<GLuint64>;

    Bindless(const MaterialBuilderHandle<UnlitMaterial>& builderHandle)
        : albedoTexture(tryLoadFromDataHandle<TextureBindless>(
              builderHandle.get().get().getAlbedoTexture(), SamplerConfig{})) {}

    Bindless(const Bindless&) = delete;
    Bindless& operator=(const Bindless&) = delete;

    Bindless(Bindless&&) = default;
    Bindless& operator=(Bindless&&) = default;

    ~Bindless() = default;

    [[nodiscard]] BufferType getUniformBuffer() const {
        BufferType buffer{albedoTexture.getBindlessHandle()};
        return buffer;
    }

    void setResident() const { albedoTexture.setResident(); }

    void setNotResident() const { albedoTexture.setNotResident(); }

   private:
    TextureBindless albedoTexture;
};

template <>
class Bindless<EmptyMaterial> {
   public:
    using BufferType = std140::Block<>;

    Bindless(const MaterialBuilderHandle<EmptyMaterial>&) {}

    Bindless(const Bindless&) = delete;
    Bindless& operator=(const Bindless&) = delete;

    Bindless(Bindless&&) = default;
    Bindless& operator=(Bindless&&) = default;

    ~Bindless() = default;

    [[nodiscard]] BufferType getUniformBuffer() const {
        BufferType buffer{};
        return buffer;
    }

    void setResident() const {}
    void setNotResident() const {}
};

template <>
struct IsEmptyMaterialT<Bindless<EmptyMaterial>> : std::true_type {};

template <typename Material>
class MaterialPack<Bindless<Material>> {
   public:
    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    static void bind(const MaterialPackHandle<Bindless<Material>>& materialPack,
                     const UniformLocations& uniformLocations) {
        if (currentPackIndex != materialPack) {
            if (!currentPackIndex.isInvalid()) {
                auto& currentPack = currentPackIndex.get().get();
                currentPack.setNotResident();
            }
            auto& newPack = materialPack.get().get();

            newPack.setResident();
            currentPackIndex = materialPack;
        }
        auto& currentPack = currentPackIndex.get().get();

        BindingState::bindBuffer<BufferBindings::Storage>(
            currentPack.materialUniforms.getBuffer(),
            materialPackBufferBinding);
    }

    size_t numMaterials() const noexcept {
        return materialUniforms.getNumElements();
    }

   private:
    using BufferType = typename Bindless<Material>::BufferType;
    friend class MaterialPackBuilder<Material>;

    MaterialPack(
        const std::vector<MaterialBuilderHandle<Material>>& materialHandles)
        : materialData(std::views::transform(materialHandles,
                                             [](const auto& materialHandle) {
                                                 return Bindless<Material>{
                                                     materialHandle};
                                             }) |
                       std::ranges::to<std::vector>()),
          materialUniforms(std140::UniformArrayBuilder<BufferType>()
                               .pushMulti(std::views::transform(
                                   materialData,
                                   [](const auto& materialData) {
                                       return materialData.getUniformBuffer();
                                   }))
                               .build()),
          isResident(false) {}

    void setResident() const {
        if (!isResident) {
            for (auto& material : materialData) {
                material.setResident();
            }
            isResident = true;
        }
    }

    void setNotResident() const {
        if (isResident) {
            for (auto& material : materialData) {
                material.setNotResident();
            }
            isResident = false;
        }
    }

    inline static auto currentPackIndex =
        MaterialPackHandle<Bindless<Material>>::getInvalid();

    std::vector<Bindless<Material>> materialData;
    std140::UniformArray<BufferType> materialUniforms;
    mutable bool isResident;
};
