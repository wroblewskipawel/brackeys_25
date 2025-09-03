#pragma once

#include "collections/slot_map.h"

class Skin;
class SkinHandle;
class Animation;
class AnimationHandle;
class AnimationPlayer;
class AnimationBuilder;
class AnimationStorage;

SkinHandle registerSkin(Skin&&) noexcept;
AnimationHandle registerAnimation(Animation&&) noexcept;

class AnimationStorage {
   private:
    friend SkinHandle registerSkin(Skin&&) noexcept;
    friend AnimationHandle registerAnimation(Animation&&) noexcept;
    friend class Skin;
    friend class SkinHandle;
    friend class Animation;
    friend class AnimationHandle;
    friend class AnimationPlayer;
    friend class AnimationBuilder;

    inline static SlotMap<Skin, Shared> skinStorage = {};
    inline static SlotMap<Animation, Shared> animationStorage = {};
};

class SkinHandle {
   public:
    SkinHandle& operator=(const SkinHandle& other) noexcept {
        AnimationStorage::skinStorage.pop(std::move(handle));
        handle = other.handle.copyHandle(AnimationStorage::skinStorage);
        return *this;
    };

    SkinHandle(SkinHandle&&) = default;
    SkinHandle& operator=(SkinHandle&&) = default;

    ~SkinHandle() { AnimationStorage::skinStorage.pop(std::move(handle)); }

    const Ref<Skin, Shared> get() const noexcept {
        return AnimationStorage::skinStorage.get(handle);
    }

    Ref<Skin, Shared> get() noexcept {
        return AnimationStorage::skinStorage.get(handle);
    }

    static SkinHandle getInvalid() noexcept {
        return SkinHandle(Handle<Skin, Shared>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    SkinHandle copy() const noexcept { return SkinHandle(*this); }

   private:
    friend SkinHandle registerSkin(Skin&&) noexcept;
    SkinHandle(Handle<Skin, Shared>&& handle) : handle(std::move(handle)) {}
    SkinHandle(const SkinHandle& handle) noexcept
        : handle(handle.handle.copyHandle(AnimationStorage::skinStorage)) {}

    Handle<Skin, Shared> handle;
};

class AnimationHandle {
   public:
    AnimationHandle& operator=(const AnimationHandle& other) noexcept {
        handle = other.handle.copyHandle(AnimationStorage::animationStorage);
        return *this;
    };

    AnimationHandle(AnimationHandle&&) = default;
    AnimationHandle& operator=(AnimationHandle&&) = default;

    ~AnimationHandle() {
        AnimationStorage::animationStorage.pop(std::move(handle));
    }

    bool operator==(const AnimationHandle& other) const noexcept {
        return handle == other.handle;
    }

    const Ref<Animation, Shared> get() const noexcept {
        return AnimationStorage::animationStorage.get(handle);
    }

    Ref<Animation, Shared> get() noexcept {
        return AnimationStorage::animationStorage.get(handle);
    }

    static AnimationHandle getInvalid() noexcept {
        return AnimationHandle(Handle<Animation, Shared>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    AnimationHandle copy() const noexcept { return AnimationHandle(*this); }

   private:
    friend AnimationHandle registerAnimation(Animation&&) noexcept;
    AnimationHandle(Handle<Animation, Shared>&& handle)
        : handle(std::move(handle)) {}
    AnimationHandle(const AnimationHandle& handle) noexcept
        : handle(handle.handle.copyHandle(AnimationStorage::animationStorage)) {
    }

    Handle<Animation, Shared> handle;
};

inline SkinHandle registerSkin(Skin&& skin) noexcept {
    auto handle = AnimationStorage::skinStorage.emplace(std::move(skin));
    return SkinHandle(std::move(handle));
}

inline AnimationHandle registerAnimation(Animation&& animation) noexcept {
    auto handle =
        AnimationStorage::animationStorage.emplace(std::move(animation));
    return AnimationHandle(std::move(handle));
}
