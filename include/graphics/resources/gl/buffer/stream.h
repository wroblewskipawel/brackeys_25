#pragma once

#include <glad/glad.h>

#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "collections/pin.h"
#include "concepts/range.h"
#include "graphics/resources/buffer/ring.h"
#include "graphics/resources/gl/buffer/binding.h"

template <typename Type>
class StreamBuffer {
   public:
    StreamBuffer(size_t pageSize, size_t reserveNumPages = 0,
                 size_t initialRingSize = 3) noexcept
        : hostBuffer(pageSize, reserveNumPages, initialRingSize),
          currentBufferIndex(0, 0),
          isWriteFinished(true) {
        appendBuffers(initialRingSize + reserveNumPages);
    };

    StreamBuffer(const StreamBuffer&) = delete;
    StreamBuffer& operator=(const StreamBuffer&) = delete;

    StreamBuffer(StreamBuffer&&) noexcept = default;
    StreamBuffer& operator=(StreamBuffer&&) noexcept = default;

    ~StreamBuffer() noexcept {
        glDeleteBuffers(deviceBuffers.size(), deviceBuffers.data());
    }

    // TODO: Come up with more elegant way of buffer state transition
    // Consider using type-styem to handle state transitions
    void endGeneration() noexcept {
        if (isWriteFinished) {
            std::println(
                std::cerr,
                "StreamBuffer::endGeneration: Attempted to endGeneration "
                "before beginGeneration is called");
            std::abort();
        }

        flushCurrentBuffer();
        isWriteFinished = true;
    }

    void beginGeneration() noexcept {
        if (!isWriteFinished) {
            std::println(
                std::cerr,
                "StreamBuffer::beginGeneration: Attempted to beginGeneration "
                "before endGeneration is called");
            std::abort();
        }

        currentBufferIndex = hostBuffer.nextGeneration();
        isWriteFinished = false;
    }

    auto pushData(const Type& item) noexcept {
        return pushData(std::views::single(item));
    }

    template <typename Range>
        requires RefConstRange<Range, Type>
    auto pushDataContiguous(Range&& data) noexcept {
        if (isWriteFinished) {
            std::println(std::cerr,
                         "StreamBuffer::pushData: Attempted to pushData "
                         "before beginGeneration is called");
            std::abort();
        }

        auto allocation =
            hostBuffer.pushDataContiguous(std::forward<Range>(data));
        registerHostAllocation(allocation);
        return allocation;
    }

    template <typename Range>
        requires RefConstRange<Range, Type>
    auto pushData(Range&& data) noexcept {
        if (isWriteFinished) {
            std::println(std::cerr,
                         "StreamBuffer::pushData: Attempted to pushData "
                         "before beginGeneration is called");
            std::abort();
        }

        auto allocations = hostBuffer.pushData(std::forward<Range>(data));
        for (const auto& allocation : allocations) {
            registerHostAllocation(allocation);
        }
        return allocations;
    }

    template <auto Binding>
    void bindBuffer(const BufferAllocation<Type>& allocation,
                    GLuint bindingIndex) const noexcept {
        static_assert(std::is_same_v<decltype(Binding), BufferBindings>);

        checkAccessAllowed(allocation);
        BindingState::bindBuffer<Binding>(
            deviceBuffers[allocation.physicalBufferIndex], bindingIndex);
    };

    auto getBuffer(const BufferAllocation<Type>& allocation) const noexcept {
        checkAccessAllowed(allocation);
        return PinVal<GLuint>(deviceBuffers[allocation.physicalBufferIndex]);
    }

   private:
    void checkAccessAllowed(
        const BufferAllocation<Type>& allocation) const noexcept {
        if (!isWriteFinished) {
            std::println(std::cerr,
                         "StreamBuffer::bindBuffer: Attempted to bindBuffer "
                         "before endGeneration is called");
            std::abort();
        }
        if (allocation.generation != currentBufferIndex.generation) {
            std::println(std::cerr,
                         "StreamBuffer::bindBuffer: Invalid BuffferAllocation "
                         "generation");
            std::abort();
        }
    }

    void registerHostAllocation(
        const BufferAllocation<Type>& allocation) noexcept {
        if (allocation.physicalBufferIndex != currentBufferIndex.bufferIndex) {
            flushCurrentBuffer();
            currentBufferIndex = allocation.getBufferIndices();
            if (currentBufferIndex.bufferIndex >= deviceBuffers.size()) {
                auto lastBufferIndex = deviceBuffers.size() - 1;
                auto requiredBuffers =
                    currentBufferIndex.bufferIndex - lastBufferIndex;
                appendBuffers(requiredBuffers);
            }
        }
    }

    void flushCurrentBuffer() noexcept {
        const auto hostData = hostBuffer.getBufferRange(currentBufferIndex);
        auto numInstances = std::ranges::distance(hostData);
        glNamedBufferSubData(deviceBuffers[currentBufferIndex.bufferIndex], 0,
                             numInstances * sizeof(Type), &(*hostData.begin()));
    }

    auto getBufferByteSize() const noexcept {
        return sizeof(Type) * hostBuffer.pageSize();
    }

    void appendBuffers(size_t appendCount) noexcept {
        auto currentCount = deviceBuffers.size();
        deviceBuffers.resize(currentCount + appendCount);
        auto appendedBuffers = std::ranges::subrange(
            deviceBuffers.begin() + currentCount, deviceBuffers.end());
        glCreateBuffers(appendCount, &(*appendedBuffers.begin()));
        for (const auto& buffer : appendedBuffers) {
            glNamedBufferStorage(buffer, getBufferByteSize(), nullptr,
                                 GL_DYNAMIC_STORAGE_BIT);
        }
    }

    DynamicRing<Type> hostBuffer;
    std::vector<GLuint> deviceBuffers;
    GenerationIndices currentBufferIndex;
    bool isWriteFinished;
};

template <typename Type>
struct IsStreamT : std::false_type {};

template <typename Type>
struct IsStreamT<StreamBuffer<Type>> : std::true_type {};

template <typename Type>
inline constexpr bool IsStreamV = IsStreamT<std::remove_cvref_t<Type>>::value;

template <typename Type>
concept StreamBufferType = IsStreamV<Type>;
