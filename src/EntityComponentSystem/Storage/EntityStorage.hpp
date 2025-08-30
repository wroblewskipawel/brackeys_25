#ifndef ENTITYSTORAGE_HPP
#define ENTITYSTORAGE_HPP

#include <unordered_map>
#include <bitset>
#include <typeindex>
#include <cstddef>
#include <stdexcept>
#include <vector>

using EntityID = std::size_t;

constexpr size_t MAX_COMPONENTS = 64;

struct EntityData {
    std::bitset<MAX_COMPONENTS> componentMask;                      // Which components the entity has
    std::unordered_map<std::type_index, size_t> componentIndices;   // Component index inside its storage
    // TODO: Change unordered_map to array (vector for now)
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

        auto& data = it->second;
        size_t typeHash = std::hash<std::type_index>{}(std::type_index(typeid(T))) % MAX_COMPONENTS;

        data.componentMask.set(typeHash, true);
        data.componentIndices[std::type_index(typeid(T))] = componentIndex;
    }

    template<typename T>
    void removeComponent(EntityID id) {
        auto it = entities.find(id);
        if (it == entities.end()) return;

        auto& data = it->second;
        size_t typeHash = std::hash<std::type_index>{}(std::type_index(typeid(T))) % MAX_COMPONENTS;

        data.componentMask.set(typeHash, false);
        data.componentIndices.erase(std::type_index(typeid(T)));
    }

    bool hasEntity(EntityID id) const {
        return entities.find(id) != entities.end();
    }

    template<typename T>
    bool hasComponent(EntityID id) const {
        auto it = entities.find(id);
        if (it == entities.end()) return false;

        size_t typeHash = std::hash<std::type_index>{}(std::type_index(typeid(T))) % MAX_COMPONENTS;
        return it->second.componentMask.test(typeHash);
    }

    template<typename T>
    size_t getComponentIndex(EntityID id) const {
        auto it = entities.find(id);
        if (it == entities.end()) {
            throw std::runtime_error("Entity does not exist!");
        }

        auto compIt = it->second.componentIndices.find(std::type_index(typeid(T)));
        if (compIt == it->second.componentIndices.end()) {
            throw std::runtime_error("Entity does not have this component!");
        }

        return compIt->second;
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
