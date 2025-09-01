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
        : meshes(std::move(other.meshes)),
          instanceBuffers(std::move(other.instanceBuffers)),
          materialPack(other.materialPack),
          meshPack(other.meshPack),
          vao(other.vao) {
        other.vao = 0;
        other.meshPack = MeshPackHandle<Vertex>::getInvalid();
        other.materialPack = MaterialPackHandle<Material>::getInvalid();
    };

    DrawPack& operator=(DrawPack&& other) noexcept {
        if (this != &other) {
            vao = other.vao;
            meshPack = other.meshPack;
            materialPack = other.materialPack;
            meshes = std::move(other.meshes);
            instanceBuffers = std::move(other.instanceBuffers);

            other.vao = 0;
            other.meshPack = MeshPackHandle<Material>::getInvalid();
            other.materialPack = MaterialPackHandle<Material>::getInvalid();
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
        MaterialPackHandle<Material> materialPack,
        MeshPackHandle<Vertex> meshPack) noexcept
        : meshes(drawData.size()),
          instanceBuffers(drawData.size()),
          materialPack(materialPack),
          meshPack(meshPack),
          vao(getVertexArray<Vertex>()) {
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
        MeshPack<Vertex>::bind(meshPack);
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
    MeshPackHandle<Vertex> meshPack;
    GLuint vao;
};

template <typename Vertex, typename Material>
class DrawPackBuilder {
   public:
    DrawPackBuilder(MeshPackHandle<Vertex> meshPack,
                    MaterialPackHandle<Material> materialPack) noexcept
        : meshPack(meshPack), materialPack(materialPack) {}

    DrawPackBuilder& addDraw(MeshHandle<Vertex> meshHandle,
                             MaterialHandle<Material> materialHandle,
                             glm::mat4 modelMatrix) {
        Mesh mesh = getMesh(meshHandle);
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
        Mesh mesh = getMesh(meshHandle);
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
        return DrawPack<Vertex, Material>{drawData, materialPack, meshPack};
    }

   private:
    MeshPackHandle<Vertex> meshPack;
    MaterialPackHandle<Material> materialPack;
    std::unordered_map<DrawInfo, std::vector<glm::mat4>> drawData;
};
