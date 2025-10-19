#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "graphics/window/imgui.h"
#include "graphics/window/imgui/widget.h"
#include "graphics/window/opengl.h"

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

class Frame {
   public:
    Frame(Window& window) noexcept : window(window) { window.beginFrame(); };

    template <typename Type>
        requires WidgetListType<Type>
    void draw(Type& widgets) {
        widgets.draw();
    }

    ~Frame() noexcept { window.endFrame(); }

   private:
    Window& window;
};
