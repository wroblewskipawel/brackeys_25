#pragma once

#include "collections/slot_map.h"

template <typename Vertex>
class MeshPack;

template <typename Vertex>
class MeshPackHandle;

template <typename Vertex>
class MeshPackStorage;

template <typename Vertex>
class MeshPackBuilder;

template <typename Vertex>
struct MeshHandle;

template <typename Vertex>
MeshPackHandle<Vertex> registerMeshPack(MeshPack<Vertex>&& mesh) noexcept;

template <typename Vertex>
class MeshPackStorage {
   private:
    friend MeshPackHandle<Vertex> registerMeshPack<Vertex>(
        MeshPack<Vertex>&&) noexcept;
    friend class MeshHandle<Vertex>;
    friend class MeshPack<Vertex>;
    friend class MeshPackHandle<Vertex>;
    friend class MeshPackBuilder<Vertex>;

    inline static SlotMap<MeshPack<Vertex>, Shared> meshPackStorage;
};

template <typename Vertex>
class MeshPackHandle {
   public:
    MeshPackHandle& operator=(const MeshPackHandle& other) noexcept {
        MeshPackStorage<Vertex>::meshPackStorage.pop(std::move(handle));
        handle =
            other.handle.copyHandle(MeshPackStorage<Vertex>::meshPackStorage);
        return *this;
    };

    MeshPackHandle(MeshPackHandle&&) = default;
    MeshPackHandle& operator=(MeshPackHandle&&) = default;

    ~MeshPackHandle() {
        MeshPackStorage<Vertex>::meshPackStorage.pop(std::move(handle));
    }

    bool operator==(const MeshPackHandle& other) const noexcept {
        return handle == other.handle;
    }

    const Ref<MeshPack<Vertex>, Shared> get() const noexcept {
        return MeshPackStorage<Vertex>::meshPackStorage.get(handle);
    }

    Ref<MeshPack<Vertex>, Shared> get() noexcept {
        return MeshPackStorage<Vertex>::meshPackStorage.get(handle);
    }

    static MeshPackHandle getInvalid() noexcept {
        return MeshPackHandle(Handle<MeshPack<Vertex>, Shared>::getInvalid());
    }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

    MeshPackHandle copy() const noexcept { return MeshPackHandle(*this); }

   private:
    friend MeshPackHandle<Vertex> registerMeshPack<Vertex>(
        MeshPack<Vertex>&&) noexcept;

    MeshPackHandle(Handle<MeshPack<Vertex>, Shared>&& handle)
        : handle(std::move(handle)) {}
    MeshPackHandle(const MeshPackHandle& handle) noexcept
        : handle(handle.handle.copyHandle(
              MeshPackStorage<Vertex>::meshPackStorage)) {}

    Handle<MeshPack<Vertex>, Shared> handle;
};

template <typename Vertex>
inline MeshPackHandle<Vertex> registerMeshPack(
    MeshPack<Vertex>&& mesh) noexcept {
    auto handle =
        MeshPackStorage<Vertex>::meshPackStorage.emplace(std::move(mesh));
    return MeshPackHandle<Vertex>(std::move(handle));
}
