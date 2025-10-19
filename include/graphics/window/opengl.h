#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>

#include "graphics/resources/gl/vertex_array.h"
#include "graphics/debug.h"

class OpenGlContext {
   public:
    static void init() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::println(std::cerr, "Failed to initialize OpenGL");
            std::abort();
        };

        #ifndef NDEBUG
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                                GL_TRUE);
        #endif

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_FRAMEBUFFER_SRGB);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glClearDepth(1.0f);
        glDepthFunc(GL_LEQUAL);
    }

    static void beginFrame() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static void endFrame() {}

    static void terminate() {
        VertexArrayStorage::destroyVertexArrays();
    }
};
