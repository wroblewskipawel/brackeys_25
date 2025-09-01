#pragma once

#include <iostream>
#include <optional>
#include <ranges>
#include <vector>

template <typename Item>
class SlotMap;

template <typename Item>
class Handle {
   public:
    constexpr static auto invalidValue = std::numeric_limits<uint32_t>::max();

    Handle(const Handle&) = default;
    Handle& operator=(const Handle&) = default;

    Handle(Handle&&) = default;
    Handle& operator=(Handle&&) = default;

    bool isInvalid() const noexcept { return storageIndex == invalidValue; }

   private:
    friend class SlotMap<Item>;

    Handle(uint32_t generation, uint32_t index)
        : generation(generation), storageIndex(index) {}

    static Handle getInvalid() noexcept {
        return Handle(invalidValue, invalidValue);
    }

    uint32_t generation;
    uint32_t storageIndex;
};

template <typename Item>
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

   private:
    friend class SlotMap<Item>;

    Ref(Item* itemRef) : itemRef(itemRef) {};

    Ref(const Ref&) = delete;
    Ref& operator=(const Ref&) = delete;

    Ref(Ref&&) = delete;
    Ref& operator=(Ref&&) = delete;

    Item* itemRef;
};

template <typename Item>
class SlotMap {
   public:
    using Handle = Handle<Item>;
    using Ref = Ref<Item>;

    SlotMap() = default;

    SlotMap(const SlotMap&) = delete;
    SlotMap& operator=(const SlotMap&) = delete;

    SlotMap(SlotMap&&) = delete;
    SlotMap& operator=(SlotMap&&) = delete;

    const Ref get(Handle handle) const noexcept {
        Item* itemRef = nullptr;
        if (isHandleValid(handle)) {
            itemRef = storageCells[handle.storageIndex].operator->();
        }
        return Ref(itemRef);
    }

    Ref get(Handle handle) noexcept {
        Item* itemRef = nullptr;
        if (isHandleValid(handle)) {
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
        for(auto& [i, generation]: std::views::enumerate(cellGenerations)) {
            freeCells[i] = i;
            generation += 1;
        }
        auto moveStorage = std::vector<std::optional<Item>>(storageCells.size());
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
        return handle;
    }
 
    void pop(Handle handle) noexcept {
        if (isHandleValid(handle)) {
            storageCells[handle.storageIndex] = std::nullopt;
            cellGenerations[handle.storageIndex] += 1;
            freeCells.push_back(handle.storageIndex);
        }
    }

   private:
    Handle allocateCell() noexcept {
        auto handle = Handle::getInvalid();
        if (!freeCells.empty()) {
            auto storageIndex = freeCells.back();
            freeCells.pop_back();
            handle = Handle(cellGenerations[storageIndex], storageIndex);
        } else if (storageCells.size() != Handle::invalidValue) {
            auto storageIndex = static_cast<uint32_t>(storageCells.size());
            storageCells.emplace_back(std::nullopt);
            cellGenerations.emplace_back(0);
            handle = Handle(0, storageIndex);
        }
        return handle;
    }

    bool isHandleValid(Handle handle) const noexcept {
        return !handle.isInvalid() &&
               cellGenerations[handle.storageIndex] == handle.generation;
    }

    std::vector<std::optional<Item>> storageCells;
    std::vector<uint32_t> cellGenerations;
    std::vector<uint32_t> freeCells;
};
