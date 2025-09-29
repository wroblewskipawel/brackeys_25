#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"

template <typename Vertex, typename Material, typename Instance>
class StaticPack {
   public:
    StaticPack(const StaticPack&) = delete;
    StaticPack& operator=(const StaticPack&) = delete;

    StaticPack(StaticPack&& other) noexcept
        : meshes(std::move(other.meshes)),
          instanceBuffers(std::move(other.instanceBuffers)),
          materialPack(std::move(other.materialPack)),
          meshPack(std::move(other.meshPack)) {
        other.meshPack = MeshPackHandle<Vertex>::getInvalid();
        other.materialPack = MaterialPackHandle<Material>::getInvalid();
    };

    StaticPack& operator=(StaticPack&& other) noexcept {
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

    ~StaticPack() {
        glDeleteBuffers(instanceBuffers.size(), instanceBuffers.data());
    }

   private:
    friend class StaticPackBuilder<Vertex, Material, Instance>;
    friend class StaticStage<Vertex, Material, Instance>;

    struct DrawInstanced {
        DrawInfo drawInfo;
        size_t numInstances;
    };

    StaticPack(std::unordered_map<DrawInfo, std::vector<Instance>>&& drawData,
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
        if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
            MaterialPack<Material>::bind(materialPack);
        }
        for (const auto& [draw, instanceBuffer] :
             std::views::zip(meshes, instanceBuffers)) {
            VertexArray<Vertex, Instance>::getVertexArray()
                .bindBuffer<BindingIndex::InstanceAttributes>(
                    BindingInfo {
                        .buffer = instanceBuffer,
                        .offset = 0,
                    }
                );
            if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
                glUniform1ui(uniformLocations.materialIndex,
                             static_cast<GLuint>(draw.drawInfo.materialIndex));
            }
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

template <typename Vertex, typename Material, typename Instance>
class StaticPackBuilder {
   public:
    using Model = Model<Vertex, Material>;

    StaticPackBuilder(const MeshPackHandle<Vertex>& meshPack,
                      const MaterialPackHandle<Material>& materialPack) noexcept
        : meshPack(meshPack.copy()), materialPack(materialPack.copy()) {}

    StaticPackBuilder& addDraw(const Model& model, Instance instanceData) {
        Mesh mesh = getMesh(model.mesh);
        DrawInfo drawInfo{mesh, model.material.packItemIndex};
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

    StaticPackBuilder& addDrawMulti(const Model& model,
                                    std::vector<Instance>&& instanceData) {
        Mesh mesh = getMesh(model.mesh);
        DrawInfo drawInfo{mesh, model.material.packItemIndex};
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

    StaticPack<Vertex, Material, Instance> build() {
        return StaticPack<Vertex, Material, Instance>{
            std::move(drawData), std::move(materialPack), std::move(meshPack)};
    }

   private:
    MeshPackHandle<Vertex> meshPack;
    MaterialPackHandle<Material> materialPack;
    std::unordered_map<DrawInfo, std::vector<Instance>> drawData;
};
