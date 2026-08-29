#pragma once

#include <ranges>
#include <cstddef>

namespace utils::ranges {
    template<typename Range>
    auto enumerate(Range&& range) {
        return std::views::zip(std::views::iota(std::size_t{0}), std::forward<Range>(range));
    }
}
