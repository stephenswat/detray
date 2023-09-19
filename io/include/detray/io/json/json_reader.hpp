/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/io/common/detail/file_handle.hpp"
#include "detray/io/common/detail/utils.hpp"
#include "detray/io/common/geometry_reader.hpp"
#include "detray/io/common/homogeneous_material_reader.hpp"
#include "detray/io/json/json.hpp"
#include "detray/io/json/json_serializers.hpp"
#include "detray/tools/detector_builder.hpp"

// System include(s)
#include <ios>
#include <iostream>
#include <string>

namespace detray {

/// @brief Function that reads the common header part of a file
inline common_header_payload read_json_header(const std::string& file_name) {

    // Read json file
    io::detail::file_handle file{file_name,
                                 std::ios_base::in | std::ios_base::binary};
    nlohmann::json in_json;
    *file >> in_json;

    // Reads the header from file
    header_payload<> h = in_json["header"];

    // Need only the common part here
    const common_header_payload& header = h.common;

    if (header.tag < detail::minimal_io_version) {
        std::cout
            << "WARNING: File was generated with a different detray version"
            << std::endl;
    }

    return header;
}

/// @brief Class that adds json functionality to common reader types.
///
/// Assemble the json readers from the common reader types, which handle the
/// volume builders, and this class, which provides the payload data from the
/// json stream. It also inlcudes the respective @c to_json and @c from_json
/// functions for the payloads ("json_serializers").
///
/// @note The resulting reader types will fulfill @c reader_interface through
/// the common readers they are being extended with
template <class detector_t, template <class> class common_reader_t>
class json_reader final : public common_reader_t<detector_t> {

    using base_reader = common_reader_t<detector_t>;

    public:
    /// Set json file extension
    json_reader() : base_reader(".json") {}

    /// Writes the geometry to file with a given name
    virtual void read(
        detector_builder<typename detector_t::metadata,
                         typename detector_t::bfield_type::backend_t,
                         volume_builder>& det_builder,
        typename detector_t::name_map& name_map,
        const std::string& file_name) override {

        // Read json from file
        io::detail::file_handle file{file_name,
                                     std::ios_base::in | std::ios_base::binary};
        nlohmann::json in_json;
        *file >> in_json;

        // Reads the data from file and returns the corresponding io payloads
        base_reader::deserialize(det_builder, name_map, in_json["data"]);
    }
};

/// Reads the tracking geometry from file in json format
template <typename detector_t>
using json_geometry_reader = json_reader<detector_t, geometry_reader>;

/// Reads a homogeneous material descritption from file in json format
template <typename detector_t>
using json_homogeneous_material_reader =
    json_reader<detector_t, homogeneous_material_reader>;

}  // namespace detray
