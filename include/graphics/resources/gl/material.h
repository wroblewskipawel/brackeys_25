#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <glm/glm.hpp>

#include "graphics/resources/gl/buffer/binding.h"
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/gl/texture/bindless.h"
#include "graphics/resources/material.h"
#include "graphics/storage/gl/material.h"

constexpr size_t materialPackBufferBinding = 0;

class UnlitMaterial {
   public:
    using BufferType = std140::Block<GLuint64>;
    using BuilderHandleType = MaterialBuilderHandle<UnlitMaterial>;

    UnlitMaterial(const BuilderHandleType& builderHandle)
        : albedoTexture(tryLoadFromDataHandle<TextureBindless>(
              builderHandle.get().get().albedoTexture, SamplerConfig{})) {}

    UnlitMaterial(const UnlitMaterial&) = delete;
    UnlitMaterial& operator=(const UnlitMaterial&) = delete;

    UnlitMaterial(UnlitMaterial&&) = default;
    UnlitMaterial& operator=(UnlitMaterial&&) = default;

    ~UnlitMaterial() = default;

    BufferType getUniformBuffer() const {
        BufferType buffer{albedoTexture.getBindlessHandle()};
        return buffer;
    }

    void setResident() const { albedoTexture.setResident(); }

    void setNotResident() const { albedoTexture.setNotResident(); }

   private:
    TextureBindless albedoTexture;
};

class EmptyMaterial {
   public:
    using BufferType = std140::Block<>;
    using BuilderHandleType = MaterialBuilderHandle<EmptyMaterial>;

    EmptyMaterial(const BuilderHandleType&) {}

    EmptyMaterial(const EmptyMaterial&) = delete;
    EmptyMaterial& operator=(const EmptyMaterial&) = delete;

    EmptyMaterial(EmptyMaterial&&) = default;
    EmptyMaterial& operator=(EmptyMaterial&&) = default;

    ~EmptyMaterial() = default;

    BufferType getUniformBuffer() const {
        BufferType buffer{};
        return buffer;
    }

    void setResident() const {}
    void setNotResident() const {}
};

template <typename Material>
class MaterialData {
   public:
    MaterialData(std::vector<Material>&& materials)
        : materialData(std::move(materials)), isResident(false) {}

    MaterialData(const MaterialData&) = delete;
    MaterialData& operator=(const MaterialData&) = delete;

    MaterialData(MaterialData&&) = default;
    MaterialData& operator=(MaterialData&&) = default;

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

   private:
    std::vector<Material> materialData;
    mutable bool isResident;
};

template <typename Material>
class MaterialPackBuilder;

template <typename Material>
class MaterialPack {
   public:
    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    static void bind(const MaterialPackHandle<Material>& materialPack) {
        if (currentPackIndex != materialPack) {
            if (!currentPackIndex.isInvalid()) {
                auto& currentPack = currentPackIndex.get().get();
                currentPack.materialData.setNotResident();
            }
            auto& newPack = materialPack.get().get();

            newPack.materialData.setResident();
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
    using BufferType = typename Material::BufferType;
    friend class MaterialPackBuilder<Material>;

    MaterialPack(std140::UniformArrayBuilder<BufferType>&& materialUniforms,
                 std::vector<Material>&& materialData)
        : materialUniforms(materialUniforms.build()),
          materialData(std::move(materialData)) {}

    inline static auto currentPackIndex =
        MaterialPackHandle<Material>::getInvalid();

    std140::UniformArray<BufferType> materialUniforms;
    MaterialData<Material> materialData;
};

template <typename Material>
class MaterialPackBuilder {
   public:
    MaterialPackBuilder() = default;

    MaterialPackBuilder(const MaterialPackBuilder&) = delete;
    MaterialPackBuilder& operator=(const MaterialPackBuilder&) = delete;

    MaterialPackBuilder(MaterialPackBuilder&&) = default;
    MaterialPackBuilder& operator=(MaterialPackBuilder&&) = default;

    ~MaterialPackBuilder() = default;

    using BuilderHandleType = typename Material::BuilderHandleType;

    MaterialPackBuilder& addMaterial(const BuilderHandleType& builderHandle) {
        materialData.emplace_back(builderHandle.copy());
        materialUniforms.push(materialData.back().getUniformBuffer());
        return *this;
    }

    MaterialPackBuilder& addMaterialMulti(
        const std::vector<BuilderHandleType>& builderHandles) {
        for (const auto& handle : builderHandles) {
            addMaterial(handle);
        }
        return *this;
    }

    MaterialPackHandle<Material> build() {
        if (materialData.size() == 0) {
            return MaterialPackHandle<Material>::getInvalid();
        }

        auto materialPack = MaterialPack<Material>(std::move(materialUniforms),
                                                   std::move(materialData));

        return registerMaterialPack(std::move(materialPack));
    }

   private:
    using BufferType = typename Material::BufferType;

    std140::UniformArrayBuilder<BufferType> materialUniforms;
    std::vector<Material> materialData;
};
