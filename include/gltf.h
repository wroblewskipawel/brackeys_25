#pragma once

#include <fx/gltf.h>
#include <glad/glad.h>

#include <concepts>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <optional>
#include <queue>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "material.h"
#include "mesh.h"
#include "skin.h"

template <typename Data>
fx::gltf::Accessor::Type getAccessorType() {
    if constexpr (std::is_same_v<Data, float> ||
                  std::is_same_v<Data, int32_t> ||
                  std::is_same_v<Data, uint32_t> ||
                  std::is_same_v<Data, int8_t> ||
                  std::is_same_v<Data, uint8_t> ||
                  std::is_same_v<Data, int16_t> ||
                  std::is_same_v<Data, uint16_t>) {
        return fx::gltf::Accessor::Type::Scalar;
    } else if constexpr (std::is_same_v<Data, glm::vec2>) {
        return fx::gltf::Accessor::Type::Vec2;
    } else if constexpr (std::is_same_v<Data, glm::vec3>) {
        return fx::gltf::Accessor::Type::Vec3;
    } else if constexpr (std::is_same_v<Data, glm::vec4>) {
        return fx::gltf::Accessor::Type::Vec4;
    } else if constexpr (std::is_same_v<Data, glm::uvec2>) {
        return fx::gltf::Accessor::Type::Vec2;
    } else if constexpr (std::is_same_v<Data, glm::uvec3>) {
        return fx::gltf::Accessor::Type::Vec3;
    } else if constexpr (std::is_same_v<Data, glm::uvec4>) {
        return fx::gltf::Accessor::Type::Vec4;
    } else if constexpr (std::is_same_v<Data, glm::quat>) {
        return fx::gltf::Accessor::Type::Vec4;
    } else if constexpr (std::is_same_v<Data, glm::mat2>) {
        return fx::gltf::Accessor::Type::Mat2;
    } else if constexpr (std::is_same_v<Data, glm::mat3>) {
        return fx::gltf::Accessor::Type::Mat3;
    } else if constexpr (std::is_same_v<Data, glm::mat4>) {
        return fx::gltf::Accessor::Type::Mat4;
    } else {
        static_assert(!std::is_same_v<Data, Data>, "Unsupported data type");
    }
}

template <typename Target, typename Source = Target>
std::pair<std::function<bool(Target&)>, size_t> getDataReader(
    const fx::gltf::Document& document, const fx::gltf::Accessor& accessor) {
    static_assert(std::is_trivially_copyable_v<Target> &&
                  std::is_trivially_copyable_v<Source> &&
                  std::constructible_from<Target, Source>);
    if (accessor.type != getAccessorType<Target>()) {
        return {nullptr, 0};
    }

    const auto& bufferView = document.bufferViews.at(accessor.bufferView);
    const auto& buffer = document.buffers.at(bufferView.buffer);
    const size_t count = accessor.count;

    size_t offset = bufferView.byteOffset + accessor.byteOffset;
    size_t stride =
        bufferView.byteStride ? bufferView.byteStride : sizeof(Source);
    size_t element = 0;

    auto reader = [=](Target& item) mutable {
        if (element >= count) {
            return false;
        }
        if constexpr (std::is_same_v<Target, Source>) {
            std::memcpy(&item, &buffer.data[offset + stride * element++],
                        sizeof(Target));
        } else {
            Source temp;
            std::memcpy(&temp, &buffer.data[offset + stride * element++],
                        sizeof(Source));
            item = temp;
        }
        return true;
    };
    return {reader, count};
}

template <typename Attribute>
std::pair<std::function<bool(Attribute&)>, size_t> getAttributeReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attributeName);

template <>
inline std::pair<std::function<bool(glm::vec2&)>, size_t> getAttributeReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attributeName) {
    const auto& accessor =
        document.accessors[primitive.attributes.at(attributeName)];
    auto reader = [&]()
        -> std::optional<std::pair<std::function<bool(glm::vec2&)>, size_t>> {
        switch (accessor.componentType) {
            case fx::gltf::Accessor::ComponentType::Float:
                return getDataReader<glm::vec2>(document, accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedByte:
                return getDataReader<glm::vec2, glm::u8vec2>(document,
                                                             accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedShort:
                return getDataReader<glm::vec2, glm::u16vec2>(document,
                                                              accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedInt:
                return getDataReader<glm::vec2, glm::u32vec2>(document,
                                                              accessor);
            default:
                return std::nullopt;
        }
    }();
    if (!reader.has_value()) {
        std::println(std::cerr,
                     "getAttributeReader:  Failed to create attribute reader");
        std::abort();
    }
    return *reader;
}

// Consider how to handle getAttributeReader for vec types
// without need for explicit specializations for float/integer vec types
template <>
inline std::pair<std::function<bool(glm::uvec4&)>, size_t> getAttributeReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attributeName) {
    const auto& accessor =
        document.accessors[primitive.attributes.at(attributeName)];
    auto reader = [&]()
        -> std::optional<std::pair<std::function<bool(glm::uvec4&)>, size_t>> {
        switch (accessor.componentType) {
            case fx::gltf::Accessor::ComponentType::Float:
                return getDataReader<glm::uvec4, glm::vec4>(document, accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedByte:
                return getDataReader<glm::uvec4, glm::u8vec4>(document,
                                                              accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedShort:
                return getDataReader<glm::uvec4, glm::u16vec4>(document,
                                                               accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedInt:
                return getDataReader<glm::uvec4>(document, accessor);
            default:
                return std::nullopt;
        }
    }();
    if (!reader.has_value()) {
        std::println(std::cerr,
                     "getAttributeReader:  Failed to create attribute reader");
        std::abort();
    }
    return *reader;
}

template <>
inline std::pair<std::function<bool(glm::vec3&)>, size_t> getAttributeReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attributeName) {
    const auto& accessor =
        document.accessors[primitive.attributes.at(attributeName)];
    auto reader = [&]()
        -> std::optional<std::pair<std::function<bool(glm::vec3&)>, size_t>> {
        switch (accessor.componentType) {
            case fx::gltf::Accessor::ComponentType::Float:
                return getDataReader<glm::vec3>(document, accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedByte:
                return getDataReader<glm::vec3, glm::u8vec3>(document,
                                                             accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedShort:
                return getDataReader<glm::vec3, glm::u16vec3>(document,
                                                              accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedInt:
                return getDataReader<glm::vec3, glm::u32vec3>(document,
                                                              accessor);
            default:
                return std::nullopt;
        }
    }();
    if (!reader.has_value()) {
        std::println(std::cerr,
                     "getAttributeReader:  Failed to create attribute reader");
        std::abort();
    }
    return *reader;
}

template <>
inline std::pair<std::function<bool(glm::vec4&)>, size_t> getAttributeReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive,
    const std::string& attributeName) {
    const auto& accessor =
        document.accessors[primitive.attributes.at(attributeName)];
    auto reader = [&]()
        -> std::optional<std::pair<std::function<bool(glm::vec4&)>, size_t>> {
        switch (accessor.componentType) {
            case fx::gltf::Accessor::ComponentType::Float:
                return getDataReader<glm::vec4>(document, accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedByte:
                return getDataReader<glm::vec4, glm::u8vec4>(document,
                                                             accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedShort:
                return getDataReader<glm::vec4, glm::u16vec4>(document,
                                                              accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedInt:
                return getDataReader<glm::vec4, glm::u32vec4>(document,
                                                              accessor);
            default:
                return std::nullopt;
        }
    }();
    if (!reader.has_value()) {
        std::println(std::cerr,
                     "getAttributeReader:  Failed to create attribute reader");
        std::abort();
    }
    return *reader;
}

inline std::pair<std::function<bool(GLuint&)>, size_t> getIndexReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive) {
    const auto& accessor = document.accessors[primitive.indices];
    auto reader = [&]()
        -> std::optional<std::pair<std::function<bool(GLuint&)>, size_t>> {
        switch (accessor.componentType) {
            case fx::gltf::Accessor::ComponentType::UnsignedByte:
                return getDataReader<GLuint, GLubyte>(document, accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedShort:
                return getDataReader<GLuint, GLushort>(document, accessor);
            case fx::gltf::Accessor::ComponentType::UnsignedInt:
                return getDataReader<GLuint>(document, accessor);
            default:
                return std::nullopt;
        }
    }();
    if (!reader.has_value()) {
        std::println(std::cerr,
                     "getIndexReader:  Failed to create index reader");
        std::abort();
    }
    return *reader;
}

template <typename Vertex>
std::pair<std::function<bool(Vertex&)>, size_t> getVertexReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive);

template <>
inline std::pair<std::function<bool(UnlitVertex&)>, size_t> getVertexReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive) {
    auto [positionReader, positionCount] =
        getAttributeReader<glm::vec3>(document, primitive, "POSITION");
    auto [texCoordReader, texCoordCount] =
        getAttributeReader<glm::vec2>(document, primitive, "TEXCOORD_0");
    auto reader = [positionReader,
                   texCoordReader](UnlitVertex& vertex) mutable {
        return positionReader(vertex.position) &&
               texCoordReader(vertex.texCoord);
    };
    return {reader, std::min(positionCount, texCoordCount)};
}

template <>
inline std::pair<std::function<bool(UnlitAnimatedVertex&)>, size_t>
getVertexReader(const fx::gltf::Document& document,
                const fx::gltf::Primitive& primitive) {
    auto [positionReader, positionCount] =
        getAttributeReader<glm::vec3>(document, primitive, "POSITION");
    auto [texCoordReader, texCoordCount] =
        getAttributeReader<glm::vec2>(document, primitive, "TEXCOORD_0");
    auto [jointsReader, jointsCount] =
        getAttributeReader<glm::uvec4>(document, primitive, "JOINTS_0");
    auto [weightsReader, weightsCount] =
        getAttributeReader<glm::vec4>(document, primitive, "WEIGHTS_0");
    auto reader = [positionReader, texCoordReader, jointsReader,
                   weightsReader](UnlitAnimatedVertex& vertex) mutable {
        return positionReader(vertex.position) &&
               texCoordReader(vertex.texCoord) && jointsReader(vertex.joints) &&
               weightsReader(vertex.weights);
    };
    return {reader, std::min({positionCount, texCoordCount, jointsCount,
                              weightsCount})};
}

template <>
inline std::pair<std::function<bool(ColoredVertex&)>, size_t> getVertexReader(
    const fx::gltf::Document& document, const fx::gltf::Primitive& primitive) {
    auto [positionReader, positionCount] =
        getAttributeReader<glm::vec3>(document, primitive, "POSITION");
    // color attribute could also be a vec4,
    // truncating to vec3 is fine as long as the stride
    // for buffer reader equals to vec4 size, currently it is not handled
    // properly
    auto [colorReader, colorCount] =
        getAttributeReader<glm::vec3>(document, primitive, "COLOR_0");
    auto reader = [positionReader, colorReader](ColoredVertex& vertex) mutable {
        return positionReader(vertex.position) && colorReader(vertex.color);
    };
    return {reader, std::min(positionCount, colorCount)};
}

template <typename Vertex>
MeshData<Vertex> readMeshData(const fx::gltf::Document& document,
                              const fx::gltf::Primitive& primitive) {
    auto [vertexReader, vertexCount] =
        getVertexReader<Vertex>(document, primitive);
    auto [indexReader, indexCount] = getIndexReader(document, primitive);

    std::vector<Vertex> vertices(vertexCount);
    for (size_t vertex = 0; vertex < vertexCount; vertex++) {
        if (!vertexReader(vertices[vertex])) {
            std::println(std::cerr, "readMeshData: Failed to read vertex data");
            std::abort();
        }
    }

    std::vector<GLuint> indices(indexCount);
    for (size_t index = 0; index < indexCount; index++) {
        if (!indexReader(indices[index])) {
            std::println(std::cerr, "readMeshData: Failed to read index data");
            std::abort();
        }
    }
    return MeshData<Vertex>{std::move(vertices), std::move(indices)};
}

std::function<std::optional<TextureData>(void)> getTextureDataReader(
    const std::filesystem::path& documentRootPath,
    const fx::gltf::Document& document, const fx::gltf::Texture& texture) {
    const auto& image = document.images.at(texture.source);
    return [&]() -> std::optional<TextureData> {
        if (!image.uri.empty()) {
            if (image.IsEmbeddedResource()) {
                std::vector<uint8_t> imageData{};
                image.MaterializeData(imageData);
                return TextureData::loadFromBuffer(
                    imageData.data(), imageData.size(), TextureFormat::RGB);
            } else {
                auto filePath =
                    fx::gltf::detail::GetDocumentRootPath(documentRootPath) /
                    image.uri;
                return TextureData::loadFromFile(filePath, TextureFormat::RGB);
            }
        } else {
            const auto& bufferView = document.bufferViews[image.bufferView];
            const auto& buffer = document.buffers[bufferView.buffer];
            return TextureData::loadFromBuffer(
                &buffer.data[bufferView.byteOffset], bufferView.byteLength,
                TextureFormat::RGB);
        }
    };
}

template <typename Material>
typename Material::BuilderType readMaterialData(
    const std::filesystem::path& documentRootPath,
    const fx::gltf::Document& document, const fx::gltf::Material& material);

template <>
typename UnlitMaterial::BuilderType readMaterialData<UnlitMaterial>(
    const std::filesystem::path& documentRootPath,
    const fx::gltf::Document& document, const fx::gltf::Material& material) {
    // baseColorTexture could be empty, add support for handling such missing
    // data cases
    auto albedoTexture = document.textures.at(
        material.pbrMetallicRoughness.baseColorTexture.index);
    auto albedoTextureReader =
        getTextureDataReader(documentRootPath, document, albedoTexture);

    UnlitMaterial::BuilderType builder{};
    builder.setAlbedoTextureData(albedoTextureReader());
    return builder;
}

// Instead we could omit the EmptyMaterial creation entirely leveraging
// constexpr conditional compilation For now leave this as it is to keep the
// same DocumentReader behavior for any material type
template <>
typename EmptyMaterial::BuilderType readMaterialData<EmptyMaterial>(
    const std::filesystem::path& documentRootPath,
    const fx::gltf::Document& document, const fx::gltf::Material& material) {
    EmptyMaterial::BuilderType builder{};
    return builder;
}

inline std::vector<glm::mat4> readInverseBindMatrices(
    const fx::gltf::Document& document, const fx::gltf::Skin& skin) {
    auto inverseBindAccessor = document.accessors[skin.inverseBindMatrices];
    auto [inverseBindReader, numMatrices] =
        getDataReader<glm::mat4>(document, inverseBindAccessor);
    std::vector<glm::mat4> inverseBindMatrices(numMatrices);
    for (auto& bindMatrix : inverseBindMatrices) {
        inverseBindReader(bindMatrix);
    }
    return inverseBindMatrices;
}

struct SkinData {
    Skin skin;
    std::unordered_map<int32_t, BuilderJoint> nodeJointMap;
};

inline SkinData readSkinData(const fx::gltf::Document& document,
                             const fx::gltf::Skin& skin) {
    auto jointDataMap =
        std::unordered_map<int32_t, std::pair<uint32_t, glm::mat4>>();
    auto invserseBind = readInverseBindMatrices(document, skin);
    for (const auto& [i, jointData] :
         std::views::zip(skin.joints, invserseBind) | std::views::enumerate) {
        const auto& [jointIndex, bindMatrix] = jointData;
        jointDataMap[jointIndex] = {static_cast<uint32_t>(i), bindMatrix};
    }

    SkinBuilder skinBuilder{};

    std::unordered_map<int32_t, BuilderJoint> nodeJointMap;
    std::queue<std::pair<int32_t, BuilderJoint>> toVisit;
    toVisit.push({skin.skeleton, skinBuilder.getRoot()});
    while (!toVisit.empty()) {
        auto [nodeId, parentJoint] = toVisit.front();
        toVisit.pop();
        const auto& [jointLocation, bindMatrix] = jointDataMap[nodeId];
        auto skinJoint =
            skinBuilder.addJoint(bindMatrix, parentJoint, jointLocation);
        nodeJointMap.insert({nodeId, skinJoint});
        const auto& node = document.nodes[nodeId];
        for (auto child : node.children) {
            toVisit.push({child, skinJoint});
        }
    }

    return SkinData{
        .skin = skinBuilder.build(),
        .nodeJointMap = nodeJointMap,
    };
}

template <typename Item>
struct SamplerReader {
    std::pair<std::function<bool(float&)>, size_t> inputReader;
    std::pair<std::function<bool(Item&)>, size_t> outputReader;

    Keyframes<Item> readData() {
        auto& [timeReader, numTimeKeys] = inputReader;
        auto& [valueReader, numValueKeys] = outputReader;

        if (numTimeKeys != numValueKeys) {
            std::println(std::cerr,
                         "SamplerReader: numTimeKeys, numValueKeys mismatch");
            std::abort();
        }

        std::vector<float> timeKeys(numTimeKeys);
        for (auto& time : timeKeys) {
            timeReader(time);
        }

        std::vector<Item> valueKeys(numValueKeys);
        for (auto& value : valueKeys) {
            valueReader(value);
        }

        return Keyframes<Item>{std::move(valueKeys), std::move(timeKeys)};
    }
};

template <typename Item>
SamplerReader<Item> getSamplerReader(
    const fx::gltf::Document& document,
    const fx::gltf::Animation::Sampler& sampler) {
    const auto& inputAccessor = document.accessors[sampler.input];
    const auto& outputAccessor = document.accessors[sampler.output];

    auto inputReader = getDataReader<float>(document, inputAccessor);
    auto outputReader = getDataReader<Item>(document, outputAccessor);

    return SamplerReader<Item>{inputReader, outputReader};
}

struct NodeChannels {
    std::optional<size_t> translationSampler{std::nullopt};
    std::optional<size_t> scaleSampler{std::nullopt};
    std::optional<size_t> rotationSampler{std::nullopt};
};

inline AnimatedJoint readJointAnimation(const NodeChannels& channels,
                                        const fx::gltf::Document& document,
                                        const fx::gltf::Animation& animation) {
    std::optional<SamplerReader<glm::vec3>> translationReader{std::nullopt};
    std::optional<SamplerReader<glm::vec3>> scaleReader{std::nullopt};
    std::optional<SamplerReader<glm::quat>> rotationReader{std::nullopt};

    if (channels.translationSampler.has_value()) {
        const auto& sampler = animation.samplers[*channels.translationSampler];
        translationReader = getSamplerReader<glm::vec3>(document, sampler);
    }

    if (channels.scaleSampler.has_value()) {
        const auto& sampler = animation.samplers[*channels.scaleSampler];
        scaleReader = getSamplerReader<glm::vec3>(document, sampler);
    }

    if (channels.rotationSampler.has_value()) {
        const auto& sampler = animation.samplers[*channels.rotationSampler];
        rotationReader = getSamplerReader<glm::quat>(document, sampler);
    }

    auto translationKeys = translationReader.has_value()
                               ? (*translationReader).readData()
                               : constValueKeyframe(glm::vec3(0.0));
    auto scaleKeys = scaleReader.has_value()
                         ? (*scaleReader).readData()
                         : constValueKeyframe(glm::vec3(1.0));
    auto rotationKeys = rotationReader.has_value()
                            ? (*rotationReader).readData()
                            : constValueKeyframe(glm::quat(1.0, 0.0, 0.0, 0.0));

    return AnimatedJoint(Channels{.translationKeys = translationKeys,
                                  .scaleKeys = scaleKeys,
                                  .rotationKeys = rotationKeys});
}

inline bool isSkinAnimation(const SkinData& skinData,
                            const fx::gltf::Animation& animation) {
    for (const auto& channel : animation.channels) {
        const auto& target = channel.target;
        if (!target.empty() && !skinData.nodeJointMap.contains(target.node)) {
            return false;
        }
    }
    return true;
}

inline Animation readAnimation(const SkinData& skinData,
                               const fx::gltf::Document& document,
                               const fx::gltf::Animation& animation) {
    std::unordered_map<int32_t, NodeChannels> nodeChannelsMap;

    for (const auto& channel : animation.channels) {
        const auto& target = channel.target;
        if (!target.empty()) {
            auto& nodeChannels = nodeChannelsMap[target.node];
            if (target.path == "translation") {
                nodeChannels.translationSampler = channel.sampler;
                continue;
            }
            if (target.path == "scale") {
                nodeChannels.scaleSampler = channel.sampler;
                continue;
            }
            if (target.path == "rotation") {
                nodeChannels.rotationSampler = channel.sampler;
                continue;
            }
        }
    }

    auto animationBuilder = AnimationBuilder(skinData.skin);

    for (const auto& [nodeIndex, nodeChannels] : nodeChannelsMap) {
        auto jointIndex = skinData.nodeJointMap.at(nodeIndex);
        animationBuilder.animateJoint(
            jointIndex, readJointAnimation(nodeChannels, document, animation));
    }

    return animationBuilder.build();
}
template <typename Vertex, typename Material>
class DocumentReader {
   public:
    DocumentReader(const std::filesystem::path& filePath) {
        auto document = [&]() -> std::optional<fx::gltf::Document> {
            auto extension = filePath.extension();
            if (extension == ".gltf") {
                return fx::gltf::LoadFromText(filePath);
            }
            if (extension == ".glb") {
                return fx::gltf::LoadFromBinary(filePath);
            }
            std::println(std::cerr, "DocumentReader: Unsupported file type");
            return std::nullopt;
        }();
        if (document.has_value()) {
            // Consider lazy loading on mesh access requests
            loadDocument(filePath, *document);
        }
    }

    DocumentReader(const DocumentReader&) = delete;
    DocumentReader(DocumentReader&&) = delete;

    DocumentReader& operator=(const DocumentReader&) = delete;
    DocumentReader& operator=(DocumentReader&&) = delete;

    const std::vector<MeshData<Vertex>>& getMeshes() const noexcept {
        return meshes;
    }

    std::vector<MeshData<Vertex>> takeMeshes() noexcept {
        return std::move(meshes);
    }

    using MaterialBuilder = typename Material::BuilderType;

    const std::vector<MaterialBuilder>& getMaterials() const noexcept {
        return materials;
    }

    std::vector<MaterialBuilder> takeMaterials() noexcept {
        return std::move(materials);
    }

    const std::vector<Animation>& getAnimations() const noexcept {
        return animations;
    }

    std::vector<Animation> takeAnimations() noexcept {
        return std::move(animations);
    }

   private:
    void loadDocument(const std::filesystem::path& documentRootPath,
                      const fx::gltf::Document& document) noexcept {
        loadMeshes(document);
        loadMaterials(documentRootPath, document);
        if constexpr (isAnimatedVertex<Vertex>()) {
            loadAnimations(document);
        }
    }

    void loadMeshes(const fx::gltf::Document& document) noexcept {
        for (const auto& mesh : document.meshes) {
            for (const auto& primitive : mesh.primitives) {
                meshes.emplace_back(readMeshData<Vertex>(document, primitive));
            }
        }
    }

    void loadAnimations(const fx::gltf::Document& document) noexcept {
        auto skinDatas = std::vector<SkinData>{};
        skinDatas.reserve(document.skins.size());
        for (const auto& skin : document.skins) {
            skinDatas.emplace_back(readSkinData(document, skin));
        }
        for (const auto& animation : document.animations) {
            for (const auto& skinData : skinDatas) {
                if (isSkinAnimation(skinData, animation)) {
                    animations.emplace_back(
                        readAnimation(skinData, document, animation));
                }
            }
        }
    }

    void loadMaterials(const std::filesystem::path& documentRootPath,
                       const fx::gltf::Document& document) noexcept {
        for (const auto& material : document.materials) {
            materials.emplace_back(readMaterialData<Material>(
                documentRootPath, document, material));
        }
    }

    std::vector<MeshData<Vertex>> meshes;
    std::vector<MaterialBuilder> materials;
    std::vector<Animation> animations;
};
