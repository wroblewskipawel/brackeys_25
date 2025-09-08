#pragma once

#include <glm/glm.hpp>
#include <type_traits>

#include "graphics/resources/material.h"
#include "graphics/resources/mesh.h"
#include "graphics/storage/material.h"
#include "graphics/storage/mesh.h"

template <typename Vertex, typename Material>
class ModelDataBuilder;

template <typename Vertex, typename Material>
class ModelData {
   public:
    const auto& getMesh() const noexcept { return meshData; }

    const auto& getMaterial() const noexcept { return materialBuilder; }

    const auto& getAnimations() const noexcept { return animations; }

    const auto& getName() const noexcept { return modelName; }

   private:
    friend class ModelDataBuilder<Vertex, Material>;

    ModelData(const MeshDataHandle<Vertex>& meshData,
              const MaterialBuilderHandle<Material>& materialBuilder,
              const std::vector<AnimationHandle>& animations,
              const std::string& modelName) noexcept
        : meshData(meshData.copy()),
          materialBuilder(materialBuilder.copy()),
          animations(copyVector(animations)),
          modelName(modelName) {}

    MeshDataHandle<Vertex> meshData;
    MaterialBuilderHandle<Material> materialBuilder;
    std::vector<AnimationHandle> animations;
    std::string modelName;
};

template <typename Vertex, typename Material>
class ModelDataBuilder {
   public:
    ModelDataBuilder() noexcept
        : meshData(MeshDataHandle<Vertex>::getInvalid()),
          materialBuilder(MaterialBuilderHandle<Material>::getInvalid()) {}

    auto& withMesh(const MeshDataHandle<Vertex>& meshHandle) {
        meshData = meshHandle.copy();
        return *this;
    }

    auto& withMaterial(const MaterialBuilderHandle<Material>& materialHandle) {
        static_assert(!std::is_same_v<Material, EmptyMaterial>,
                      "ModelDataBuilder::withMaterial called on model with "
                      "EmptyMaterial material type");
        materialBuilder = materialHandle.copy();
        return *this;
    }

    auto& withAnimation(
        const MaterialBuilderHandle<Material>& animationHandle) {
        static_assert(isAnimatedVertex<Vertex>(),
                      "ModelDataBuilder::withAnimation called on model with "
                      "non-Animated Vertex type");
        animations.emplace_back(animationHandle.copy());
        return *this;
    }

    auto& withName(const std::string& name) {
        modelName = name;
        return *this;
    }

    auto build() const noexcept {
        return ModelData<Vertex, Material>(meshData, materialBuilder,
                                           animations, modelName);
    }

   private:
    MeshDataHandle<Vertex> meshData;
    MaterialBuilderHandle<Material> materialBuilder;
    std::vector<AnimationHandle> animations;
    std::string modelName;
};

template <typename Vertex>
MeshDataHandle<Vertex> getCubeMesh();

template <>
MeshDataHandle<ColoredVertex> getCubeMesh() {
    static auto cubeHandle = MeshDataHandle<ColoredVertex>::getInvalid();
    if (cubeHandle.isInvalid()) {
        std::vector<ColoredVertex> vertices{
            // Bottom-back-left
            {.position = {-0.5f, -0.5f, -0.5f},
             .color = {0.0f, 0.0f, 0.0f}},  // 0: black
            // Bottom-back-right
            {.position = {0.5f, -0.5f, -0.5f},
             .color = {1.0f, 0.0f, 0.0f}},  // 1: red
            // Top-back-left
            {.position = {-0.5f, 0.5f, -0.5f},
             .color = {0.0f, 1.0f, 0.0f}},  // 2: green
            // Top-back-right
            {.position = {0.5f, 0.5f, -0.5f},
             .color = {1.0f, 1.0f, 0.0f}},  // 3: yellow
            // Bottom-front-left
            {.position = {-0.5f, -0.5f, 0.5f},
             .color = {0.0f, 0.0f, 1.0f}},  // 4: blue
            // Bottom-front-right
            {.position = {0.5f, -0.5f, 0.5f},
             .color = {1.0f, 0.0f, 1.0f}},  // 5: magenta
            // Top-front-left
            {.position = {-0.5f, 0.5f, 0.5f},
             .color = {0.0f, 1.0f, 1.0f}},  // 6: cyan
            // Top-front-right
            {.position = {0.5f, 0.5f, 0.5f},
             .color = {1.0f, 1.0f, 1.0f}}  // 7: white
        };
        std::vector<uint32_t> indices{// Front face (z = +0.5)
                                      4, 5, 6, 5, 7, 6,
                                      // Back face (z = -0.5)
                                      0, 2, 1, 1, 2, 3,
                                      // Left face (x = -0.5)
                                      0, 4, 2, 2, 4, 6,
                                      // Right face (x = +0.5)
                                      1, 3, 5, 3, 7, 5,
                                      // Bottom face (y = -0.5)
                                      0, 1, 4, 1, 5, 4,
                                      // Top face (y = +0.5)
                                      2, 6, 3, 3, 6, 7};
        cubeHandle =
            registerMeshData(MeshData(std::move(vertices), std::move(indices)));
    }
    return cubeHandle.copy();
}

template <>
MeshDataHandle<UnlitVertex> getCubeMesh() {
    static auto cubeHandle = MeshDataHandle<UnlitVertex>::getInvalid();
    if (cubeHandle.isInvalid()) {
        // Each face has its own 4 vertices with unique UVs
        std::vector<UnlitVertex> vertices{
            // Front face (z = +0.5)
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},  // 0: left-bottom
            {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},   // 1: right-bottom
            {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},   // 2: left-top
            {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},    // 3: right-top

            // Back face (z = -0.5)
            {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},   // 4: left-bottom
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},  // 5: right-bottom
            {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},    // 6: left-top
            {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},   // 7: right-top

            // Left face (x = -0.5)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},  // 8: left-bottom
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},   // 9: right-bottom
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},   // 10: left-top
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},    // 11: right-top

            // Right face (x = +0.5)
            {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},   // 12: left-bottom
            {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},  // 13: right-bottom
            {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},    // 14: left-top
            {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},   // 15: right-top

            // Bottom face (y = -0.5)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},  // 16: left-bottom
            {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},   // 17: right-bottom
            {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}},   // 18: left-top
            {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}},    // 19: right-top

            // Top face (y = +0.5)
            {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},   // 20: left-bottom
            {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}},    // 21: right-bottom
            {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},  // 22: left-top
            {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},   // 23: right-top
        };

        std::vector<uint32_t> indices{// Front face
                                      0, 1, 2, 1, 3, 2,
                                      // Back face
                                      4, 5, 6, 5, 7, 6,
                                      // Left face
                                      8, 9, 10, 9, 11, 10,
                                      // Right face
                                      12, 13, 14, 13, 15, 14,
                                      // Bottom face
                                      16, 17, 18, 17, 19, 18,
                                      // Top face
                                      20, 21, 22, 21, 23, 22};

        cubeHandle =
            registerMeshData(MeshData(std::move(vertices), std::move(indices)));
    }
    return cubeHandle.copy();
}
