#pragma once

#include "collections/slot_map.h"

class Skin;
class Animation;
class AnimationPlayer;
class AnimationBuilder;

using SkinHandle = Handle<Skin>;
using AnimationHandle = Handle<Animation>;

class AnimationStorage {
    private:
    friend class Skin;
    friend class Animation;
    friend class AnimationPlayer;
    friend class AnimationBuilder;

    inline static SlotMap<Skin> skinStorage = {};
    inline static SlotMap<Animation> animationStorage = {};
};
