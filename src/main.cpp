#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <cmath>
#include <stdexcept>
#include <iostream>

#include "ImGui/ImGui.hpp"
#include "InputHandler/InputHandler.hpp"
#include "MusicManager/MusicManager.hpp"

#include "EntityComponentSystem/ECS.hpp"
#include "EntityComponentSystem/Components/MovableComponent.hpp"
#include "EntityComponentSystem/Systems/MovementSystem.hpp"

// Vertex Shader source code
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform vec2 offset;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.xy + offset, aPos.z, 1.0);\n"
"}\0";
//Fragment Shader source code
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"}\n\0";

void debugSystem(ECS& ecs, const float& deltaTime) {
    auto& movables = ecs.getStorage<MovableComponent>();
    std::cout << "Debug: " << movables.getAll().size() << " movables tracked.\n";
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize OpenGL");
    };

    glfwSetKeyCallback(window, keyCallback);
    setupImGui(window);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint offsetLoc = glGetUniformLocation(shaderProgram, "offset");

    GLfloat vertices[] =
    {
        -0.5f, -0.5f * float(std::sqrt(3)) / 3, 0.0f, // Lower left corner
        0.5f, -0.5f * float(std::sqrt(3)) / 3, 0.0f, // Lower right corner
        0.0f, 0.5f * float(std::sqrt(3)) * 2 / 3, 0.0f // Upper corner
    };

    GLuint VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    ECS ecs;

    EntityID player = ecs.createEntity();
    EntityID enemy  = ecs.createEntity();

    ecs.addComponent(player, MovableComponent{0,0,1.0f,1.0f});
    ecs.addComponent(enemy, MovableComponent{5,5,1.0f,1.0f});

    ecs.nextStage(ECS::StageType::Parallel)
          .addSystem(movementSystem)
       .nextStage(ECS::StageType::Sequential)
          .addSystem(debugSystem);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ecs.update(deltaTime);

        updateImGui(window);

        // Music test
        if (gInputHandler.isClicked(Key::R)) gMusicManager.play(SoundID::Coin);
        if (gInputHandler.isPressed(Key::Space))
            gMusicManager.play(SoundID::Explosion);

        const auto x = ecs.getStorage<MovableComponent>().getAll()[player];
        glUniform2f(offsetLoc, x.x, x.y);

        gInputHandler.update();

        // Clear
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Render
        renderImGui();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyImGui();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
