#pragma once

#include "collections/slot_map.h"

template <typename Vertex>
class MeshPack;

template <typename Vertex>
class MeshPackBuilder;

template <typename Vertex>
struct MeshHandle;

template <typename Vertex>
using MeshPackHandle = Handle<MeshPack<Vertex>>;

template <typename Vertex>
class MeshPackStorage {
   private:
    friend class MeshHandle<Vertex>;
    friend class MeshPack<Vertex>;
    friend class MeshPackBuilder<Vertex>;

    inline static SlotMap<MeshPack<Vertex>> meshPackStorage;
};
