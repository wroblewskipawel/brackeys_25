#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include <array>
#include <GLFW/glfw3.h>

enum class Key {
    W = 0,
    A,
    S,
    D,
    R,
    Tab,
    Space,
    Escape,
    Num_1,
    Num_2,
    Num_3,
    COUNT
};

// Three stages for each key
enum class KeyState {
    Released,
    Pressed,
    Clicked
};

class InputHandler {
    std::array<KeyState, static_cast<size_t>(Key::COUNT)> keyStates{};

public:
    void pressKey(Key key);
    void releaseKey(Key key);
    void update();

    bool isPressed(Key key) const;
    bool isClicked(Key key) const;
    bool isReleased(Key key) const;
};

extern InputHandler gInputHandler;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

#endif // INPUTHANDLER_HPP
