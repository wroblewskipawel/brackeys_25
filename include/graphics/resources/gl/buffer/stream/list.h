#pragma once

#include "collections/unique_list.h"
#include "graphics/resources/gl/buffer/stream.h"
#include "graphics/storage/gl/stream.h"

template <typename... Types>
using ConfigList = UniqueTypeList<StreamBufferConfig<Types>...>;

template <typename... Types>
struct ConfigListHelper {
    template <typename Type>
    auto& getConfig() const noexcept {
        return configList.template get<StreamBufferConfig<Type>>();
    }

    const ConfigList<Types...>& configList;
};

template <typename... Types>
using BufferHandleList = UniqueTypeList<StreamHandle<Types>...>;

template <typename... Types>
class StreamListBuilder;

template <typename... Types>
class StreamList {
   public:
    void beginGeneration() noexcept { (beginStreamGeneration<Types>(), ...); }

    void endGeneration() noexcept { (endStreamGeneration<Types>(), ...); }

    template <typename Type>
    auto& getStreamHandle() const noexcept {
        return buffers.template get<StreamHandle<Type>>();
    }

    template <typename Type>
    auto& getStreamHandle() noexcept {
        return buffers.template get<StreamHandle<Type>>();
    }

   private:
    friend class StreamListBuilder<Types...>;

    StreamList(const ConfigListHelper<Types...>& configList) noexcept
        : buffers{registerStreamBuffer(
              configList.template getConfig<Types>().build())...} {};

    template <typename Type>
    void beginStreamGeneration() noexcept {
        getStreamHandle<Type>().get().get().beginGeneration();
    }

    template <typename Type>
    void endStreamGeneration() noexcept {
        getStreamHandle<Type>().get().get().endGeneration();
    }

    BufferHandleList<Types...> buffers;
};

template <>
class StreamListBuilder<> {
   public:
    template <typename Type>
    auto append(StreamBufferConfig<Type>&& config) {
        return StreamListBuilder<Type>(
            ConfigList<Type>{std::forward<StreamBufferConfig<Type>>(config)});
    };
};

template <typename... Types>
class StreamListBuilder {
   public:
    template <typename Type>
    auto append(StreamBufferConfig<Type>&& config) {
        return StreamListBuilder<Type, Types...>(ConfigList<Type, Types...>{
            std::forward<StreamBufferConfig<Type>>(config),
            std::move(configList)});
    }

   private:
    template <typename...>
    friend class StreamListBuilder;
    template <typename, typename, typename, typename>
    friend class RendererBuilder;

    auto build() const noexcept {
        return StreamList(ConfigListHelper(configList));
    }

    StreamListBuilder(ConfigList<Types...>&& configList) noexcept
        : configList(std::move(configList)) {}

    ConfigList<Types...> configList;
};
