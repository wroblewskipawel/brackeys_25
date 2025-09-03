#pragma once

#include "collections/slot_map.h"

template <typename Vertex>
struct MeshData;

template <typename Vertex>
class MeshDataStorage;

template <typename Vertex>
class MeshPackBuilder;

template <typename Vertex>
class MeshDataHandle;

template <typename Vertex>
MeshDataHandle<Vertex> registerMeshData(MeshData<Vertex>&&) noexcept;

template <typename Vertex>
class MeshDataStorage {
   private:
    friend MeshDataHandle<Vertex> registerMeshData<Vertex>(
        MeshData<Vertex>&&) noexcept;
    friend class MeshData<Vertex>;
    friend class MeshDataHandle<Vertex>;
    friend class MeshPackBuilder<Vertex>;

    inline static SlotMap<MeshData<Vertex>, Shared> meshStorage = {};
};

template <typename Vertex>
class MeshDataHandle {
   public:
    MeshDataHandle& operator=(const MeshDataHandle& other) noexcept {
        MeshDataStorage<Vertex>::meshStorage.pop(std::move(handle));
        handle = other.handle.copyHandle(MeshDataStorage<Vertex>::meshStorage);
        return *this;
    };

    MeshDataHandle(MeshDataHandle&&) = default;
    MeshDataHandle& operator=(MeshDataHandle&&) = default;

    ~MeshDataHandle() {
        MeshDataStorage<Vertex>::meshStorage.pop(std::move(handle));
    }

    bool operator==(const MeshDataHandle& other) const noexcept {
        return handle == other.handle;
    }

    const Ref<MeshData<Vertex>, Shared> get() const noexcept {
        return MeshDataStorage<Vertex>::meshStorage.get(handle);
    }

    Ref<MeshData<Vertex>, Shared> get() noexcept {
        return MeshDataStorage<Vertex>::meshStorage.get(handle);
    }

    static MeshDataHandle getInvalid() noexcept {
        return MeshDataHandle(Handle<MeshData<Vertex>, Shared>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    MeshDataHandle copy() const noexcept { return MeshDataHandle(*this); }

   private:
    friend MeshDataHandle<Vertex> registerMeshData<Vertex>(
        MeshData<Vertex>&&) noexcept;

    MeshDataHandle(Handle<MeshData<Vertex>, Shared>&& handle)
        : handle(std::move(handle)) {}
    MeshDataHandle(const MeshDataHandle& handle) noexcept
        : handle(
              handle.handle.copyHandle(MeshDataStorage<Vertex>::meshStorage)) {}
    Handle<MeshData<Vertex>, Shared> handle;
};

template <typename Vertex>
inline MeshDataHandle<Vertex> registerMeshData(
    MeshData<Vertex>&& mesh) noexcept {
    auto handle = MeshDataStorage<Vertex>::meshStorage.emplace(std::move(mesh));
    return MeshDataHandle<Vertex>(std::move(handle));
}
