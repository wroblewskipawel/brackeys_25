#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <vector>


void setInstanceAttributes(GLuint vao, GLuint nextLocation) {
    glVertexArrayAttribBinding(vao, nextLocation, 1);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              0 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, 1);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              1 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, 1);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              2 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, 1);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              3 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);
    glVertexArrayBindingDivisor(vao, 1, 1);
}

template <typename Vertex>
GLuint getVertexArray();

struct UnlitVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

template <>
GLuint getVertexArray<UnlitVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);
        glVertexArrayAttribFormat(vao, nextLocation, 2, GL_FLOAT, GL_FALSE,
                                  offsetof(UnlitVertex, texCoord));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);

        glBindVertexArray(0);
    }
    return vao;
}

struct ColoredVertex {
    glm::vec3 position;
    glm::vec3 color;
};

template <>
GLuint getVertexArray<ColoredVertex>() {
    static GLuint vao{0};
    if (vao == 0) {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(ColoredVertex, position));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);
        glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(ColoredVertex, color));
        glVertexArrayAttribBinding(vao, nextLocation, 0);
        glEnableVertexArrayAttrib(vao, nextLocation++);

        setInstanceAttributes(vao, nextLocation);

        glBindVertexArray(0);
    }
    return vao;
}

template <typename Vertex>
void destroyVertexArray() {
    GLuint vao = getVertexArray<Vertex>();
    glDeleteVertexArrays(1, &vao);
}

void destroyVertexArrays() {
    destroyVertexArray<UnlitVertex>();
    destroyVertexArray<ColoredVertex>();
}

struct MeshBuffers {
    GLuint vbo{0};
    GLuint ebo{0};
};

template <typename Vertex>
struct MeshData {
    MeshData(std::vector<Vertex>&& vertices, std::vector<GLuint>&& indices)
        : vertices(std::move(vertices)), indices(std::move(indices)) {}

    MeshData(const MeshData&) = delete;
    MeshData& operator=(const MeshData&) = delete;

    MeshData(MeshData&&) = default;
    MeshData& operator=(MeshData&&) = default;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

template <typename Vertex>
struct MeshHandle {
    static MeshHandle invalid() {
        return MeshHandle{std::numeric_limits<size_t>::max(),
                          std::numeric_limits<size_t>::max()};
    }

    size_t packIndex;
    size_t meshIndex;
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

template <typename Vertex>
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

    DrawPackBuilder<Vertex> createDrawPack() const noexcept {
        return DrawPackBuilder<Vertex>(*this);
    }

   private:
    friend class MeshPackBuilder<Vertex>;
    friend class DrawPackBuilder<Vertex>;

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
        for (auto&& mesh : meshes) {
            meshDatas.emplace_back(std::move(mesh));
            handles.emplace_back(
                MeshHandle<Vertex>{packIndex, meshDatas.size() - 1});
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

template <typename Vertex>
class Stage;

template <typename Vertex>
class DrawPack {
   public:
    DrawPack(const DrawPack&) = delete;
    DrawPack& operator=(const DrawPack&) = delete;

    DrawPack(DrawPack&& other) noexcept {
        buffers = other.buffers;
        vao = other.vao;
        meshes = std::move(other.meshes);
        instanceBuffers = std::move(other.instanceBuffers);

        other.buffers = {};
        other.vao = 0;
        other.meshes.clear();
        other.instanceBuffers.clear();
    };

    DrawPack& operator=(DrawPack&& other) noexcept {
        if (this != &other) {
            buffers = other.buffers;
            vao = other.vao;
            meshes = std::move(other.meshes);
            instanceBuffers = std::move(other.instanceBuffers);

            other.buffers = {};
            other.vao = 0;
            other.meshes.clear();
            other.instanceBuffers.clear();
        }
        return *this;
    };

    ~DrawPack() {
        glDeleteBuffers(instanceBuffers.size(), instanceBuffers.data());
    }

   private:
    friend class DrawPackBuilder<Vertex>;
    friend class Stage<Vertex>;

    struct DrawInstanced {
        Mesh mesh;
        size_t numInstances;
    };

    DrawPack(const std::unordered_map<Mesh, std::vector<glm::mat4>>& drawData,
             MeshBuffers buffers, GLuint vao) noexcept
        : meshes(drawData.size()),
          instanceBuffers(drawData.size()),
          buffers(buffers),
          vao(vao) {
        glCreateBuffers(instanceBuffers.size(), instanceBuffers.data());
        for (const auto& [i, meshDrawData] : std::views::enumerate(drawData)) {
            const auto& [mesh, meshMatrices] = meshDrawData;
            meshes[i] = {mesh, meshMatrices.size()};
            glNamedBufferStorage(instanceBuffers[i],
                                 sizeof(glm::mat4) * meshMatrices.size(),
                                 meshMatrices.data(), GL_NONE);
        }
    }

    void draw() {
        glBindVertexArray(vao);
        glVertexArrayVertexBuffer(vao, 0, buffers.vbo, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(vao, buffers.ebo);
        for (const auto& [draw, instanceBuffer] :
             std::views::zip(meshes, instanceBuffers)) {
            glVertexArrayVertexBuffer(vao, 1, instanceBuffer, 0,
                                      sizeof(glm::mat4));
            glDrawElementsInstanced(
                GL_TRIANGLES, draw.mesh.indexCount, GL_UNSIGNED_INT,
                (void*)(draw.mesh.indexOffset * sizeof(GLuint)),
                draw.numInstances);
        }
    }

    std::vector<DrawInstanced> meshes;
    std::vector<GLuint> instanceBuffers;
    MeshBuffers buffers;
    GLuint vao;
};

template <typename Vertex>
class DrawPackBuilder {
   public:
    DrawPackBuilder& addDraw(MeshHandle<Vertex> meshHandle,
                             glm::mat4 modelMatrix) {
        Mesh mesh = meshPack.getMesh(meshHandle);
        if (drawData.find(mesh) != drawData.end()) {
            drawData[mesh].emplace_back(modelMatrix);
        } else {
            drawData[mesh] = {std::move(modelMatrix)};
        }
        return *this;
    }

    DrawPackBuilder& addDrawMulti(MeshHandle<Vertex> meshHandle,
                                  std::vector<glm::mat4>&& modelMatrices) {
        Mesh mesh = meshPack.getMesh(meshHandle);
        if (drawData.find(mesh) != drawData.end()) {
            drawData[mesh].insert(drawData[mesh].end(),
                                  std::move(modelMatrices));
        } else {
            drawData[mesh] = std::move(modelMatrices);
        }
        return *this;
    }

    DrawPack<Vertex> build() {
        GLuint vao = getVertexArray<Vertex>();
        return DrawPack<Vertex>{drawData, meshPack.buffers, vao};
    }

   private:
    friend class MeshPack<Vertex>;
    DrawPackBuilder(const MeshPack<Vertex>& meshPack) noexcept
        : meshPack(meshPack) {}

    const MeshPack<Vertex>& meshPack;
    std::unordered_map<Mesh, std::vector<glm::mat4>> drawData;
};
