#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"

template <typename Vertex, typename Material, typename Instance>
class StaticPackBuilder;

template <typename Vertex, typename Material, typename Instance>
class StaticPack;

template <typename Vertex, typename Material, typename Instance, size_t BufferSize>
class DynamicPack;

template <typename Vertex, typename Material, typename Instance, size_t BufferSize>
class DynamicPackBuilder;

template <typename Vertex, typename Material, typename Instance, size_t BufferSize>
class AnimatedPack;

template <typename Vertex, typename Material, typename Instance, size_t BufferSize>
class AnimatedPackBuilder;

template <typename Vertex, typename Material, typename Instance>
class Stage;

struct DrawInfo {
    Mesh mesh;
    size_t materialIndex;

    bool operator==(const DrawInfo& other) const noexcept {
        return mesh == other.mesh && materialIndex == other.materialIndex;
    }
};
 namespace std {
template <>
struct hash<DrawInfo> {
    std::size_t operator()(const DrawInfo& drawInfo) const noexcept {
        std::size_t h1 = std::hash<Mesh>{}(drawInfo.mesh);
        std::size_t h2 = std::hash<size_t>{}(drawInfo.materialIndex);
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std
