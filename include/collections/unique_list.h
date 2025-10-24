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

template <typename>
struct Unwrap;

template <typename... TypeList>
struct Unwrap<UniqueTypeListBuilder<TypeList...>> {
    using Type = UniqueTypeListBuilder<typename TypeList::Type...>;
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

template <template <typename> typename, typename>
struct Filter;

template <template <typename> typename FilterType>
struct Filter<FilterType, UniqueTypeListBuilder<>> {
    using Type = UniqueTypeListBuilder<>;
};

template <template <typename> typename FilterType, typename Head,
          typename... Tail>
struct Filter<FilterType, UniqueTypeListBuilder<Head, Tail...>> {
   private:
    using FilteredTail =
        Filter<FilterType, UniqueTypeListBuilder<Tail...>>::Type;

   public:
    using Type = std::conditional_t<
        FilterType<Head>::value,
        typename Concatenate<UniqueTypeListBuilder<Head>, FilteredTail>::Type,
        FilteredTail>;
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
struct UniqueTypeList<> {
    UniqueTypeList() = default;

    template <typename... Args>
    UniqueTypeList(Args&&... args) {}
};

template <typename Type, typename... Types>
class UniqueTypeList<Type, Types...> {
   public:
    UniqueTypeList() = default;

    UniqueTypeList(const UniqueTypeList&) = default;
    UniqueTypeList& operator=(const UniqueTypeList&) = default;

    UniqueTypeList(UniqueTypeList&&) = default;
    UniqueTypeList& operator=(UniqueTypeList&&) = default;

    template <typename... Args>
    UniqueTypeList(Args&&... args)
        : value(std::forward<Args>(args)...),
          next(std::forward<Args>(args)...) {}

    UniqueTypeList(Type&& type, Types&&... types)
        : value(std::forward<Type>(type)),
          next(std::forward<Types>(types)...) {}

    UniqueTypeList(Type&& type, UniqueTypeList<Types...>&& list)
        : value(std::forward<Type>(type)), next(std::move(list)) {}

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

    friend bool operator==(const UniqueTypeList& lhs,
                           const UniqueTypeList& rhs) noexcept
        requires std::equality_comparable<Type>
    {
        return lhs.value == rhs.value && lhs.next == rhs.next;
    }

   private:
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
    using TypeList = TypeList<Type, Types...>;

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

    constexpr auto unwrap() {
        return typename Unwrap<UniqueTypeListBuilder<Type, Types...>>::Type{};
    }

    template <template <typename> class FilterType>
    constexpr auto filter() {
        return typename Filter<FilterType,
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
