#ifndef ECS_HPP
#define ECS_HPP

#include <unordered_map>
#include <functional>
#include <typeindex>
#include <vector>
#include "Storage/ComponentStorage.hpp"
#include "Storage/EntityStorage.hpp"

class ECS {
public:
    enum class StageType { Sequential, Parallel };

private:
    EntityID nextEntity = 0;
    std::unordered_map<std::type_index, std::shared_ptr<IStorage>> storages;

    struct Stage {
        StageType type;
        std::vector<std::function<void(ECS&, const float&)>> systems;
    };
    std::vector<Stage> stages;

public:
    EntityStorage entityStorage{};

    EntityID createEntity();
    void removeEntity(EntityID id);
    ECS& nextStage(StageType type);
    ECS& addSystem(std::function<void(ECS&, const float&)> fn);
    void update(const float& deltaTime);

    template<typename T>
    ComponentStorage<T>& getStorage() {
        auto type = std::type_index(typeid(T));
        if (storages.find(type) == storages.end()) {
            storages[type] = std::make_shared<ComponentStorage<T>>();
        }
        return *std::static_pointer_cast<ComponentStorage<T>>(storages[type]);
    }

    template<typename T>
    void addComponent(EntityID entity, const T& component) {
        auto component_index = getStorage<T>().add(entity, component);
        entityStorage.addComponent<T>(entity, component_index);
    }
};

#endif //ECS_HPP
