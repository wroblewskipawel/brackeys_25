#pragma once

#include "collections/slot_map/static.h"

class Skin;
using SkinHandle = StaticHandle<Skin, Shared>;

inline SkinHandle registerSkin(Skin&& skin) noexcept {
    return registerResource<Skin, Shared>(std::move(skin));
}

template <typename Key>
inline const PinRef<Skin> getSkinByKey(const Key& key) noexcept {
    return getKey<Key, Skin, Shared>(key);
}

template <typename Key>
inline SkinHandle tryGetOwnedSkinByKey(const Key& key) noexcept {
    return tryGetOwned<Key, Skin, Shared>(key);
}

namespace unsafe {
template <typename Key>
inline PinRef<Skin> getSkinByKey(const Key& key) noexcept {
    return getKey<Key, Skin, Shared>(key);
}

}  // namespace unsafe

class Animation;
using AnimationHandle = StaticHandle<Animation, Shared>;
;

inline AnimationHandle registerAnimation(Animation&& animation) noexcept {
    return registerResource<Animation, Shared>(std::move(animation));
}

template <typename Key>
inline const PinRef<Animation> getAnimationByKey(const Key& key) noexcept {
    return getKey<Key, Animation, Shared>(key);
}

template <typename Key>
inline AnimationHandle tryGetOwnedAnimationByKey(const Key& key) noexcept {
    return tryGetOwned<Key, Animation, Shared>(key);
}

namespace unsafe {
template <typename Key>
inline PinRef<Animation> getAnimationByKey(const Key& key) noexcept {
    return getKey<Key, Animation, Shared>(key);
}
}  // namespace unsafe
