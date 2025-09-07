#pragma once

#include <iostream>
#include <optional>

#include "collections/pin_ref.h"

template <typename Key, typename Item>
struct MapVector {
    std::vector<Item> itemStorage;
    std::unordered_map<Key, size_t> nameMap;

    size_t emplace(Key&& key, Item&& data) noexcept {
        if (nameMap.find(key) != nameMap.end()) {
            std::println(std::cerr,
                         "MapVector::emplace: tried to emplace item with "
                         "duplicated key {}",
                         key);
            std::abort();
        }
        auto dataIndex = itemStorage.size();
        itemStorage.emplace_back(std::move(data));
        nameMap.emplace(std::move(key), dataIndex);
        return dataIndex;
    }

    std::optional<size_t> tryGetIndex(const Key& key) const noexcept {
        auto itemIndex = nameMap.find(key);
        if (itemIndex != nameMap.end()) {
            return itemIndex->second;
        }
        return std::nullopt;
    }

    std::optional<Key> tryGetKey(size_t index) const noexcept {
        for (const auto& [key, itemIndex] : nameMap) {
            if (index == itemIndex) {
                return key;
            }
        }
        return std::nullopt;
    }

    PinRef<Item> getAtIndex(size_t index) noexcept {
        return PinRef(&itemStorage[index]);
    }

    const PinRef<Item> getAtIndex(size_t index) const noexcept {
        return PinRef(&itemStorage[index]);
    }

    Item takeAtIndex(size_t index) const noexcept {
        return std::move(itemStorage[index]);
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
