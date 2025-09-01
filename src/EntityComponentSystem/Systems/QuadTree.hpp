#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "../Storage/EntityStorage.hpp"

struct AABB {
    float x, y;    // center
    float halfW, halfH;

    bool contains(float px, float py) const {
        return (px >= x - halfW && px <= x + halfW &&
                py >= y - halfH && py <= y + halfH);
    }

    bool intersects(const AABB& other) const {
        return !(other.x - other.halfW > x + halfW ||
                 other.x + other.halfW < x - halfW ||
                 other.y - other.halfH > y + halfH ||
                 other.y + other.halfH < y - halfH);
    }
};

class QuadTree {
    static constexpr int CAPACITY = 4;
    AABB boundary;
    std::vector<EntityID> entities;
    bool divided = false;

    std::unique_ptr<QuadTree> northeast, northwest, southeast, southwest;

public:
    QuadTree(const AABB& boundary) : boundary(boundary) {}

    void subdivide() {
        float x = boundary.x;
        float y = boundary.y;
        float hw = boundary.halfW / 2.f;
        float hh = boundary.halfH / 2.f;

        northeast = std::make_unique<QuadTree>(AABB{x + hw, y - hh, hw, hh});
        northwest = std::make_unique<QuadTree>(AABB{x - hw, y - hh, hw, hh});
        southeast = std::make_unique<QuadTree>(AABB{x + hw, y + hh, hw, hh});
        southwest = std::make_unique<QuadTree>(AABB{x - hw, y + hh, hw, hh});

        divided = true;
    }

    bool insert(EntityID id, float x, float y) {
        if (!boundary.contains(x, y)) return false;

        if (entities.size() < CAPACITY) {
            entities.push_back(id);
            return true;
        }

        if (!divided) subdivide();

        if (northeast->insert(id, x, y)) return true;
        if (northwest->insert(id, x, y)) return true;
        if (southeast->insert(id, x, y)) return true;
        if (southwest->insert(id, x, y)) return true;

        return false;
    }

    void query(const AABB& range, std::vector<EntityID>& found) const {
        if (!boundary.intersects(range)) return;

        for (auto id : entities) {
            found.push_back(id);
        }

        if (divided) {
            northeast->query(range, found);
            northwest->query(range, found);
            southeast->query(range, found);
            southwest->query(range, found);
        }
    }
};
