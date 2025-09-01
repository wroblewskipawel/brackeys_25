#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "graphics/storage/animation.h"

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
        auto nextKeyIndex = currentKeyIndex;
        while (nextKeyIndex < keys.size() &&
               times[nextKeyIndex] < currentTime) {
            nextKeyIndex++;
        }
        return nextKeyIndex < 0 ? 0 : nextKeyIndex;
    }

    Item getCurrentValue(float currentTime, size_t currentKeyIndex) const {
        auto currentKey = keys[currentKeyIndex];
        if (currentKeyIndex < keys.size() - 1) {
            auto deltaTime = currentTime - times[currentKeyIndex];
            if (deltaTime > 0.0) {
                float interpolateRatio =
                    deltaTime /
                    (times[currentKeyIndex + 1] - times[currentKeyIndex]);
                auto nextKey = keys[currentKeyIndex + 1];
                return interpolate(
                    Sample{currentKey, nextKey, interpolateRatio});
            }
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
