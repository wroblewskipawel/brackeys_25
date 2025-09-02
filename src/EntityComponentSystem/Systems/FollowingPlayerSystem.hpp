#pragma once
#include "../ECS.hpp"

inline void followingPlayerSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<FollowPlayerComponent>()
                                               .andHas<PositionComponent>()
                                               .andHas<MovableComponent>()
                                               .get();
    auto player = ecs.getEntitiesWithComponent<PlayerMovementComponent>().get();
    PositionComponent* playerPos = nullptr;
    if (player.size() == 1) {
        playerPos = ecs.getComponent<PositionComponent>(*player.begin());
    }
    if (playerPos == nullptr) {
        return;
    }
    for (auto entity : entities) {
        auto& [playerX, playerY, playerZ] = *playerPos;
        auto& [entityX, entityY, entityZ] = *ecs.getComponent<PositionComponent>(entity);
        auto& [entityDx, entityDy, entitySpeed, entityAcc] = *ecs.getComponent<MovableComponent>(entity);

        float dirX = playerX - entityX;
        float dirY = playerY - entityY;

        float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length > 0.0f) {
            dirX /= length;
            dirY /= length;
        }

        entityDx = dirX * entitySpeed;
        entityDy = dirY * entitySpeed;
    }
}
