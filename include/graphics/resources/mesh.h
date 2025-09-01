#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <type_traits>

#include "graphics/storage/mesh.h"
struct ColoredVertex {
    glm::vec3 position;
    glm::vec3 color;
};

struct UnlitVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

struct UnlitAnimatedVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::uvec4 joints;
    glm::vec4 weights;
};

template<typename Vertex>
struct IsAnimatedVertex {
    static constexpr bool value = std::is_same_v<Vertex, UnlitAnimatedVertex>;
};

template<typename Vertex>
constexpr bool isAnimatedVertex() {
    return IsAnimatedVertex<Vertex>::value;
}

template <typename Vertex>
struct MeshData {
    static MeshDataHandle<Vertex> registerMeshData(MeshData&& meshData) {
        return MeshDataStorage<Vertex>::meshStorage.emplace(std::move(meshData));
    }

    MeshData(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
        : vertices(std::move(vertices)), indices(std::move(indices)) {}

    MeshData(const MeshData&) = delete;
    MeshData& operator=(const MeshData&) = delete;

    MeshData(MeshData&&) = default;
    MeshData& operator=(MeshData&&) = default;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
