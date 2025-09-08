#pragma once

#include <filesystem>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <unordered_map>

#include "collections/map_vector.h"
#include "collections/pin_ref.h"
#include "collections/unique_list.h"
#include "collections/unique_list/vector_list.h"
#include "graphics/assets/gltf.h"
#include "graphics/assets/model.h"
#include "graphics/resources/material.h"
#include "graphics/storage/animation.h"
#include "graphics/storage/material.h"
#include "graphics/storage/mesh.h"

template <typename, typename>
struct AssetsIndicesStorage;

template <typename... Vertices, typename... Materials>
struct AssetsIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>> {
    template <typename Vertex, typename Material>
    using Ref = PinRef<ModelIndices<Vertex, Material>>;

    template <typename Vertex, typename Material>
    using Indices = NamedVector<ModelIndices<Vertex, Material>>;

    template <typename Vertex, typename Material>
    using IndicesMap =
        std::unordered_map<std::string, Indices<Vertex, Material>>;

    // For the Product it would be more intuitive to keep the same ordering as
    // in the resulting lsit product types, here to get the IndicesMap<Vertex,
    // Material> we have to first declare the Materials list and then Vertex
    // list
    using MapStorageBuilder =
        typename Product<IndicesMap, UniqueTypeListBuilder<Materials...>,
                         UniqueTypeListBuilder<Vertices...>>::Type;
    using MapStorage = typename MapStorageBuilder::UniqueTypeList;

    template <typename Vertex, typename Material>
    auto& getNamespaceIndices(const std::string& indicesNamespace) noexcept {
        auto& indicesMap =
            indicesMapStorage.get<IndicesMap<Vertex, Material>>();
        if (!indicesMap.contains(indicesNamespace)) {
            indicesMap.emplace(std::string(indicesNamespace),
                               Indices<Vertex, Material>{});
        }
        return indicesMap.at(indicesNamespace);
    }

    template <typename Vertex, typename Material>
    const auto& getNamespaceIndices(
        const std::string& indicesNamespace) const noexcept {
        const auto& indicesMap =
            indicesMapStorage.get<IndicesMap<Vertex, Material>>();
        return indicesMap.at(indicesNamespace);
    }

    template <typename Vertex, typename Material>
    const Ref<Vertex, Material> getModelIndices(
        const std::string& modelNamespace,
        const std::string& modelName) const noexcept {
        auto& namespaceIndices =
            getNamespaceIndices<Vertex, Material>(modelNamespace);
        return namespaceIndices.tryGet(modelName);
    }

    template <typename Vertex, typename Material>
    const Ref<Vertex, Material> getModelIndices(
        const std::string& modelNamespace, size_t modelIndex) const noexcept {
        auto& namespaceIndices =
            getNamespaceIndices<Vertex, Material>(modelNamespace);
        return namespaceIndices.getAtIndex(modelIndex);
    }

    MapStorage indicesMapStorage;
};

struct IndicesOffsets {
    size_t meshOffset;
    size_t materialOffset;
    size_t animationOffset;
};

template <typename, typename>
class AssetsBundle;

template <typename... Vertices, typename... Materials>
class AssetsBundle<TypeList<Vertices...>, TypeList<Materials...>> {
   public:
    using IndexStorage =
        AssetsIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>>;

    template <typename Vertex, typename Material>
    using Indices = typename IndexStorage::template Indices<Vertex, Material>;

    AssetsBundle() = default;

    template <typename Vertex, typename Material>
    auto& pushDocument(const std::string& documentNamespace,
                       const std::filesystem::path& filePath) {
        auto document = DocumentReader<Vertex, Material>(filePath);
        auto indicesOffsets = IndicesOffsets{
            .meshOffset = appendMeshes(document.takeMeshes()),
            .materialOffset = appendMaterials(document.takeMaterials()),
            .animationOffset = appendAnimations(document.takeAnimations()),
        };
        registerDocumentIndices(documentNamespace, document.takeIndices(),
                                indicesOffsets);
        return *this;
    };

    template <typename Vertex, typename Material>
    auto& pushModel(const std::string& modelNamepace,
                    const ModelData<Vertex, Material>& modelData) {
        auto meshIndex = tryPushMesh(modelData.getMesh());
        auto materialIndex = static_cast<size_t>(handle::invalidValue);
        if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
            materialIndex = tryPushMaterial(modelData.getMaterial());
        }
        auto animationIndices = std::vector<size_t>();
        if constexpr (isAnimatedVertex<Vertex>()) {
            animationIndices = tryPushAnimations(modelData.getAnimations());
        }
        auto modelIndices = ModelIndices<Vertex, Material>{
            .meshIndex = meshIndex,
            .materialIndex = materialIndex,
            .animationIndices = std::move(animationIndices),
        };

        auto& namespaceIndices =
            documenIndexMap.getNamespaceIndices<Vertex, Material>(
                modelNamepace);
        const auto& modelName = modelData.getName();
        if (modelName.empty()) {
            namespaceIndices.emplace(std::move(modelIndices));
        } else {
            namespaceIndices.emplace(std::string(modelName),
                                     std::move(modelIndices));
        }
        return *this;
    };

    const auto& getAnimations() const noexcept { return animationStorage; }

    const auto& getMeshes() const noexcept { return meshStorage; }

    const auto& getMaterials() const noexcept { return materialStorage; }

    const auto& getIndicesMap() const noexcept { return documenIndexMap; }

   private:
    auto appendAnimations(std::vector<AnimationHandle>&& animations) noexcept {
        auto firstIndex = static_cast<uint32_t>(animationStorage.size());
        animationStorage.insert(animationStorage.end(),
                                std::make_move_iterator(animations.begin()),
                                std::make_move_iterator(animations.end()));
        return firstIndex;
    }

    template <typename Material>
    auto appendMaterials(
        std::vector<MaterialBuilderHandle<Material>>&& materials) noexcept {
        auto firstIndex =
            materialStorage.size<MaterialBuilderHandle<Material>>();
        for (auto&& material : std::move(materials)) {
            materialStorage.insert<MaterialBuilderHandle<Material>>(
                std::move(material));
        }
        return firstIndex;
    }

    template <typename Vertex>
    auto appendMeshes(std::vector<MeshDataHandle<Vertex>>&& meshes) noexcept {
        auto firstIndex = meshStorage.size<MeshDataHandle<Vertex>>();
        for (auto&& mesh : std::move(meshes)) {
            meshStorage.insert<MeshDataHandle<Vertex>>(std::move(mesh));
        }
        return firstIndex;
    }

    template <typename Vertex, typename Material>
    auto registerDocumentIndices(const std::string& indicesNamespace,
                                 Indices<Vertex, Material>&& indices,
                                 IndicesOffsets offsets) noexcept {
        for (auto& meshIndices : indices.getItems()) {
            meshIndices.meshIndex += offsets.meshOffset;
            meshIndices.materialIndex += offsets.materialOffset;
            for (auto& animationIndex : meshIndices.animationIndices) {
                animationIndex += offsets.animationOffset;
            }
        }

        auto& namespaceIndices =
            documenIndexMap.getNamespaceIndices<Vertex, Material>(
                indicesNamespace);
        namespaceIndices.extend(std::move(indices));
    }

    template <typename Vertex>
    auto tryPushMesh(const MeshDataHandle<Vertex>& mesh) noexcept {
        using MeshHandle = MeshDataHandle<Vertex>;
        auto meshIndex = meshStorage.find<MeshHandle>(mesh);
        if (meshIndex == meshStorage.size<MeshHandle>()) {
            meshStorage.insert<MeshHandle>(mesh.copy());
        }
        return meshIndex;
    }

    template <typename Material>
    auto tryPushMaterial(
        const MaterialBuilderHandle<Material>& material) noexcept {
        using MaterialHandle = MaterialBuilderHandle<Material>;
        auto materialIndex = materialStorage.find<MaterialHandle>(material);
        if (materialIndex == materialStorage.size<MaterialHandle>()) {
            materialStorage.insert<MaterialHandle>(material.copy());
        }
        return materialIndex;
    }

    auto tryPushAnimations(
        const std::vector<AnimationHandle>& animations) noexcept {
        auto indices = std::vector<size_t>(animations.size());
        for (const auto& [index, animation] :
             std::views::enumerate(animations)) {
            auto animationIndex = findAnimation(animation);
            if (animationIndex == animationStorage.size()) {
                animationStorage.emplace_back(animation.copy());
            }
            indices[index] = animationIndex;
        }
        return indices;
    }

    auto findAnimation(const AnimationHandle& animation) const noexcept {
        auto result = std::find(animationStorage.begin(),
                                animationStorage.end(), animation);
        return std::distance(animationStorage.begin(), result);
    }

    std::vector<AnimationHandle> animationStorage;
    VectorList<MeshDataHandle<Vertices>...> meshStorage;
    VectorList<MaterialBuilderHandle<Materials>...> materialStorage;
    IndexStorage documenIndexMap;
};
