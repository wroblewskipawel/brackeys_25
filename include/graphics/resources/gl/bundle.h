#pragma once

#include <unordered_map>
#include <unordered_set>

#include "collections/unique_list.h"
#include "graphics/assets/bundle.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/mesh.h"
#include "graphics/storage/material.h"
#include "graphics/storage/mesh.h"

template <typename... Vertices>
using MeshPackHandleList = UniqueTypeList<MeshPackHandle<Vertices>...>;

template <typename... Vertices>
using MeshDataList = VectorList<MeshDataHandle<Vertices>...>;

template <typename Vertices, typename PackHandleList, typename PackDataList>
void loadMeshPacks(Vertices remaining, PackHandleList& packHandles,
                   const PackDataList& packData) noexcept;

template <typename... PackVertices, typename... DataVertices>
void loadMeshPacks(TypeList<> remaining,
                   MeshPackHandleList<PackVertices...>& handleList,
                   const MeshDataList<DataVertices...>& dataList) noexcept {}

template <typename Vertex, typename... Vertices, typename... PackVertices,
          typename... DataVertices>
void loadMeshPacks(TypeList<Vertex, Vertices...> remaining,
                   MeshPackHandleList<PackVertices...>& handleList,
                   const MeshDataList<DataVertices...>& dataList) noexcept {
    auto packBuilder = MeshPackBuilder<Vertex>();
    packBuilder.addMeshMulti(
        dataList.template getStorage<MeshDataHandle<Vertex>>());
    auto& packHandle = handleList.template get<MeshPackHandle<Vertex>>();
    packHandle = packBuilder.build();

    loadMeshPacks(TypeList<Vertices...>{}, handleList, dataList);
}

template <typename... Vertices>
class MeshPackList {
   public:
    template <typename... Data>
    MeshPackList(const MeshDataList<Data...>& packData) noexcept {
        loadMeshPacks(TypeList<Vertices...>{}, packList, packData);
    };

    template <typename Vertex>
    auto& getPackHandleRef() const noexcept {
        return packList.template get<MeshPackHandle<Vertex>>();
    }

   private:
    MeshPackHandleList<Vertices...> packList;
};

template <typename... Materials>
using MaterialPackHandleList = VectorList<MaterialPackHandle<Materials>...>;

template <typename Material, typename... Materials>
using MaterialPackIndex = typename MaterialPackHandleList<
    Materials...>::template Index<MaterialPackHandle<Material>>;

template <typename Material, typename... Materials>
using MaterialPackIndexMap = std::unordered_map<
    size_t, std::pair<size_t, MaterialPackIndex<Material, Materials...>>>;

template <typename... Materials>
using MaterialPackMaterialMap =
    UniqueTypeList<MaterialPackIndexMap<Materials, Materials...>...>;

template <typename... Materials>
using MaterialBuilderHandleList =
    VectorList<MaterialBuilderHandle<Materials>...>;

template <typename Material>
auto partitionMaterials(
    const std::vector<MaterialBuilderHandle<Material>>& materials) {
    auto materialMap = std::unordered_map<
        TextureDims, std::pair<std::unordered_map<size_t, size_t>,
                               std::vector<MaterialBuilderHandle<Material>>>>{};
    for (auto [materialIndex, materialHandle] :
         std::views::enumerate(materials)) {
        auto textureDims = materialHandle.get().get().getTextureDimensions();
        auto materialIt = materialMap.find(textureDims);
        if (materialIt == materialMap.end()) {
            materialMap.emplace(
                std::piecewise_construct, std::forward_as_tuple(textureDims),
                std::forward_as_tuple(
                    std::unordered_map<size_t, size_t>{},
                    std::vector<MaterialBuilderHandle<Material>>{}));
        }
        auto& materialMapEntry = materialMap.find(textureDims)->second;
        materialMapEntry.first.emplace(
            std::piecewise_construct, std::forward_as_tuple(materialIndex),
            std::forward_as_tuple(materialMapEntry.second.size()));
        materialMapEntry.second.emplace_back(materialHandle.copy());
    }
    return materialMap;
}

template <typename Materials, typename PackHandleList, typename PackHandleMap,
          typename PackDataList>
void loadMaterialPacks(Materials remaining, PackHandleList& packHandles,
                       PackHandleMap& packMap,
                       const PackDataList& packData) noexcept;

template <typename... PackMaterials, typename... DataMaterials>
void loadMaterialPacks(
    TypeList<> remaining, MaterialPackHandleList<PackMaterials...>& handleList,
    MaterialPackMaterialMap<PackMaterials...>& handleMap,
    const MaterialBuilderHandleList<DataMaterials...>& dataList) noexcept {}

template <typename Material, typename... Materials, typename... PackMaterials,
          typename... DataMaterials>
void loadMaterialPacks(
    TypeList<Material, Materials...> remaining,
    MaterialPackHandleList<PackMaterials...>& handleList,
    MaterialPackMaterialMap<PackMaterials...>& handleMap,
    const MaterialBuilderHandleList<DataMaterials...>& dataList) noexcept {
    if constexpr (!EmptyMaterialType<Material>) {
        for (const auto& [_, materialsMapEntry] : partitionMaterials(
                 dataList
                     .template getStorage<MaterialBuilderHandle<Material>>())) {
            auto packIndex = handleList.insert(
                MaterialPackBuilder<Material>()
                    .addMaterialMulti(materialsMapEntry.second)
                    .build());
            auto& packIndexMap = handleMap.template get<
                MaterialPackIndexMap<Material, PackMaterials...>>();
            for (auto [materialIndex, packMaterialIndex] :
                 materialsMapEntry.first) {
                packIndexMap.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(materialIndex),
                    std::forward_as_tuple(packMaterialIndex, packIndex));
            }
        }
    } else {
        auto packIndex =
            handleList.insert(MaterialPackHandle<Material>::getInvalid());
        auto& packIndexMap = handleMap.template get<
            MaterialPackIndexMap<Material, PackMaterials...>>();
        packIndexMap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(std::numeric_limits<uint32_t>::max()),
            std::forward_as_tuple(std::numeric_limits<uint32_t>::max(),
                                  packIndex));
    }
    loadMaterialPacks(TypeList<Materials...>{}, handleList, handleMap,
                      dataList);
}

template <typename... Materials>
class MaterialPackList {
   public:
    template <typename... Data>
    MaterialPackList(
        const MaterialBuilderHandleList<Data...>& packData) noexcept {
        loadMaterialPacks(TypeList<Materials...>{}, packList, packMap,
                          packData);
    };

    template <typename Material>
    auto& getPackHandleRef(size_t materialIndex,
                           size_t& packMaterialIndex) const noexcept {
        auto& packIndexMap =
            packMap
                .template get<MaterialPackIndexMap<Material, Materials...>>();
        auto packListIndex = packIndexMap.find(materialIndex);
        if (packListIndex == packIndexMap.end()) {
            std::println(std::cerr,
                         "MaterialPackList::getPackHandleRef: Invalid "
                         "materialIndex");
            std::abort();
        }
        packMaterialIndex = packListIndex->second.first;
        return packList.get(packListIndex->second.second);
    }

   private:
    MaterialPackHandleList<Materials...> packList;
    MaterialPackMaterialMap<Materials...> packMap;
};

template <typename, typename>
class ResourceBundle;

template <typename... Vertices, typename... Materials>
class ResourceBundle<TypeList<Vertices...>, TypeList<Materials...>> {
   public:
    using IndexStorage =
        AssetsIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>>;

    template <typename Vertex, typename Material>
    using Ref = typename IndexStorage::template Ref<Vertex, Material>;

    template <typename Vertex, typename Material>
    auto getModel(const std::string& modelNamespace,
                  const std::string& modelName) const noexcept {
        const auto modelIndices =
            documenIndexMap.template getModelIndices<Vertex, Material>(
                modelNamespace, modelName);
        return tryGetModel(modelIndices);
    }

    template <typename Vertex, typename Material>
    auto getModel(const std::string& modelNamespace,
                  size_t modelIndex) const noexcept {
        const auto modelIndices =
            documenIndexMap.template getModelIndices<Vertex, Material>(
                modelNamespace, modelIndex);
        return tryGetModel(modelIndices);
    }

    // Add constexpr check if the Vertex type is AnimatedVertex type
    template <typename Vertex, typename Material>
    auto getModelAnimations(const std::string& modelNamespace,
                            const std::string& modelName) const noexcept {
        const auto modelIndices =
            documenIndexMap.template getModelIndices<Vertex, Material>(
                modelNamespace, modelName);
        return tryGetAnimations(modelIndices);
    }

    template <typename Vertex, typename Material>
    auto getModelAnimations(const std::string& modelNamespace,
                            size_t modelIndex) const noexcept {
        const auto modelIndices =
            documenIndexMap.template getModelIndices<Vertex, Material>(
                modelNamespace, modelIndex);
        return tryGetAnimations(modelIndices);
    }

   private:
    friend class Window;

    ResourceBundle(
        const AssetsBundle<TypeList<Vertices...>, TypeList<Materials...>>&
            assetsBundle) noexcept
        : animations(copyVector(assetsBundle.getAnimations())),
          meshPacks(assetsBundle.getMeshes()),
          materialPacks(assetsBundle.getMaterials()),
          documenIndexMap(assetsBundle.getIndicesMap()) {}

    template <typename Vertex, typename Material>
    auto tryGetModel(const Ref<Vertex, Material>& modelRef) const noexcept {
        size_t packMaterialIndex = 0;
        auto meshIndex = modelRef.get().meshIndex;
        auto materialIndex = modelRef.get().materialIndex;
        auto& materialPackRef =
            materialPacks.template getPackHandleRef<Material>(
                materialIndex, packMaterialIndex);
        auto model = Model<Vertex, Material>::getInvalid();
        if (modelRef.isValid()) {
            model.mesh.packItemIndex = meshIndex;
            model.mesh.packHandle =
                meshPacks.template getPackHandleRef<Vertex>().copy();
            model.material.packItemIndex = packMaterialIndex;
            model.material.packHandle = materialPackRef.copy();
        }
        return std::move(model);
    }

    template <typename Vertex, typename Material>
    auto tryGetAnimations(
        const Ref<Vertex, Material>& modelRef) const noexcept {
        auto modelAnimations = std::vector<AnimationHandle>();
        if (modelRef.isValid()) {
            for (const auto& animationIndex : modelRef.get().animationIndices) {
                modelAnimations.emplace_back(animations[animationIndex].copy());
            }
        }
        return std::move(modelAnimations);
    }

    std::vector<AnimationHandle> animations;
    MeshPackList<Vertices...> meshPacks;
    MaterialPackList<Materials...> materialPacks;
    IndexStorage documenIndexMap;
};

template <typename... Vertices, typename... Materials>
ResourceBundle(
    const AssetsBundle<TypeList<Vertices...>, TypeList<Materials...>>&)
    -> ResourceBundle<TypeList<Vertices...>, TypeList<Materials...>>;
