#pragma once

struct HitBoxComponent {
    float r = 0.0f;
    std::unordered_set<size_t> collidedWith;
};
