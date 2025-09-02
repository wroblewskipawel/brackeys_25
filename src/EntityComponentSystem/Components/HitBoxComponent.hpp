#pragma once

struct HitBoxComponent {
    float r = 0.0f;
    std::vector<size_t> collidedWith;
};
