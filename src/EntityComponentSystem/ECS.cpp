#include "ECS.hpp"

#include <stdexcept>
#include <future>

EntityID ECS::createEntity() {
    entityStorage.addEntity(nextEntity);
    return nextEntity++;
}

void ECS::removeEntity(EntityID id) {
    for (auto& [type, storage] : storages) {
        storage->removeEntity(id, entityStorage);
    }
    entityStorage.removeEntity(id);
}

ECS& ECS::nextStage(StageType type) {
    stages.push_back({type, {}});
    return *this;
}

ECS& ECS::addSystem(std::function<void(ECS&, const float&)> fn) {
    if (stages.empty()) {
        throw std::runtime_error("No stage defined. Call nextStage() first.");
    }
    stages.back().systems.push_back(fn);
    return *this;
}

void ECS::update(const float& deltaTime) {
    for (auto& stage : stages) {
        if (stage.type == StageType::Parallel) {
            std::vector<std::future<void>> tasks;
            for (auto& sys : stage.systems) {
                tasks.push_back(std::async(std::launch::async, [&]() {
                    sys(*this, deltaTime);
                }));
            }
            for (auto& t : tasks) t.get();
        } else {
            for (auto& sys : stage.systems) {
                sys(*this, deltaTime);
            }
        }
    }
}
