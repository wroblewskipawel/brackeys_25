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
class DrawPackBuilder;

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

template <typename Vertex, typename Material, typename Instance>
class DrawPack {
   public:
    DrawPack(const DrawPack&) = delete;
    DrawPack& operator=(const DrawPack&) = delete;

    DrawPack(DrawPack&& other) noexcept
        : meshes(std::move(other.meshes)),
          instanceBuffers(std::move(other.instanceBuffers)),
          materialPack(std::move(other.materialPack)),
          meshPack(std::move(other.meshPack)) {
        other.meshPack = MeshPackHandle<Vertex>::getInvalid();
        other.materialPack = MaterialPackHandle<Material>::getInvalid();
    };

    DrawPack& operator=(DrawPack&& other) noexcept {
        if (this != &other) {
            meshPack = other.meshPack;
            materialPack = other.materialPack;
            meshes = std::move(other.meshes);
            instanceBuffers = std::move(other.instanceBuffers);

            other.meshPack = MeshPackHandle<Material>::getInvalid();
            other.materialPack = MaterialPackHandle<Material>::getInvalid();
        }
        return *this;
    };

    ~DrawPack() {
        glDeleteBuffers(instanceBuffers.size(), instanceBuffers.data());
    }

   private:
    friend class DrawPackBuilder<Vertex, Material, Instance>;
    friend class Stage<Vertex, Material, Instance>;

    struct DrawInstanced {
        DrawInfo drawInfo;
        size_t numInstances;
    };

    DrawPack(std::unordered_map<DrawInfo, std::vector<Instance>>&& drawData,
             MaterialPackHandle<Material>&& materialPack,
             MeshPackHandle<Vertex>&& meshPack) noexcept
        : meshes(drawData.size()),
          instanceBuffers(drawData.size()),
          materialPack(std::move(materialPack)),
          meshPack(std::move(meshPack)) {
        glCreateBuffers(instanceBuffers.size(), instanceBuffers.data());
        for (const auto& [i, meshDrawData] : std::views::enumerate(drawData)) {
            const auto& [drawInfo, instances] = meshDrawData;
            meshes[i] = DrawInstanced(drawInfo, instances.size());
            glNamedBufferStorage(instanceBuffers[i],
                                 sizeof(Instance) * instances.size(),
                                 instances.data(), GL_NONE);
        }
    }

    void draw(const UniformLocations& uniformLocations) {
        MeshPack<Vertex>::bind(meshPack);
        MaterialPack<Material>::bind(materialPack);
        for (const auto& [draw, instanceBuffer] :
             std::views::zip(meshes, instanceBuffers)) {
            VertexArray<Vertex, Instance>::getVertexArray()
                .bindBuffer<BindingIndex::InstanceAttributes>(instanceBuffer);
            glUniform1ui(uniformLocations.materialIndex,
                         static_cast<GLuint>(draw.drawInfo.materialIndex));
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
};

template <typename Vertex, typename Material>
class Model {
   public:
    template <typename Instance>
    static DrawPackBuilder<Vertex, Material, Instance> drawPackBuilder(
        const MeshPackHandle<Vertex>& meshPack,
        const MaterialPackHandle<Material>& materialPack) noexcept {
        return DrawPackBuilder<Vertex, Material, Instance>(meshPack,
                                                           materialPack);
    }
};

template <typename Vertex, typename Material, typename Instance>
class DrawPackBuilder {
   public:
    DrawPackBuilder(const MeshPackHandle<Vertex>& meshPack,
                    const MaterialPackHandle<Material>& materialPack) noexcept
        : meshPack(meshPack.copy()), materialPack(materialPack.copy()) {}

    DrawPackBuilder& addDraw(const MeshHandle<Vertex>& meshHandle,
                             const MaterialHandle<Material>& materialHandle,
                             Instance instanceData) {
        Mesh mesh = getMesh(meshHandle);
        DrawInfo drawInfo{mesh, materialHandle.materialIndex};
        auto drawDataIt = drawData.find(drawInfo);
        if (drawDataIt != drawData.end()) {
            drawDataIt->second.emplace_back(instanceData);
        } else {
            std::vector<Instance> instances{instanceData};
            drawData.emplace(std::piecewise_construct,
                             std::forward_as_tuple(drawInfo),
                             std::forward_as_tuple(std::move(instances)));
        }
        return *this;
    }

    DrawPackBuilder& addDrawMulti(
        const MeshHandle<Vertex>& meshHandle,
        const MaterialHandle<Material>& materialHandle,
        std::vector<Instance>&& instanceData) {
        Mesh mesh = getMesh(meshHandle);
        DrawInfo drawInfo{mesh, materialHandle.uniformIndex};
        auto drawDataIt = drawData.find(drawInfo);
        if (drawDataIt != drawData.end()) {
            drawDataIt->second.insert(drawDataIt->second.end(),
                                      std::move(instanceData));
        } else {
            drawData.emplace(std::piecewise_construct,
                             std::forward_as_tuple(drawInfo),
                             std::forward_as_tuple(std::move(instanceData)));
        }
        return *this;
    }

    DrawPack<Vertex, Material, Instance> build() {
        return DrawPack<Vertex, Material, Instance>{
            std::move(drawData), std::move(materialPack), std::move(meshPack)};
    }

   private:
    MeshPackHandle<Vertex> meshPack;
    MaterialPackHandle<Material> materialPack;
    std::unordered_map<DrawInfo, std::vector<Instance>> drawData;
};
