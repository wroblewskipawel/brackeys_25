#pragma once
#include "../ECS.hpp"

inline bool collide(const float& aX, const float& aY, const float& aR,
                    const float& bX, const float& bY, const float& bR) {
    return (bX - aX) * (bX - aX) + (bY - aY) * (bY - aY) < (aR + bR) * (aR + bR);
}

constexpr float repulsive_force = 3.f;

inline void collidingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<CollidingComponent>().andHas<PositionComponent>().get();

    for (auto it1 = entities.begin(); it1 != entities.end(); ++it1) {
        auto it2 = it1;
        ++it2;

        for (; it2 != entities.end(); ++it2) {
            const auto& entityA = *it1;
            const auto& entityB = *it2;
            auto ApComponent = ecs.getComponent<PositionComponent>(entityA);
            auto AcComponent = ecs.getComponent<CollidingComponent>(entityA);
            auto& [Ax, Ay, _] = *ApComponent;
            auto& [Ar] = *AcComponent;

            auto BpComponent = ecs.getComponent<PositionComponent>(entityB);
            auto BcComponent = ecs.getComponent<CollidingComponent>(entityB);
            auto& [Bx, By, _] = *BpComponent;
            auto& [Br] = *BcComponent;

            if (collide(Ax, Ay, Ar, Bx, By, Br)) {
                auto AmComponent = ecs.getComponent<MovableComponent>(entityA);
                if (AmComponent != nullptr) {
                    float DX1 = (Ax - Bx) * repulsive_force * deltaTime;
                    float DY1 = (Ay - By) * repulsive_force * deltaTime;
                    if (DX1 == 0.0f)
                        DX1 = (Ar + Br) / 2;
                    if (DY1 == 0.0f)
                        DY1 = (Ar + Br) / 2;
                    Ax += DX1;
                    Ay += DY1;
                }

                auto BmComponent = ecs.getComponent<MovableComponent>(entityB);
                if (BmComponent != nullptr) {
                    float DX2 = (Bx - Ax) * repulsive_force * deltaTime;
                    float DY2 = (By - Ay) * repulsive_force * deltaTime;
                    if (DX2 == 0.0f)
                        DX2 = (Ar + Br) / 2;
                    if (DY2 == 0.0f)
                        DY2 = (Ar + Br) / 2;
                    Bx += DX2;
                    By += DY2;
                }
            }
        }
    }
}
