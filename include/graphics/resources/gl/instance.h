#pragma once

#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "graphics/resources/gl/vertex_array.h"

template <typename Instance>
struct InstanceOffsets {
    uint32_t bufferIndex;
    uint32_t bufferOffset;
    uint32_t numInstances;
    uint32_t bufferGeneration;
};

template <typename Instance, size_t InstanceCount>
class InstanceBuffer {
   public:
    using InstanceOffsets = InstanceOffsets<Instance>;

    InstanceBuffer(size_t initialBufferCount) {
        // Require positive count to initialize buffers at construction time,to
        // ensure that writePosition == 0 is valid index to underlying storage
        // buffers;
        if (initialBufferCount == 0) {
            std::println(
                std::cerr,
                "InstanceBuffer initialBufferCount must be greater that 0");
            std::abort();
        }
        allocateBuffers(initialBufferCount);
    };

    InstanceBuffer(const InstanceBuffer&) = delete;
    InstanceBuffer& operator=(const InstanceBuffer&) = delete;

    InstanceBuffer(InstanceBuffer&&) = default;
    InstanceBuffer& operator=(InstanceBuffer&&) = default;

    ~InstanceBuffer() {
        glDeleteBuffers(instanceBuffers.size(), instanceBuffers.data());
    }

    std::vector<InstanceOffsets> pushInstances(
        const std::vector<Instance>& instances) {
        size_t numToCopy = instances.size();
        auto rangeBegin = instances.begin();

        auto requiredWrites = reserveData(numToCopy);
        auto instanceOffsets = std::vector<InstanceOffsets>(requiredWrites);

        for (auto writeIndex : std::views::iota(requiredWrites)) {
            moveWritePositionIfRequired();
            auto availableSlots = getCurrentBufferAvailableSlots();
            auto chunkSize = std::min(numToCopy, availableSlots);
            instanceOffsets[writeIndex] = pushDataUnchecked(
                std::ranges::subrange(rangeBegin, rangeBegin + chunkSize));
            rangeBegin += chunkSize;
            numToCopy -= chunkSize;
        }
        return std::move(instanceOffsets);
    }

    void flush() noexcept {
        for (auto& [buffer, data, count] :
             std::views::zip(instanceBuffers, instanceData, instanceCount)) {
            glNamedBufferSubData(buffer, 0, getCopySize(count), data.data());
        }
    }

    void reset() noexcept {
        std::fill(instanceCount.begin(), instanceCount.end(), 0);
        bufferGeneration += 1;
        writePosition = 0;
    }

    template <typename Vertex>
    void bindInstaceBuffer(const InstanceOffsets& offsets) const noexcept {
        if (offsets.bufferGeneration != bufferGeneration) {
            std::println(std::cerr,
                         "InstanceBuffer::bindInstanceBuffer received offsets "
                         "from previous frame");
            std::abort();
        }
        VertexArray<Vertex, Instance>::getVertexArray()
            .bindBuffer<BindingIndex::InstanceAttributes>(
                instanceBuffers[offsets.bufferIndex]);
    }

   private:
    static constexpr size_t getStorageSize() noexcept {
        return sizeof(Instance) * InstanceCount;
    }

    static constexpr size_t getCopySize(size_t numInstances) noexcept {
        return sizeof(Instance) * numInstances;
    }

    template <typename Range>
        requires std::is_convertible_v<std::ranges::range_value_t<Range>,
                                       Instance>
    InstanceOffsets pushDataUnchecked(Range&& instances) {
        auto numInstances = std::ranges::distance(instances);
        auto& currentData = getCurrentData();
        auto& currentCount = getCurrentCount();
        auto bufferOffset = currentCount;
        auto rangeBegin = currentData.begin() + bufferOffset;
        auto rangeEnd = rangeBegin + numInstances;
        auto writeRange = std::ranges::subrange(rangeBegin, rangeEnd);
        writeRange = std::ranges::copy(instances);
        currentCount += numInstances;
        return InstanceOffsets{
            .bufferIndex = writePosition,
            .bufferOffset = bufferOffset,
            .numInstances = numInstances,
            .bufferGeneration = bufferGeneration,
        };
    }

    uint32_t reserveData(size_t numAdditionalInstances) noexcept {
        auto requiredWrites = 1;
        auto currentAvailable = getCurrentBufferAvailableSlots();
        if (currentAvailable < numAdditionalInstances) {
            auto nextBufferInstances =
                numAdditionalInstances - currentAvailable;
            auto nextBufferWrites =
                std::ceil_div(nextBufferInstances, InstanceCount);
            auto nextBufferAvailable = getNextBuffersAvailableSlots();
            if (nextBufferAvailable < nextBufferInstances) {
                auto numBuffersRequired = std::ceil_div(
                    nextBufferInstances - nextBufferAvailable, InstanceCount);
                allocateBuffers(numBuffersRequired);
            }
            requiredWrites += nextBufferWrites;
        }
        return requiredWrites;
    }

    void allocateBuffers(size_t numBuffers) noexcept {
        auto insertIndex = instanceBuffers.size();
        instanceBuffers.resize(insertIndex + numBuffers);
        instanceData.resize(insertIndex + numBuffers);
        instanceCount.resize(insertIndex + numBuffers, 0);
        glCreateBuffers(numBuffers, &instanceBuffers[insertIndex]);
        for (auto bufferIndex : std::views::iota(numBuffers)) {
            glNamedBufferStorage(instanceBuffers[insertIndex + bufferIndex],
                                 getStorageSize(), nullptr,
                                 GL_DYNAMIC_STORAGE_BIT);
        }
    }

    auto getNextBuffersAvailableSlots() noexcept const {
        auto numFreeBuffers = instanceCount.size() - writePosition - 1;
        return InstanceCount * numFreeBuffers;
    }

    auto getCurrentBufferAvailableSlots() noexcept const {
        return InstanceCount - getCurrentCount();
    }

    auto& getCurrentBuffer() noexcept { return instanceBuffers[writePosition]; }
    auto& getCurrentData() noexcept { return instanceData[writePosition]; }
    auto& getCurrentCount() noexcept { return instanceCount[writePosition]; }

    void moveWritePositionIfRequired() noexcept {
        auto nextbufferAvailable = writePosition < (instanceCount.size() - 1);
        if (nextbufferAvailable && getCurrentBufferAvailableSlots() == 0) {
            writePosition += 1;
        }
    }

    std::vector<GLuint> instanceBuffers;
    std::vector<std::array<Instance, InstanceCount>> instanceData;
    std::vector<size_t> instanceCount;
    size_t writePosition{0};
    uint32_t bufferGeneration{0};
};
