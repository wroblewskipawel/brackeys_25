#pragma once

#include <ranges>
#include <unordered_map>

#include "collections/slot_map.h"

template <typename Key, typename Item, typename Ownership>
class KeyHandleMap {
   public:
    using Ref = Ref<Item, Ownership>;
    using Handle = Handle<Item, Ownership>;
    using HandleId = HandleId<Item, Ownership>;

    bool insert(Key&& key, const Handle& handle) noexcept {
        auto [_, registered] =
            handleMap.emplace(std::move(key), handle.getId());
        return registered;
    }

    bool erase(const Key& key) noexcept {
        auto item = handleMap.find();
        if (item != handleMap.end()) {
            handleMap.erase(item);
            return true;
        }
        return false;
    }

    void clear() noexcept { handleMap.clear(); }

    Handle tryGetOwned(const Key& key,
                       SlotMap<Item, Ownership>& slotMap) noexcept {
        return getHandleId(key).tryGetOwned(slotMap);
    }

    const Ref get(const Key& key,
                  const SlotMap<Item, Ownership>& slotMap) const noexcept {
        return unsafe::get(getHandleId(key), slotMap);
    }

    Ref get(const Key& key, SlotMap<Item, Ownership>& slotMap) noexcept {
        return unsafe::get(getHandleId(key), slotMap);
    }

    auto forEach(const SlotMap<Item, Ownership>& slotMap) const noexcept {
        return std::views::transform(
                   handleMap,
                   [&](const auto& entry) {
                       return std::make_pair(
                           entry->first, unsafe::get(entry->second, slotMap));
                   }) |
               std::views::filter(
                   [](const auto& item) { return item->second.isValid(); });
    }

    auto forEach(SlotMap<Item, Ownership>& slotMap) noexcept {
        return std::views::transform(
                   handleMap,
                   [&](const auto& entry) {
                       return std::make_pair(
                           entry->first, unsafe::get(entry->second, slotMap));
                   }) |
               std::views::filter(
                   [](const auto& item) { return item->second.isValid(); });
    }

    HandleId getHandleId(const Key& key) const {
        auto handle = HandleId::getInvalid();
        auto item = handleMap.find(key);
        if (item != handleMap.end()) {
            handle = item->second;
        }
        return handle;
    }

   private:
    std::unordered_map<Key, HandleId> handleMap;
};
