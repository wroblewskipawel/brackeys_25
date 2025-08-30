#ifndef ENTITYSTORAGE_HPP
#define ENTITYSTORAGE_HPP

#include <array>
#include <bitset>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../Components/ComponentIndexes.hpp"

using EntityID = std::size_t;

struct EntityData {
    std::bitset<COMPONENT_COUNT> componentMask;                          // Which components the entity has
    std::array<std::optional<size_t>, COMPONENT_COUNT> componentIndices; // Component index inside its storage
};

class EntityStorage {
private:
    std::unordered_map<EntityID, EntityData> entities;

public:
    void addEntity(EntityID id) {
        if (entities.find(id) != entities.end()) {
            throw std::runtime_error("Entity already exists!");
        }
        entities[id] = EntityData{};
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
    }

    template<typename T>
    void removeComponent(EntityID id) {
        auto it = entities.find(id);
        if (it == entities.end()) return;

        constexpr auto typeIndex = static_cast<size_t>(ComponentToType<T>::index);

        it->second.componentMask.set(typeIndex, false);
        it->second.componentIndices[typeIndex].reset();
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
