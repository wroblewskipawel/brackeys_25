#include "ImGui.hpp"
#include "../InputHandler/InputHandler.hpp"

void setupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void updateImGui(GLFWwindow* window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("IDK");
    ImGui::Text("Hello World!");
    // Debug input states
    if (gInputHandler.isKeyPressed(Key::W)) ImGui::Text("W pressed");
    if (gInputHandler.isKeyPressed(Key::A)) ImGui::Text("A pressed");
    if (gInputHandler.isKeyPressed(Key::S)) ImGui::Text("S pressed");
    if (gInputHandler.isKeyPressed(Key::D)) ImGui::Text("D pressed");
    if (gInputHandler.isKeyPressed(Key::R)) ImGui::Text("R pressed");
    if (gInputHandler.isKeyPressed(Key::Tab)) ImGui::Text("Tab pressed");
    if (gInputHandler.isKeyPressed(Key::Space)) ImGui::Text("Space pressed");
    if (gInputHandler.isKeyPressed(Key::Num_1)) ImGui::Text("1 pressed");
    if (gInputHandler.isKeyPressed(Key::Num_2)) ImGui::Text("2 pressed");
    if (gInputHandler.isKeyPressed(Key::Num_3)) ImGui::Text("3 pressed");
    if (gInputHandler.isKeyPressed(Key::Escape)) {
        ImGui::Text("Escape pressed -> Closing window");
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    ImGui::End();
}

void renderImGui() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void destroyImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}