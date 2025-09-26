/**
 * @file broadcast_equipment_data.h
 * @brief Test fixture data for broadcast equipment integration tests
 */

#pragma once

#include <vector>
#include <string>

// Forward declarations for test data structures
struct ETIStreamInfo {
    std::string multicast_address;
    int port;
    std::string name;
    int bitrate_bps;
    bool is_active;
    double signal_quality;
};

struct ServiceInfo {
    enum ServiceType {
        Unknown = 0,
        Audio = 1,
        Data = 2,
        DABPlus = 3,
        Packet = 4
    };
    
    uint32_t service_id;
    uint16_t ensemble_id;
    std::string label;
    std::string ensemble_label;
    ServiceType type;
    std::string type_string;
    bool is_active;
    int quality;
    int bitrate;
    bool is_dab_plus;
};

struct EnsembleInfo {
    uint16_t ensemble_id;
    std::string label;
    std::string country;
    int service_count;
    int dab_plus_count;
    double total_bitrate;
    bool is_active;
};

namespace test_fixtures {
namespace broadcast_equipment {

extern const std::vector<ETIStreamInfo> SAMPLE_ETI_STREAMS;
extern const std::vector<ServiceInfo> SAMPLE_DAB_SERVICES;
extern const EnsembleInfo SAMPLE_ENSEMBLE;

} // namespace broadcast_equipment
} // namespace test_fixtures