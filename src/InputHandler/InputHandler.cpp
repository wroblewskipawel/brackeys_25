#include "InputHandler.hpp"

InputHandler gInputHandler;

void keyCallback(GLFWwindow* window, const int key, int scancode,
                 const int action, int mods) {
    switch (key) {
        case GLFW_KEY_W:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::W) : gInputHandler.releaseKey(Key::W);
            break;
        case GLFW_KEY_A:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::A) : gInputHandler.releaseKey(Key::A);
            break;
        case GLFW_KEY_S:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::S) : gInputHandler.releaseKey(Key::S);
            break;
        case GLFW_KEY_D:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::D) : gInputHandler.releaseKey(Key::D);
            break;
        case GLFW_KEY_R:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::R) : gInputHandler.releaseKey(Key::R);
            break;
        case GLFW_KEY_TAB:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Tab) : gInputHandler.releaseKey(Key::Tab);
            break;
        case GLFW_KEY_SPACE:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Space) : gInputHandler.releaseKey(Key::Space);
            break;
        case GLFW_KEY_ESCAPE:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Escape) : gInputHandler.releaseKey(Key::Escape);
            break;
        case GLFW_KEY_1:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Num_1) : gInputHandler.releaseKey(Key::Num_1);
            break;
        case GLFW_KEY_2:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Num_2) : gInputHandler.releaseKey(Key::Num_2);
            break;
        case GLFW_KEY_3:
            (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Num_3) : gInputHandler.releaseKey(Key::Num_3);
            break;
        default:;
    }
}

void InputHandler::pressKey(Key key) {
    keyStates.set(static_cast<size_t>(key), true);
}

void InputHandler::releaseKey(Key key) {
    keyStates.set(static_cast<size_t>(key), false);
}

bool InputHandler::isKeyPressed(Key key) const {
    return keyStates.test(static_cast<size_t>(key));
}