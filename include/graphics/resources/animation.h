#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ranges>
#include <vector>

#include "graphics/resources/animation/joint.h"
#include "graphics/storage/animation.h"

class BuilderJoint;

class SkinBuilder;

class SkinJoint {
   public:
    int32_t getIndex() const { return index; }

   private:
    int32_t index;
    friend class BuilderJoint;

    SkinJoint(int32_t index) : index{index} {};
};

class BuilderJoint {
   private:
    friend class SkinBuilder;
    friend class AnimationBuilder;

    SkinJoint getIndex() const { return SkinJoint(jointIndex); }
    BuilderJoint(int32_t jointIndex, uint32_t builderIndex)
        : jointIndex{jointIndex}, builderIndex{builderIndex} {};

    int32_t jointIndex;
    uint32_t builderIndex;
};

class Skin {
   public:
    void apply(std::vector<glm::mat4>& localTransforms) const {
        applyJointRelations(localTransforms);
        applyBindPose(localTransforms);
        applyJointOrdering(localTransforms);
    }

    Skin(const Skin&) = default;
    Skin& operator=(const Skin&) = default;

    Skin(Skin&&) = default;
    Skin& operator=(Skin&&) = default;

   private:
    friend class SkinBuilder;
    friend class AnimationBuilder;

    Skin(std::vector<glm::mat4>&& inverseBindMatrices,
         std::vector<SkinJoint>&& jointsTree,
         std::vector<uint32_t>&& outputLocations, uint32_t skinIndex)
        : inverseBindMatrices(std::move(inverseBindMatrices)),
          jointsTree(std::move(jointsTree)),
          outputLocations(std::move(outputLocations)),
          skinIndex(skinIndex) {}

    void applyJointRelations(std::vector<glm::mat4>& transforms) const {
        for (const auto& [i, parent] :
             std::views::enumerate(jointsTree) | std::views::drop(1)) {
            transforms[i] = transforms[parent.getIndex()] * transforms[i];
        }
    }

    void applyBindPose(std::vector<glm::mat4>& transforms) const {
        for (const auto& [i, bind] :
             std::views::enumerate(inverseBindMatrices)) {
            transforms[i] *= bind;
        }
    }

    // Consider updating mesh vertices joints index data instead of
    // reordering matrices to adhere to original gltf2.0 document node ordering
    // each frame Is it possible to maintain both current "linear" update logic
    // and origial ordering??
    void applyJointOrdering(std::vector<glm::mat4>& transforms) const {
        std::vector<glm::mat4> orderedTransforms(transforms.size());
        for (const auto& [i, transform] : std::views::enumerate(transforms)) {
            orderedTransforms[outputLocations[i]] = transform;
        }
        transforms = std::move(orderedTransforms);
    }

    // This should be const
    uint32_t skinIndex;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<SkinJoint> jointsTree;
    std::vector<uint32_t> outputLocations;
};

class SkinBuilder {
   public:
    SkinBuilder() : builderIndex{nextBuilderIndex++} {};

    SkinBuilder(const SkinBuilder&) = delete;
    SkinBuilder& operator=(const SkinBuilder&) = delete;

    SkinBuilder(SkinBuilder&&) = default;
    SkinBuilder& operator=(SkinBuilder&&) = default;

    BuilderJoint getRoot() const { return BuilderJoint(-1, builderIndex); }

    BuilderJoint addJoint(glm::mat4 inverseBindMatrix, BuilderJoint parent,
                          uint32_t outputLocation) {
        if (parent.builderIndex != builderIndex) {
            std::println(std::cerr,
                         "SkinBuilder: addJoint parent BuilderJoint doesn't "
                         "originate from current SkinBuilder");
            std::abort();
        }
        auto jointIndex = static_cast<int32_t>(inverseBindMatrices.size());
        inverseBindMatrices.emplace_back(inverseBindMatrix);
        jointsTree.emplace_back(parent.getIndex());
        outputLocations.emplace_back(outputLocation);
        return BuilderJoint(jointIndex, builderIndex);
    };

    SkinHandle build() {
        return registerSkin(Skin(std::move(inverseBindMatrices),
                                 std::move(jointsTree),
                                 std::move(outputLocations), builderIndex));
    }

   private:
    inline static uint32_t nextBuilderIndex = 0;

    uint32_t builderIndex;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<SkinJoint> jointsTree;
    std::vector<uint32_t> outputLocations;
};

class AnimationBuilder;
class AnimationPlayer;

class Animation {
   public:
    size_t numJoints() const { return joints.size(); }

   private:
    friend class AnimationPlayer;
    friend class AnimationBuilder;

    const Skin& getSkin() const noexcept { return skinHandle.get().get(); }

    Animation(const SkinHandle& skinHandle,
              std::vector<std::optional<AnimatedJoint>>&& joints)
        : skinHandle(skinHandle.copy()), joints(std::move(joints)) {}

    SkinHandle skinHandle;
    std::vector<std::optional<AnimatedJoint>> joints;
};

class AnimationBuilder {
   public:
    AnimationBuilder(const SkinHandle& skinHandle)
        : skinHandle(skinHandle.copy()) {
        const auto& skinRef = skinHandle.get().get();
        joints.resize(skinRef.jointsTree.size());
        skinIndex = skinRef.skinIndex;
    }

    void animateJoint(BuilderJoint jointIndex, AnimatedJoint&& animationData) {
        if (jointIndex.builderIndex != skinIndex) {
            std::println(std::cerr,
                         "AnimationBuilder: animateJoint BuilderJoint doesn't "
                         "originate from Skin");
            std::abort();
        }
        joints[jointIndex.jointIndex] = std::move(animationData);
    }

    AnimationHandle build() {
        return registerAnimation(Animation(skinHandle, std::move(joints)));
    }

   private:
    friend class Skin;

    uint32_t skinIndex;
    SkinHandle skinHandle;
    std::vector<std::optional<AnimatedJoint>> joints;
};

class AnimationPlayer {
   public:
    AnimationPlayer(const AnimationHandle& animationHandle)
        : animationHandle(animationHandle.copy()),
          currentTime(0.0),
          loops(false) {
        auto& animationRef = animationHandle.get().get();
        auto durations =
            std::views::transform(animationRef.joints, [](const auto& joint) {
                return joint.has_value() ? (*joint).getDuration() : 0.0;
            });
        duration = *std::max_element(durations.begin(), durations.end());
        currentKeyframes.resize(animationRef.numJoints());
    }

    void update(float deltaTime) {
        const auto& animationRef = animationHandle.get().get();
        if (currentTime < duration) {
            currentTime += deltaTime;
            if (loops && currentTime >= duration) {
                reset();
            }
            for (auto [joint, currentKeyframe] :
                 std::views::zip(animationRef.joints, currentKeyframes)) {
                if (joint.has_value()) {
                    currentKeyframe = (*joint).getKeyframeIndices(
                        currentTime, currentKeyframe);
                }
            }
        }
    }

    void reset() {
        currentTime = 0.0;
        for (auto& keyframe : currentKeyframes) {
            keyframe = {0, 0, 0};
        }
    }

    void loopAnimation(bool shouldLoop) { loops = shouldLoop; }

    std::vector<glm::mat4> getJointTransforms() const {
        const auto& animationRef = animationHandle.get().get();
        auto localTransforms = getJointsLocalTransform(animationRef);
        animationRef.getSkin().apply(localTransforms);
        return localTransforms;
    }

   private:
    std::vector<glm::mat4> getJointsLocalTransform(
        const Animation& animationData) const {
        auto jointTransforms =
            std::vector<glm::mat4>(animationData.joints.size());
        for (const auto& [i, joint] :
             std::views::enumerate(animationData.joints)) {
            jointTransforms[i] =
                joint.has_value()
                    ? (*joint)
                          .getJoint(currentTime, currentKeyframes[i])
                          .toMatrix()
                    : glm::mat4(1.0);
        }
        return jointTransforms;
    }

    AnimationHandle animationHandle;
    std::vector<KeyframeIndices> currentKeyframes;
    float currentTime;
    float duration;
    bool loops;
};
