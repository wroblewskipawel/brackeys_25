#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "concepts/range.h"
#include "graphics/resources/animation.h"
#include "graphics/resources/buffer/ring.h"
#include "graphics/resources/gl/buffer/stream.h"
#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/stream.h"

constexpr size_t jointMatrixBufferBinding = 1;

template <typename Vertex, typename Material, typename Instance>
class AnimatedPack {
   public:
    using Model = Model<Vertex, Material>;

    AnimatedPack(const StreamHandle<Instance>& instanceStream,
                 const StreamHandle<glm::mat4>& jointStream) noexcept
        : instanceStream(instanceStream.copy()),
          jointStream(jointStream.copy()) {
        static_assert(
            isAnimatedVertex<Vertex>(),
            "AnimatedPack Vertex type argument is not AnimatedVertex");
    }

    AnimatedPack(const AnimatedPack&) = delete;
    AnimatedPack& operator=(const AnimatedPack&) = delete;

    AnimatedPack(AnimatedPack&& other) noexcept
        : jointStream(std::move(other.jointStream)),
          instanceStream(std::move(other.instanceStream)),
          drawCallMap(std::move(other.drawCallMap)) {
        other.instanceStream = StreamHandle<Instance>::getInvalid();
        other.jointStream = StreamHandle<glm::mat4>::getInvalid();
    }

    AnimatedPack& operator=(AnimatedPack&& other) noexcept {
        if (this != &other) {
            drawCallMap = std::move(other.drawCallMap);
            instanceStream = other.instanceStream;
            jointStream = other.jointStream;
            other.jointStream = StreamHandle<glm::mat4>::getInvalid();
            other.instanceStream = StreamHandle<Instance>::getInvalid();
        }
        return *this;
    }

    AnimatedPack& addDraw(const Model& model, const Instance& instanceData,
                          const AnimationPlayer& sampler) {
        return addDrawMulti(model, std::views::single(instanceData),
                            std::views::single(std::cref(sampler)));
    }

    template <typename Instances, typename Samplers>
        requires RefConstRange<Instances, Instance> &&
                 RefConstRange<Samplers, AnimationPlayer>
    AnimatedPack& addDrawMulti(const Model& model, Instances&& instanceData,
                               Samplers&& samplers) {
        drawCallMap.pushDrawCalls(
            model, getDrawCalls(model, std::forward<Instances>(instanceData),
                                std::forward<Samplers>(samplers)));
        return *this;
    }

    void clear() noexcept { drawCallMap.clear(); }

    struct Draw {
        DrawInfo drawInfo;
        GLuint jointMatrixCount;
        BufferAllocation<Instance> instanceAllocation;
        BufferAllocation<glm::mat4> jointAllocation;

        bool canJoin(const Draw& other) const noexcept {
            return jointMatrixCount == other.jointMatrixCount &&
                   instanceAllocation.canJoin(other.instanceAllocation) &&
                   jointAllocation.canJoin(other.jointAllocation);
        }

        void join(const Draw& other) noexcept {
            instanceAllocation.join(other.instanceAllocation);
            jointAllocation.join(other.jointAllocation);
        }
    };

   private:
    friend class AnimatedStage<Vertex, Material, Instance>;

    void draw(const UniformLocations& uniformLocations) {
        for (auto& [packHandles, drawCalls] : drawCallMap.getDrawCalls()) {
            if (drawCalls.empty()) continue;
            packHandles.bind();
            auto& stream = instanceStream.get().get();
            for (const auto& draw : drawCalls) {
                auto& instanceAllocation = draw.instanceAllocation;
                auto instanceBuffer = stream.getBuffer(instanceAllocation);
                VertexArray<Vertex, Instance>::getVertexArray()
                    .bindBuffer<BindingIndex::InstanceAttributes>(BindingInfo{
                        .buffer = instanceBuffer.get(),
                        .offset = 0,
                    });
                if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
                    glUniform1ui(
                        uniformLocations.materialIndex,
                        static_cast<GLuint>(draw.drawInfo.materialIndex));
                }
                jointStream.get().get().bindBuffer<BufferBindings::Storage>(
                    draw.jointAllocation, jointMatrixBufferBinding);
                glUniform1ui(uniformLocations.jointMatrixCount,
                             draw.jointMatrixCount);
                glUniform1ui(uniformLocations.jointMatrixOffset,
                             draw.jointAllocation.bufferOffset);
                glDrawElementsInstancedBaseInstance(
                    GL_TRIANGLES, draw.drawInfo.meshOffsets.indexCount,
                    GL_UNSIGNED_INT,
                    (void*)(draw.drawInfo.meshOffsets.indexOffset *
                            sizeof(GLuint)),
                    instanceAllocation.numInstances,
                    instanceAllocation.bufferOffset);
            }
        }
    }

    template <typename Instances>
        requires RefConstRange<Instances, Instance>
    auto writeInstanceData(Instances&& range) noexcept {
        return instanceStream.get().get().pushData(
            std::forward<Instances>(range));
    }

    template <typename Samplers>
        requires RefConstRange<Samplers, AnimationPlayer>
    auto writeJointeData(Samplers&& samplers) noexcept {
        auto numSamplers = std::ranges::distance(samplers);
        auto allocations = std::vector<BufferAllocation<glm::mat4>>{};
        allocations.reserve(numSamplers);
        auto& joints = jointStream.get().get();
        for (const AnimationPlayer& player : samplers) {
            allocations.emplace_back(
                joints.pushDataContiguous(player.getJointTransforms()));
        }
        // Following assues that all AnimationPlayers in the range have
        // the same numJoints (share the same skeleton), this is assumption is
        // never checked
        // TODO: Add checks for this invariable
        const AnimationPlayer& samplerFront = samplers.front();
        auto numJoints = samplerFront.numJoints();
        return std::make_pair(allocations, numJoints);
    }

    template <typename Instances, typename Samplers>
        requires RefConstRange<Instances, Instance> &&
                 RefConstRange<Samplers, AnimationPlayer>
    auto getDrawCalls(const Model& model, Instances&& instanceData,
                      Samplers&& samplers) noexcept {
        auto instanceAllocations =
            writeInstanceData(std::forward<Instances>(instanceData));
        auto [jointAllocations, instanceNumJoints] =
            writeJointeData(std::forward<Samplers>(samplers));

        auto drawInfo = DrawInfo(model);
        auto drawCalls = std::vector<Draw>{};
        drawCalls.reserve(instanceAllocations.size());

        auto joints = jointAllocations.begin();
        auto instances = instanceAllocations.begin();
        while (joints != jointAllocations.end() &&
               instances != instanceAllocations.end()) {
            auto& joint = *joints;
            auto& instance = *instances;

            auto jointInstances = joint.numInstances / instanceNumJoints;
            auto instanceJoints = instance.numInstances * instanceNumJoints;
            if (jointInstances < instance.numInstances) {
                drawCalls.emplace_back(Draw(drawInfo, instanceNumJoints,
                                            instance.takeFirst(jointInstances),
                                            joint));
                ++joints;
            } else if (joint.numInstances > instanceJoints) {
                drawCalls.emplace_back(Draw(drawInfo, instanceNumJoints,
                                            instance,
                                            joint.takeFirst(instanceJoints)));
                ++instances;
            } else {
                drawCalls.emplace_back(
                    Draw(drawInfo, instanceNumJoints, instance, joint));
                ++instances;
                ++joints;
            }
        }
        return drawCalls;
    }

    StreamHandle<glm::mat4> jointStream;
    StreamHandle<Instance> instanceStream;
    DrawCallMap<AnimatedPack> drawCallMap;
};
