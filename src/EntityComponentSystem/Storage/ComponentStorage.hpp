#pragma once

#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

#include "EntityStorage.hpp"

using EntityID = std::size_t;

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void removeEntity(EntityID id, EntityStorage& es) = 0;
    virtual void removeComponent(EntityID id, EntityStorage& es) = 0;
};

enum class CellState {
    Free = 0,
    Occupied,
};

struct Cell {
    CellState state;
    size_t entityId;
};

template<typename T>
class ComponentStorage : public IStorage {
private:
    static constexpr size_t npos = static_cast<size_t>(-1);
    static constexpr size_t initial_reserve_size = 1024;
public:
    std::vector<T> components;
    std::vector<Cell> entityIDs;
    size_t firstFreeCell = npos;

    ComponentStorage() {
        components.reserve(initial_reserve_size);
        entityIDs.reserve(initial_reserve_size);
    }

    size_t add(EntityID id, const T& component) {
        if (firstFreeCell == npos) {
            entityIDs.push_back({CellState::Occupied, id});
            components.push_back(component);
            return components.size() - 1;
        }
        const auto currentIndex = firstFreeCell;
        const auto nextFreeCell = entityIDs[firstFreeCell].entityId;
        entityIDs[currentIndex]  = {CellState::Occupied, id};
        components[currentIndex] = component;
        firstFreeCell = nextFreeCell;
        return currentIndex;
    }

    T* get(EntityID id) {
        for (size_t i = 0; i < entityIDs.size(); ++i) {
            if (entityIDs[i].state == CellState::Occupied && entityIDs[i].entityId == id) {
                return &components[i];
            }
        }
        return nullptr;
    }

    T* getByIndex(size_t index) {
        if (entityIDs[index].state == CellState::Free) {
            return nullptr;
        }
        return &components[index];
    }

    void removeEntity(EntityID id, EntityStorage& es) override {
        for (size_t i = 0; i < entityIDs.size(); ++i) {
            if (entityIDs[i].entityId == id) {
                if (entityIDs[i].state == CellState::Occupied) break;
                entityIDs[i] = { CellState::Free, firstFreeCell};
                firstFreeCell = i;
                break;
            }
        }
    }

    void removeComponent(EntityID id, EntityStorage& es) override {
        for (size_t i = 0; i < entityIDs.size(); ++i) {
            if (entityIDs[i].entityId == id) {
                entityIDs.erase(entityIDs.begin() + i);
                components.erase(components.begin() + i);
                return;
            }
        }
        es.removeComponent<T>(id);
    }

    std::vector<T>& getAll() { return components; }
    std::vector<Cell>& getEntities() { return entityIDs; }
};
