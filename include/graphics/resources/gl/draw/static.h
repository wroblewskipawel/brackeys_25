#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "graphics/resources/gl/buffer/static.h"
#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"

template <typename Vertex, typename Material, typename Instance>
class StaticPack {
   public:
    StaticPack(const StaticPack&) = delete;
    StaticPack& operator=(const StaticPack&) = delete;

    StaticPack(StaticPack&&) = default;
    StaticPack& operator=(StaticPack&& other) = default;

   private:
    friend class StaticPackBuilder<Vertex, Material, Instance>;
    friend class StaticStage<Vertex, Material, Instance>;

    struct Draw {
        DrawInfo drawInfo;
        StaticBuffer<Instance> instanceBuffer;

        bool canJoin(const Draw& other) const noexcept { return false; }

        void join(const Draw& other) noexcept { std::unreachable(); }

        auto getDrawCall(
            const UniformLocations& uniformLocations) const noexcept {
            auto bufferInfo = instanceBuffer.getBufferInfo();
            VertexArray<Vertex, Instance>::getVertexArray()
                .bindBuffer<BindingIndex::InstanceAttributes>(BindingInfo{
                    .buffer = bufferInfo.buffer,
                    .offset = 0,
                });
            if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
                glUniform1ui(uniformLocations.materialIndex,
                             static_cast<GLuint>(drawInfo.materialIndex));
            }
            return [this, bufferInfo]() {
                glDrawElementsInstanced(
                    GL_TRIANGLES, drawInfo.meshOffsets.indexCount,
                    GL_UNSIGNED_INT,
                    (void*)(drawInfo.meshOffsets.indexOffset * sizeof(GLuint)),
                    bufferInfo.numItems);
            };
        }
    };

    StaticPack(std::unordered_map<DrawInfo, std::vector<Instance>>&& drawData,
               PackHandles<Vertex, Material>&& packHandles) noexcept
        : packHandles(std::move(packHandles)) {
        drawCalls.reserve(drawData.size());
        for (const auto& [drawInfo, instances] : drawData) {
            drawCalls.emplace_back(
                Draw(drawInfo, StaticBuffer<Instance>(instances)));
        }
    }

    void draw(const UniformLocations& uniformLocations) {
        packHandles.bind();
        for (const auto& drawCall : drawCalls) {
            drawCall.getDrawCall(uniformLocations)();
        }
    }

    std::vector<Draw> drawCalls;
    PackHandles<Vertex, Material> packHandles;
};

template <typename Vertex, typename Material, typename Instance>
class StaticPackBuilder {
   public:
    using Model = Model<Vertex, Material>;

    StaticPackBuilder(PackHandles<Vertex, Material>&& packHandles) noexcept
        : packHandles(std::move(packHandles)) {}

    StaticPackBuilder& addDraw(const Model& model, Instance instanceData) {
        auto drawInfo = DrawInfo(model);
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
        auto drawInfo = DrawInfo(model);
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
        return StaticPack<Vertex, Material, Instance>{std::move(drawData),
                                                      std::move(packHandles)};
    }

   private:
    PackHandles<Vertex, Material> packHandles;
    std::unordered_map<DrawInfo, std::vector<Instance>> drawData;
};
