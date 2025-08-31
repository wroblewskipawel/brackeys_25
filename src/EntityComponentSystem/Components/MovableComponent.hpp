#pragma once

struct MovableComponent {
    float dx = 0.0f;
    float dy = 0.0f;
    float speed = 0.0f;
    float acceleration = 0.0f;

    MovableComponent(float speed, float acceleration) : speed(speed), acceleration(acceleration) {}
};
