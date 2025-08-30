#pragma once

#include <glm/glm.hpp>

#include "../ECS.hpp"
#include "../Components/MovableComponent.hpp"
#include "../Components/RenderableComponent.hpp"

inline void renderingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<MovableComponent>().get();
    for (const auto& entity : entities) {
        auto movement = ecs.getComponent<MovableComponent>(entity);
        auto& [x, y, speedX, speedY] = *movement;

        auto* coloredMesh = ecs.getComponent<RenderableColored>(entity);
        if (coloredMesh != nullptr) {
            auto transform = glm::vec3(x, y, 0.0);
            auto draw = coloredMesh->withTransform(glm::translate(glm::mat4(1.0), transform));
            renderingQueues.coloredQueue->emplace_back(draw);
            continue;
        }

        auto* unlitMesh = ecs.getComponent<RenderableUnlit>(entity);
        if (unlitMesh) {
            auto transform = glm::vec3(x, y, 0.0);
            auto draw = unlitMesh->withTransform(glm::translate(glm::mat4(1.0), transform));
            renderingQueues.unlitQueue->emplace_back(draw);
            continue;
        }
    }
}
