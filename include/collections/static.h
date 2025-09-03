#pragma once

#include "collections/slot_map.h"

template <typename Item, typename Ownership>
class StaticHandle;

template <typename Item, typename Ownership>
StaticHandle<Item, Ownership> registerResource(Item&&) noexcept;

template <typename Item, typename Ownership>
class StaticStorage {
   private:
    friend StaticHandle<Item, Ownership> registerResource<Item, Ownership>(
        Item&&) noexcept;
    friend class StaticHandle<Item, Ownership>;

    inline static SlotMap<Item, Ownership> staticStorage = {};
};

template <typename Item>
class StaticHandle<Item, Shared> {
   public:
    StaticHandle& operator=(const StaticHandle& other) noexcept {
        getStorage().pop(std::move(handle));
        handle = other.handle.copyHandle(getStorage());
        return *this;
    };

    StaticHandle(StaticHandle&&) = default;
    StaticHandle& operator=(StaticHandle&&) = default;

    ~StaticHandle() { getStorage().pop(std::move(handle)); }

    const Ref<Item, Shared> get() const noexcept {
        return getStorage().get(handle);
    }

    Ref<Item, Shared> get() noexcept { return getStorage().get(handle); }

    static StaticHandle getInvalid() noexcept {
        return StaticHandle(Handle<Item, Shared>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    StaticHandle copy() const noexcept { return StaticHandle(*this); }

    bool operator==(const StaticHandle& other) const noexcept {
        return handle == other.handle;
    }

   private:
    friend StaticHandle<Item, Shared> registerResource<Item, Shared>(
        Item&&) noexcept;

    static auto& getStorage() noexcept {
        return StaticStorage<Item, Shared>::staticStorage;
    }

    StaticHandle(Handle<Item, Shared>&& handle) : handle(std::move(handle)) {}
    StaticHandle(const StaticHandle& handle) noexcept
        : handle(handle.handle.copyHandle(getStorage())) {}

    Handle<Item, Shared> handle;
};

template <typename Item>
class StaticHandle<Item, Unique> {
   public:
    StaticHandle& operator=(const StaticHandle& other) = delete;
    StaticHandle(const StaticHandle& handle) = delete;

    StaticHandle(StaticHandle&&) = default;
    StaticHandle& operator=(StaticHandle&&) = default;

    ~StaticHandle() { getStorage().pop(std::move(handle)); }

    const Ref<Item, Unique> get() const noexcept {
        return getStorage().get(handle);
    }

    Ref<Item, Unique> get() noexcept { return getStorage().get(handle); }

    static StaticHandle getInvalid() noexcept {
        return StaticHandle(Handle<Item, Unique>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

   private:
    friend StaticHandle<Item, Unique> registerResource<Item, Unique>(
        Item&&) noexcept;

    static auto& getStorage() noexcept {
        return StaticStorage<Item, Unique>::staticStorage;
    }

    StaticHandle(Handle<Item, Unique>&& handle) : handle(std::move(handle)) {}

    Handle<Item, Unique> handle;
};

template <typename Item, typename Ownership>
inline StaticHandle<Item, Ownership> registerResource(Item&& item) noexcept {
    auto handle =
        StaticStorage<Item, Ownership>::staticStorage.emplace(std::move(item));
    return StaticHandle<Item, Ownership>(std::move(handle));
}
