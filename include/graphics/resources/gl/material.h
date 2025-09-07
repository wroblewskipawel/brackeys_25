#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <glm/glm.hpp>

#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/material.h"
#include "graphics/storage/gl/material.h"

constexpr size_t materialPackBufferBinding = 0;

template <typename Material>
class MaterialPack;

template <typename Material>
class MaterialData;

class UnlitMaterial {
   public:
    using BufferType = std140::Block<GLuint64>;
    using BuilderType = MaterialBuilder<UnlitMaterial>;

    UnlitMaterial(const BuilderType& builder)
        : albedoTexture(
              tryLoadFromDataHandle(builder.albedoTexture, SamplerConfig{})) {}

    UnlitMaterial(const UnlitMaterial&) = delete;
    UnlitMaterial& operator=(const UnlitMaterial&) = delete;

    UnlitMaterial(UnlitMaterial&&) = default;
    UnlitMaterial& operator=(UnlitMaterial&&) = default;

    ~UnlitMaterial() = default;

    BufferType getUniformBuffer() const {
        BufferType buffer{albedoTexture.getBindlessHandle()};
        return buffer;
    }

   private:
    friend class MaterialPack<UnlitMaterial>;
    friend class MaterialData<UnlitMaterial>;

    void setResident() { albedoTexture.setResident(); }

    void setNotResident() { albedoTexture.setNotResident(); }

    Texture albedoTexture;
};

class EmptyMaterial {
   public:
    using BufferType = std140::Block<>;
    using BuilderType = MaterialBuilder<EmptyMaterial>;

    EmptyMaterial(const BuilderType&) {}

    EmptyMaterial(const EmptyMaterial&) = delete;
    EmptyMaterial& operator=(const EmptyMaterial&) = delete;

    EmptyMaterial(EmptyMaterial&&) = default;
    EmptyMaterial& operator=(EmptyMaterial&&) = default;

    ~EmptyMaterial() = default;

    BufferType getUniformBuffer() const {
        BufferType buffer{};
        return buffer;
    }

   private:
    friend class MaterialPack<EmptyMaterial>;
    friend class MaterialData<EmptyMaterial>;

    void setResident() {}
    void setNotResident() {}
};

template <typename Material>
struct MaterialHandle {
    size_t materialIndex;
    MaterialPackHandle<Material> packHandle;

    MaterialHandle copy() const noexcept {
        return {materialIndex, packHandle.copy()};
    }

    static auto getPackHandles(
        const MaterialPackHandle<Material>& packHandle) noexcept {
        const auto& materialPack = packHandle.get().get();
        auto materialHandles = std::vector<MaterialHandle<Material>>();
        for (size_t materialIndex = 0;
             materialIndex < materialPack.numMaterials(); materialIndex++) {
            materialHandles.emplace_back(
                MaterialHandle(materialIndex, packHandle.copy()));
        }
        return materialHandles;
    }
};

template <typename Material>
inline auto getPackHandles(
    const MaterialPackHandle<Material>& packHandle) noexcept {
    return MaterialHandle<Material>::getPackHandles(packHandle);
}

template <typename Material>
class MaterialPackBuilder;

template <typename Material>
class MaterialData {
   public:
    MaterialData(std::vector<Material>&& materials)
        : materialData(std::move(materials)), isResident(false) {}

    MaterialData(const MaterialData&) = delete;
    MaterialData& operator=(const MaterialData&) = delete;

    MaterialData(MaterialData&&) = default;
    MaterialData& operator=(MaterialData&&) = default;

    void setResident() {
        if (!isResident) {
            for (auto& material : materialData) {
                material.setResident();
            }
            isResident = true;
        }
    }

    void setNotResident() {
        if (isResident) {
            for (auto& material : materialData) {
                material.setNotResident();
            }
            isResident = false;
        }
    }

   private:
    std::vector<Material> materialData;
    bool isResident;
};

template <typename Material>
class MaterialPack {
   public:
    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    static void bind(MaterialPackHandle<Material>& materialPack) {
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
        currentPack.materialUniforms.bind(GL_SHADER_STORAGE_BUFFER,
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
          materialData(std::move(materialData)),
          isResident{false} {}

    inline static auto currentPackIndex =
        MaterialPackHandle<Material>::getInvalid();

    std140::UniformArray<BufferType> materialUniforms;
    MaterialData<Material> materialData;
    bool isResident;
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

    using BuilderType = typename Material::BuilderType;

    MaterialPackBuilder& addMaterial(const BuilderType& materialBuilder) {
        materialData.emplace_back(materialBuilder);
        materialUniforms.push(materialData.back().getUniformBuffer());
        return *this;
    }

    MaterialPackBuilder& addMaterialMulti(
        const std::vector<BuilderType>& materialBuilders) {
        for (const auto& materialBuilder : materialBuilders) {
            addMaterial(materialBuilder);
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
