#pragma once

#include <type_traits>
#include <utility>

template <typename... Types>
struct UniqueTypeList;

template <typename... Types>
struct UniqueTypeListBuilder;

template <typename Search, typename... Types>
struct ContainsType;

template <typename Search>
struct ContainsType<Search> {
    static const bool value = false;
};

template <typename Search, typename Type, typename... Types>
struct ContainsType<Search, Type, Types...> {
    static const bool value =
        std::is_same_v<Search, Type> || ContainsType<Search, Types...>::value;
};

template <>
struct UniqueTypeList<> {};

template <typename Type, typename... Types>
class UniqueTypeList<Type, Types...> {
   public:
    template <typename Search>
    constexpr Search& get() noexcept {
        if constexpr (std::is_same_v<Search, Type>) {
            return value;
        } else {
            if constexpr (sizeof...(Types) == 0) {
                static_assert(false, "Search not found in UniqueTypeList::get");
            } else {
                return next.get<Search>();
            }
        }
    }

   private:
    template <typename...>
    friend class UniqueTypeListBuilder;
    template <typename...>
    friend class UniqueTypeList;

    UniqueTypeList() = default;

    UniqueTypeList(Type&& type, Types&&... types)
        : value(std::forward<Type>(type)),
          next(std::forward<Types>(types)...) {}

    Type value;
    UniqueTypeList<Types...> next;
};

template <>
class UniqueTypeListBuilder<> {
   public:
    template <typename Next>
    constexpr UniqueTypeListBuilder<Next> withType() {
        return UniqueTypeListBuilder<Next>{};
    };
};

template <typename Type, typename... Types>
class UniqueTypeListBuilder<Type, Types...> {
   public:
    using UniqueTypeList = UniqueTypeList<Type, Types...>;
    using NextBuilder = UniqueTypeListBuilder<Types...>;

    template <typename Next>
    constexpr UniqueTypeListBuilder<Next, Type, Types...> withType() {
        static_assert(!ContainsType<Next, Type, Types...>::value,
                      "UniqueTypeListBuilder withType received duplicate type");
        return UniqueTypeListBuilder<Next, Type, Types...>{};
    };

    template <typename... Args>
    constexpr static UniqueTypeList build(Args&&... args) {
        return UniqueTypeList(std::forward<Args>(args)...);
    }
};
