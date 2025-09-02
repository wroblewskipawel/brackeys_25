#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/resources/mesh.h"
#include "graphics/storage/gl/mesh.h"

struct MeshBuffers {
    GLuint vbo{0};
    GLuint ebo{0};

    MeshBuffers() = default;

    MeshBuffers(const MeshBuffers&) = delete;
    MeshBuffers& operator=(const MeshBuffers&) = delete;

    MeshBuffers(MeshBuffers&& other) noexcept : vbo(other.vbo), ebo(other.ebo) {
        other.vbo = 0;
        other.ebo = 0;
    };
    MeshBuffers& operator=(MeshBuffers&& other) noexcept {
        if (this != &other) {
            vbo = other.vbo;
            ebo = other.ebo;
            other.vbo = 0;
            other.ebo = 0;
        }
    };
};

struct Mesh {
    size_t indexCount;
    size_t indexOffset;
    size_t vertexOffset;

    bool operator==(const Mesh& other) const noexcept {
        return indexCount == other.indexCount &&
               indexOffset == other.indexOffset &&
               vertexOffset == other.vertexOffset;
    }
};

template <typename Vertex>
struct MeshHandle {
    size_t meshIndex;
    MeshPackHandle<Vertex> packHandle;

    static auto getPackHandles(MeshPackHandle<Vertex> packHandle) noexcept {
        const auto& meshPack =
            MeshPackStorage<Vertex>::meshPackStorage.get(packHandle).get();
        auto materialHandles = std::vector<MeshHandle<Vertex>>();
        for (size_t meshIndex = 0; meshIndex < meshPack.numMeshes();
             meshIndex++) {
            materialHandles.emplace_back(MeshHandle(meshIndex, packHandle));
        }
        return materialHandles;
    }
};

template <typename Vertex>
inline auto getPackHandles(MeshPackHandle<Vertex> packHandle) noexcept {
    return MeshHandle<Vertex>::getPackHandles(packHandle);
}

namespace std {
template <>
struct hash<Mesh> {
    std::size_t operator()(const Mesh& mesh) const noexcept {
        std::size_t h1 = std::hash<size_t>{}(mesh.indexCount);
        std::size_t h2 = std::hash<size_t>{}(mesh.indexOffset);
        std::size_t h3 = std::hash<size_t>{}(mesh.vertexOffset);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
}  // namespace std

template <typename Vertex>
class MeshPackBuilder;

template <typename Vertex>
class MeshPack {
   public:
    static Mesh getMesh(MeshHandle<Vertex> meshHandle) noexcept {
        return MeshPackStorage<Vertex>::meshPackStorage
            .get(meshHandle.packHandle)
            .get()
            .getMesh(meshHandle.meshIndex);
    }

    static void bind(MeshPackHandle<Vertex> packHandle) {
        auto& vertexArray = VertexArray<Vertex, glm::mat4>::getVertexArray();
        auto& meshPack =
            MeshPackStorage<Vertex>::meshPackStorage.get(packHandle).get();
        vertexArray.bindBuffer<BindingIndex::VertexAttributes>(
            meshPack.buffers.vbo);
        vertexArray.bindBuffer<BindingIndex::ElementBuffer>(
            meshPack.buffers.ebo);
        vertexArray.bind();
    }

    ~MeshPack() { glDeleteBuffers(2, &buffers.vbo); }

    MeshPack(const MeshPack&) = delete;
    MeshPack& operator=(const MeshPack&) = delete;

    MeshPack(MeshPack&& other) noexcept
        : buffers(std::move(other.buffers)), meshes(std::move(other.meshes)) {}

    MeshPack& operator=(MeshPack&& other) noexcept {
        if (this != &other) {
            glDeleteBuffers(2, &buffers.vbo);
            buffers = std::move(other.buffers);
            meshes = std::move(other.meshes);
        }
        return *this;
    }

    MeshBuffers getBuffers() const noexcept { return buffers; }

    size_t numMeshes() const noexcept { return meshes.size(); }

   private:
    friend class MeshPackBuilder<Vertex>;

    static MeshPackHandle<Vertex> registerMeshPack(
        MeshPack&& meshPack) noexcept {
        return MeshPackStorage<Vertex>::meshPackStorage.emplace(
            std::move(meshPack));
    }

    MeshPack(MeshBuffers&& buffers, std::vector<Mesh>&& meshes)
        : buffers(std::move(buffers)), meshes(std::move(meshes)) {}

    Mesh getMesh(size_t meshIndex) const noexcept { return meshes[meshIndex]; }

    inline static auto currentPackIndex = MeshPackHandle<Vertex>::getInvalid();

    MeshBuffers buffers;
    std::vector<Mesh> meshes;
};

template <typename Vertex>
inline Mesh getMesh(MeshHandle<Vertex> meshHandle) noexcept {
    return MeshPack<Vertex>::getMesh(meshHandle);
}

template <typename Vertex>
class MeshPackBuilder {
   public:
    MeshPackBuilder() = default;

    MeshPackBuilder(const MeshPackBuilder&) = delete;
    MeshPackBuilder& operator=(const MeshPackBuilder&) = delete;

    MeshPackBuilder(MeshPackBuilder&&) = delete;
    MeshPackBuilder& operator=(MeshPackBuilder&&) = delete;

    MeshPackBuilder& addMesh(MeshDataHandle<Vertex> mesh) {
        meshDatas.emplace_back(mesh);
        return *this;
    }

    MeshPackBuilder& addMeshMulti(
        const std::vector<MeshDataHandle<Vertex>>& meshes) {
        for (auto mesh : meshes) {
            addMesh(mesh);
        }
        return *this;
    }

    MeshPackHandle<Vertex> build() {
        MeshBuffers buffers{};
        std::vector<Mesh> meshes;

        size_t indexOffset = 0;
        size_t vertexOffset = 0;
        for (auto meshData : meshDatas) {
            const auto& meshRef =
                MeshDataStorage<Vertex>::meshStorage.get(meshData).get();

            Mesh mesh{.indexCount = meshRef.indices.size(),
                      .indexOffset = indexOffset,
                      .vertexOffset = vertexOffset};
            indexOffset += mesh.indexCount;
            vertexOffset += meshRef.vertices.size();
            meshes.push_back(mesh);
        }

        std::vector<Vertex> vertices(vertexOffset);
        std::vector<GLuint> indices(indexOffset);

        vertexOffset = 0;
        indexOffset = 0;
        for (auto meshData : meshDatas) {
            const auto& meshRef =
                MeshDataStorage<Vertex>::meshStorage.get(meshData).get();

            auto numVertices = meshRef.vertices.size();
            auto numIndices = meshRef.indices.size();
            std::memcpy(&vertices[vertexOffset], meshRef.vertices.data(),
                        numVertices * sizeof(Vertex));
            std::memcpy(&indices[indexOffset], meshRef.indices.data(),
                        numIndices * sizeof(GLuint));
            for (size_t i = 0; i < numIndices; i++) {
                indices[indexOffset + i] += vertexOffset;
            }
            vertexOffset += numVertices;
            indexOffset += numIndices;
        }

        glCreateBuffers(2, &buffers.vbo);
        glNamedBufferStorage(buffers.vbo, vertices.size() * sizeof(Vertex),
                             vertices.data(), GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(buffers.ebo, indices.size() * sizeof(GLuint),
                             indices.data(), GL_DYNAMIC_STORAGE_BIT);

        return MeshPack<Vertex>::registerMeshPack(
            MeshPack<Vertex>(std::move(buffers), std::move(meshes)));
    }

   private:
    std::vector<MeshDataHandle<Vertex>> meshDatas;
};
