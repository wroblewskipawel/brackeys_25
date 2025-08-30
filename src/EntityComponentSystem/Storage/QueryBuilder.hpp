#pragma once

#include "EntityStorage.hpp"

class QueryBuilder {
private:
    EntityStorage& storage;
    ComponentBitMask bitMask;

public:
    QueryBuilder(EntityStorage& storage) : storage(storage) {}

    template<typename T>
    QueryBuilder& andHas() {
        constexpr auto typeIndex = static_cast<size_t>(ComponentToType<T>::index);
        bitMask.set(typeIndex, true);
        return *this;
    }

    const std::unordered_set<EntityID>& get() {
        return storage.query(bitMask);
    }
};
