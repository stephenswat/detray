/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2020-2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

// Project include(s).
#include "detray/coordinates/cylindrical2.hpp"

#include "detray/masks/masks.hpp"
#include "detray/test/types.hpp"
#include "detray/tracks/tracks.hpp"

// GTest include(s).
#include <gtest/gtest.h>

// System include(s).
#include <limits>

using namespace detray;

using point2 = test::point2;
using point3 = test::point3;
using vector3 = test::vector3;
using transform3 = test::transform3;
using matrix_operator = typename transform3::matrix_actor;
using size_type = typename matrix_operator::size_ty;
template <size_type ROWS, size_type COLS>
using matrix_type = typename matrix_operator::template matrix_type<ROWS, COLS>;

constexpr scalar isclose{1e-5f};

// This test cylindrical2 coordinate
GTEST_TEST(detray_coordinates, cylindrical2) {

    // Preparation work
    const vector3 z = {0.f, 0.f, 1.f};
    const vector3 x = {1.f, 0.f, 0.f};
    const point3 t = {2.f, 3.f, 4.f};
    const transform3 trf(t, z, x);
    const cylindrical2<transform3> c2;
    // Global position on surface
    const point3 global1 = {3.4142136f, 4.4142136f, 9.f};
    const vector3 mom = {1.f, 2.f, 3.f};
    const vector3 d = vector::normalize(mom);
    const scalar time{0.1f};
    const scalar charge{-1.f};

    const scalar r{2.f};
    const scalar hz{detail::invalid_value<scalar>()};
    mask<cylinder2D> mask{0u, r, -hz, hz};

    // Global to local transformation
    const point3 local = c2.global_to_local(trf, global1, d);

    // Check if the local position is correct
    ASSERT_NEAR(local[0], r * constant<scalar>::pi_4, isclose);
    ASSERT_NEAR(local[1], 5.f, isclose);

    // Local to global transformation
    const point3 global2 = c2.local_to_global(trf, local);

    // Check if the same global position is obtained
    ASSERT_NEAR(global1[0], global2[0], isclose);
    ASSERT_NEAR(global1[1], global2[1], isclose);
    ASSERT_NEAR(global1[2], global2[2], isclose);

    // Free track parameter
    const free_track_parameters<transform3> free_params(global1, time, mom,
                                                        charge);
    const auto free_vec1 = free_params.vector();

    const auto bound_vec = c2.free_to_bound_vector(trf, free_vec1);
    const auto free_vec2 = c2.bound_to_free_vector(trf, mask, bound_vec);

    const matrix_operator m;

    // Check if the bound vector is correct
    ASSERT_NEAR(m.element(bound_vec, 0u, 0u), r * constant<scalar>::pi_4,
                isclose);
    ASSERT_NEAR(m.element(bound_vec, 1u, 0u), 5.f, isclose);
    ASSERT_NEAR(m.element(bound_vec, 2u, 0u), 1.1071487f,
                isclose);  // atan(2)
    ASSERT_NEAR(m.element(bound_vec, 3u, 0u), 0.64052231f,
                isclose);  // atan(sqrt(5)/3)
    ASSERT_NEAR(m.element(bound_vec, 4u, 0u), -1.f / 3.7416574f, isclose);
    ASSERT_NEAR(m.element(bound_vec, 5u, 0u), 0.1f, isclose);

    // Check if the same free vector is obtained
    for (unsigned int i = 0u; i < 8u; i++) {
        ASSERT_NEAR(m.element(free_vec1, i, 0u), m.element(free_vec2, i, 0u),
                    isclose);
    }

    // Normal vector
    const vector3 n = c2.normal(trf, local);
    ASSERT_NEAR(n[0], constant<scalar>::inv_sqrt2, isclose);
    ASSERT_NEAR(n[1], constant<scalar>::inv_sqrt2, isclose);
    ASSERT_NEAR(n[2], 0.f, isclose);

    // Test Jacobian transformation
    const matrix_type<6, 6> J = c2.free_to_bound_jacobian(trf, free_vec1) *
                                c2.bound_to_free_jacobian(trf, mask, bound_vec);

    for (unsigned int i = 0u; i < 6u; i++) {
        for (unsigned int j = 0u; j < 6u; j++) {
            if (i == j) {
                EXPECT_NEAR(m.element(J, i, j), 1.f, isclose);
            } else {
                EXPECT_NEAR(m.element(J, i, j), 0.f, isclose);
            }
        }
    }
}
