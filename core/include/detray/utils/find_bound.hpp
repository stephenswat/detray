/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2024 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s).
#include "detray/definitions/detail/qualifiers.hpp"
#include "detray/utils/ranges.hpp"

// System include(s).
#include <iterator>

namespace detray {

/// Implementation of @c upper_bound
/// @see
/// https://github.com/gcc-mirror/gcc/blob/8f87b3c5ecd47f6ac0d7407ae5d436a12fb169dd/libstdc%2B%2B-v3/include/bits/stl_algo.h
template <typename forward_itr_t, typename T>
DETRAY_HOST_DEVICE inline constexpr forward_itr_t upper_bound(
    forward_itr_t first, forward_itr_t last, const T& value) {

    static_assert(detray::ranges::forward_iterator_v<forward_itr_t>);

    using difference_t =
        typename std::iterator_traits<forward_itr_t>::difference_type;

    difference_t len{detray::ranges::distance(first, last)};

    // binary search
    while (len > 0) {
        difference_t half{len >> 1};
        forward_itr_t middle{first};
        detray::ranges::advance(middle, half);
        if (value < *middle) {
            len = half;
        } else {
            first = middle;
            ++first;
            len -= half + 1;
        }
    }
    return first;
}

/// Implementation of @c lower_bound
/// @see
/// https://github.com/gcc-mirror/gcc/blob/8f87b3c5ecd47f6ac0d7407ae5d436a12fb169dd/libstdc%2B%2B-v3/include/bits/stl_algobase.h
template <typename forward_itr_t, typename T>
DETRAY_HOST_DEVICE inline constexpr forward_itr_t lower_bound(
    forward_itr_t first, forward_itr_t last, const T& value) {

    static_assert(detray::ranges::forward_iterator_v<forward_itr_t>);

    using difference_t =
        typename std::iterator_traits<forward_itr_t>::difference_type;

    difference_t len{detray::ranges::distance(first, last)};

    // binary search
    while (len > 0) {
        difference_t half{len >> 1};
        forward_itr_t middle{first};
        detray::ranges::advance(middle, half);
        if (*middle < value) {
            first = middle;
            ++first;
            len -= half + 1;
        } else {
            len = half;
        }
    }
    return first;
}

}  // namespace detray
