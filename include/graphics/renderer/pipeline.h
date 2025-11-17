#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "collections/unique_list.h"
#include "graphics/resources/gl/shader.h"

template <typename... Stages>
class Pipeline {
   public:
    template <typename... Args>
    Pipeline(Args&&... args) noexcept : stages(std::forward<Args>(args)...){};

    void execute(const CameraMatrices& cameraMatrices) noexcept {
        (execute<Stages>(cameraMatrices), ...);
    }

    void clear() noexcept { (clear<Stages>(), ...); }

    template <typename Stage>
    auto& getStage() const noexcept {
        return stages.template get<Stage>();
    }

    template <typename Stage>
    auto& getStage() noexcept {
        return stages.template get<Stage>();
    }

   private:
    using StageList = UniqueTypeList<Stages...>;

    template <typename Stage>
    void execute(const CameraMatrices& cameraMatrices) noexcept {
        stages.template get<Stage>().execute(cameraMatrices);
    }

    template <typename Stage>
    void clear() noexcept {
        stages.template get<Stage>().clear();
    }

    StageList stages;
};
