/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s).
#include "detray/definitions/qualifiers.hpp"
#include "detray/definitions/track_parametrization.hpp"
#include "detray/propagator/base_actor.hpp"
#include "detray/tracks/detail/track_helper.hpp"

namespace detray {

template <typename transform3_t>
struct resetter : actor {

    struct state {};

    struct kernel {

        /// @name Type definitions for the struct
        /// @{

        // Matrix actor
        using matrix_actor = typename transform3_t::matrix_actor;

        /// @}

        using output_type = bool;

        template <typename mask_group_t, typename surface_t,
                  typename propagator_state_t>
        DETRAY_HOST_DEVICE inline output_type operator()(
            const mask_group_t& mask_group, const surface_t& surface,
            propagator_state_t& propagation) const {

            // Stepper and Navigator states
            auto& navigation = propagation._navigation;
            auto& stepping = propagation._stepping;

            // Retrieve surfaces and transform store
            const auto& det = navigation.detector();
            const auto& transform_store = det->transform_store();

            // Intersection
            const auto& is = navigation.current();

            // Transform
            const auto& trf3 = transform_store[surface.transform()];

            // Mask
            const auto& mask = mask_group[is->mask_index];
            auto local_coordinate = mask.local_type();

            // Reset the path length
            stepping._s = 0;

            // Reset jacobian coordinate transformation at the current surface
            stepping._jac_to_global = local_coordinate.bound_to_free_jacobian(
                trf3, mask, stepping._bound_params.vector());

            // Reset jacobian transport to identity matrix
            matrix_actor().set_identity(stepping._jac_transport);

            return true;
        }
    };

    template <typename propagator_state_t>
    DETRAY_HOST_DEVICE void operator()(propagator_state_t& propagation) const {

        auto& navigation = propagation._navigation;

        // Do covariance transport when the track is on surface
        if (navigation.is_on_module()) {

            const auto& det = navigation.detector();
            const auto& surface_container = det->surfaces();
            const auto& mask_store = det->mask_store();

            // Intersection
            const auto& is = navigation.current();

            // Surface
            const auto& surface = surface_container[is->index];

            auto succeed = mask_store.template execute<kernel>(
                surface.mask_type(), surface, propagation);
        }
    }
};

}  // namespace detray
