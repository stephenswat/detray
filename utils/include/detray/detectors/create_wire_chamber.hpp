/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/core/detector.hpp"
#include "detray/core/detector_metadata.hpp"
#include "detray/definitions/units.hpp"
#include "detray/detectors/detector_helper.hpp"
#include "detray/masks/masks.hpp"
#include "detray/materials/predefined_materials.hpp"
#include "detray/tools/bounding_volume.hpp"
#include "detray/tools/grid_builder.hpp"
#include "detray/utils/axis_rotation.hpp"
#include "detray/utils/unit_vectors.hpp"

// Vecmem include(s)
#include <vecmem/memory/memory_resource.hpp>

// Covfie include(s)
#include <covfie/core/backend/primitive/constant.hpp>
#include <covfie/core/field.hpp>
#include <covfie/core/vector.hpp>

namespace detray {

namespace {

using transform3_t = __plugin::transform3<detray::scalar>;
using point3 = __plugin::point3<detray::scalar>;
using vector3 = __plugin::vector3<detray::scalar>;
using point2 = __plugin::point2<detray::scalar>;

}  // namespace

struct wire_chamber_config {

    /// Number of layers
    unsigned int m_n_layers{10u};

    /// Half z of cylinder chamber
    scalar m_half_z{1000.f * unit<scalar>::mm};

    /// Field vector for an homogenoues b-field
    vector3 m_bfield_vec{0.f, 0.f, 2.f * unit<scalar>::T};

    constexpr wire_chamber_config &n_layers(const unsigned int n) {
        m_n_layers = n;
        return *this;
    }

    constexpr wire_chamber_config &half_z(const scalar hz) {
        m_half_z = hz;
        return *this;
    }

    wire_chamber_config &bfield_vec(const vector3 &field_vec) {
        m_bfield_vec = field_vec;
        return *this;
    }

    wire_chamber_config &bfield_vec(const scalar x, const scalar y,
                                    const scalar z) {
        m_bfield_vec = {x, y, z};
        return *this;
    }

    constexpr unsigned int n_layers() const { return m_n_layers; }
    constexpr scalar half_z() const { return m_half_z; }
    constexpr const vector3 &bfield_vec() const { return m_bfield_vec; }

};  // wire chamber config

template <typename container_t = host_container_types>
auto create_wire_chamber(vecmem::memory_resource &resource,
                         const wire_chamber_config &cfg) {

    // Detector type
    using detector_t = detector<default_metadata, covfie::field, container_t>;

    using nav_link_t = typename detector_t::surface_type::navigation_link;
    using mask_id = typename detector_t::surface_type::mask_id;
    using material_id = typename detector_t::surface_type::material_id;
    using mask_link_type = typename detector_t::surface_type::mask_link;
    using material_link_type = typename detector_t::surface_type::material_link;
    constexpr auto wire_id = mask_id::e_cell_wire;
    constexpr auto rod_id = material_id::e_rod;
    constexpr auto leaving_world{detail::invalid_value<nav_link_t>()};

    // Detector configurations
    const scalar cyl_half_z{cfg.half_z()};
    constexpr scalar inner_cyl_rad{500.f * unit<scalar>::mm};
    constexpr scalar cell_size = 10.f * unit<scalar>::mm;
    constexpr scalar stereo_angle = 50.f * unit<scalar>::mrad;
    const material<scalar> wire_mat = tungsten<scalar>();
    constexpr scalar wire_rad = 15.f * unit<scalar>::um;

    // B field
    using const_bfield_bknd_t =
        covfie::backend::constant<covfie::vector::vector_d<scalar, 3>,
                                  covfie::vector::vector_d<scalar, 3>>;
    const auto &B = cfg.bfield_vec();
    auto bfield = covfie::field<const_bfield_bknd_t>(
        const_bfield_bknd_t::configuration_t{B[0], B[1], B[2]});

    // Create detector
    detector_t det(resource, std::move(bfield));

    // Detector and volume names
    typename detector_t::name_map name_map = {{0u, "wire_chamber"}};

    // geometry context object
    typename detector_t::geometry_context ctx0{};

    // Beam collision volume
    detail::detector_helper<transform3_t>().create_cyl_volume(
        det, resource, ctx0, 0.f, inner_cyl_rad, -cyl_half_z, cyl_half_z,
        {leaving_world, 1u, leaving_world, leaving_world});

    name_map[1u] = "beam_vol_0";

    // Layer volumes
    const unsigned int n_layers{cfg.n_layers()};
    for (unsigned int i_lay = 0; i_lay < n_layers; i_lay++) {

        // Create a volume
        const scalar inner_layer_rad =
            inner_cyl_rad + static_cast<scalar>(i_lay) * cell_size * 2.f;
        const scalar outer_layer_rad =
            inner_cyl_rad + static_cast<scalar>(i_lay + 1) * cell_size * 2.f;

        if (i_lay < n_layers - 1) {
            detail::detector_helper<transform3_t>().create_cyl_volume(
                det, resource, ctx0, inner_layer_rad, outer_layer_rad,
                -cyl_half_z, cyl_half_z,
                {i_lay, i_lay + 2, leaving_world, leaving_world});
        } else {
            detail::detector_helper<transform3_t>().create_cyl_volume(
                det, resource, ctx0, inner_layer_rad, outer_layer_rad,
                -cyl_half_z, cyl_half_z,
                {i_lay, leaving_world, leaving_world, leaving_world});
        }

        // Current vol
        auto &vol = det.volumes().back();

        // Layer configuration
        const scalar center_layer_rad = inner_layer_rad + cell_size;
        const scalar theta = 2 * cell_size / center_layer_rad;
        const unsigned int n_wires_per_layer =
            static_cast<unsigned int>(2 * constant<scalar>::pi / theta);

        // Get volume ID
        auto volume_idx = vol.index();
        name_map[volume_idx + 1u] = "layer_vol_" + std::to_string(volume_idx);

        auto mask_volume_link{static_cast<nav_link_t>(volume_idx)};

        // Containers per volume
        typename detector_t::surface_container_t surfaces(&resource);
        typename detector_t::mask_container masks(resource);
        typename detector_t::material_container materials(resource);
        typename detector_t::transform_container transforms(resource);

        // Wire center positions
        detray::dvector<point3> m_centers{};
        for (unsigned int i_w = 0u; i_w < n_wires_per_layer; i_w++) {
            const scalar x =
                center_layer_rad * std::cos(theta * static_cast<scalar>(i_w));
            const scalar y =
                center_layer_rad * std::sin(theta * static_cast<scalar>(i_w));
            const scalar z = 0.f;

            m_centers.push_back({x, y, z});
        }

        for (auto &m_center : m_centers) {

            // Surfaces with the linking into the local containers
            mask_link_type mask_link = {wire_id,
                                        masks.template size<wire_id>()};
            material_link_type material_link{rod_id,
                                             materials.template size<rod_id>()};
            const auto trf_index = transforms.size(ctx0);
            surfaces.emplace_back(trf_index, mask_link, material_link,
                                  volume_idx, dindex_invalid,
                                  surface_id::e_sensitive);

            // The wire bounds
            masks.template emplace_back<wire_id>(
                empty_context{}, mask_volume_link, cell_size, cyl_half_z);
            materials.template emplace_back<rod_id>(empty_context{}, wire_mat,
                                                    wire_rad);

            // Build the transform
            vector3 z_axis{0.f, 0.f, 1.f};
            vector3 r_axis = vector::normalize(m_center);
            const scalar sign = (i_lay % 2 == 0) ? 1 : -1;
            z_axis = axis_rotation<transform3_t>(r_axis,
                                                 sign * stereo_angle)(z_axis);
            vector3 x_axis =
                unit_vectors<vector3>().make_curvilinear_unit_u(z_axis);
            transforms.emplace_back(ctx0, m_center, z_axis, x_axis);
        }

        // Iterate the surfaces and update their links
        const auto trf_offset{det.transform_store().size(ctx0)};
        auto sf_offset{det.n_surfaces()};

        for (auto &sf_desc : surfaces) {
            // Make sure the volume was constructed correctly
            assert(sf_desc.volume() < det.volumes().size());

            // Update the surface links accroding to number of data in detector
            const auto sf = surface{det, sf_desc};
            sf.template visit_mask<detail::mask_index_update>(sf_desc);
            sf.template visit_material<detail::material_index_update>(sf_desc);
            sf_desc.update_transform(trf_offset);
            sf_desc.set_index(sf_offset++);

            // Copy surface descriptor into global lookup
            det.add_surface_to_lookup(sf_desc);
        }

        //
        // Fill Grid
        //

        // Get relevant ids
        using geo_obj_ids = typename detector_t::geo_obj_ids;
        constexpr auto cyl_id = detector_t::masks::id::e_portal_cylinder2;
        constexpr auto grid_id = detector_t::sf_finders::id::e_cylinder2_grid;

        using cyl_grid_t =
            typename detector_t::surface_container::template get_type<grid_id>;
        auto gbuilder =
            grid_builder<detector_t, cyl_grid_t, detray::detail::fill_by_pos>{};

        // The disc portals are at the end of the portal range by construction
        auto portal_mask_idx = (det.portals(vol).end() - 3u)->mask().index();
        const auto &cyl_mask =
            det.mask_store().template get<cyl_id>().at(portal_mask_idx);

        // Add new grid to the detector
        gbuilder.init_grid(cyl_mask, {100u, 1u});
        gbuilder.fill_grid(detector_volume{det, vol}, surfaces, transforms,
                           masks, ctx0);

        det.surface_store().template push_back<grid_id>(gbuilder.get());
        vol.template set_link<geo_obj_ids::e_sensitive>(
            grid_id, det.surface_store().template size<grid_id>() - 1u);

        // Add transforms, masks and material to detector
        det.append_masks(std::move(masks));
        det.append_transforms(std::move(transforms));
        det.append_materials(std::move(materials));

        // Add volume grid
        // TODO: Fill it

        // Dimensions of the volume grid: minr, min phi, minz, maxr, maxphi,
        // maxz
        // TODO: Adapt to number of layers
        mask<cylinder3D> vgrid_dims{0u,     0.f,   -constant<scalar>::pi,
                                    -600.f, 180.f, constant<scalar>::pi,
                                    600.f};
        std::array<std::size_t, 3> n_vgrid_bins{1u, 1u, 1u};

        grid_factory_type<typename detector_t::volume_finder> vgrid_factory{};
        auto vgrid = vgrid_factory.template new_grid<
            n_axis::open<n_axis::label::e_r>,
            n_axis::circular<n_axis::label::e_phi>,
            n_axis::open<n_axis::label::e_z>, n_axis::irregular<>,
            n_axis::regular<>, n_axis::irregular<>>(vgrid_dims, n_vgrid_bins);
        det.set_volume_finder(std::move(vgrid));
    }

    return std::make_pair(std::move(det), std::move(name_map));
}

}  // namespace detray