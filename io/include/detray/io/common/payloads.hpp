/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022-2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/definitions/geometry.hpp"
#include "detray/definitions/grid_axis.hpp"
#include "detray/io/common/detail/definitions.hpp"

// System include(s)
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// Raw indices (std::size_t) denote links between data components in different
/// files, while links used in detray detector objects are modelled as e.g.
/// @c single_link_payload
namespace detray {

/// @brief a payload for common information
struct common_header_payload {
    std::string version{}, detector{}, tag{}, date{};
};

/// @brief a payload for common and extra information
template <typename sub_header_payload_t = bool>
struct header_payload {
    common_header_payload common{};
    std::optional<sub_header_payload_t> sub_header;
};

/// @brief A payload for a single object link
struct single_link_payload {
    std::size_t link{std::numeric_limits<std::size_t>::max()};
};

/// @brief A payload for a typed object link
template <typename type_id_t>
struct typed_link_payload {
    using type_id = type_id_t;

    type_id type{type_id::unknown};
    std::size_t index{std::numeric_limits<std::size_t>::max()};
};

/// Geometry payloads
/// @{

/// @brief a payload for the geometry specific part of the file header
struct geo_sub_header_payload {
    std::size_t n_volumes{0ul}, n_surfaces{0ul};
};

/// @brief a payload for the geometry file header
using geo_header_payload = header_payload<geo_sub_header_payload>;

/// @brief A payload object to link a surface to its material
using material_link_payload = typed_link_payload<io::detail::material_type>;

/// @brief A payload object to link a volume to its acceleration data structures
using acc_links_payload = typed_link_payload<io::detail::acc_type>;

/// @brief A payload for an affine transformation in homogeneous coordinates
struct transform_payload {
    std::array<real_io, 3u> tr{};
    // Column major
    std::array<real_io, 9u> rot{};
};

/// @brief A payload object for surface masks
struct mask_payload {
    using mask_shape = io::detail::mask_shape;

    mask_shape shape{mask_shape::unknown};
    single_link_payload volume_link{};
    std::vector<real_io> boundaries{};
};

/// @brief  A payload for surfaces
struct surface_payload {
    std::optional<std::size_t> index_in_coll;
    transform_payload transform{};
    mask_payload mask{};
    std::optional<material_link_payload> material;
    single_link_payload source{};
    // Write the surface barcode as an additional information
    std::uint64_t barcode{std::numeric_limits<std::uint64_t>::max()};
    detray::surface_id type{detray::surface_id::e_sensitive};
};

/// @brief A payload for volumes
struct volume_payload {
    std::string name{};
    detray::volume_id type{detray::volume_id::e_cylinder};
    transform_payload transform{};
    std::vector<surface_payload> surfaces{};
    // Index of the volume in the detector volume container
    single_link_payload index{};
    // Optional accelerator data structures
    std::optional<std::vector<acc_links_payload>> acc_links{};
};

/// @}

/// Material payloads
/// @{

/// @brief a payload for the material specific part of the file header
struct homogeneous_material_sub_header_payload {
    std::size_t n_slabs{0ul}, n_rods{0ul};
};

/// @brief a payload for the homogeneous material file header
using homogeneous_material_header_payload =
    header_payload<homogeneous_material_sub_header_payload>;

/// @brief A payload object for a material parametrization
struct material_payload {
    std::array<real_io, 7u> params{};
};

/// @brief A payload object for a material slab/rod
struct material_slab_payload {
    using type = io::detail::material_type;

    material_link_payload mat_link{};
    real_io thickness{std::numeric_limits<real_io>::max()};
    material_payload mat{};
};

/// @brief A payload object for the material contained in a volume
struct material_volume_payload {
    single_link_payload volume_link{};
    std::vector<material_slab_payload> mat_slabs{};
    std::optional<std::vector<material_slab_payload>> mat_rods;
};

/// @brief A payload for the homogeneous material description of a detector
struct detector_homogeneous_material_payload {
    std::vector<material_volume_payload> volumes{};
};

/// @}

/// Payloads for a uniform grid
/// @{

/// @brief a payload for the grid specific part of the file header
struct grid_sub_header_payload {
    std::size_t n_grids{0u};
};

/// @brief a payload for the grid file header
using grid_header_payload = header_payload<grid_sub_header_payload>;

/// @brief axis definition and bin edges
struct axis_payload {
    /// axis lookup type
    n_axis::binning binning{n_axis::binning::e_regular};
    n_axis::bounds bounds{n_axis::bounds::e_closed};
    n_axis::label label{n_axis::label::e_r};

    std::size_t bins{0u};
    std::vector<real_io> edges{};
};

/// @brief A payload for a grid bin
template <typename content_t = std::size_t>
struct grid_bin_payload {
    std::vector<unsigned int> loc_index{};
    std::vector<content_t> content{};
};

/// @brief A payload for a grid definition
template <typename bin_content_t = std::size_t>
struct grid_payload {
    using grid_type = io::detail::acc_type;

    single_link_payload volume_link{};
    acc_links_payload acc_link{};

    std::vector<axis_payload> axes{};
    std::vector<grid_bin_payload<bin_content_t>> bins{};
    std::optional<transform_payload> transform;
};

/// @brief A payload for the grid collections of a detector
template <typename bin_content_t = std::size_t>
struct detector_grids_payload {
    std::vector<grid_payload<bin_content_t>> grids = {};
};

/// @}

/// @brief A payload for a detector geometry
struct detector_payload {
    std::vector<volume_payload> volumes = {};
    grid_payload<std::size_t> volume_grid;
};

}  // namespace detray
