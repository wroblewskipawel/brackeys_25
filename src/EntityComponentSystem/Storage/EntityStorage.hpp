#ifndef ENTITYSTORAGE_HPP
#define ENTITYSTORAGE_HPP

#include <array>
#include <bitset>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../Components/ComponentIndexes.hpp"

using EntityID = std::size_t;
using ComponentBitMask = std::bitset<COMPONENT_COUNT>;

struct EntityData {
    ComponentBitMask componentMask;  // Which components the entity has
    std::array<std::optional<size_t>, COMPONENT_COUNT>
        componentIndices;  // Component index inside its storage
};

inline bool is_subset(const ComponentBitMask& a, const ComponentBitMask& b) {
    return (a & b) == a;
}

// TODO:
// 1. Add entity builder.
//      a. Add addComponentWithBuilder method.
//      b. Add flushComponents method that will write bitmask to the components after the entity is fully built.

class EntityStorage {
private:
    std::unordered_map<EntityID, EntityData> entities;
    std::unordered_map<ComponentBitMask, std::unordered_set<EntityID>> components;

public:
    void addEntity(EntityID id) {
        if (entities.find(id) != entities.end()) {
            throw std::runtime_error("Entity already exists!");
        }
        entities[id] = EntityData{};
    }

    const std::unordered_set<EntityID>& query(const ComponentBitMask& bitMask) {
        auto result = components.find(bitMask);
        if (result == components.end()) {
            std::unordered_set<EntityID> set;
            for (auto& [entityId, entityData] : entities) {
                if (is_subset(bitMask, entityData.componentMask)) {
                    set.insert(entityId);
                }
            }
            components[bitMask] = set;
        }
        return components[bitMask];
    }

    void removeEntity(EntityID id) {
        entities.erase(id);
    }

    template<typename T>
    void addComponent(EntityID id, size_t componentIndex) {
        auto it = entities.find(id);
        if (it == entities.end()) {
            throw std::runtime_error("Entity does not exist!");
        }

        constexpr auto typeIndex = static_cast<size_t>(ComponentToType<T>::index);

        it->second.componentMask.set(typeIndex, true);
        it->second.componentIndices[typeIndex] = componentIndex;

        const auto entityBitMaks = it->second.componentMask;
        for (auto& [bitmask, ids] : components) {
            if (is_subset(entityBitMaks, bitmask)) {
                ids.insert(id);
            }
        }
    }

    template<typename T>
    void removeComponent(EntityID id) {
        auto it = entities.find(id);
        if (it == entities.end()) return;

        constexpr auto typeIndex = static_cast<size_t>(ComponentToType<T>::index);

        const auto maskBefore = it->second.componentMask;
        it->second.componentMask.set(typeIndex, false);
        it->second.componentIndices[typeIndex].reset();
        const auto maskAfter = it->second.componentMask;

        for (auto& [bitmask, ids] : components) {
            if (is_subset(maskBefore, bitmask)) {
                ids.erase(id);
            }
            if (is_subset(maskAfter, bitmask)) {
                ids.insert(id);
            }
        }
    }

    bool hasEntity(EntityID id) const {
        return entities.find(id) != entities.end();
    }

    template<typename T>
    bool hasComponent(EntityID id) const {
        auto it = entities.find(id);
        if (it == entities.end()) return false;

        constexpr auto typeIndex = static_cast<size_t>(ComponentToType<T>::index);
        return it->second.componentMask.test(typeIndex);
    }

    template<typename T>
    size_t getComponentIndex(EntityID id) const {
        auto it = entities.find(id);
        if (it == entities.end()) {
            throw std::runtime_error("Entity does not exist!");
        }

        constexpr auto typeIndex = static_cast<size_t>(ComponentToType<T>::index);
        auto& optIndex = it->second.componentIndices[typeIndex];
        if (!optIndex.has_value()) {
            throw std::runtime_error("Entity does not have this component!");
        }
        return optIndex.value();
    }

    std::vector<EntityID> getAllEntities() const {
        std::vector<EntityID> ids;
        ids.reserve(entities.size());
        for (const auto& [id, _] : entities) {
            ids.push_back(id);
        }
        return ids;
    }

    template<typename T>
    friend class ComponentStorage;
};

#endif // ENTITYSTORAGE_HPP
