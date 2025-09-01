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
#include "graphics/storage/gl/material.h"

template <typename Vertex, typename Material>
class DrawPackBuilder;

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

    DrawPack(DrawPack&& other) noexcept
        : buffers(other.buffers),
          vao(other.vao),
          materialPack(other.materialPack),
          meshes(std::move(other.meshes)),
          instanceBuffers(std::move(other.instanceBuffers)) {
        other.buffers = {};
        other.vao = 0;
        other.materialPack = MaterialPackHandle<Material>::getInvalid();
    };

    DrawPack& operator=(DrawPack&& other) noexcept {
        if (this != &other) {
            buffers = other.buffers;
            vao = other.vao;
            materialPack = other.materialPack;
            meshes = std::move(other.meshes);
            instanceBuffers = std::move(other.instanceBuffers);

            other.buffers = {};
            other.vao = 0;
            other.materialPack = MaterialPackHandle<Material>::getInvalid();
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
        MaterialPackHandle<Material> materialPack, MeshBuffers buffers,
        GLuint vao) noexcept
        : meshes(drawData.size()),
          instanceBuffers(drawData.size()),
          materialPack(materialPack),
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
        MaterialPack<Material>::bind(materialPack);
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
    MaterialPackHandle<Material> materialPack;
    MeshBuffers buffers;
    GLuint vao;
};

template <typename Vertex, typename Material>
class DrawPackBuilder {
   public:
    DrawPackBuilder(const MeshPack<Vertex>& meshPack,
                    MaterialPackHandle<Material> materialPack) noexcept
        : meshPack(meshPack), materialPack(materialPack) {}

    DrawPackBuilder& addDraw(MeshHandle<Vertex> meshHandle,
                             MaterialHandle<Material> materialHandle,
                             glm::mat4 modelMatrix) {
        Mesh mesh = meshPack.getMesh(meshHandle);
        DrawInfo drawInfo{mesh, materialHandle.materialIndex};
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
        return DrawPack<Vertex, Material>{drawData, materialPack,
                                          meshPack.getBuffers(),
                                          getVertexArray<Vertex>()};
    }

   private:
    const MeshPack<Vertex>& meshPack;
    MaterialPackHandle<Material> materialPack;
    std::unordered_map<DrawInfo, std::vector<glm::mat4>> drawData;
};
