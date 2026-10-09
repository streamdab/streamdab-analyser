/**
 * @file broadcast_equipment_data.cpp
 * @brief Test fixture data for broadcast equipment integration tests
 */

#include "broadcast_equipment_data.h"

namespace test_fixtures {
namespace broadcast_equipment {

const std::vector<ETIStreamInfo> SAMPLE_ETI_STREAMS = {
    {
        "239.192.0.1",  // multicast_address
        9200,           // port
        "Bangkok DAB Test Stream",  // name
        1536000,        // bitrate_bps
        true,           // is_active
        95.5            // signal_quality
    },
    {
        "239.192.0.2", 
        9201,
        "Chiang Mai DAB Stream",
        1152000,
        true,
        88.2
    }
};

const std::vector<ServiceInfo> SAMPLE_DAB_SERVICES = {
    {
        0x1001,                    // service_id
        0x2001,                    // ensemble_id
        "Radio Thailand",          // label
        "Bangkok DAB Ensemble",    // ensemble_label
        ServiceInfo::Audio,        // type
        "Audio",                   // type_string
        true,                      // is_active
        95,                        // quality
        128,                       // bitrate
        false,                     // is_dab_plus
    },
    {
        0x1002,
        0x2001, 
        "Thai Public Radio",
        "Bangkok DAB Ensemble",
        ServiceInfo::DABPlus,
        "DAB+",
        true,
        92,
        96,
        true
    }
};

const EnsembleInfo SAMPLE_ENSEMBLE = {
    0x2001,                    // ensemble_id
    "Bangkok DAB Ensemble",    // label
    "Thailand",               // country
    2,                        // service_count
    1,                        // dab_plus_count
    1.152,                    // total_bitrate
    true                      // is_active
};

} // namespace broadcast_equipment
} // namespace test_fixtures