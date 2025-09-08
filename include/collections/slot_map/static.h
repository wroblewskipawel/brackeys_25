#pragma once

#include "collections/slot_map.h"
#include "collections/slot_map/handle_map.h"

template <typename Item, typename Ownership>
class StaticHandle;

template <typename Key, typename Item, typename Ownership>
class StaticKeyMap;

template <typename Item, typename Ownership>
StaticHandle<Item, Ownership> registerResource(Item&&) noexcept;

template <typename Key, typename Item>
PinRef<Item> getKey(const Key&) noexcept;

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
    friend PinRef<Item> getKey<Key, Item>(const Key&) noexcept;
    friend StaticHandle<Item, Ownership> tryGetOwned<Key, Item, Ownership>(
        const Key&) noexcept;
    friend bool eraseKey<Key, Item, Ownership>(const Key&) noexcept;
    friend bool clearKeys<Key, Item, Ownership>() noexcept;

    static PinRef<Item> getKeyImpl(const Key& key) {
        return staticMap.get(key,
                             StaticStorage<Item, Ownership>::staticStorage);
    }

    static StaticHandle<Item, Ownership> tryGetOwnedImpl(const Key& key) {
        return StaticStorage<Item, Ownership>::tryGetOwnedImpl(
            staticMap.getHandleId(key));
    }

    inline static KeyHandleMap<Key, Item, Ownership> staticMap = {};
};

template <typename Item, typename Ownership>
class StaticHandle {
   public:
    StaticHandle& operator=(const StaticHandle& other) noexcept {
        getStorage().pop(std::move(handle));
        handle = other.handle.copyHandle(getStorage());
        return *this;
    };

    // Default constructor initializing handle as invalid provided for
    // convinient Handle type list default initialization
    StaticHandle() noexcept : handle(Handle<Item, Ownership>::getInvalid()) {}

    StaticHandle(StaticHandle&&) = default;
    StaticHandle& operator=(StaticHandle&&) = default;

    // Check StaticHandle lifetime to minimize pop calls on moved-from, or
    // erroneously copied handles
    ~StaticHandle() { getStorage().pop(std::move(handle)); }

    const PinRef<Item> get() const noexcept { return getStorage().get(handle); }

    PinRef<Item> get() noexcept { return getStorage().get(handle); }

    static StaticHandle getInvalid() noexcept {
        return StaticHandle(Handle<Item, Ownership>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    StaticHandle copy() const noexcept { return StaticHandle(*this); }

    friend auto copyVector(
        const std::vector<StaticHandle>& handleVector) noexcept {
        auto vectorCopy = std::vector<StaticHandle>();
        vectorCopy.reserve(handleVector.size());
        for (const auto& handle : handleVector) {
            vectorCopy.emplace_back(handle.copy());
        }
        return vectorCopy;
    }

    template <typename Key>
    bool registerKey(Key&& key) const noexcept {
        return getKeyMap<Key>().insert(std::forward<Key>(key), handle);
    }

    friend class std::hash<StaticHandle>;

    friend bool operator==(const StaticHandle& lhs, const StaticHandle& rhs) {
        return lhs.handle == rhs.handle;
    }

    friend bool operator<(const StaticHandle& lhs, const StaticHandle& rhs) {
        return lhs.handle < rhs.handle;
    }

   private:
    friend class StaticStorage<Item, Ownership>;
    friend StaticHandle<Item, Ownership> registerResource<Item, Ownership>(
        Item&&) noexcept;

    static auto& getStorage() noexcept {
        return StaticStorage<Item, Ownership>::staticStorage;
    }

    template <typename Key>
    static auto& getKeyMap() noexcept {
        return StaticKeyMap<Key, Item, Ownership>::staticMap;
    }

    StaticHandle(Handle<Item, Ownership>&& handle)
        : handle(std::move(handle)) {}

    StaticHandle(const StaticHandle& handle) noexcept
        : handle(handle.handle.copyHandle(getStorage())) {}

    Handle<Item, Ownership> handle;
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
PinRef<Item> getKey(const Key& key) noexcept {
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

namespace std {
template <typename Item, typename Ownership>
struct hash<StaticHandle<Item, Ownership>> {
    std::size_t operator()(
        const StaticHandle<Item, Ownership>& handle) const noexcept {
        return std::hash<HandleId<Item, Ownership>>{}(handle.handle.getId());
    }
};
};  // namespace std
