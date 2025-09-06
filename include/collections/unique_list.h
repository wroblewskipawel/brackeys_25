#pragma once

#include <type_traits>
#include <utility>

template <typename... Types>
struct TypeList {};

template <typename... Types>
struct UniqueTypeList;

template <typename... Types>
struct UniqueTypeListBuilder;

template <template <typename> typename, typename>
struct Wrap;

template <template <typename> typename WrapType, typename... TypeList>
struct Wrap<WrapType, UniqueTypeListBuilder<TypeList...>> {
    using Type = UniqueTypeListBuilder<WrapType<TypeList>...>;
};

template <template <typename, typename> typename, typename, typename>
struct Zip;

template <template <typename, typename> typename ZipType, typename... FirstList,
          typename... SecondList>
struct Zip<ZipType, UniqueTypeListBuilder<FirstList...>,
           UniqueTypeListBuilder<SecondList...>> {
    using Type = UniqueTypeListBuilder<ZipType<FirstList, SecondList>...>;
};

template <typename, typename>
struct Concatenate;

template <typename... FirstList, typename... SecondList>
struct Concatenate<UniqueTypeListBuilder<FirstList...>,
                   UniqueTypeListBuilder<SecondList...>> {
    using Type = UniqueTypeListBuilder<FirstList..., SecondList...>;
};

template <template <typename, typename> typename, typename, typename>
struct Append;

template <template <typename, typename> typename ComposeType,
          typename AppendType, typename... TypeList>
struct Append<ComposeType, AppendType, UniqueTypeListBuilder<TypeList...>> {
    using Type = UniqueTypeListBuilder<ComposeType<TypeList, AppendType>...>;
};

template <template <typename, typename> typename, typename, typename>
struct Product;

template <template <typename, typename> typename ProducType,
          typename... SecondList>
struct Product<ProducType, UniqueTypeListBuilder<>,
               UniqueTypeListBuilder<SecondList...>> {
   public:
    using Type = UniqueTypeListBuilder<>;
};

template <template <typename, typename> class ProductType, typename First,
          typename... FirstList, typename... SecondList>
struct Product<ProductType, UniqueTypeListBuilder<First, FirstList...>,
               UniqueTypeListBuilder<SecondList...>> {
   private:
    using Head = typename Append<ProductType, First,
                                 UniqueTypeListBuilder<SecondList...>>::Type;
    using Tail =
        typename Product<ProductType, UniqueTypeListBuilder<FirstList...>,
                         UniqueTypeListBuilder<SecondList...>>::Type;

   public:
    using Type = typename Concatenate<Head, Tail>::Type;
};

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
    UniqueTypeList() = default;

    UniqueTypeList(Type&& type, Types&&... types)
        : value(std::forward<Type>(type)),
          next(std::forward<Types>(types)...) {}

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

    template <typename Search>
    constexpr const Search& get() const noexcept {
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
    // template <typename...>
    // friend class UniqueTypeListBuilder;
    // template <typename...>
    // friend class UniqueTypeList;

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

    template <template <typename> class WrapType>
    constexpr auto wrap() {
        return typename Wrap<WrapType,
                             UniqueTypeListBuilder<Type, Types...>>::Type{};
    }

    template <template <typename, typename> class ZipType, typename Other,
              typename... Others>
    constexpr auto zip(UniqueTypeListBuilder<Other, Others...>) {
        return typename Zip<ZipType, UniqueTypeListBuilder<Type, Types...>,
                            UniqueTypeListBuilder<Other, Others...>>::Type{};
    }

    template <template <typename, typename> class ComposeType,
              typename AppendType>
    constexpr auto compose() {
        return typename Append<ComposeType, AppendType,
                               UniqueTypeListBuilder<Type, Types...>>::Type{};
    }

    template <template <typename, typename> class ProductType,
              typename... Appends>
    constexpr auto product(UniqueTypeListBuilder<Appends...>) {
        return typename Product<ProductType, UniqueTypeListBuilder<Appends...>,
                                UniqueTypeListBuilder<Type, Types...>>::Type{};
    };
};
