#pragma once

#include "collections/slot_map.h"
#include "collections/slot_map/handle_map.h"

template <typename Item, typename Ownership>
class StaticHandle;

template <typename Key, typename Item, typename Ownership>
class StaticKeyMap;

template <typename Item, typename Ownership>
StaticHandle<Item, Ownership> registerResource(Item&&) noexcept;

template <typename Key, typename Item, typename Ownership>
Ref<Item, Ownership> getKey(const Key&) noexcept;

template <typename Key, typename Item, typename Ownership>
StaticHandle<Item, Ownership> tryGetOwned(const Key&) noexcept;

template <typename Key, typename Item, typename Ownership>
bool eraseKey(const Key&) noexcept;

template <typename Key, typename Item, typename Ownership>
bool clearKeys() noexcept;

template <typename Item, typename Ownership>
class StaticStorage {
   private:
    friend StaticHandle<Item, Ownership> registerResource<Item, Ownership>(
        Item&&) noexcept;
    friend class StaticHandle<Item, Ownership>;
    template <typename, typename, typename>
    friend class StaticKeyMap;

    static StaticHandle<Item, Ownership> tryGetOwnedImpl(
        HandleId<Item, Ownership> handle) {
        return StaticHandle<Item, Ownership>(
            handle.tryGetOwned(StaticStorage<Item, Ownership>::staticStorage));
    }

    inline static SlotMap<Item, Ownership> staticStorage = {};
};

template <typename Key, typename Item, typename Ownership>
class StaticKeyMap {
   private:
    friend class StaticHandle<Item, Ownership>;
    friend Ref<Item, Ownership> getKey<Key, Item, Ownership>(
        const Key&) noexcept;
    friend StaticHandle<Item, Ownership> tryGetOwned<Key, Item, Ownership>(
        const Key&) noexcept;
    friend bool eraseKey<Key, Item, Ownership>(const Key&) noexcept;
    friend bool clearKeys<Key, Item, Ownership>() noexcept;

    static Ref<Item, Ownership> getKeyImpl(const Key& key) {
        return staticMap.get(key,
                             StaticStorage<Item, Ownership>::staticStorage);
    }

    static StaticHandle<Item, Ownership> tryGetOwnedImpl(const Key& key) {
        return StaticStorage<Item, Ownership>::tryGetOwnedImpl(
            staticMap.getHandleId(key));
    }

    inline static KeyHandleMap<Key, Item, Ownership> staticMap = {};
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

    // Check StaticHandle lifetime to minimize pop calls on moved-from, or
    // erroneously copied handles
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

    template <typename Key>
    bool registerKey(Key&& key) const noexcept {
        return getKeyMap<Key>().insert(std::forward<Key>(key), handle);
    }

   private:
    friend class StaticStorage<Item, Shared>;
    friend StaticHandle<Item, Shared> registerResource<Item, Shared>(
        Item&&) noexcept;

    static auto& getStorage() noexcept {
        return StaticStorage<Item, Shared>::staticStorage;
    }

    template <typename Key>
    static auto& getKeyMap() noexcept {
        return StaticKeyMap<Key, Item, Shared>::staticMap;
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

    template <typename Key>
    bool registerKey(Key&& key) const noexcept {
        return getKeyMap<Key>().insert(std::forward<Key>(key), handle);
    }

   private:
    friend class StaticStorage<Item, Unique>;
    friend StaticHandle<Item, Unique> registerResource<Item, Unique>(
        Item&&) noexcept;

    static auto& getStorage() noexcept {
        return StaticStorage<Item, Unique>::staticStorage;
    }

    template <typename Key>
    static auto& getKeyMap() noexcept {
        return StaticKeyMap<Key, Item, Unique>::staticMap;
    }

    StaticHandle(Handle<Item, Unique>&& handle) : handle(std::move(handle)) {}

    Handle<Item, Unique> handle;
};

template <typename Item, typename Ownership>
inline StaticHandle<Item, Ownership> registerResource(Item&& item) noexcept {
    return StaticHandle<Item, Ownership>(
        StaticStorage<Item, Ownership>::staticStorage.emplace(
            std::forward<Item>(item)));
}

template <typename Key, typename Item, typename Ownership>
bool registerKey(Key&& key,
                 const StaticHandle<Item, Ownership>& handle) noexcept {
    return handle.registerKey(std::forward<Key>(key));
}

template <typename Key, typename Item, typename Ownership>
Ref<Item, Ownership> getKey(const Key& key) noexcept {
    return StaticKeyMap<Key, Item, Ownership>::getKeyImpl(key);
}

template <typename Key, typename Item, typename Ownership>
StaticHandle<Item, Ownership> tryGetOwned(const Key& key) noexcept {
    return StaticKeyMap<Key, Item, Ownership>::tryGetOwnedImpl(key);
}

template <typename Key, typename Item, typename Ownership>
bool eraseKey(const Key& key) noexcept {
    return StaticKeyMap<Key, Item, Ownership>::staticMap.erase(key);
}

template <typename Key, typename Item, typename Ownership>
bool clearKeys() noexcept {
    StaticKeyMap<Key, Item, Ownership>::staticMap.clear();
}
