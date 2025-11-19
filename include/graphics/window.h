#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "graphics/renderer.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/window/imgui.h"
#include "graphics/window/imgui/widget.h"
#include "graphics/window/opengl.h"

template <typename Renderer>
class Frame;

class Window {
   public:
    Window() {
        if (!glfwInit()) {
            std::println(std::cerr, "Failed to initialize glfw");
            std::abort();
        };

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

#ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

        window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
        if (!window) {
            glfwTerminate();
            std::println(std::cerr, "Failed to initialize glfw");
            std::abort();
        }

        glfwMakeContextCurrent(window);
        OpenGlContext::init();
        ImguiContext::init(window);
    };

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    ~Window() {
        ImguiContext::terminate();
        OpenGlContext::terminate();
        glfwTerminate();
    };

    bool shouldClose() noexcept { return glfwWindowShouldClose(window); }

   private:
    template<typename>
    friend class Frame;

    void beginFrame() noexcept {
        glfwPollEvents();
        OpenGlContext::beginFrame();
        ImguiContext::beginFrame();
    }

    void endFrame() noexcept {
        ImguiContext::endFrame();
        OpenGlContext::endFrame();
        glfwSwapBuffers(window);
    }

    GLFWwindow* window;
};

template <typename Renderer>
class Frame {
   public:
    Frame(Window& window, Renderer& renderer,
          const CameraMatrices& cameraMatrices) noexcept
        : window(window), renderer(renderer), cameraMatrices(cameraMatrices) {
        window.beginFrame();
        renderer.beginFrame();
    };

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    Frame(Frame&&) = delete;
    Frame& operator=(Frame&&) = delete;

    template <typename Type>
        requires WidgetListType<Type>
    auto& draw(Type& widgets) {
        widgets.draw();
        return *this;
    }

    template <typename Vertex, typename Material, typename Instance>
    auto& draw(const StaticBatchHandle<Vertex, Material, Instance>& packHandle,
               const glm::mat4& instanceOffset = glm::mat4(1.0f)) {
        renderer.addDraw(packHandle, instanceOffset);
        return *this;
    }

    template <typename Vertex, typename Material, typename Instance>
    auto& draw(const Model<Vertex, Material>& model, const Instance& instance) {
        renderer.addDraw(model, instance);
        return *this;
    }

    template <typename Vertex, typename Material, typename Instance>
    auto& draw(const Model<Vertex, Material>& model, const Instance& instance,
               const AnimationPlayer& sampler) {
        renderer.addDraw(model, instance, sampler);
        return *this;
    }

    template <typename Vertex, typename Material, typename Instance>
    auto& draw(const std::vector<StaticBatchHandle<Vertex, Material, Instance>>&
                   batchHandles,
               const glm::mat4& instanceOffset = glm::mat4(1.0f)) {
        renderer.addDraw(batchHandles, instanceOffset);
        return *this;
    }

    ~Frame() noexcept {
        renderer.endFrame(cameraMatrices);
        window.endFrame();
    }

   private:
    Window& window;
    Renderer& renderer;
    CameraMatrices cameraMatrices;
};
