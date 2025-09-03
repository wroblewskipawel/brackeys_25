#pragma once

#include "collections/slot_map.h"

template <typename Material>
class MaterialPack;

template <typename Material>
class MaterialPackBuilder;

template <typename Material>
struct MaterialHandle;

template <typename Material>
class MaterialPackHandle;

template <typename Material>
MaterialPackHandle<Material> registerMaterialPack(
    MaterialPack<Material>&&) noexcept;

template <typename Material>
class MaterialPackStorage {
   private:
    friend MaterialPackHandle<Material> registerMaterialPack<Material>(
        MaterialPack<Material>&&) noexcept;
    friend class MaterialHandle<Material>;
    friend class MaterialPack<Material>;
    friend class MaterialPackHandle<Material>;
    friend class MaterialPackBuilder<Material>;

    inline static SlotMap<MaterialPack<Material>, Shared> materialPackStorage;
};

template <typename Material>
class MaterialPackHandle {
   public:
    MaterialPackHandle& operator=(const MaterialPackHandle& other) noexcept {
        MaterialPackStorage<Material>::materialPackStorage.pop(
            std::move(handle));
        handle = other.handle.copyHandle(
            MaterialPackStorage<Material>::materialPackStorage);
        return *this;
    };

    MaterialPackHandle(MaterialPackHandle&&) = default;
    MaterialPackHandle& operator=(MaterialPackHandle&&) = default;

    ~MaterialPackHandle() {
        MaterialPackStorage<Material>::materialPackStorage.pop(
            std::move(handle));
    }

    bool operator==(const MaterialPackHandle& other) const noexcept {
        return handle == other.handle;
    }

    const Ref<MaterialPack<Material>, Shared> get() const noexcept {
        return MaterialPackStorage<Material>::materialPackStorage.get(handle);
    }

    Ref<MaterialPack<Material>, Shared> get() noexcept {
        return MaterialPackStorage<Material>::materialPackStorage.get(handle);
    }

    static MaterialPackHandle getInvalid() noexcept {
        return MaterialPackHandle(
            Handle<MaterialPack<Material>, Shared>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    MaterialPackHandle copy() const noexcept {
        return MaterialPackHandle(*this);
    }

   private:
    friend MaterialPackHandle<Material> registerMaterialPack<Material>(
        MaterialPack<Material>&&) noexcept;
    MaterialPackHandle(Handle<MaterialPack<Material>, Shared>&& handle)
        : handle(std::move(handle)) {}
    MaterialPackHandle(const MaterialPackHandle& handle) noexcept
        : handle(handle.handle.copyHandle(
              MaterialPackStorage<Material>::materialPackStorage)) {}

    Handle<MaterialPack<Material>, Shared> handle;
};

template <typename Material>
inline MaterialPackHandle<Material> registerMaterialPack(
    MaterialPack<Material>&& materialPack) noexcept {
    auto handle = MaterialPackStorage<Material>::materialPackStorage.emplace(
        std::move(materialPack));
    return MaterialPackHandle<Material>(std::move(handle));
}
