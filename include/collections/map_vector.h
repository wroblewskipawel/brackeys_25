#pragma once

#include <optional>

#include "collections/pin_ref.h"

template <typename Key, typename Item>
struct MapVector {
    std::vector<Item> itemStorage;
    std::unordered_map<Key, size_t> nameMap;

    void emplace(Key&& key, Item&& data) noexcept {
        auto dataIndex = itemStorage.size();
        itemStorage.emplace_back(std::move(data));
        nameMap.emplace(std::move(key), dataIndex);
    }

    PinRef<Item> tryGet(const Key& key) noexcept {
        Item* itemRef = nullptr;
        auto itemIndex = nameMap.find(key);
        if (itemIndex != nameMap.end()) {
            itemRef = &itemStorage[itemIndex->second];
        }
        return PinRef(itemRef);
    };

    const PinRef<Item> tryGet(const Key& key) const noexcept {
        Item* itemRef = nullptr;
        auto itemIndex = nameMap.find(key);
        if (itemIndex != nameMap.end()) {
            itemRef = &itemStorage[itemIndex->second];
        }
        return PinRef(itemRef);
    };

    std::optional<Item> tryTake(const Key& key) noexcept {
        auto itemIndex = nameMap.find(key);
        if (itemIndex != nameMap.end()) {
            return std::move(itemStorage[itemIndex->second]);
        }
        return std::nullopt;
    };

    std::vector<Item> takeItems() noexcept { return std::move(itemStorage); }
};

template <typename Item>
using NamedVector = MapVector<std::string, Item>;
