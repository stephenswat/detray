/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/geometry/surface.hpp"
#include "detray/intersection/intersection.hpp"
#include "detray/plugins/svgtools/conversion/point.hpp"
#include "detray/plugins/svgtools/meta/proto/landmark.hpp"

namespace detray::svgtools::conversion {

/// @returns The proto landmark of a detray point.
template <typename point3_t, typename d_point3_t>
inline auto landmark(d_point3_t& position) {
    using p_landmark_t = svgtools::meta::proto::landmark<point3_t>;
    p_landmark_t p_lm;
    p_lm._position = svgtools::conversion::point<point3_t>(position);

    return p_lm;
}

}  // namespace detray::svgtools::conversion
