#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "graphics/resources/buffer/ring.h"
#include "graphics/resources/gl/buffer/ring.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/stream.h"

template <typename Vertex, typename Material, typename Instance,
          size_t BufferSize>
class DynamicPack {
   public:
    using Model = Model<Vertex, Material>;

    DynamicPack(const DynamicPack&) = delete;
    DynamicPack& operator=(const DynamicPack&) = delete;

    DynamicPack(DynamicPack&& other) noexcept
        : streamBuffer(std::move(other.streamBuffer)),
          materialPack(std::move(other.materialPack)),
          meshPack(std::move(other.meshPack)) {
        other.meshPack = MeshPackHandle<Vertex>::getInvalid();
        other.materialPack = MaterialPackHandle<Material>::getInvalid();
        other.streamBuffer = StreamHandle<Instance, BufferSize>::getInvalid();
    };

    DynamicPack& operator=(DynamicPack&& other) noexcept {
        if (this != &other) {
            meshPack = other.meshPack;
            materialPack = other.materialPack;
            streamBuffer = other.streamBuffer;

            other.streamBuffer =
                StreamHandle<Instance, BufferSize>::getInvalid();
            other.meshPack = MeshPackHandle<Material>::getInvalid();
            other.materialPack = MaterialPackHandle<Material>::getInvalid();
        }
        return *this;
    };

    DynamicPack& addDraw(const Model& model, const Instance& instanceData) {
        return addDraw(model, std::views::single(instanceData));
    }

    template <typename Range>
        requires std::is_convertible_v<std::ranges::range_value_t<Range>,
                                       Instance>
    DynamicPack& addDraw(const Model& model, Range&& instanceData) {
        Mesh mesh = getMesh(model.mesh);
        DrawInfo drawInfo{mesh, model.material.packItemIndex};
        auto instanceAllocations = streamBuffer.get().get().pushData(
            std::forward<Range>(instanceData));
        pushDrawCalls(drawInfo, instanceAllocations);
        return *this;
    }

    void clear() noexcept { drawCalls.clear(); }

   private:
    friend class DynamicPackBuilder<Vertex, Material, Instance, BufferSize>;
    friend class DynamicStage<Vertex, Material, Instance, BufferSize>;

    struct Draw {
        DrawInfo drawInfo;
        BufferAllocation<Instance> instanceAllocation;
    };

    DynamicPack(MaterialPackHandle<Material>&& materialPack,
                MeshPackHandle<Vertex>&& meshPack,
                StreamHandle<Instance, BufferSize>&& streamBuffer) noexcept
        : streamBuffer(std::move(streamBuffer)),
          materialPack(std::move(materialPack)),
          meshPack(std::move(meshPack)) {}

    void draw(const UniformLocations& uniformLocations) {
        MeshPack<Vertex>::bind(meshPack);
        if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
            MaterialPack<Material>::bind(materialPack);
        }
        auto& stream = streamBuffer.get().get();
        for (const auto& draw : drawCalls) {
            auto& instanceAllocation = draw.instanceAllocation;
            auto instanceBuffer = stream.getBuffer(instanceAllocation);
            VertexArray<Vertex, Instance>::getVertexArray()
                .bindBuffer<BindingIndex::InstanceAttributes>(BindingInfo{
                    .buffer = instanceBuffer.get(),
                    .offset = static_cast<GLuint>(
                        instanceAllocation.bufferOffset * sizeof(Instance)),
                });
            if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
                glUniform1ui(uniformLocations.materialIndex,
                             static_cast<GLuint>(draw.drawInfo.materialIndex));
            }
            glDrawElementsInstanced(
                GL_TRIANGLES, draw.drawInfo.mesh.indexCount, GL_UNSIGNED_INT,
                (void*)(draw.drawInfo.mesh.indexOffset * sizeof(GLuint)),
                instanceAllocation.numInstances);
        }
    }

    void pushDrawCalls(
        DrawInfo drawInfo,
        std::vector<BufferAllocation<Instance>> instanceAllocations) noexcept {
        auto allocationsBegin = instanceAllocations.begin();
        if (!drawCalls.empty() && drawCalls.back().instanceAllocation.tryJoin(
                                      instanceAllocations.front())) {
            allocationsBegin += 1;
        }
        for (const auto& instanceAllocation : std::ranges::subrange(
                 allocationsBegin, instanceAllocations.end())) {
            drawCalls.emplace_back(Draw(drawInfo, instanceAllocation));
        }
    }

    StreamHandle<Instance, BufferSize> streamBuffer;
    MaterialPackHandle<Material> materialPack;
    MeshPackHandle<Vertex> meshPack;
    std::vector<Draw> drawCalls;
};

template <typename Vertex, typename Material, typename Instance,
          size_t BufferSize>
class DynamicPackBuilder {
   public:
    using Model = Model<Vertex, Material>;

    DynamicPackBuilder(
        const MeshPackHandle<Vertex>& meshPack,
        const MaterialPackHandle<Material>& materialPack,
        const StreamHandle<Instance, BufferSize>& streamBuffer) noexcept
        : meshPack(meshPack.copy()),
          materialPack(materialPack.copy()),
          streamBuffer(streamBuffer.copy()) {}

    DynamicPack<Vertex, Material, Instance, BufferSize> build() {
        return DynamicPack<Vertex, Material, Instance, BufferSize>{
            std::move(materialPack), std::move(meshPack),
            std::move(streamBuffer)};
    }

   private:
    MeshPackHandle<Vertex> meshPack;
    MaterialPackHandle<Material> materialPack;
    StreamHandle<Instance, BufferSize> streamBuffer;
};
