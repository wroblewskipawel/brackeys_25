#ifndef IMGUI_HPP
#define IMGUI_HPP

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../InputHandler/InputHandler.hpp"

inline void setupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
};

inline void updateImGui(GLFWwindow* window, size_t numOfEntities, float deltaTime) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");
    ImGui::Text("Entities: %zu", numOfEntities);

    float fps = 1.0f / deltaTime;
    ImGui::Text("FPS: %.1f", fps);

    // Debug input states
    if (gInputHandler.isClicked(Key::W)) ImGui::Text("W clicked");
    if (gInputHandler.isPressed(Key::W)) ImGui::Text("W pressed");
    if (gInputHandler.isReleased(Key::W)) ImGui::Text("W released");
    if (gInputHandler.isPressed(Key::A)) ImGui::Text("A pressed");
    if (gInputHandler.isPressed(Key::S)) ImGui::Text("S pressed");
    if (gInputHandler.isPressed(Key::D)) ImGui::Text("D pressed");
    if (gInputHandler.isPressed(Key::R)) ImGui::Text("R pressed");
    if (gInputHandler.isPressed(Key::Tab)) ImGui::Text("Tab pressed");
    if (gInputHandler.isPressed(Key::Space)) ImGui::Text("Space pressed");
    if (gInputHandler.isPressed(Key::Num_1)) ImGui::Text("1 pressed");
    if (gInputHandler.isPressed(Key::Num_2)) ImGui::Text("2 pressed");
    if (gInputHandler.isPressed(Key::Num_3)) ImGui::Text("3 pressed");
    if (gInputHandler.isPressed(Key::Escape)) {
        ImGui::Text("Escape pressed -> Closing window");
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    ImGui::End();
}

inline void renderImGui() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

inline void destroyImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif //IMGUI_HPP
