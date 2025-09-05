#pragma once

#include <algorithm>
#include <iostream>
#include <optional>
#include <ranges>
#include <vector>

template <typename Item, typename Ownership>
class SlotMap;

template <typename Item, typename Ownership>
class Handle;

template <typename Item, typename Ownership>
struct HandleId;

class Unique;

class Shared;

template <typename Ownership>
struct OwnershipData;

template <typename Item, typename Ownership>
struct CopyHandle {
    static Handle<Item, Ownership> copy(
        const Handle<Item, Ownership>& handle,
        SlotMap<Item, Ownership>& collection) noexcept;

    static Handle<Item, Unique> tryGetOwned(
        HandleId<Item, Unique> handle,
        SlotMap<Item, Unique>& collection) noexcept;
};

template <>
struct OwnershipData<Unique> {
    bool pop(uint32_t storageIndex) noexcept {}

    void pushNew(uint32_t storageIndex) noexcept {}

    void pushExisting(uint32_t storageIndex) noexcept {}

    void share(uint32_t storageIndex) noexcept {}

    void clear() noexcept {}
};

template <>
struct OwnershipData<Shared> {
    bool pop(uint32_t storageIndex) noexcept {
        referenceCount[storageIndex] -= 1;
        return referenceCount[storageIndex] == 0;
    }

    void pushNew(uint32_t storageIndex) noexcept {
        referenceCount.emplace_back(1);
    }

    void pushExisting(uint32_t storageIndex) noexcept {
        referenceCount[storageIndex] = 1;
    }

    void share(uint32_t storageIndex) noexcept {
        referenceCount[storageIndex] += 1;
    }

    void clear() noexcept {
        std::fill(referenceCount.begin(), referenceCount.end(), 0);
    }

    std::vector<uint32_t> referenceCount;
};

namespace handle {
constexpr static auto invalidValue = std::numeric_limits<uint32_t>::max();
}

template <typename Item, typename Ownership>
struct HandleId {
    using Handle = Handle<Item, Ownership>;

    uint32_t generation;
    uint32_t storageIndex;

    static HandleId getInvalid() noexcept {
        return HandleId{.generation = handle::invalidValue,
                        .storageIndex = handle::invalidValue};
    }

    bool isInvalid() const noexcept {
        return storageIndex == handle::invalidValue;
    }

    bool operator==(const HandleId& other) const noexcept {
        return generation == other.generation &&
               storageIndex == other.storageIndex;
    }

    Handle tryGetOwned(SlotMap<Item, Ownership>& collection) const noexcept {
        return CopyHandle<Item, Ownership>::tryGetOwned(*this, collection);
    }
};

template <typename Item, typename Ownership = Unique>
class Handle {
   public:
    using HandleId = HandleId<Item, Ownership>;

    static Handle getInvalid() noexcept {
        return Handle(handle::invalidValue, handle::invalidValue);
    }

    Handle(Handle&& other) noexcept
        : generation(other.generation), storageIndex(other.storageIndex) {
        other.setInvalid();
    };

    Handle& operator=(Handle&& other) {
        if (this != &other) {
            generation = other.generation;
            storageIndex = other.storageIndex;

            other.setInvalid();
        }
        return *this;
    };

    bool isInvalid() const noexcept {
        return storageIndex == handle::invalidValue;
    }

    bool operator==(const Handle& other) const noexcept {
        return generation == other.generation &&
               storageIndex == other.storageIndex;
    }

    Handle copyHandle(SlotMap<Item, Ownership>& collection) const noexcept {
        return CopyHandle<Item, Ownership>::copy(*this, collection);
    }

    ~Handle() noexcept {
        if (!this->isInvalid()) {
            std::println(
                std::cerr,
                "Handle destroyed without prior removal of resource from the "
                "storage: leaking Resource memory allocation");
            std::abort();
        }
    }

    HandleId getId() const noexcept {
        return HandleId{.generation = generation, .storageIndex = storageIndex};
    }

   private:
    friend class SlotMap<Item, Ownership>;
    friend class CopyHandle<Item, Ownership>;

    void setInvalid() noexcept {
        generation = handle::invalidValue;
        storageIndex = handle::invalidValue;
    }

    Handle(const Handle&) = default;
    Handle& operator=(const Handle&) = default;

    Handle(uint32_t generation, uint32_t index)
        : generation(generation), storageIndex(index) {}

    uint32_t generation;
    uint32_t storageIndex;
};

namespace std {
template <typename Item, typename Ownership>
struct hash<HandleId<Item, Ownership>> {
    std::size_t operator()(
        const HandleId<Item, Ownership>& handleId) const noexcept {
        std::size_t h1 = std::hash<size_t>{}(handleId.generation);
        std::size_t h2 = std::hash<size_t>{}(handleId.storageIndex);
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

template <typename Item, typename Ownership = Unique>
class Ref {
   public:
    const Item& get() const noexcept {
        if (itemRef == nullptr) {
            std::println(std::cerr, "Ref: get called on null reference");
            std::abort();
        }
        return *itemRef;
    }

    Item& get() noexcept {
        if (itemRef == nullptr) {
            std::println(std::cerr, "Ref: get called on null reference");
            std::abort();
        }
        return *itemRef;
    }

    bool isValid() const noexcept { return itemRef != nullptr; }

   private:
    friend class SlotMap<Item, Ownership>;

    Ref(Item* itemRef) : itemRef(itemRef) {};

    Ref(const Ref&) = delete;
    Ref& operator=(const Ref&) = delete;

    Ref(Ref&&) = delete;
    Ref& operator=(Ref&&) = delete;

    Item* itemRef;
};

namespace unsafe {
template <typename Item, typename Ownership>
struct Index;
}

template <typename Item, typename Ownership = Unique>
class SlotMap {
   public:
    using Handle = Handle<Item, Ownership>;
    using Ref = Ref<Item, Ownership>;

    SlotMap() = default;

    SlotMap(const SlotMap&) = delete;
    SlotMap& operator=(const SlotMap&) = delete;

    SlotMap(SlotMap&&) = delete;
    SlotMap& operator=(SlotMap&&) = delete;

    const Ref get(const Handle& handle) const noexcept {
        Item* itemRef = nullptr;
        if (isHandleValid(handle.getId())) {
            itemRef = storageCells[handle.storageIndex].operator->();
        }
        return Ref(itemRef);
    }

    Ref get(const Handle& handle) noexcept {
        Item* itemRef = nullptr;
        if (isHandleValid(handle.getId())) {
            itemRef = storageCells[handle.storageIndex].operator->();
        }
        return Ref(itemRef);
    }

    auto forEach() const noexcept {
        return std::views::filter(storageCells,
                                  [](auto& cell) { return cell.has_value(); }) |
               std::views::transform([](auto& cell) { return *cell; });
    }

    auto forEach() noexcept {
        return std::views::filter(storageCells,
                                  [](auto& cell) { return cell.has_value(); }) |
               std::views::transform([](auto& cell) { return *cell; });
    }

    auto&& take() noexcept {
        freeCells = std::vector(storageCells.size());
        for (auto& [i, generation] : std::views::enumerate(cellGenerations)) {
            freeCells[i] = i;
            generation += 1;
        }
        ownershipData.clear();
        auto moveStorage =
            std::vector<std::optional<Item>>(storageCells.size());
        std::swap(moveStorage, storageCells);
        return std::move(moveStorage);
    }

    template <typename... Args>
    Handle emplace(Args&&... args) noexcept {
        auto handle = allocateCell();
        if (!handle.isInvalid()) {
            storageCells[handle.storageIndex].emplace(
                std::forward<Args>(args)...);
        }
        return std::move(handle);
    }

    bool pop(Handle&& handle) noexcept {
        auto removed = false;
        if (isHandleValid(handle.getId()) &&
            ownershipData.pop(handle.storageIndex)) {
            storageCells[handle.storageIndex] = std::nullopt;
            cellGenerations[handle.storageIndex] += 1;
            freeCells.push_back(handle.storageIndex);
            removed = true;
        }
        handle = Handle::getInvalid();
        return removed;
    }

   private:
    friend class CopyHandle<Item, Ownership>;

    friend class unsafe::Index<Item, Ownership>;

    Handle allocateCell() noexcept {
        auto handle = Handle::getInvalid();
        if (!freeCells.empty()) {
            auto storageIndex = freeCells.back();
            freeCells.pop_back();
            ownershipData.pushExisting(storageIndex);
            handle = Handle(cellGenerations[storageIndex], storageIndex);
        } else if (storageCells.size() != handle::invalidValue) {
            auto storageIndex = static_cast<uint32_t>(storageCells.size());
            storageCells.emplace_back(std::nullopt);
            cellGenerations.emplace_back(0);
            ownershipData.pushNew(storageIndex);
            handle = Handle(0, storageIndex);
        }
        return std::move(handle);
    }

    bool isHandleValid(HandleId<Item, Ownership> handle) const noexcept {
        return handle.storageIndex < cellGenerations.size() &&
               cellGenerations[handle.storageIndex] == handle.generation;
    }

    const Ref get(HandleId<Item, Ownership> handle) const noexcept {
        Item* itemRef = nullptr;
        if (isHandleValid(handle)) {
            itemRef = storageCells[handle.storageIndex].operator->();
        }
        return Ref(itemRef);
    }

    Ref get(HandleId<Item, Ownership> handle) noexcept {
        Item* itemRef = nullptr;
        if (isHandleValid(handle)) {
            itemRef = storageCells[handle.storageIndex].operator->();
        }
        return Ref(itemRef);
    }

    std::vector<std::optional<Item>> storageCells;
    std::vector<uint32_t> cellGenerations;
    std::vector<uint32_t> freeCells;
    OwnershipData<Ownership> ownershipData;
};

template <typename Item>
struct CopyHandle<Item, Unique> {
    static Handle<Item, Unique> copy(
        const Handle<Item, Unique>& handle,
        SlotMap<Item, Unique>& collection) noexcept {
        static_assert(
            false, "Attempted to copyHandle to resource with Unique ownership");
        return Handle<Item, Unique>::getInvalid();
    }

    static Handle<Item, Unique> tryGetOwned(
        HandleId<Item, Unique> handle,
        SlotMap<Item, Unique>& collection) noexcept {
        static_assert(false,
                      "Attempted to obtain owning handle from non-owning for "
                      "resource with Unique ownership");
        return std::nullopt;
    }
};

template <typename Item>
struct CopyHandle<Item, Shared> {
    static Handle<Item, Shared> copy(
        const Handle<Item, Shared>& handle,
        SlotMap<Item, Shared>& collection) noexcept {
        collection.ownershipData.share(handle.storageIndex);
        return handle;
    }

    static Handle<Item, Shared> tryGetOwned(
        HandleId<Item, Shared> handle,
        SlotMap<Item, Shared>& collection) noexcept {
        auto owningHandle = Handle<Item, Shared>::getInvalid();
        if (collection.isHandleValid(handle)) {
            collection.ownershipData.share(handle.storageIndex);
            owningHandle =
                Handle<Item, Shared>(handle.generation, handle.storageIndex);
        }
        return owningHandle;
    }
};

namespace unsafe {
template <typename Item, typename Ownership>
struct Index {
    static Ref<Item, Ownership> get(
        HandleId<Item, Ownership> handleId,
        SlotMap<Item, Ownership>& slotMap) noexcept {
        return slotMap.get(handleId);
    }
    static const Ref<Item, Ownership> get(
        HandleId<Item, Ownership> handleId,
        const SlotMap<Item, Ownership>& slotMap) noexcept {
        return slotMap.get(handleId);
    }
};

template <typename Item, typename Ownership>
Ref<Item, Ownership> get(HandleId<Item, Ownership> handleId,
                         SlotMap<Item, Ownership>& slotMap) noexcept {
    return Index<Item, Ownership>::get(handleId, slotMap);
}

template <typename Item, typename Ownership>
const Ref<Item, Ownership> get(
    HandleId<Item, Ownership> handleId,
    const SlotMap<Item, Ownership>& slotMap) noexcept {
    return Index<Item, Ownership>::get(handleId, slotMap);
}
}  // namespace unsafe
