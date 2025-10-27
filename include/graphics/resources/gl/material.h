#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <glm/glm.hpp>

#include "graphics/resources/gl/buffer/binding.h"
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/texture.h"
#include "graphics/resources/gl/texture/bindless.h"
#include "graphics/resources/material.h"
#include "graphics/storage/gl/material.h"

constexpr size_t materialPackBufferBinding = 0;

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

    template <template <typename> typename MaterialData>
    auto build() {
        if (materialHandles.size() == 0) {
            return MaterialPackHandle<MaterialData<Material>>::getInvalid();
        }

        return registerMaterialPack(
            MaterialPack<MaterialData<Material>>(materialHandles));
    }

   private:
    std::vector<MaterialBuilderHandle> materialHandles;
};
