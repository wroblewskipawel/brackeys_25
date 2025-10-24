// Intention of the following implementation is to have generic type that could
// handle allocating and binding stream buffers for arbitrary collection of
// types required for given instanced draw call,
//
// e.g. draw of Animated mesh requires buffer with both instance model matrix
// and joint matrices for the given instance, following code should allow to
// push ranges of data types required for instanced drwa call of such mesh type,
// handle writing data to stream buffers and prepare strutures containting
// collection of BufferAllocations<T> that are valid to bound for single
// instanced draw call, each buffer is guaranteed to hold data required for n
// instances issues in the draw call
//
// It is required for the type list to be able to hold streams of same data type
// (e.g. glm::mat4 for model matrix and glm::mat4 for jointMatrices)
//
// Limitation:
//
// Current implementation of UniqueTypeList requires for the types to be
// unique to allow for correct retrieval of reference to the given stream
// buffer. Hence implementation based on trivial reuse of UniqueTypeList is not
// feasible.
// NewType pattern (type wrapper for given data type) could be used to give
// stream types, that share the same underlying data type, a unique type name
// that could be used with UniqueTypeList.
//
// As currently only AnimatedMesh types are using this kind of pattern for
// binding data for its draw calls (see. AnimatedDrawMap implementation),
// pursuing this implementation idea is not worthwhile for the moment.
//
// Following implementation in unused and kept here for the reference
//
// Example usage:
// {
//     auto streamPack = StreamPack(StreamBuffer<glm::mat4>(256),
//                                  StreamBuffer<glm::mat4>(512));
//
//     auto animationPlayers = std::vector<AnimationPlayer>{};
//     animationPlayers.emplace_back(animations[0]);
//     animationPlayers.emplace_back(animations[0]);
//     animationPlayers.emplace_back(animations[0]);
//
//     auto instanceMatrx = std::vector<glm::mat4>{
//         glm::mat4{1.0f}, glm::mat4{1.0f}, glm::mat4{1.0f}};
//
//     streamPack.beginGeneration();
//     streamPack.pushData(getDataWriter(instanceMatrx),
//     getDataWriter(animationPlayers)); streamPack.endGeneration();
// }

#pragma once

#include <glm/glm.hpp>
#include <type_traits>
#include <vector>

#include "collections/unique_list.h"
#include "concepts/range.h"
#include "graphics/resources/animation.h"
#include "graphics/resources/buffer/ring.h"
#include "graphics/resources/gl/buffer/stream.h"

template <typename... Types>
using StreamPackBuffers = UniqueTypeList<StreamBuffer<Types>...>;

template <typename... Types>
using Allocations = UniqueTypeList<std::vector<BufferAllocation<Types>>...>;

template <typename... Ranges>
    requires(BufferAllocationType<std::ranges::range_value_t<Ranges>> && ...)
using AllocationRanges = UniqueTypeList<Ranges...>;

template <typename... Types>
AllocationRanges<Types...> getAlocationRanges(
    Allocations<Types...>&& allocations) {
    return {allocations.get<std::vector<BufferAllocation<Types>>>()...};
}

template <typename... Types>
using AllocationPack = UniqueTypeList<BufferAllocation<Types>...>;

template <typename Type>
struct InstanceNumData {
    size_t numData;

    friend bool operator==(const InstanceNumData& lhs,
                           const InstanceNumData& rhs) noexcept {
        return lhs.numData == rhs.numData;
    }
};

template <typename Range, typename Type>
    requires RefConstRange<Range, Type>
class InstanceDataWriter {
   public:
    using StreamType = Type;

    InstanceDataWriter(Range&& range) : range(range) {}

    auto write(StreamBuffer<Type>& buffer) noexcept {
        return buffer.pushData(range);
    }

    auto getInstanceNumDatas() const noexcept {
        return InstanceNumData<StreamType>{.numData = 1};
    }

   private:
    Range range;
};

template <typename Range>
    requires RefConstRange<Range, AnimationPlayer>
class InstanceDataWriter<Range, AnimationPlayer> {
   public:
    using StreamType = glm::mat4;

    InstanceDataWriter(Range&& range) : range(range) {}

    auto write(StreamBuffer<StreamType>& buffer) noexcept {
        auto numSamplers = std::ranges::distance(range);
        auto allocations = std::vector<BufferAllocation<glm::mat4>>{};
        allocations.reserve(numSamplers);
        for (const AnimationPlayer& player : range) {
            allocations.emplace_back(
                buffer.pushDataContiguous(player.getJointTransforms()));
        }
        return allocations;
    }

    auto getInstanceNumDatas() const noexcept {
        return InstanceNumData<StreamType>{.numData =
                                               range.begin()->numJoints()};
    }

   private:
    Range range;
};

template <std::ranges::range Range>
InstanceDataWriter<Range,
                   std::remove_cvref_t<std::ranges::range_value_t<Range>>>
getDataWriter(Range&& range) noexcept {
    return {std::forward<Range>(range)};
}

template <typename... Types>
using PackInstaceNumDatas = UniqueTypeList<InstanceNumData<Types>...>;

template <typename Type>
struct IsRangeWriterT : std::false_type {};

template <typename Range, typename Type>
struct IsRangeWriterT<InstanceDataWriter<Range, Type>> : std::true_type {};

template <typename Type>
inline constexpr bool IsRangeWriterV =
    IsRangeWriterT<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept RangeWriter = IsRangeWriterV<Type>;

template <typename Writer, typename Type>
concept IsCompatible =
    RangeWriter<Writer> && std::is_same_v<typename Writer::StreamType, Type>;

template <RangeWriter... Writers>
auto getPackInstanceNumDatas(Writers&&... writers) noexcept {
    return PackInstaceNumDatas<typename Writers::StreamType...>{
        writers.getInstanceNumDatas()...};
}

template <typename... Types>
class StreamPack {
   public:
    StreamPack(StreamBuffer<Types>&&... buffers)
        : packBuffers(std::forward<StreamBuffer<Types>>(buffers)...) {}

    template <typename... Writers>
        requires(IsCompatible<Writers, Types> && ...)
    auto pushData(Writers&&... writers) noexcept {
        auto numDatas =
            getPackInstanceNumDatas(std::forward<Writers>(writers)...);
        auto allocations = getAllocations(std::forward<Writers>(writers)...);

        return std::vector<AllocationPack<Types...>>{};
    };

   private:
    template <typename... Writers>
        requires(IsCompatible<Writers, Types> && ...)
    auto getAllocations(Writers&&... writers) noexcept {
        return Allocations<Types...>(
            pushRange<Writers, Types>(std::forward<Writers>(writers))...);
    };

    template <typename... Ranges>
        requires(RefConstRange<Ranges, BufferAllocation<Types>> && ...)
    auto getNextAllocationNumInsatnces(
        const PackInstaceNumDatas<Types...>& numInstanceDatas,
        Ranges&&... allocations) noexcept {
        return std::min(
            {getAllocationNumInsatnces(numInstanceDatas, allocations)...});
    }

    template <typename Range, typename Type>
        requires RefConstRange<Range, BufferAllocation<Type>>
    auto getAllocationNumInsatnces(
        const PackInstaceNumDatas<Types...>& numInstanceDatas,
        const Range& allocations) noexcept {
        auto numInstaneData =
            numInstanceDatas.get<InstanceNumData<Type>>().numData;
        auto allocationDatas = allocations.begin()->numInstances;
        return allocationDatas / numInstaneData;
    }

    template <typename Type>
    auto& getStream() noexcept {
        return packBuffers.get<StreamBuffer<Type>>();
    }

    template <typename Writer, typename Type>
        requires IsCompatible<Writer, Type>
    auto pushRange(Writer&& writer) noexcept {
        return writer.write(getStream<Type>());
    }

    StreamPackBuffers<Types...> packBuffers;
};
