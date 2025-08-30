#include "CppUTest/TestHarness.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"

#include "EntityComponentSystem/ECS.hpp"

TEST_GROUP(EntityComponentSystemGroup) {
    void setup() {
        // Runs before each test
    }
    void teardown() {
        // Runs after each test
    }
};

TEST(EntityComponentSystemGroup, SimpleTest) {
    ECS ecs;

    size_t numOfEntities = 10;

    for (size_t i = 0; i < numOfEntities; ++i) {
        auto entity = ecs.createEntity();
        ecs.addComponent(entity, MovableComponent{});
        ecs.addComponent(entity, MovableComponent{});
    }

    CHECK_EQUAL(numOfEntities, ecs.entityStorage.getAllEntities().size());

    size_t entity = numOfEntities / 2;
    auto b = ecs.entityStorage.hasComponent<MovableComponent>(entity);
    CHECK_TRUE(b);
}

TEST(EntityComponentSystemGroup, ecsComponentStorageCellStateTest) {
    ECS ecs;

    auto entity1 = ecs.createEntity();
    auto entity2 = ecs.createEntity();
    auto entity3 = ecs.createEntity();

    ecs.addComponent(entity1, MovableComponent{});
    ecs.addComponent(entity2, MovableComponent{});
    ecs.addComponent(entity3, MovableComponent{});

    ecs.removeEntity(entity2);
    auto stateOfRemovedEntity = ecs.getStorage<MovableComponent>().getEntities()[entity2].state;
    CHECK_EQUAL(static_cast<size_t>(stateOfRemovedEntity), static_cast<size_t>(CellState::Free));

    auto nextEntity = ecs.createEntity();
    ecs.addComponent(nextEntity, MovableComponent{});
    auto cellOfAddedEntityWithIndexOfRemovedEntity = ecs.getStorage<MovableComponent>().getEntities()[entity2];
    auto addedIndex = cellOfAddedEntityWithIndexOfRemovedEntity.entityId;
    auto addedState = cellOfAddedEntityWithIndexOfRemovedEntity.state;
    CHECK_EQUAL(static_cast<size_t>(addedState), static_cast<size_t>(CellState::Occupied));
    CHECK_EQUAL(addedIndex, nextEntity);
}