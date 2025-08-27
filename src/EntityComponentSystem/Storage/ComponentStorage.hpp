#ifndef COMPONENTSTORAGE_HPP
#define COMPONENTSTORAGE_HPP

#include <vector>
#include <memory>
#include <cstddef>

using EntityID = std::size_t;

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual void remove(EntityID id) = 0;
};

template<typename T>
class ComponentStorage : public IStorage {
public:
    std::vector<T> components;
    std::vector<EntityID> entityIDs;

    void add(EntityID id, const T& component) {
        entityIDs.push_back(id);
        components.push_back(component);
    }

    T* get(EntityID id) {
        for (size_t i = 0; i < entityIDs.size(); ++i) {
            if (entityIDs[i] == id) {
                return &components[i];
            }
        }
        return nullptr;
    }

    void remove(EntityID id) override {
        for (size_t i = 0; i < entityIDs.size(); ++i) {
            if (entityIDs[i] == id) {
                entityIDs.erase(entityIDs.begin() + i);
                components.erase(components.begin() + i);
                return;
            }
        }
    }

    std::vector<T>& getAll() { return components; }
    std::vector<EntityID>& getEntities() { return entityIDs; }
};

#endif //COMPONENTSTORAGE_HPP
