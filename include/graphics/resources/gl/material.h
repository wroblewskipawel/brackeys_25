#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <glm/glm.hpp>

#include "graphics/resources/material.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/gl/buffer/std140.h"

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
        : albedoTexture(tryLoadFromData(builder.albedoTexture, SamplerConfig{})) {}

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
    size_t uniformIndex;
    size_t packIndex;
};

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
struct MaterialPackRef {
    size_t packIndex;
    GLuint uniformBuffer;
};

template <typename Material>
class MaterialPack {
   public:
    MaterialPack(const MaterialPack&) = delete;
    MaterialPack& operator=(const MaterialPack&) = delete;

    MaterialPack(MaterialPack&&) = default;
    MaterialPack& operator=(MaterialPack&&) = default;

    ~MaterialPack() {
        auto packData = materialDataStorage.find(packIndex);
        if (packData != materialDataStorage.end())
            packData->second.setResident();
        materialDataStorage.erase(packIndex);
    };

    static void bind(const MaterialPackRef<Material>& ref) {
        if (currentMeshPack != ref.packIndex) {
            if (currentMeshPack != std::numeric_limits<size_t>::max()) {
                auto currentPackData = materialDataStorage.find(currentMeshPack);
                if (currentPackData != materialDataStorage.end())
                    currentPackData->second.setNotResident();
            }
            auto newPackData = materialDataStorage.find(ref.packIndex);
            if (newPackData != materialDataStorage.end())
                newPackData->second.setResident();
            currentMeshPack = ref.packIndex;
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, materialPackBufferBinding,
                         ref.uniformBuffer);
    }

    MaterialPackRef<Material> getPackRef() const {
        return MaterialPackRef<Material>{packIndex,
                                         materialUniforms.getBuffer()};
    }

   private:
    using BufferType = typename Material::BufferType;
    friend class MaterialPackBuilder<Material>;

    MaterialPack(std140::UniformArrayBuilder<BufferType>&& materialUniforms,
                 std::vector<Material>&& materialData, size_t packIndex)
        : materialUniforms(materialUniforms.build()),
          packIndex{packIndex},
          isResident{false} {
        materialDataStorage.emplace(std::piecewise_construct,
                                     std::forward_as_tuple(packIndex),
                                     std::forward_as_tuple(std::move(materialData)));
    }

    inline static size_t currentMeshPack = std::numeric_limits<size_t>::max();
    inline static std::unordered_map<size_t, MaterialData<Material>>
        materialDataStorage;

    std140::UniformArray<BufferType> materialUniforms;
    size_t packIndex;
    bool isResident;
};

template <typename Material>
class MaterialPackBuilder {
   public:
    MaterialPackBuilder() { packIndex = nextPackIndex++; };

    MaterialPackBuilder(const MaterialPackBuilder&) = delete;
    MaterialPackBuilder& operator=(const MaterialPackBuilder&) = delete;

    MaterialPackBuilder(MaterialPackBuilder&&) = default;
    MaterialPackBuilder& operator=(MaterialPackBuilder&&) = default;

    ~MaterialPackBuilder() = default;

    using BuilderType = typename Material::BuilderType;

    MaterialHandle<Material> addMaterial(const BuilderType& materialBuilder) {
        materialData.emplace_back(materialBuilder);
        materialUniforms.push(materialData.back().getUniformBuffer());
        return MaterialHandle<Material>{materialData.size() - 1, packIndex};
    }

    std::vector<MaterialHandle<Material>> addMaterialMulti(
        const std::vector<BuilderType>& materialBuilders) {
        std::vector<MaterialHandle<Material>> handles;
        handles.reserve(materialBuilders.size());
        for (const auto& materialBuilder : materialBuilders) {
            handles.emplace_back(addMaterial(materialBuilder));
        }
        return handles;
    }

    MaterialPack<Material> build() {
        return MaterialPack<Material>(std::move(materialUniforms),
                                      std::move(materialData), packIndex);
    }

   private:
    inline static size_t nextPackIndex = 0;
    using BufferType = typename Material::BufferType;

    std140::UniformArrayBuilder<BufferType> materialUniforms;
    std::vector<Material> materialData;
    size_t packIndex;
};
