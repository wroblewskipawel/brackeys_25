#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <cmath>
#include "../ECS.hpp"
#include "QuadTree.hpp"

inline bool collide(const float& aX, const float& aY, const float& aR,
                    const float& bX, const float& bY, const float& bR) {
    return (bX - aX) * (bX - aX) + (bY - aY) * (bY - aY) < (aR + bR) * (aR + bR);
}

constexpr float repulsive_force = 3.f;

inline void collidingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entitiesSet = ecs.getEntitiesWithComponent<CollidingComponent>()
                          .andHas<PositionComponent>().get();

    std::vector<EntityID> entities(entitiesSet.begin(), entitiesSet.end());

    AABB worldBounds{0.f, 0.f, 500.f, 500.f};
    QuadTree quadTree(worldBounds);

    for (auto entity : entities) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
        if (pos) quadTree.insert(entity, pos->x, pos->y);
    }

    struct Vec2 { float x=0.f, y=0.f; };
    std::unordered_map<EntityID, Vec2> displacements;
    std::mutex dispMutex;

    auto worker = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            EntityID entity = entities[i];
            auto posA = ecs.getComponent<PositionComponent>(entity);
            auto colA = ecs.getComponent<CollidingComponent>(entity);
            if (!posA || !colA) continue;

            bool movableA = ecs.entityStorage.hasComponent<MovableComponent>(entity);

            AABB range{posA->x, posA->y, colA->r, colA->r};
            std::vector<EntityID> nearby;
            quadTree.query(range, nearby);

            for (auto other : nearby) {
                if (other <= entity) continue;

                auto posB = ecs.getComponent<PositionComponent>(other);
                auto colB = ecs.getComponent<CollidingComponent>(other);
                if (!posB || !colB) continue;

                bool movableB = ecs.entityStorage.hasComponent<MovableComponent>(other);

                float dx = posB->x - posA->x;
                float dy = posB->y - posA->y;
                float distSq = dx*dx + dy*dy;
                float dist = std::sqrt(distSq);
                if (dist == 0.f) { dx = 1.f; dy = 0.f; dist = 1.f; }

                float overlap = (colA->r + colB->r - dist);
                if (overlap > 0.f) {
                    float nx = dx / dist;
                    float ny = dy / dist;
                    float pushA = movableA ? 0.5f * overlap : 0.f;
                    float pushB = movableB ? 0.5f * overlap : 0.f;

                    Vec2 dispA{ -nx * pushA, -ny * pushA };
                    Vec2 dispB{  nx * pushB,  ny * pushB };

                    std::lock_guard<std::mutex> lock(dispMutex);
                    displacements[entity].x += dispA.x;
                    displacements[entity].y += dispA.y;
                    displacements[other].x += dispB.x;
                    displacements[other].y += dispB.y;
                }
            }
        }
    };

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    std::vector<std::thread> threads;
    size_t chunkSize = (entities.size() + numThreads - 1) / numThreads;

    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunkSize;
        size_t end = std::min(start + chunkSize, entities.size());
        threads.emplace_back(worker, start, end);
    }

    for (auto& th : threads) th.join();

    for (auto& [entity, disp] : displacements) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
        if (!pos) continue;
        if (!ecs.entityStorage.hasComponent<MovableComponent>(entity)) continue;

        pos->x += disp.x;
        pos->y += disp.y;
    }
}
