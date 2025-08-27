#include "InputHandler.hpp"

InputHandler gInputHandler;

void keyCallback(GLFWwindow* window, const int key, int scancode,
                 const int action, int mods) {
    switch (key) {
        case GLFW_KEY_W:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::W) : gInputHandler.releaseKey(Key::W); break;
        case GLFW_KEY_A:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::A) : gInputHandler.releaseKey(Key::A); break;
        case GLFW_KEY_S:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::S) : gInputHandler.releaseKey(Key::S); break;
        case GLFW_KEY_D:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::D) : gInputHandler.releaseKey(Key::D); break;
        case GLFW_KEY_R:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::R) : gInputHandler.releaseKey(Key::R); break;
        case GLFW_KEY_TAB:    (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Tab) : gInputHandler.releaseKey(Key::Tab); break;
        case GLFW_KEY_SPACE:  (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Space) : gInputHandler.releaseKey(Key::Space); break;
        case GLFW_KEY_ESCAPE: (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Escape) : gInputHandler.releaseKey(Key::Escape); break;
        case GLFW_KEY_1:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Num_1) : gInputHandler.releaseKey(Key::Num_1); break;
        case GLFW_KEY_2:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Num_2) : gInputHandler.releaseKey(Key::Num_2); break;
        case GLFW_KEY_3:      (action == GLFW_PRESS) ? gInputHandler.pressKey(Key::Num_3) : gInputHandler.releaseKey(Key::Num_3); break;
        default: ;
    }
}

void InputHandler::pressKey(Key key) {
    if (const auto idx = static_cast<size_t>(key);
        keyStates[idx] == KeyState::Released)
        keyStates[idx] = KeyState::Clicked;
    else
        keyStates[idx] = KeyState::Pressed;
}

void InputHandler::releaseKey(Key key) {
    keyStates[static_cast<size_t>(key)] = KeyState::Released;
}

void InputHandler::update() {
    for (auto &state : keyStates) {
        if (state == KeyState::Clicked) {
            state = KeyState::Pressed;
        }
    }
}

bool InputHandler::isPressed(Key key) const {
    const KeyState state = keyStates[static_cast<size_t>(key)];
    return state == KeyState::Pressed || state == KeyState::Clicked;
}

bool InputHandler::isClicked(Key key) const {
    return keyStates[static_cast<size_t>(key)] == KeyState::Clicked;
}

bool InputHandler::isReleased(Key key) const {
    return keyStates[static_cast<size_t>(key)] == KeyState::Released;
}