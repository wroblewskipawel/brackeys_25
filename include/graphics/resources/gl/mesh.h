#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <vector>
#include <type_traits>

#include "graphics/resources/mesh.h"
#include "graphics/resources/gl/shader.h"

void setInstanceAttributes(GLuint vao, GLuint nextLocation) {
    // Instance Attrib: glm::mat4
    constexpr size_t modelMatrixBufferIndex = 1;

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              0 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              1 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              2 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, modelMatrixBufferIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              3 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);
    glVertexArrayBindingDivisor(vao, modelMatrixBufferIndex, 1);

    // Consider in future for glMultiDrawElements
    // // Instance Attrib: GLuint64 materialPackIndex
    // constexpr size_t materialPackIndexBufferIndex = 2;

    // glVertexArrayAttribBinding(vao, nextLocation,
    // materialPackIndexBufferIndex); glVertexArrayAttribIFormat(vao,
    // nextLocation, 1, GL_UNSIGNED_INT,
    //                            0 * sizeof(GLuint));
    // glEnableVertexArrayAttrib(vao, nextLocation++);
    // glVertexArrayBindingDivisor(vao, materialPackIndexBufferIndex, 1);
}

template <typename Vertex>
GLuint getVertexArray();

template <>
inline GLuint getVertexArray<ColoredVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(ColoredVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(ColoredVertex, color));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);
    }
    return vao;
}

template <>
inline GLuint getVertexArray<UnlitVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);

        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        glVertexArrayAttribFormat(vao, nextLocation, 2, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitVertex, texCoord));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);
    }
    return vao;
}

template <>
inline GLuint getVertexArray<UnlitAnimatedVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);

        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitAnimatedVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        glVertexArrayAttribFormat(vao, nextLocation, 2, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitAnimatedVertex, texCoord));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        glVertexArrayAttribIFormat(vao, nextLocation, 4, GL_UNSIGNED_INT,
                                  offsetof(UnlitAnimatedVertex, joints));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitAnimatedVertex, weights));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);
    }
    return vao;
}

template <typename Vertex>
inline void destroyVertexArray() {
    GLuint vao = getVertexArray<Vertex>();
    glDeleteVertexArrays(1, &vao);
}

inline void destroyVertexArrays() {
    destroyVertexArray<ColoredVertex>();
    destroyVertexArray<UnlitVertex>();
    destroyVertexArray<UnlitAnimatedVertex>();
}

struct MeshBuffers {
    GLuint vbo{0};
    GLuint ebo{0};
};

struct Mesh {
    static Mesh invalid() {
        return Mesh{.indexCount = 0, .indexOffset = 0, .vertexOffset = 0};
    }

    size_t indexCount;
    size_t indexOffset;
    size_t vertexOffset;

    bool operator==(const Mesh& other) const noexcept {
        return indexCount == other.indexCount &&
               indexOffset == other.indexOffset &&
               vertexOffset == other.vertexOffset;
    }
};

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

template <typename Vertex, typename Material>
class DrawPackBuilder;

template <typename Vertex>
class MeshPackBuilder;

template <typename Vertex>
class MeshPack {
   public:
    ~MeshPack() { glDeleteBuffers(2, &buffers.vbo); }

    MeshPack(const MeshPack&) = delete;
    MeshPack& operator=(const MeshPack&) = delete;

    MeshPack(MeshPack&& other) noexcept
        : buffers(other.buffers),
          meshes(std::move(other.meshes)),
          packIndex(other.packIndex) {
        other.packIndex = std::numeric_limits<size_t>::max();
        other.buffers = {};
    }

    MeshPack& operator=(MeshPack&& other) noexcept {
        if (this != &other) {
            glDeleteBuffers(2, &buffers.vbo);
            buffers = other.buffers;
            meshes = std::move(other.meshes);
            packIndex = other.packIndex;
            other.packIndex = std::numeric_limits<size_t>::max();
            other.buffers = {};
        }
        return *this;
    }

    template <typename Material>
    DrawPackBuilder<Vertex, Material> createDrawPack(
        const MaterialPack<Material>& materialPack) const noexcept {
        return DrawPackBuilder<Vertex, Material>(*this, materialPack);
    }

   private:
    friend class MeshPackBuilder<Vertex>;
    template <typename, typename>
    friend class DrawPackBuilder;

    MeshPack(MeshBuffers buffers, std::vector<Mesh>&& meshes, size_t packIndex)
        : buffers(buffers), meshes(std::move(meshes)), packIndex(packIndex) {}

    Mesh getMesh(MeshHandle<Vertex> handle) const noexcept {
        if (handle.packIndex != packIndex ||
            handle.meshIndex >= meshes.size()) {
            return Mesh::invalid();
        }
        return meshes[handle.meshIndex];
    }

    MeshBuffers buffers;
    std::vector<Mesh> meshes;
    size_t packIndex;
};

template <typename Vertex>
class MeshPackBuilder {
   public:
    MeshPackBuilder() : packIndex(packCount++) {}

    MeshPackBuilder(const MeshPackBuilder&) = delete;
    MeshPackBuilder& operator=(const MeshPackBuilder&) = delete;

    MeshPackBuilder(MeshPackBuilder&&) = delete;
    MeshPackBuilder& operator=(MeshPackBuilder&&) = delete;

    MeshHandle<Vertex> addMesh(MeshData<Vertex>&& mesh) {
        meshDatas.emplace_back(std::move(mesh));
        return MeshHandle<Vertex>{packIndex, meshDatas.size() - 1};
    }

    std::vector<MeshHandle<Vertex>> addMeshMulti(
        std::vector<MeshData<Vertex>>&& meshes) {
        std::vector<MeshHandle<Vertex>> handles;
        handles.reserve(meshes.size());
        for (auto&& mesh : meshes) {
            handles.emplace_back(addMesh(std::move(mesh)));
        }
        return handles;
    }

    MeshPack<Vertex> build() {
        MeshBuffers buffers{};
        std::vector<Mesh> meshes;

        size_t indexOffset = 0;
        size_t vertexOffset = 0;
        for (const auto& meshData : meshDatas) {
            Mesh mesh{.indexCount = meshData.indices.size(),
                      .indexOffset = indexOffset,
                      .vertexOffset = vertexOffset};
            indexOffset += mesh.indexCount;
            vertexOffset += meshData.vertices.size();
            meshes.push_back(mesh);
        }

        std::vector<Vertex> vertices(vertexOffset);
        std::vector<GLuint> indices(indexOffset);

        vertexOffset = 0;
        indexOffset = 0;
        for (const auto& meshData : meshDatas) {
            auto numVertices = meshData.vertices.size();
            auto numIndices = meshData.indices.size();
            std::memcpy(&vertices[vertexOffset], meshData.vertices.data(),
                        numVertices * sizeof(Vertex));
            std::memcpy(&indices[indexOffset], meshData.indices.data(),
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

        return MeshPack<Vertex>(buffers, std::move(meshes), packIndex);
    }

   private:
    inline static size_t packCount{0};
    size_t packIndex;
    std::vector<MeshData<Vertex>> meshDatas;
};

template <typename Vertex, typename Material>
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

template <typename Vertex, typename Material>
class DrawPack {
   public:
    DrawPack(const DrawPack&) = delete;
    DrawPack& operator=(const DrawPack&) = delete;

    DrawPack(DrawPack&& other) noexcept {
        buffers = other.buffers;
        vao = other.vao;
        materialPackRef = other.materialPackRef;
        meshes = std::move(other.meshes);
        instanceBuffers = std::move(other.instanceBuffers);

        other.buffers = {};
        other.vao = 0;
        other.materialPackRef = {};
        other.meshes.clear();
        other.instanceBuffers.clear();
    };

    DrawPack& operator=(DrawPack&& other) noexcept {
        if (this != &other) {
            buffers = other.buffers;
            vao = other.vao;
            materialPackRef = other.materialPackRef;
            meshes = std::move(other.meshes);
            instanceBuffers = std::move(other.instanceBuffers);

            other.buffers = {};
            other.vao = 0;
            other.materialPackRef = {};
            other.meshes.clear();
            other.instanceBuffers.clear();
        }
        return *this;
    };

    ~DrawPack() {
        glDeleteBuffers(instanceBuffers.size(), instanceBuffers.data());
    }

   private:
    friend class DrawPackBuilder<Vertex, Material>;
    friend class Stage<Vertex, Material>;

    struct DrawInstanced {
        DrawInfo drawInfo;
        size_t numInstances;
    };

    DrawPack(
        const std::unordered_map<DrawInfo, std::vector<glm::mat4>>& drawData,
        MaterialPackRef<Material> materialPackRef, MeshBuffers buffers,
        GLuint vao) noexcept
        : meshes(drawData.size()),
          instanceBuffers(drawData.size()),
          materialPackRef(materialPackRef),
          buffers(buffers),
          vao(vao) {
        glCreateBuffers(instanceBuffers.size(), instanceBuffers.data());
        for (const auto& [i, meshDrawData] : std::views::enumerate(drawData)) {
            const auto& [drawInfo, instances] = meshDrawData;
            meshes[i] = DrawInstanced(drawInfo, instances.size());
            glNamedBufferStorage(instanceBuffers[i],
                                 sizeof(glm::mat4) * instances.size(),
                                 instances.data(), GL_NONE);
        }
    }

    void draw(const UniformLocations& uniformLocations) {
        glBindVertexArray(vao);
        glVertexArrayVertexBuffer(vao, 0, buffers.vbo, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(vao, buffers.ebo);
        MaterialPack<Material>::bind(materialPackRef);
        for (const auto& [draw, instanceBuffer] :
             std::views::zip(meshes, instanceBuffers)) {
            glUniform1ui(uniformLocations.materialIndex,
                         static_cast<GLuint>(draw.drawInfo.materialIndex));
            glVertexArrayVertexBuffer(vao, 1, instanceBuffer, 0,
                                      sizeof(glm::mat4));
            glDrawElementsInstanced(
                GL_TRIANGLES, draw.drawInfo.mesh.indexCount, GL_UNSIGNED_INT,
                (void*)(draw.drawInfo.mesh.indexOffset * sizeof(GLuint)),
                draw.numInstances);
        }
    }

    std::vector<DrawInstanced> meshes;
    std::vector<GLuint> instanceBuffers;
    MaterialPackRef<Material> materialPackRef;
    MeshBuffers buffers;
    GLuint vao;
};

template <typename Vertex, typename Material>
class DrawPackBuilder {
   public:
    DrawPackBuilder(const MeshPack<Vertex>& meshPack,
                    const MaterialPack<Material>& materialPack) noexcept
        : meshPack(meshPack), materialPack(materialPack) {}

    DrawPackBuilder& addDraw(MeshHandle<Vertex> meshHandle,
                             MaterialHandle<Material> materialHandle,
                             glm::mat4 modelMatrix) {
        Mesh mesh = meshPack.getMesh(meshHandle);
        DrawInfo drawInfo{mesh, materialHandle.uniformIndex};
        auto drawDataIt = drawData.find(drawInfo);
        if (drawDataIt != drawData.end()) {
            drawDataIt->second.emplace_back(modelMatrix);
        } else {
            std::vector<glm::mat4> modelMatrices{modelMatrix};
            drawData.emplace(std::piecewise_construct,
                             std::forward_as_tuple(drawInfo),
                             std::forward_as_tuple(std::move(modelMatrices)));
        }
        return *this;
    }

    DrawPackBuilder& addDrawMulti(MeshHandle<Vertex> meshHandle,
                                  MaterialHandle<Material> materialHandle,
                                  std::vector<glm::mat4>&& modelMatrices) {
        Mesh mesh = meshPack.getMesh(meshHandle);
        DrawInfo drawInfo{mesh, materialHandle.uniformIndex};
        auto drawDataIt = drawData.find(drawInfo);
        if (drawDataIt != drawData.end()) {
            drawDataIt->second.insert(drawDataIt->second.end(),
                                      std::move(modelMatrices));
        } else {
            drawData.emplace(std::piecewise_construct,
                             std::forward_as_tuple(drawInfo),
                             std::forward_as_tuple(std::move(modelMatrices)));
        }
        return *this;
    }

    DrawPack<Vertex, Material> build() {
        GLuint vao = getVertexArray<Vertex>();
        return DrawPack<Vertex, Material>{drawData, materialPack.getPackRef(),
                                          meshPack.buffers, vao};
    }

   private:
    const MeshPack<Vertex>& meshPack;
    const MaterialPack<Material>& materialPack;
    std::unordered_map<DrawInfo, std::vector<glm::mat4>> drawData;
};
