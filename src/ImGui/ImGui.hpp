#ifndef IMGUI_HPP
#define IMGUI_HPP

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

void setupImGui(GLFWwindow* window);
void updateImGui(GLFWwindow* window);
void renderImGui();
void destroyImGui();

#endif //IMGUI_HPP
