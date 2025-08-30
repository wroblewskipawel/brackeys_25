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
