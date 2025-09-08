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

#include "collections/map_vector.h"
#include "collections/pin_ref.h"
#include "graphics/resources/animation.h"
#include "graphics/resources/animation/joint.h"
#include "graphics/resources/material.h"
#include "graphics/resources/mesh.h"
#include "graphics/storage/animation.h"
#include "graphics/storage/material.h"
#include "graphics/storage/mesh.h"
#include "graphics/storage/texture.h"

template <typename Data>
constexpr fx::gltf::Accessor::Type getAccessorType() {
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
MeshDataHandle<Vertex> readMeshData(const fx::gltf::Document& document,
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
    return registerMeshData(MeshData(std::move(vertices), std::move(indices)));
}

std::string getTextureKey(const std::filesystem::path& documentPath,
                          const fx::gltf::Document& document,
                          const fx::gltf::Texture& texture) {
    const auto& image = document.images.at(texture.source);
    if (!image.uri.empty() && !image.IsEmbeddedResource()) {
        return (fx::gltf::detail::GetDocumentRootPath(documentPath) / image.uri)
            .string();
    };
    return (documentPath / (!image.name.empty()
                                ? image.name
                                : std::format("Image.{}", texture.source)))
        .string();
}

std::optional<TextureData> readTextureData(
    const std::filesystem::path& documentPath,
    const fx::gltf::Document& document, const fx::gltf::Texture& texture) {
    const auto& image = document.images.at(texture.source);
    if (!image.uri.empty()) {
        if (image.IsEmbeddedResource()) {
            std::vector<uint8_t> imageData{};
            image.MaterializeData(imageData);
            return TextureData::loadFromBuffer(
                imageData.data(), imageData.size(), TextureFormat::RGBA);
        } else {
            auto filePath =
                fx::gltf::detail::GetDocumentRootPath(documentPath) / image.uri;
            return TextureData::loadFromFile(filePath, TextureFormat::RGBA);
        }
    } else {
        const auto& bufferView = document.bufferViews[image.bufferView];
        const auto& buffer = document.buffers[bufferView.buffer];
        return TextureData::loadFromBuffer(&buffer.data[bufferView.byteOffset],
                                           bufferView.byteLength,
                                           TextureFormat::RGBA);
    }
}

TextureDataHandle getTextureDataHandle(
    const std::filesystem::path& documentPath,
    const fx::gltf::Document& document, const fx::gltf::Texture& texture) {
    auto textureKey = getTextureKey(documentPath, document, texture);
    auto existingDataHandle = tryGetOwnedTextureDataByKey(textureKey);
    if (!existingDataHandle.isInvalid()) {
        return existingDataHandle;
    }
    auto textureData = readTextureData(documentPath, document, texture);
    if (textureData.has_value()) {
        auto newTexture = registerTextureData(std::move(*textureData));
        newTexture.registerKey(std::move(textureKey));
        return newTexture;
    };
    return TextureDataHandle::getInvalid();
};

template <typename Material>
typename MaterialBuilderHandle<Material> readMaterialData(
    const std::filesystem::path& documentPath,
    const fx::gltf::Document& document, const fx::gltf::Material& material);

template <>
typename MaterialBuilderHandle<UnlitMaterial> readMaterialData<UnlitMaterial>(
    const std::filesystem::path& documentPath,
    const fx::gltf::Document& document, const fx::gltf::Material& material) {
    // baseColorTexture could be empty, add support for handling such missing
    // data cases
    auto albedoTexture = document.textures.at(
        material.pbrMetallicRoughness.baseColorTexture.index);
    auto albedoTextureData =
        getTextureDataHandle(documentPath, document, albedoTexture);

    MaterialBuilder<UnlitMaterial> builder{};
    builder.setAlbedoTextureData(std::move(albedoTextureData));
    return registerMaterialBuilder(std::move(builder));
}

// Instead we could omit the EmptyMaterial creation entirely leveraging
// constexpr conditional compilation For now leave this as it is to keep the
// same DocumentReader behavior for any material type
template <>
typename MaterialBuilderHandle<EmptyMaterial> readMaterialData<EmptyMaterial>(
    const std::filesystem::path& documentPath,
    const fx::gltf::Document& document, const fx::gltf::Material& material) {
    MaterialBuilder<EmptyMaterial> builder{};
    return registerMaterialBuilder(std::move(builder));
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
    SkinHandle skin;
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

inline AnimationHandle readAnimation(const SkinData& skinData,
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
struct ModelIndices {
    size_t meshIndex;
    size_t materialIndex;
    std::vector<size_t> animationIndices;
};

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

    using MeshDataHandle = MeshDataHandle<Vertex>;

    const PinRef<MeshDataHandle> getMesh(size_t meshIndex) const noexcept {
        return meshes.getAtIndex(meshIndex);
    }

    MeshDataHandle takeMesh(size_t meshIndex) noexcept {
        return meshes.takeAtIndex(meshIndex);
    }

    const PinRef<MeshDataHandle> tryGetMesh(
        const std::string& name) const noexcept {
        return meshes.tryGet(name);
    }

    std::optional<MeshDataHandle> tryTakeMesh(
        const std::string& name) noexcept {
        return meshes.tryTake(name);
    }

    const std::vector<MeshDataHandle>& getMeshes() const noexcept {
        return meshes.dataStorage;
    }

    std::vector<MeshDataHandle> takeMeshes() noexcept {
        return meshes.takeItems();
    }

    using MaterialBuilderHandle = MaterialBuilderHandle<Material>;

    const PinRef<MaterialBuilderHandle> getMaterial(
        size_t materialIndex) const noexcept {
        return materials.getAtIndex(materialIndex);
    }

    MaterialBuilderHandle takeMaterial(size_t materialIndex) noexcept {
        return materials.takeAtIndex(materialIndex);
    }

    const PinRef<MaterialBuilderHandle> tryGetMaterial(
        const std::string& name) const noexcept {
        return materials.tryGet(name);
    }

    std::optional<MaterialBuilderHandle> tryTakeMaterial(
        const std::string& name) noexcept {
        return materials.tryTake(name);
    }

    const std::vector<MaterialBuilderHandle>& getMaterials() const noexcept {
        return materials.dataStorage;
    }

    std::vector<MaterialBuilderHandle> takeMaterials() noexcept {
        return materials.takeItems();
    }

    const PinRef<AnimationHandle> getAnimation(
        size_t animationIndex) const noexcept {
        return animations.getAtIndex(animationIndex);
    }

    AnimationHandle takeAnimation(size_t animationIndex) noexcept {
        return animations.takeAtIndex(animationIndex);
    }

    const PinRef<AnimationHandle> tryGetAnimation(
        const std::string& name) const noexcept {
        return animations.tryGet(name);
    }

    std::optional<AnimationHandle> tryTakeAnimation(
        const std::string& name) noexcept {
        return animations.tryTake(name);
    }

    const std::vector<AnimationHandle>& getAnimations() const noexcept {
        return animations.dataStorage;
    }

    std::vector<AnimationHandle> takeAnimations() noexcept {
        return animations.takeItems();
    }

    using ModelIndices = ModelIndices<Vertex, Material>;

    const PinRef<ModelIndices> getIndices(size_t meshIndex) const noexcept {
        return modelIndices.getAtIndex(meshIndex);
    }

    ModelIndices takeIndices(size_t meshIndex) noexcept {
        return modelIndices.takeAtIndex(meshIndex);
    }

    const PinRef<ModelIndices> tryGetIndices(
        const std::string& name) const noexcept {
        return modelIndices.tryGet(name);
    }

    std::optional<ModelIndices> tryTakeIndices(
        const std::string& name) noexcept {
        return modelIndices.tryTake(name);
    }

    const NamedVector<ModelIndices>& getIndices() const noexcept {
        return modelIndices.dataStorage;
    }

    NamedVector<ModelIndices> takeIndices() noexcept {
        return std::move(modelIndices);
    }

    std::optional<std::string> tryGetModelName(
        size_t meshIndex) const noexcept {
        return modelIndices.tryGetKey(meshIndex);
    }

    std::optional<size_t> tryModelIndex(
        const std::string& meshName) const noexcept {
        return modelIndices.tryGetIndex(meshName);
    }

   private:
    void loadDocument(const std::filesystem::path& documentPath,
                      const fx::gltf::Document& document) noexcept {
        loadMeshes(document);
        loadMaterials(documentPath, document);
        if constexpr (isAnimatedVertex<Vertex>()) {
            loadAnimations(document);
        }
    }

    void loadMeshes(const fx::gltf::Document& document) noexcept {
        for (const auto& [documentMeshIndex, mesh] :
             std::views::enumerate(document.meshes)) {
            if (mesh.primitives.size() != 1) {
                std::println(
                    std::cout,
                    "DocumentReader::LoadMeshes: Skipping {} mesh processing "
                    "as it contists of multiple primitives. "
                    "Currently only single primitive meshes are supported",
                    mesh.name);
                continue;
            }
            const auto& primitive = mesh.primitives.front();
            if (primitive.material < 0) {
                std::println(
                    std::cout,
                    "DocumentReader::LoadMeshes: Skipping {} mesh processing "
                    "as it has undefined material property."
                    "Currently meshes using defailt material are not supported",
                    mesh.name);
                continue;
            }
            auto meshDataHandle = readMeshData<Vertex>(document, primitive);
            auto meshIndex = mesh.name.empty()
                                 ? meshes.emplace(std::move(meshDataHandle))
                                 : meshes.emplace(std::string(mesh.name),
                                                  std::move(meshDataHandle));
            // Currently we load every material in the document in the order
            // they are defined This makes the document material maping coherent
            // with the ordering in the created material storage
            // This could lead to unnecessary material loads, if the mesh was
            // skipped here
            auto materialIndex = static_cast<size_t>(primitive.material);
            auto indices = ModelIndices{
                .meshIndex = meshIndex,
                .materialIndex = materialIndex,
                .animationIndices = std::vector<size_t>(),
            };

            auto modelIndicesIndex =
                mesh.name.empty() ? modelIndices.emplace(std::move(indices))
                                  : modelIndices.emplace(std::string(mesh.name),
                                                         std::move(indices));
            documentMeshIndexMap.emplace(documentMeshIndex, modelIndicesIndex);
        }
    }

    void loadAnimations(const fx::gltf::Document& document) noexcept {
        auto skinDatas = std::vector<SkinData>{};
        skinDatas.reserve(document.skins.size());
        for (const auto& skin : document.skins) {
            skinDatas.emplace_back(readSkinData(document, skin));
        }
        auto skinMeshMap = getSkinMeshMap(document);
        for (const auto& animation : document.animations) {
            for (const auto& [skinIndex, skinData] :
                 std::views::enumerate(skinDatas)) {
                if (isSkinAnimation(skinData, animation)) {
                    if (animation.name.empty()) {
                    }
                    auto animationHandle =
                        readAnimation(skinData, document, animation);
                    auto animationIndex =
                        animation.name.empty()
                            ? animations.emplace(std::move(animationHandle))
                            : animations.emplace(std::string(animation.name),
                                                 std::move(animationHandle));
                    auto animationModel =
                        modelIndices.getAtIndex(skinMeshMap[skinIndex]);
                    if (animationModel.isValid()) {
                        animationModel.get().animationIndices.emplace_back(
                            animationIndex);
                    }
                }
            }
        }
    }

    void loadMaterials(const std::filesystem::path& documentPath,
                       const fx::gltf::Document& document) noexcept {
        for (const auto& material : document.materials) {
            auto materialBuilderHandle =
                readMaterialData<Material>(documentPath, document, material);
            material.name.empty()
                ? materials.emplace(std::move(materialBuilderHandle))
                : materials.emplace(std::string(material.name),
                                    std::move(materialBuilderHandle));
        }
    }

    auto getSkinMeshMap(const fx::gltf::Document& document) const noexcept {
        auto skinMeshMap = std::unordered_map<size_t, size_t>();
        for (const auto& node : document.nodes) {
            if (node.skin >= 0) {
                // By glTF2.0 if node has skin defined then it must also provide
                // valid mesh index
                auto meshIndicesIndex = documentMeshIndexMap.find(node.mesh);
                if (meshIndicesIndex != documentMeshIndexMap.end()) {
                    skinMeshMap.emplace(node.skin, meshIndicesIndex->second);
                }
            }
        }
        return skinMeshMap;
    }

    NamedVector<MeshDataHandle> meshes;
    NamedVector<MaterialBuilderHandle> materials;
    NamedVector<AnimationHandle> animations;
    NamedVector<ModelIndices> modelIndices;
    std::unordered_map<size_t, size_t> documentMeshIndexMap;
};
