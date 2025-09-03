#pragma once

#include "collections/static.h"

class Skin;
using SkinHandle = StaticHandle<Skin, Shared>;

inline SkinHandle registerSkin(Skin&& skin) noexcept {
    return registerResource<Skin, Shared>(std::move(skin));
}

class Animation;
using AnimationHandle = StaticHandle<Animation, Shared>;
;

inline AnimationHandle registerAnimation(Animation&& animation) noexcept {
    return registerResource<Animation, Shared>(std::move(animation));
}
