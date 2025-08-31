#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ranges>
#include <vector>

struct Joint {
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;

    glm::mat4 toMatrix() const {
        auto rotationMatrix = glm::mat4_cast(rotation);
        auto scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        auto translationMatrix = glm::translate(glm::mat4(1.0f), translation);
        return translationMatrix * rotationMatrix * scaleMatrix;
    }
};

template <typename Item>
struct Sample {
    Item currentKey;
    Item nextKey;
    float interpolateRatio;
};

template <typename Item>
Item interpolate(const Sample<Item>& sample);

template <>
inline glm::quat interpolate(const Sample<glm::quat>& sample) {
    return glm::slerp(sample.currentKey, sample.nextKey,
                      sample.interpolateRatio);
}

template <>
inline glm::vec3 interpolate(const Sample<glm::vec3>& sample) {
    return glm::mix(sample.currentKey, sample.nextKey, sample.interpolateRatio);
}

template <typename Item>
struct Keyframes {
    std::vector<Item> keys;
    std::vector<float> times;

    // Add debug assertions for currentKeyIndex in keys.size() rage check
    size_t getKeyframeIndex(float currentTime, size_t currentKeyIndex) const {
        if (times[currentKeyIndex + 1] < currentTime) {
            currentKeyIndex = currentKeyIndex == keys.size() - 1
                                  ? currentKeyIndex
                                  : currentKeyIndex + 1;
        }
        return currentKeyIndex;
    }

    Item getCurrentValue(float currentTime, size_t currentKeyIndex) const {
        auto currentKey = keys[currentKeyIndex];
        if (currentKeyIndex < keys.size() - 1) {
            float interpolateRatio =
                (currentTime - times[currentKeyIndex]) /
                (times[currentKeyIndex + 1] - times[currentKeyIndex]);
            auto nextKey = keys[currentKeyIndex + 1];
            return interpolate(Sample{currentKey, nextKey, interpolateRatio});
        }
        return currentKey;
    }
};

template <typename Item>
inline Keyframes<Item> constValueKeyframe(const Item& value) {
    std::vector<Item> keys = {value};
    std::vector<float> times = {0.0};
    return Keyframes{
        .keys = keys,
        .times = times,
    };
}

struct KeyframeIndices {
    size_t translationKey;
    size_t scaleKey;
    size_t rotationKey;
};


struct Channels {
    Keyframes<glm::vec3> translationKeys;
    Keyframes<glm::vec3> scaleKeys;
    Keyframes<glm::quat> rotationKeys;

    float getDuration() const {
        auto lastTranslation = translationKeys.times.back();
        auto lastScale = scaleKeys.times.back();
        auto lastRotation = rotationKeys.times.back();

        return std::max({lastTranslation, lastScale, lastRotation});
    }

    KeyframeIndices getKeyframeIndices(float currentTime,
                                       KeyframeIndices currentKeys) const {
        auto translationKey = translationKeys.getKeyframeIndex(
            currentTime, currentKeys.translationKey);
        auto scaleKey =
            scaleKeys.getKeyframeIndex(currentTime, currentKeys.scaleKey);
        auto rotationKey =
            rotationKeys.getKeyframeIndex(currentTime, currentKeys.rotationKey);
        return {.translationKey = translationKey,
                .scaleKey = scaleKey,
                .rotationKey = rotationKey};
    }
};

struct AnimatedJoint {
    Channels channels;

    AnimatedJoint(Channels&& channels) : channels(std::move(channels)) {}

    AnimatedJoint(const AnimatedJoint&) = delete;
    AnimatedJoint& operator=(const AnimatedJoint&) = delete;

    AnimatedJoint(AnimatedJoint&&) = default;
    AnimatedJoint& operator=(AnimatedJoint&&) = default;

    KeyframeIndices getKeyframeIndices(float currentTime,
                                       KeyframeIndices currentKeys) const {
        return channels.getKeyframeIndices(currentTime, currentKeys);
    }

    float getDuration() const { return channels.getDuration(); }

    Joint getJoint(float currentTime, KeyframeIndices currentKeys) const {
        auto translation = channels.translationKeys.getCurrentValue(
            currentTime, currentKeys.translationKey);
        auto scale = channels.scaleKeys.getCurrentValue(currentTime,
                                                        currentKeys.scaleKey);
        auto rotation = channels.rotationKeys.getCurrentValue(
            currentTime, currentKeys.rotationKey);
        return Joint{
            .translation = translation, .scale = scale, .rotation = rotation};
    }
};

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
    void apply(std::vector<glm::mat4> localTransforms) const {
        applyJointRelations(localTransforms);
        applyBindPose(localTransforms);
    }

    Skin(const Skin&) = default;
    Skin& operator=(const Skin&) = default;

    Skin(Skin&&) = default;
    Skin& operator=(Skin&&) = default;

   private:
    friend class SkinBuilder;
    friend class AnimationBuilder;

    Skin(std::vector<glm::mat4>&& inverseBindMatrices,
         std::vector<SkinJoint>&& jointsTree, uint32_t skinIndex)
        : inverseBindMatrices(std::move(inverseBindMatrices)),
          jointsTree(std::move(jointsTree)),
          skinIndex(skinIndex) {}

    void applyJointRelations(std::vector<glm::mat4>& transforms) const {
        for (const auto& [i, parent] : std::views::enumerate(jointsTree)) {
            transforms[i] = transforms[parent.getIndex()] * transforms[i];
        }
    }

    void applyBindPose(std::vector<glm::mat4>& transforms) const {
        for (const auto& [i, bind] :
             std::views::enumerate(inverseBindMatrices)) {
            transforms[i] *= bind;
        }
    }

    // This should be const
    uint32_t skinIndex;
    std::vector<SkinJoint> jointsTree;
    std::vector<glm::mat4> inverseBindMatrices;
};

class SkinBuilder {
   public:
    SkinBuilder() : builderIndex{nextBuilderIndex++} {};

    SkinBuilder(const SkinBuilder&) = delete;
    SkinBuilder& operator=(const SkinBuilder&) = delete;

    SkinBuilder(SkinBuilder&&) = default;
    SkinBuilder& operator=(SkinBuilder&&) = default;

    BuilderJoint getRoot() const { return BuilderJoint(-1, builderIndex); }

    BuilderJoint addJoint(glm::mat4 inverseBindMatrix, BuilderJoint parent) {
        if (parent.builderIndex != builderIndex) {
            std::println(std::cerr,
                         "SkinBuilder: addJoint parent BuilderJoint doesn't "
                         "originate from current SkinBuilder");
            std::abort();
        }
        auto jointIndex = static_cast<int32_t>(inverseBindMatrices.size());
        inverseBindMatrices.emplace_back(inverseBindMatrix);
        jointsTree.emplace_back(parent.getIndex());
        return BuilderJoint(jointIndex, builderIndex);
    };

    Skin build() {
        return Skin(std::move(inverseBindMatrices), std::move(jointsTree),
                    builderIndex);
    }

   private:
    inline static uint32_t nextBuilderIndex = 0;

    uint32_t builderIndex;
    std::vector<SkinJoint> jointsTree;
    std::vector<glm::mat4> inverseBindMatrices;
};

class AnimationBuilder;
class AnimationPlayer;

class Animation {
   private:
    friend class AnimationPlayer;
    friend class AnimationBuilder;

    Animation(Skin&& skin, std::vector<std::optional<AnimatedJoint>>&& joints,
              uint32_t animationIndex)
        : skin(std::move(skin)),
          joints(std::move(joints)),
          animationIndex(animationIndex) {}

    Skin skin;
    std::vector<std::optional<AnimatedJoint>> joints;
    uint32_t animationIndex;
};

class AnimationBuilder {
   public:
    AnimationBuilder(const Skin& skin)
        : skin(skin),
          joints(skin.jointsTree.size()),
          animationIndex(nextAnimationIndex++) {}

    void animateJoint(BuilderJoint jointIndex, AnimatedJoint&& animationData) {
        if (jointIndex.builderIndex != skin.skinIndex) {
            std::println(std::cerr,
                         "AnimationBuilder: animateJoint BuilderJoint doesn't "
                         "originate from Skin");
            std::abort();
        }
        joints[jointIndex.jointIndex] = std::move(animationData);
    }

    Animation build() {
        return Animation(Skin(skin), std::move(joints), animationIndex);
    }

   private:
    friend class Skin;


    inline static uint32_t nextAnimationIndex = 0;

    const Skin& skin;

    std::vector<std::optional<AnimatedJoint>> joints;
    uint32_t animationIndex;
};

class AnimationPlayer {
   public:
    AnimationPlayer(const Animation& animationData, bool loops)
        : animationIndex(animationData.animationIndex),
          currentTime(0.0),
          loops(loops) {
        auto durations =
            std::views::transform(animationData.joints, [](const auto& joint) {
                return joint.has_value() ? (*joint).getDuration() : 0.0;
            });
        duration = *std::max_element(durations.begin(), durations.end());
    }

    void update(const Animation& animationData, float deltaTime) {
        if (animationData.animationIndex != animationIndex) {
            std::println(std::cerr,
                         "AnimationPlayer: Player doesn't originate from "
                         "provided Animation");
            std::abort();
        }
        if (currentTime < duration) {
            currentTime += deltaTime;
            if (loops && currentTime >= duration) {
                currentTime -= duration;
            }
            for (auto [joint, currentKeyframe] :
                 std::views::zip(animationData.joints, currentKeyframes)) {
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

    std::vector<glm::mat4> getJointTransforms(const Animation& animationData) const {
        if (animationData.animationIndex != animationIndex) {
            std::println(std::cerr,
                         "AnimationPlayer: Player doesn't originate from "
                         "provided Animation");
            std::abort();
        }
        auto localTransforms = getJointsLocalTransform(animationData);
        animationData.skin.apply(localTransforms);
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

    std::vector<KeyframeIndices> currentKeyframes;
    uint32_t animationIndex;
    float currentTime;
    float duration;
    bool loops;
};
