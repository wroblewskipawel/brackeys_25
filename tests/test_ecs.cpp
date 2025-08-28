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

struct TestComponent1 {
    float a = 0.0f;
    float b = 0.0f;
};

struct TestComponent2 {
    int a = 0;
    int b = 0;
};

inline void testSystem1(ECS& ecs, const float& deltaTime) {
    auto& component1 = ecs.getStorage<TestComponent1>();
    for (size_t i = 0; i < component1.getAll().size(); ++i) {
        auto& [a, b] = component1.getAll()[i];
        a += deltaTime;
        b += deltaTime;
    }
}

inline void testSystem2(ECS& ecs, const float& deltaTime) {
    auto& component2 = ecs.getStorage<TestComponent2>();
    for (size_t i = 0; i < component2.getAll().size(); ++i) {
        auto& [a, b] = component2.getAll()[i];
        a += static_cast<int>(deltaTime);
        b += static_cast<int>(deltaTime);
    }
}

inline void testSystem3(ECS& ecs, const float& deltaTime) {
    auto& component1 = ecs.getStorage<TestComponent1>();
    auto& component2 = ecs.getStorage<TestComponent2>();
    for (size_t i = 0; i < component1.getAll().size(); ++i) {
        auto& [a1, b1] = component1.getAll()[i];
        for (size_t j = 0; j < component2.getAll().size(); ++j) {
            auto& [a2, b2] = component2.getAll()[j];
            a1 += static_cast<float>(a2);
            b1 += static_cast<float>(b2);
        }
    }
}

TEST(EntityComponentSystemGroup, SimpleTest) {
    ECS ecs;

    size_t numOfEntities = 10;

    for (size_t i = 0; i < numOfEntities; ++i) {
        auto entity = ecs.createEntity();
        ecs.addComponent(entity, TestComponent1{});
        ecs.addComponent(entity, TestComponent2{});
    }

    CHECK_EQUAL(numOfEntities, ecs.entityStorage.getAllEntities().size());

    size_t entity = numOfEntities / 2;
    auto b = ecs.entityStorage.hasComponent<TestComponent1>(entity);
    CHECK_TRUE(b);
}