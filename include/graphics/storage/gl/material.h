#pragma once

#include "collections/slot_map.h"

template <typename Material>
class MaterialPack;

template <typename Material>
class MaterialPackBuilder;

template <typename Material>
struct MaterialHandle;

template <typename Material>
using MaterialPackHandle = Handle<MaterialPack<Material>>;

template <typename Material>
class MaterialPackStorage {
   private:
    friend class MaterialHandle<Material>;
    friend class MaterialPack<Material>;
    friend class MaterialPackBuilder<Material>;

    inline static SlotMap<MaterialPack<Material>> materialPackStorage;
};
