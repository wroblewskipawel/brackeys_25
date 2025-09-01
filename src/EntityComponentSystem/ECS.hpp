#pragma once

#include <unordered_map>
#include <functional>
#include <typeindex>
#include <vector>
#include <memory>
#include "Storage/ComponentStorage.hpp"
#include "Storage/EntityStorage.hpp"
#include "Storage/QueryBuilder.hpp"
#include "mesh.h"
#include "material.h"
#include "draw.h"

struct RenderingQueues {
    std::shared_ptr<DrawQueue<UnlitVertex, UnlitMaterial>> unlitQueue{nullptr};
    std::shared_ptr<DrawQueue<ColoredVertex, EmptyMaterial>> coloredQueue{nullptr};
};

class ECS {
public:
    enum class StageType { Sequential, Parallel };

    ECS(RenderingQueues&& renderingQueues): renderingQueues(std::move(renderingQueues)) {}

private:
    EntityID nextEntity = 0;
    std::unordered_map<std::type_index, std::shared_ptr<IStorage>> storages;

    struct Stage {
        StageType type;
        std::vector<std::function<void(ECS&, const float&, RenderingQueues&)>> systems;
    };
    std::vector<Stage> stages;

    template<typename T>
    ComponentStorage<T>& getStorage() {
        auto type = std::type_index(typeid(T));
        if (storages.find(type) == storages.end()) {
            storages[type] = std::make_shared<ComponentStorage<T>>();
        }
        return *std::static_pointer_cast<ComponentStorage<T>>(storages[type]);
    }

    RenderingQueues renderingQueues;

public:
    EntityStorage entityStorage{};

    EntityID createEntity();
    void removeEntity(EntityID id);
    ECS& nextStage(StageType type);
    ECS& addSystem(std::function<void(ECS&, const float&, RenderingQueues&)> fn);
    void update(const float& deltaTime);

    template<typename T>
    T* getComponent(EntityID id) {
        auto indexInStorage = entityStorage.getComponentIndex<T>(id);
        if (indexInStorage == std::numeric_limits<size_t>::max()) {
            return nullptr;
        }
        return getStorage<T>().getByIndex(indexInStorage);
    }

    template<typename T>
    QueryBuilder getEntitiesWithComponent() {
        QueryBuilder qb(entityStorage);
        qb.andHas<T>();
        return qb;
    }

    template<typename T>
    void addComponent(EntityID entity, const T& component) {
        auto component_index = getStorage<T>().add(entity, component);
        entityStorage.addComponent<T>(entity, component_index);
    }
};
