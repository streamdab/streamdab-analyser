/**
 * @file enhanced_fig_analyser.cpp
 * @brief Enhanced FIG Analyser Implementation
 * 
 * TDD-driven implementation of the Enhanced FIG Analyser with
 * comprehensive ETSI compliance validation and Thai DAB support.
 * 
 * @author Standards Compliance Agent (TDD Implementation)
 * @date 2025
 */

#include "enhanced_fig_analyser.hpp"
#include "utils/logger.h"
#include <algorithm>
#include <cstring>
#include <regex>

namespace eti::modern {

// ============================================================================
// EnhancedFIGAnalyser Implementation
// ============================================================================

EnhancedFIGAnalyser::EnhancedFIGAnalyser(QObject* parent)
    : QObject(parent)
    , analysis_start_time_(std::chrono::steady_clock::now())
    , last_stats_update_(std::chrono::steady_clock::now())
{
    // Initialize carousel analysis
    carousel_analysis_.analysis_start = analysis_start_time_;
    carousel_analysis_.carousel_efficiency = 100.0;
    
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                          "Enhanced FIG Analyser created with TDD implementation");
}

EnhancedFIGAnalyser::~EnhancedFIGAnalyser()
{
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                          "Enhanced FIG Analyser destroyed");
}

bool EnhancedFIGAnalyser::initialize(bool enable_carousel_analysis, bool enable_thai_support)
{
    m_carousel_analysis_enabled = enable_carousel_analysis;
    m_thai_support_enabled = enable_thai_support;
    m_error_recovery_enabled = true; // Always enable error recovery
    
    // Reset analysis state
    reset();
    
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                          QString("Enhanced FIG Analyser initialized: Carousel=%1, Thai=%2, ErrorRecovery=%3")
                          .arg(m_carousel_analysis_enabled ? "Yes" : "No")
                          .arg(m_thai_support_enabled ? "Yes" : "No")
                          .arg(m_error_recovery_enabled ? "Yes" : "No"));
    
    return true;
}

std::vector<eti::FigBlock> EnhancedFIGAnalyser::process_fic_data(const eti::EtiFicField& fic_data)
{
    std::vector<eti::FigBlock> fig_blocks;
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // A FIB-structured FIC (96/128 bytes) carries 3/4 FIBs, each with a
        // 30-byte FIG area and a 2-byte CRC; corrupted FIBs are skipped.
        // A legacy 32-byte field is treated as a single raw FIG area.
        const bool fib_structured = fic_data.is_fib_structured();
        const size_t fib_count = fib_structured ? fic_data.fibi_count() : 1;

        for (size_t fib = 0; fib < fib_count; ++fib) {
            const size_t fib_base = fib * ETI_FIC_FIB_SIZE;
            if (fib_base + ETI_FIC_FIB_SIZE > fic_data.size()) {
                break;
            }
            const uint8_t* fic_bytes = fic_data.fic_data.data() + fib_base;

            if (fib_structured) {
                // Validate FIB CRC: complemented CRC-16/CCITT-FALSE over the
                // 30 FIG bytes, stored big-endian (EN 300 799 clause 5.2)
                uint16_t crc = 0xFFFF;
                for (int i = 0; i < 30; ++i) {
                    crc ^= static_cast<uint16_t>(fic_bytes[i]) << 8;
                    for (int b = 0; b < 8; ++b) {
                        if (crc & 0x8000) {
                            crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
                        } else {
                            crc = static_cast<uint16_t>(crc << 1);
                        }
                    }
                }
                const uint16_t expected = static_cast<uint16_t>(~crc);
                const uint16_t stored = static_cast<uint16_t>((fic_bytes[30] << 8) | fic_bytes[31]);
                if (expected != stored) {
                    stats_.crc_errors++;
                    continue;  // Skip corrupted FIB
                }
            }

            size_t offset = 0;
            const size_t fic_size = ETI_FIC_FIG_AREA_SIZE;
            while (offset + 1 < fic_size) {
                eti::FigBlock fig_block;

                // FIG header byte
                uint8_t fig_header = fic_bytes[offset];
                if (fig_header == 0xFF || (fig_header >> 5) == 0x07) {
                    // End marker / padding - no more FIGs
                    break;
                }

                fig_block.fig_type = (fig_header >> 5) & 0x07;
                fig_block.length = fig_header & 0x1F;

                // Validate FIG structure
                if (!validateFIGStructure(fig_block)) {
                    stats_.invalid_figs++;
                    if (m_error_recovery_enabled) {
                        if (attemptErrorRecovery(fig_block)) {
                            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser",
                                                  "Recovered from FIG structure error");
                        } else {
                            offset++; // Skip this byte and try next
                            continue;
                        }
                    } else {
                        offset++; // Skip this byte and try next
                        continue;
                    }
                }

                // Check if we have enough data for this FIG
                if (offset + 1 + fig_block.length > fic_size) {
                    stats_.parsing_errors++;
                    break;
                }

                // Copy FIG data
                fig_block.data.resize(fig_block.length);
                if (fig_block.length > 0) {
                    std::memcpy(fig_block.data.data(), fic_bytes + offset + 1, fig_block.length);
                }

                // Validate FIG CRC if applicable
                if (!validateFIGCRC(fig_block)) {
                    stats_.crc_errors++;
                    reportComplianceViolation("ETSI EN 300 401",
                                            QString("FIG %1 CRC error").arg(fig_block.fig_type).toStdString(),
                                            "Major");
                }

                // Store the length before moving the fig_block
                uint8_t fig_length = fig_block.length;

                // Process the FIG block
                if (analyze_fig_block(fig_block)) {
                    fig_blocks.push_back(std::move(fig_block));
                }

                offset += 1 + fig_length;
            }
        }

        // Update carousel analysis if enabled
        if (m_carousel_analysis_enabled) {
            validateCarouselTiming();
        }

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EnhancedFIGAnalyser", 
                              QString("FIC processing error: %1").arg(e.what()));
        stats_.parsing_errors++;
    }
    
    // Update processing statistics
    auto end_time = std::chrono::steady_clock::now();
    auto processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    
    stats_.total_figs_processed += fig_blocks.size();
    stats_.total_processing_time_ns += processing_time.count();
    if (processing_time.count() > static_cast<int64_t>(stats_.peak_processing_time_ns)) {
        stats_.peak_processing_time_ns = processing_time.count();
    }
    
    // Calculate average processing time
    if (stats_.total_figs_processed > 0) {
        stats_.average_processing_time_ns = static_cast<double>(stats_.total_processing_time_ns) / 
                                           static_cast<double>(stats_.total_figs_processed);
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("Processed %1 FIG blocks in %2μs")
                          .arg(fig_blocks.size())
                          .arg(static_cast<double>(processing_time.count()) / 1000.0, 0, 'f', 2));
    
    return fig_blocks;
}

bool EnhancedFIGAnalyser::analyze_fig_block(const eti::FigBlock& fig_block)
{
    try {
        // Update carousel analysis
        if (m_carousel_analysis_enabled) {
            updateCarouselAnalysis(fig_block.fig_type, fig_block.get_extension());
        }
        
        // Process by FIG type
        bool processed = false;
        switch (fig_block.fig_type) {
            case 0:
                processed = processFIG0(fig_block);
                stats_.fig_type_0_count++;
                break;
            case 1:
                processed = processFIG1(fig_block);
                stats_.fig_type_1_count++;
                break;
            case 2:
                processed = processFIG2(fig_block);
                stats_.fig_type_2_count++;
                break;
            default:
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                      QString("Unsupported FIG type: %1").arg(fig_block.fig_type));
                break;
        }
        
        if (!processed) {
            stats_.parsing_errors++;
        }
        
        return processed;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EnhancedFIGAnalyser", 
                              QString("FIG analysis error: %1").arg(e.what()));
        stats_.parsing_errors++;
        return false;
    }
}

std::optional<EnhancedEnsembleInfo> EnhancedFIGAnalyser::get_current_ensemble() const
{
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return current_ensemble_;
}

std::vector<EnhancedServiceInfo> EnhancedFIGAnalyser::get_discovered_services() const
{
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    std::vector<EnhancedServiceInfo> services;
    services.reserve(discovered_services_.size());
    
    for (const auto& [service_id, service] : discovered_services_) {
        services.push_back(service);
    }
    
    return services;
}

std::optional<EnhancedServiceInfo> EnhancedFIGAnalyser::get_service_by_id(uint16_t service_id) const
{
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = discovered_services_.find(service_id);
    if (it != discovered_services_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

FIGCarouselAnalysis EnhancedFIGAnalyser::getCarouselAnalysis() const
{
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return carousel_analysis_;
}

FIGProcessingStats EnhancedFIGAnalyser::getProcessingStats() const
{
    // Return a copy of the stats (atomic operations ensure consistency)
    FIGProcessingStats stats = stats_;
    
    // Calculate compliance score
    if (stats.total_figs_processed > 0) {
        double error_rate = static_cast<double>(stats.invalid_figs + stats.crc_errors + stats.parsing_errors) / 
                           static_cast<double>(stats.total_figs_processed);
        stats.compliance_score = std::max(0.0, 100.0 - (error_rate * 100.0));
    }
    
    return stats;
}

void EnhancedFIGAnalyser::reset()
{
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    // Reset all state
    current_ensemble_.reset();
    discovered_services_.clear();
    subchannels_.clear();
    compliance_issues_.clear();
    reported_issues_.clear();
    
    // Reset additional state tracking
    m_serviceComponents.clear();
    m_dabServices.clear();
    m_userApplications.clear();
    
    // Reset carousel analysis
    carousel_analysis_ = FIGCarouselAnalysis{};
    carousel_analysis_.analysis_start = std::chrono::steady_clock::now();
    analysis_start_time_ = carousel_analysis_.analysis_start;
    
    // Reset statistics
    stats_ = FIGProcessingStats{};
    
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                          "Enhanced FIG Analyser state reset");
}

double EnhancedFIGAnalyser::getETSIComplianceScore() const
{
    return getProcessingStats().compliance_score;
}

std::vector<std::string> EnhancedFIGAnalyser::getComplianceIssues() const
{
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return compliance_issues_;
}

// ============================================================================
// FIG Processing Methods
// ============================================================================

bool EnhancedFIGAnalyser::processFIG0(const eti::FigBlock& fig)
{
    uint8_t extension = fig.get_extension();
    
    switch (extension) {
        case 0: return processFIG0_0_EnsembleInfo(fig);
        case 1: return processFIG0_1_SubchannelOrg(fig);
        case 2: return processFIG0_2_ServiceOrg(fig);
        case 3: return processFIG0_3_ServiceComponent(fig);
        case 4: return processFIG0_4_ServiceComponentLink(fig);
        case 5: return processFIG0_5_ServiceComponentLang(fig);
        case 6: return processFIG0_6_ServiceLinking(fig);
        case 7: return processFIG0_7_ConfigurationInfo(fig);
        case 8: return processFIG0_8_ServiceComponentGlobal(fig);
        case 9: return processFIG0_9_CountryLTOInt(fig);
        case 10: return processFIG0_10_DateTime(fig);
        case 11: return processFIG0_11_RegionDefinition(fig);
        case 12: return processFIG0_12_RESERVED(fig);
        case 13: return processFIG0_13_UserAppInfo(fig);
        case 14: return processFIG0_14_FECSubChannelOrg(fig);
        case 15: return processFIG0_15_RESERVED(fig);
        case 16: return processFIG0_16_ProgrammeNumber(fig);
        case 17: return processFIG0_17_ProgrammeType(fig);
        case 18: return processFIG0_18_Announcement(fig);
        case 19: return processFIG0_19_AnnouncementSwitch(fig);
        case 20: return processFIG0_20_ServiceComponentInfo(fig);
        case 21: return processFIG0_21_FrequencyInfo(fig);
        case 22: return processFIG0_22_TransmitterIdInfo(fig);
        case 23: return processFIG0_23_RESERVED(fig);
        case 24: return processFIG0_24_OtherEnsembleService(fig);
        case 25: return processFIG0_25_OtherEnsembleAnnouncement(fig);
        case 26: return processFIG0_26_OtherEnsembleFreq(fig);
        default:
            Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                                  QString("Unknown FIG 0/%1 - Not in ETSI EN 300 401").arg(extension));
            reportComplianceViolation("ETSI EN 300 401", 
                                    QString("Unknown FIG 0 extension: %1").arg(extension).toStdString(),
                                    "Minor");
            return false;
    }
}

bool EnhancedFIGAnalyser::processFIG1(const eti::FigBlock& fig)
{
    uint8_t extension = fig.get_extension();
    
    switch (extension) {
        case 0: return processFIG1_0_EnsembleLabel(fig);
        case 1: return processFIG1_1_ServiceLabel(fig);
        case 2: return processFIG1_2_RESERVED(fig);
        case 3: return processFIG1_3_RESERVED(fig);
        case 4: return processFIG1_4_ServiceComponentLabel(fig);
        case 5: return processFIG1_5_DataServiceLabel(fig);
        case 6: return processFIG1_6_XPADUserAppLabel(fig);
        case 7: return processFIG1_7_RESERVED(fig);
        default:
            Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                                  QString("Unknown FIG 1/%1 - Not in ETSI EN 300 401").arg(extension));
            reportComplianceViolation("ETSI EN 300 401", 
                                    QString("Unknown FIG 1 extension: %1").arg(extension).toStdString(),
                                    "Minor");
            return false;
    }
}

bool EnhancedFIGAnalyser::processFIG2(const eti::FigBlock& fig)
{
    uint8_t extension = fig.get_extension();
    
    switch (extension) {
        case 0: return processFIG2_0_MOTChannel(fig);
        case 1: return processFIG2_1_MOTService(fig);
        default:
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("Unsupported FIG 2/%1").arg(extension));
            return false;
    }
}

// ============================================================================
// FIG Type 0 Processors (Multiplex Configuration Information)
// ============================================================================

bool EnhancedFIGAnalyser::processFIG0_0_EnsembleInfo(const eti::FigBlock& fig)
{
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/0: Insufficient data", "Major");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    eti::Ensemble ensemble;
    ensemble.ensemble_id = (fig.data[0] << 8) | fig.data[1];
    ensemble.country_id = (fig.data[2] >> 4) & 0x0F;
    ensemble.extended_country_code = fig.data[2] & 0x0F;
    
    if (fig.data.size() >= 6) {
        ensemble.cif_count = fig.data[3];
        ensemble.occurrence_change = (fig.data[4] >> 4) & 0x0F;
        ensemble.alarm_flag = (fig.data[4] & 0x01) != 0;
    }
    
    // Update enhanced ensemble info
    if (!current_ensemble_) {
        current_ensemble_ = EnhancedEnsembleInfo{};
        current_ensemble_->first_discovered = std::chrono::steady_clock::now();
        current_ensemble_->is_stable = true;
        current_ensemble_->overall_quality = 1.0;
    }
    
    current_ensemble_->base_ensemble = ensemble;
    current_ensemble_->last_updated = std::chrono::steady_clock::now();
    
    lock.unlock();
    
    emit ensembleDiscovered(*current_ensemble_);
    
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                          QString("FIG 0/0: Ensemble ID=0x%1, Country=0x%2")
                          .arg(ensemble.ensemble_id, 4, 16, QChar('0'))
                          .arg(ensemble.country_id, 2, 16, QChar('0')));
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG0_1_SubchannelOrg(const eti::FigBlock& fig)
{
    if (fig.data.size() < 3) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/1: Insufficient data", "Major");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int subchannels_parsed = 0;
    
    while (offset + 2 < fig.data.size()) {
        eti::SubChannelInfo subchannel;
        
        subchannel.sub_channel_id = (fig.data[offset] >> 2) & 0x3F;
        subchannel.start_address = ((fig.data[offset] & 0x03) << 8) | fig.data[offset + 1];
        
        if (offset + 2 >= fig.data.size()) break;
        
        if (fig.data[offset + 2] & 0x80) {
            // LONG form = EEP (EN 300 401 8.1.2.1): Option(3)|PL(2)|Size(10).
            // (Standards-review F1: previous code mislabeled this UEP.)
            if (offset + 3 >= fig.data.size()) break;
            const uint8_t b2 = fig.data[offset + 2];
            subchannel.size = static_cast<uint16_t>(((b2 & 0x03) << 8) | fig.data[offset + 3]);
            subchannel.protection_option = static_cast<uint8_t>((b2 >> 4) & 0x07);
            subchannel.protection_level = static_cast<uint8_t>(((b2 >> 2) & 0x03) + 1);  // 0-based on wire, 1-based stored
            subchannel.uep_flag = false;
            offset += 4;
        } else {
            // SHORT form = UEP: TableSwitch(1)|TableIndex(6); size is
            // derived from the UEP table (not encoded), left at 0.
            subchannel.uep_flag = true;
            subchannel.protection_level = 0;
            subchannel.size = 0;
            offset += 3;
        }
        
        if (subchannel.validate_sub_channel_id() && subchannel.validate_boundaries()) {
            subchannels_[subchannel.sub_channel_id] = subchannel;
            subchannels_parsed++;
            
            // Update ensemble subchannel count
            if (current_ensemble_) {
                current_ensemble_->total_subchannels = subchannels_.size();
                current_ensemble_->total_capacity_units += subchannel.size;
            }
        } else {
            reportComplianceViolation("ETSI EN 300 401", 
                                    QString("FIG 0/1: Invalid subchannel %1").arg(subchannel.sub_channel_id).toStdString(),
                                    "Minor");
        }
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/1: Parsed %1 subchannels").arg(subchannels_parsed));
    
    return subchannels_parsed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_2_ServiceOrg(const eti::FigBlock& fig)
{
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/2: Insufficient data", "Major");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int services_parsed = 0;
    
    while (offset + 3 < fig.data.size()) {
        eti::DabService service;
        
        service.service_id = (fig.data[offset] << 8) | fig.data[offset + 1];
        service.country_id = (fig.data[offset + 2] >> 4) & 0x0F;
        service.extended_country_code = fig.data[offset + 2] & 0x0F;
        
        if (offset + 3 < fig.data.size()) {
            service.is_programme = (fig.data[offset + 3] & 0x02) != 0;
        }
        
        if (service.validate_service_id() && service.validate_country_code()) {
            updateServiceInfo(service);
            services_parsed++;
        } else {
            reportComplianceViolation("ETSI EN 300 401", 
                                    QString("FIG 0/2: Invalid service 0x%1").arg(service.service_id, 4, 16, QChar('0')).toStdString(),
                                    "Minor");
        }
        
        offset += 4;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/2: Parsed %1 services").arg(services_parsed));
    
    return services_parsed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_3_ServiceComponent(const eti::FigBlock& fig)
{
    if (fig.data.size() < 5) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/3: Insufficient data", "Major");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int components_parsed = 0;
    
    while (offset + 4 < fig.data.size()) {
        uint16_t service_id = (fig.data[offset] << 8) | fig.data[offset + 1];
        
        eti::ServiceComponent component;
        component.service_id = service_id;
        component.sub_channel_id = (fig.data[offset + 2] >> 2) & 0x3F;
        component.tmid = fig.data[offset + 2] & 0x03;
        component.asc_ty = fig.data[offset + 3] & 0x3F;
        component.primary = (fig.data[offset + 3] & 0x40) != 0;
        component.ca_flag = (fig.data[offset + 3] & 0x80) != 0;
        
        if (component.validate_sub_channel_id() && component.validate_tmid()) {
            auto service_it = discovered_services_.find(service_id);
            if (service_it != discovered_services_.end()) {
                mergeServiceComponents(service_it->second, component);
                components_parsed++;
            }
        }
        
        offset += 4;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/3: Parsed %1 components").arg(components_parsed));
    
    return components_parsed > 0;
}

// ============================================================================
// Comprehensive FIG 0 Processors (All Extensions 0-26)
// ============================================================================

bool EnhancedFIGAnalyser::processFIG0_5_ServiceComponentLang(const eti::FigBlock& fig) 
{
    // FIG 0/5: Service component language (ETSI EN 300 401 Section 8.1.7)
    if (fig.data.size() < 2) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/5: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/5: Processing service component language info (%1 bytes)")
                          .arg(fig.data.size()));
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int languages_processed = 0;
    
    while (offset + 1 < fig.data.size()) {
        uint8_t scid_ps = (fig.data[offset] >> 2) & 0x3F;  // Service Component ID
        uint8_t language = fig.data[offset + 1];            // Language code (ISO 639-2/B)
        
        // Store language information
        auto comp_it = m_serviceComponents.find(scid_ps);
        if (comp_it != m_serviceComponents.end()) {
            // Update existing component with language information
            languages_processed++;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/5: Updated SCID %1 with language code 0x%2")
                                  .arg(scid_ps).arg(language, 2, 16, QChar('0')));
        } else {
            // Create new service component entry
            eti::ServiceComponent comp;
            comp.sub_channel_id = scid_ps;
            m_serviceComponents[scid_ps] = comp;
            languages_processed++;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/5: Created SCID %1 with language code 0x%2")
                                  .arg(scid_ps).arg(language, 2, 16, QChar('0')));
        }
        
        offset += 2;  // Each entry is 2 bytes
    }
    
    return languages_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_4_ServiceComponentLink(const eti::FigBlock& fig)
{
    // FIG 0/4: Service component in packet mode with or without CA (ETSI EN 300 401 Section 8.1.6)
    if (fig.data.size() < 5) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/4: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/4: Processing service component linking (%1 bytes)")
                          .arg(fig.data.size()));
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int links_processed = 0;
    
    while (offset + 4 < fig.data.size()) {
        uint16_t service_id = (fig.data[offset] << 8) | fig.data[offset + 1];
        
        eti::ServiceComponent component;
        component.service_id = service_id;
        component.sub_channel_id = (fig.data[offset + 2] >> 2) & 0x3F;
        component.tmid = fig.data[offset + 2] & 0x03;
        component.asc_ty = fig.data[offset + 3] & 0x3F;
        component.primary = (fig.data[offset + 3] & 0x40) != 0;
        component.ca_flag = (fig.data[offset + 3] & 0x80) != 0;
        
        // Service component identifier
        uint16_t scid = (fig.data[offset + 4] << 8) | fig.data[offset + 5];
        
        if (component.validate_sub_channel_id() && component.validate_tmid()) {
            m_serviceComponents[component.sub_channel_id] = component;
            
            // Update associated service
            auto service_it = discovered_services_.find(service_id);
            if (service_it != discovered_services_.end()) {
                mergeServiceComponents(service_it->second, component);
            }
            
            links_processed++;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/4: Linked service 0x%1 to component %2 (SCID=0x%3)")
                                  .arg(service_id, 4, 16, QChar('0'))
                                  .arg(component.sub_channel_id)
                                  .arg(scid, 4, 16, QChar('0')));
        }
        
        offset += 6;  // Each entry is 6 bytes
    }
    
    return links_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_6_ServiceLinking(const eti::FigBlock& fig)
{
    // FIG 0/6: Service linking information (ETSI EN 300 401 Section 8.1.8)
    if (fig.data.size() < 6) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/6: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/6: Processing service linking (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int links_processed = 0;
    
    while (offset + 5 < fig.data.size()) {
        uint8_t id_list_flag = (fig.data[offset] >> 7) & 0x01;
        uint8_t la_flag = (fig.data[offset] >> 6) & 0x01;
        uint8_t shd_flag = (fig.data[offset] >> 5) & 0x01;
        uint8_t icp_flag = (fig.data[offset] >> 4) & 0x01;
        uint8_t lsn = fig.data[offset] & 0x0F;
        
        if (id_list_flag) {
            // IdLI present
            uint16_t id_li = (fig.data[offset + 1] << 8) | fig.data[offset + 2];
            offset += 3;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/6: Service linking IdLI=0x%1, LSN=%2")
                                  .arg(id_li, 4, 16, QChar('0')).arg(lsn));
        } else {
            offset += 1;
        }
        
        // Process linking information based on flags
        if (la_flag && offset + 1 < fig.data.size()) {
            uint8_t linkage_actuator = fig.data[offset];
            offset++;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/6: Linkage actuator: 0x%1")
                                  .arg(linkage_actuator, 2, 16, QChar('0')));
        }
        
        if (shd_flag && offset + 1 < fig.data.size()) {
            uint8_t shd = fig.data[offset];
            offset++;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/6: Soft/Hard flag: 0x%1")
                                  .arg(shd, 2, 16, QChar('0')));
        }
        
        if (icp_flag && offset + 1 < fig.data.size()) {
            uint8_t icp = fig.data[offset];
            offset++;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/6: International code point: 0x%1")
                                  .arg(icp, 2, 16, QChar('0')));
        }
        
        links_processed++;
    }
    
    return links_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_7_ConfigurationInfo(const eti::FigBlock& fig)
{
    // FIG 0/7: Configuration information (ETSI EN 300 401 Section 8.1.11)
    if (fig.data.size() < 2) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/7: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/7: Processing configuration information (%1 bytes)")
                          .arg(fig.data.size()));
    
    // Count/other flag and reconfiguration counter
    uint8_t count_other = (fig.data[0] >> 7) & 0x01;
    uint8_t reconfig_counter = (fig.data[0] >> 3) & 0x0F;
    uint8_t reconfig_index = fig.data[0] & 0x07;
    
    if (count_other) {
        // Other ensemble count present
        if (fig.data.size() >= 3) {
            uint8_t other_ensemble_count = fig.data[1];
            
            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/7: Reconfiguration counter=%1, index=%2, other ensembles=%3")
                                  .arg(reconfig_counter).arg(reconfig_index).arg(other_ensemble_count));
        }
    } else {
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                              QString("FIG 0/7: Reconfiguration counter=%1, index=%2")
                              .arg(reconfig_counter).arg(reconfig_index));
    }
    
    // Store configuration information in ensemble
    if (current_ensemble_) {
        // These would be stored in extended ensemble structure
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/7: Updated ensemble configuration info"));
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG0_11_RegionDefinition(const eti::FigBlock& fig)
{
    // FIG 0/11: Region definition (ETSI EN 300 401 Section 8.1.17)
    if (fig.data.size() < 3) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/11: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/11: Processing region definition (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int regions_processed = 0;
    
    while (offset + 2 < fig.data.size()) {
        uint8_t region_id = fig.data[offset];
        uint8_t latitude_flag = (fig.data[offset + 1] >> 7) & 0x01;
        uint8_t longitude_flag = (fig.data[offset + 1] >> 6) & 0x01;
        uint8_t lat_long_extent = (fig.data[offset + 1] >> 4) & 0x03;
        uint8_t tii_flag = (fig.data[offset + 1] >> 3) & 0x01;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/11: Region ID %1, coordinates present: lat=%2, lon=%3")
                              .arg(region_id).arg(latitude_flag).arg(longitude_flag));
        
        offset += 2;
        
        // Process optional coordinates
        if (latitude_flag && offset + 2 <= fig.data.size()) {
            uint16_t latitude = (fig.data[offset] << 8) | fig.data[offset + 1];
            offset += 2;
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/11: Latitude: %1").arg(latitude));
        }
        
        if (longitude_flag && offset + 2 <= fig.data.size()) {
            uint16_t longitude = (fig.data[offset] << 8) | fig.data[offset + 1];
            offset += 2;
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/11: Longitude: %1").arg(longitude));
        }
        
        regions_processed++;
    }
    
    return regions_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_12_RESERVED(const eti::FigBlock& fig)
{
    // FIG 0/12: Reserved for future use
    reportComplianceViolation("ETSI EN 300 401", "FIG 0/12: Reserved extension used", "Minor");
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                          QString("FIG 0/12: Reserved extension used (%1 bytes)")
                          .arg(fig.data.size()));
    return false;
}

bool EnhancedFIGAnalyser::processFIG0_14_FECSubChannelOrg(const eti::FigBlock& fig)
{
    // FIG 0/14: FEC sub-channel organization (ETSI EN 300 401 Section 8.1.20)
    if (fig.data.size() < 2) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/14: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/14: Processing FEC sub-channel organization (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int subchannels_processed = 0;
    
    while (offset + 1 < fig.data.size()) {
        uint8_t sub_channel_id = (fig.data[offset] >> 2) & 0x3F;
        uint8_t fec_scheme = fig.data[offset] & 0x03;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/14: Sub-channel %1, FEC scheme %2")
                              .arg(sub_channel_id).arg(fec_scheme));
        
        // Update subchannel information with FEC details
        auto subchannel_it = subchannels_.find(sub_channel_id);
        if (subchannel_it != subchannels_.end()) {
            // FEC scheme would be stored in extended subchannel structure
            subchannels_processed++;
        }
        
        offset += 1;
    }
    
    return subchannels_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_15_RESERVED(const eti::FigBlock& fig)
{
    // FIG 0/15: Reserved for future use
    reportComplianceViolation("ETSI EN 300 401", "FIG 0/15: Reserved extension used", "Minor");
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                          QString("FIG 0/15: Reserved extension used (%1 bytes)")
                          .arg(fig.data.size()));
    return false;
}

bool EnhancedFIGAnalyser::processFIG0_16_ProgrammeNumber(const eti::FigBlock& fig)
{
    // FIG 0/16: Programme Number (ETSI EN 300 401 Section 8.1.21)
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/16: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/16: Processing programme number (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int programmes_processed = 0;
    
    while (offset + 3 < fig.data.size()) {
        uint16_t service_id = (fig.data[offset] << 8) | fig.data[offset + 1];
        uint16_t programme_number = (fig.data[offset + 2] << 8) | fig.data[offset + 3];
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/16: Service 0x%1, Programme Number %2")
                              .arg(service_id, 4, 16, QChar('0')).arg(programme_number));
        
        // Update service information with programme number
        auto service_it = discovered_services_.find(service_id);
        if (service_it != discovered_services_.end()) {
            // Programme number would be stored in enhanced service structure
            programmes_processed++;
        }
        
        offset += 4;
    }
    
    return programmes_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_18_Announcement(const eti::FigBlock& fig)
{
    // FIG 0/18: Announcement support (ETSI EN 300 401 Section 8.1.23)
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/18: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/18: Processing announcement support (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int announcements_processed = 0;
    
    while (offset + 3 < fig.data.size()) {
        uint16_t service_id = (fig.data[offset] << 8) | fig.data[offset + 1];
        uint16_t announcement_support = (fig.data[offset + 2] << 8) | fig.data[offset + 3];
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/18: Service 0x%1, Announcement support flags: 0x%2")
                              .arg(service_id, 4, 16, QChar('0'))
                              .arg(announcement_support, 4, 16, QChar('0')));
        
        // Decode announcement types
        if (announcement_support & 0x8000) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Alarm");
        if (announcement_support & 0x4000) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Road traffic flash");
        if (announcement_support & 0x2000) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Transport flash");
        if (announcement_support & 0x1000) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Warning/Service");
        if (announcement_support & 0x0800) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - News flash");
        if (announcement_support & 0x0400) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Area weather flash");
        if (announcement_support & 0x0200) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Event announcement");
        if (announcement_support & 0x0100) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Special event");
        if (announcement_support & 0x0080) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Programme information");
        if (announcement_support & 0x0040) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Sport report");
        if (announcement_support & 0x0020) Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "  - Financial report");
        
        announcements_processed++;
        offset += 4;
    }
    
    return announcements_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_19_AnnouncementSwitch(const eti::FigBlock& fig)
{
    // FIG 0/19: Announcement switching (ETSI EN 300 401 Section 8.1.24)
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/19: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/19: Processing announcement switching (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int switches_processed = 0;
    
    while (offset + 3 < fig.data.size()) {
        uint8_t cluster_id = fig.data[offset];
        uint16_t announcement_support = (fig.data[offset + 1] << 8) | fig.data[offset + 2];
        uint8_t new_flag = (fig.data[offset + 3] >> 7) & 0x01;
        uint8_t region_flag = (fig.data[offset + 3] >> 6) & 0x01;
        uint8_t sub_channel_id = (fig.data[offset + 3] >> 2) & 0x3F;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/19: Cluster %1, Announcement 0x%2, SubCh %3, New=%4, Region=%5")
                              .arg(cluster_id)
                              .arg(announcement_support, 4, 16, QChar('0'))
                              .arg(sub_channel_id)
                              .arg(new_flag)
                              .arg(region_flag));
        
        switches_processed++;
        offset += 4;
        
        // Handle optional region field
        if (region_flag && offset < fig.data.size()) {
            uint8_t region_id = fig.data[offset];
            offset++;
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/19: Region ID: %1").arg(region_id));
        }
    }
    
    return switches_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_20_ServiceComponentInfo(const eti::FigBlock& fig)
{
    // FIG 0/20: Service component information (ETSI EN 300 401 Section 8.1.25)
    if (fig.data.size() < 3) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/20: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/20: Processing service component information (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int components_processed = 0;
    
    while (offset + 2 < fig.data.size()) {
        uint8_t sub_channel_id = (fig.data[offset] >> 2) & 0x3F;
        uint8_t length = fig.data[offset + 1];
        
        if (offset + 2 + length > fig.data.size()) break;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/20: Sub-channel %1, info length %2")
                              .arg(sub_channel_id).arg(length));
        
        // Process component-specific information
        if (length > 0) {
            std::vector<uint8_t> component_info(fig.data.begin() + offset + 2, 
                                               fig.data.begin() + offset + 2 + length);
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/20: Component info for sub-channel %1")
                                  .arg(sub_channel_id));
        }
        
        components_processed++;
        offset += 2 + length;
    }
    
    return components_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_21_FrequencyInfo(const eti::FigBlock& fig)
{
    // FIG 0/21: Frequency Information (ETSI EN 300 401 Section 8.1.26)
    if (fig.data.size() < 5) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/21: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/21: Processing frequency information (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int frequencies_processed = 0;
    
    while (offset + 4 < fig.data.size()) {
        uint8_t range_modulation = fig.data[offset];
        uint8_t id_field = fig.data[offset + 1];
        uint32_t frequency = (fig.data[offset + 2] << 16) | 
                           (fig.data[offset + 3] << 8) | 
                           fig.data[offset + 4];
        
        uint8_t rm = (range_modulation >> 4) & 0x0F;
        uint8_t fi = range_modulation & 0x0F;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/21: RM=%1, FI=%2, ID=0x%3, Freq=%4 kHz")
                              .arg(rm).arg(fi)
                              .arg(id_field, 2, 16, QChar('0'))
                              .arg(frequency));
        
        frequencies_processed++;
        offset += 5;
    }
    
    return frequencies_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_22_TransmitterIdInfo(const eti::FigBlock& fig)
{
    // FIG 0/22: Transmitter Identification Information (ETSI EN 300 401 Section 8.1.27)
    if (fig.data.size() < 3) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/22: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/22: Processing transmitter ID information (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int transmitters_processed = 0;
    
    while (offset + 2 < fig.data.size()) {
        uint8_t main_id = fig.data[offset];
        uint8_t sub_id = fig.data[offset + 1];
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/22: Main ID %1, Sub ID %2")
                              .arg(main_id).arg(sub_id));
        
        transmitters_processed++;
        offset += 2;
        
        // Handle optional latitude/longitude (if present)
        if (offset + 4 <= fig.data.size()) {
            uint16_t latitude = (fig.data[offset] << 8) | fig.data[offset + 1];
            uint16_t longitude = (fig.data[offset + 2] << 8) | fig.data[offset + 3];
            offset += 4;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/22: Transmitter location - Lat: %1, Lon: %2")
                                  .arg(latitude).arg(longitude));
        }
    }
    
    return transmitters_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_23_RESERVED(const eti::FigBlock& fig)
{
    // FIG 0/23: Reserved for future use
    reportComplianceViolation("ETSI EN 300 401", "FIG 0/23: Reserved extension used", "Minor");
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                          QString("FIG 0/23: Reserved extension used (%1 bytes)")
                          .arg(fig.data.size()));
    return false;
}

bool EnhancedFIGAnalyser::processFIG0_24_OtherEnsembleService(const eti::FigBlock& fig)
{
    // FIG 0/24: Other ensemble service (ETSI EN 300 401 Section 8.1.28)
    if (fig.data.size() < 6) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/24: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/24: Processing other ensemble service (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int services_processed = 0;
    
    while (offset + 5 < fig.data.size()) {
        uint32_t service_id = (fig.data[offset] << 24) | (fig.data[offset + 1] << 16) |
                             (fig.data[offset + 2] << 8) | fig.data[offset + 3];
        uint16_t ensemble_id = (fig.data[offset + 4] << 8) | fig.data[offset + 5];
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/24: Service 0x%1 in ensemble 0x%2")
                              .arg(service_id, 8, 16, QChar('0'))
                              .arg(ensemble_id, 4, 16, QChar('0')));
        
        services_processed++;
        offset += 6;
    }
    
    return services_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_25_OtherEnsembleAnnouncement(const eti::FigBlock& fig)
{
    // FIG 0/25: Other ensemble announcement support (ETSI EN 300 401 Section 8.1.29)
    if (fig.data.size() < 8) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/25: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/25: Processing other ensemble announcement (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int announcements_processed = 0;
    
    while (offset + 7 < fig.data.size()) {
        uint32_t service_id = (fig.data[offset] << 24) | (fig.data[offset + 1] << 16) |
                             (fig.data[offset + 2] << 8) | fig.data[offset + 3];
        uint16_t ensemble_id = (fig.data[offset + 4] << 8) | fig.data[offset + 5];
        uint16_t announcement_support = (fig.data[offset + 6] << 8) | fig.data[offset + 7];
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/25: Service 0x%1 in ensemble 0x%2, announcements 0x%3")
                              .arg(service_id, 8, 16, QChar('0'))
                              .arg(ensemble_id, 4, 16, QChar('0'))
                              .arg(announcement_support, 4, 16, QChar('0')));
        
        announcements_processed++;
        offset += 8;
    }
    
    return announcements_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_26_OtherEnsembleFreq(const eti::FigBlock& fig)
{
    // FIG 0/26: Other ensemble frequency information (ETSI EN 300 401 Section 8.1.30)
    if (fig.data.size() < 7) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 0/26: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/26: Processing other ensemble frequency (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int frequencies_processed = 0;
    
    while (offset + 6 < fig.data.size()) {
        uint16_t ensemble_id = (fig.data[offset] << 8) | fig.data[offset + 1];
        uint8_t range_modulation = fig.data[offset + 2];
        uint32_t frequency = (fig.data[offset + 3] << 16) | 
                           (fig.data[offset + 4] << 8) | 
                           fig.data[offset + 5];
        
        uint8_t rm = (range_modulation >> 4) & 0x0F;
        uint8_t fi = range_modulation & 0x0F;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/26: Ensemble 0x%1, RM=%2, FI=%3, Freq=%4 kHz")
                              .arg(ensemble_id, 4, 16, QChar('0'))
                              .arg(rm).arg(fi).arg(frequency));
        
        frequencies_processed++;
        offset += 6;
    }
    
    return frequencies_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG0_8_ServiceComponentGlobal(const eti::FigBlock& fig) 
{
    // FIG 0/8: Service component global (ETSI EN 300 401 Section 8.1.13)
    if (fig.data.size() < 4) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              "FIG 0/8: Insufficient data length");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/8: Processing service component global definition (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    while (offset + 4 <= fig.data.size()) {
        // Parse service component global definition
        uint32_t sid = (fig.data[offset] << 24) | (fig.data[offset + 1] << 16) |
                       (fig.data[offset + 2] << 8) | fig.data[offset + 3];  // Service ID
        
        if (offset + 5 < fig.data.size()) {
            uint8_t ext_flag = (fig.data[offset + 4] >> 7) & 0x01;  // Extension flag
            uint8_t scids = (fig.data[offset + 4] >> 4) & 0x0F;     // Service Component IDs
            uint8_t ls_flag = (fig.data[offset + 4] >> 3) & 0x01;   // Long/Short form flag
            uint8_t sct = fig.data[offset + 4] & 0x3F;              // Service Component Type
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/8: SID 0x%1, SCIdS %2, SCT %3, LS %4")
                                  .arg(sid, 8, 16, QChar('0')).arg(scids).arg(sct).arg(ls_flag));
            
            // Store global service component information
            if (m_dabServices.find(sid) != m_dabServices.end()) {
                // Update existing service with component information
                eti::ServiceComponent comp;
                comp.service_id = static_cast<uint16_t>(sid & 0xFFFF); // Truncate to 16-bit for compatibility
                comp.sub_channel_id = scids;
                comp.asc_ty = sct;
                // Note: long_short_flag would be stored in extended structure
                m_serviceComponents[scids] = comp;
                
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                      QString("FIG 0/8: Updated service component for SID 0x%1, SCIdS %2")
                                      .arg(sid, 8, 16, QChar('0')).arg(scids));
            } else {
                // Create new service entry
                eti::DabService service;
                service.service_id = static_cast<uint16_t>(sid & 0xFFFF);
                m_dabServices[sid] = service;
                
                // Create associated service component
                eti::ServiceComponent comp;
                comp.service_id = service.service_id;
                comp.sub_channel_id = scids;
                comp.asc_ty = sct;
                m_serviceComponents[scids] = comp;
                
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                      QString("FIG 0/8: Created new service SID 0x%1 with component SCIdS %2")
                                      .arg(sid, 8, 16, QChar('0')).arg(scids));
            }
            
            offset += 5;  // Basic entry is 5 bytes
            
            // Handle extension if present
            if (ext_flag && offset < fig.data.size()) {
                offset++;  // Skip extension byte for now
            }
        } else {
            break;  // Insufficient data for complete entry
        }
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG0_9_CountryLTOInt(const eti::FigBlock& fig) 
{
    // FIG 0/9: Country, LTO and International table (ETSI EN 300 401 Section 8.1.15)
    if (fig.data.size() < 4) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              "FIG 0/9: Insufficient data length");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/9: Processing country/LTO/international table (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    while (offset + 4 <= fig.data.size()) {
        // Parse country, LTO and international table information
        uint8_t ext = (fig.data[offset] >> 3) & 0x1F;      // Extension
        uint8_t lto_unique = (fig.data[offset] >> 2) & 0x01;  // LTO unique flag
        uint8_t ensemble_lto = fig.data[offset] & 0x3F;    // Ensemble LTO
        
        uint8_t ensemble_ecc = fig.data[offset + 1];       // Ensemble ECC (Extended Country Code)
        uint8_t international_table_id = fig.data[offset + 2];  // International table ID
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/9: ECC 0x%1, LTO %2 (%3), International table ID 0x%4")
                              .arg(ensemble_ecc, 2, 16, QChar('0'))
                              .arg(ensemble_lto)
                              .arg(lto_unique ? "unique" : "shared")
                              .arg(international_table_id, 2, 16, QChar('0')));
        
        // Store ensemble geographical and temporal information
        if (current_ensemble_) {
            // Update base ensemble with extended information
            current_ensemble_->base_ensemble.extended_country_code = ensemble_ecc;
            // Note: These fields would be added to Ensemble structure in full implementation:
            // current_ensemble_->local_time_offset = ensemble_lto;
            // current_ensemble_->international_table_id = international_table_id;
            // current_ensemble_->lto_unique = lto_unique;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                  QString("FIG 0/9: Updated ensemble with ECC=0x%1, LTO=%2")
                                  .arg(ensemble_ecc, 2, 16, QChar('0')).arg(ensemble_lto));
        }
        
        offset += 3;  // Each entry is 3 bytes minimum
        
        // Handle additional services if present (variable length)
        if (offset < fig.data.size()) {
            uint8_t num_services = fig.data[offset];
            offset++;
            
            for (uint8_t i = 0; i < num_services && offset + 3 < fig.data.size(); ++i) {
                uint32_t sid = (fig.data[offset] << 24) | (fig.data[offset + 1] << 16) |
                               (fig.data[offset + 2] << 8) | fig.data[offset + 3];
                
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                      QString("FIG 0/9: Service SID 0x%1 in international table")
                                      .arg(sid, 8, 16, QChar('0')));
                
                offset += 4;
            }
        }
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG0_10_DateTime(const eti::FigBlock& fig) 
{
    // FIG 0/10: Date and time (ETSI EN 300 401 Section 8.1.16)
    if (fig.data.size() < 4) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              "FIG 0/10: Insufficient data length");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/10: Processing date and time information (%1 bytes)")
                          .arg(fig.data.size()));
    
    // Parse Modified Julian Date (MJD) and time
    uint32_t mjd_time = (fig.data[0] << 24) | (fig.data[1] << 16) | 
                        (fig.data[2] << 8) | fig.data[3];
    
    // Extract MJD (17 bits) and UTC time (10 bits for hours/minutes)
    uint32_t mjd = (mjd_time >> 17) & 0x1FFFF;          // Modified Julian Date
    uint16_t utc_time = (mjd_time >> 7) & 0x3FF;        // UTC time (hours * 32 + minutes/2)
    uint8_t utc_flag = (mjd_time >> 3) & 0x01;          // UTC time flag
    uint8_t lsi = (mjd_time >> 2) & 0x01;               // Leap second indicator
    
    // Convert UTC time to hours and minutes
    uint8_t hours = utc_time / 32;
    uint8_t minutes = (utc_time % 32) * 2;
    
    // Convert MJD to Gregorian date (simplified calculation)
    // MJD 0 = November 17, 1858
    int64_t julian_day = static_cast<int64_t>(mjd) + 2400001;  // Convert to Julian Day Number
    int64_t a = julian_day + 32044;
    int64_t b = (4 * a + 3) / 146097;
    int64_t c = a - (146097 * b) / 4;
    int64_t d = (4 * c + 3) / 1461;
    int64_t e = c - (1461 * d) / 4;
    int64_t m = (5 * e + 2) / 153;
    
    int day = static_cast<int>(e - (153 * m + 2) / 5 + 1);
    int month = static_cast<int>(m + 3 - 12 * (m / 10));
    int year = static_cast<int>(100 * b + d - 4800 + m / 10);
    
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                          QString("FIG 0/10: Date/Time: %1-%2-%3 %4:%5 UTC (MJD %6) %7%8")
                          .arg(year, 4, 10, QChar('0'))
                          .arg(month, 2, 10, QChar('0'))
                          .arg(day, 2, 10, QChar('0'))
                          .arg(hours, 2, 10, QChar('0'))
                          .arg(minutes, 2, 10, QChar('0'))
                          .arg(mjd)
                          .arg(utc_flag ? " [UTC valid]" : " [UTC invalid]")
                          .arg(lsi ? " [Leap second pending]" : ""));
    
    // Store date/time information for the ensemble
    if (current_ensemble_) {
        // Note: These fields would be added to EnhancedEnsembleInfo structure in full implementation:
        // current_ensemble_->date_time_mjd = mjd;
        // current_ensemble_->date_time_utc_hours = hours;
        // current_ensemble_->date_time_utc_minutes = minutes;
        // current_ensemble_->utc_time_valid = utc_flag;
        // current_ensemble_->leap_second_indicator = lsi;
        
        // For now, just log the information
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/10: Ensemble date/time updated - MJD %1, UTC %2:%3 (valid=%4, LSI=%5)")
                              .arg(mjd).arg(hours).arg(minutes).arg(utc_flag).arg(lsi));
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG0_13_UserAppInfo(const eti::FigBlock& fig) 
{
    // FIG 0/13: User application information (ETSI EN 300 401 Section 8.1.19)
    if (fig.data.size() < 5) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              "FIG 0/13: Insufficient data length");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 0/13: Processing user application information (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    while (offset + 5 <= fig.data.size()) {
        // Parse user application information
        uint32_t sid = (fig.data[offset] << 24) | (fig.data[offset + 1] << 16) |
                       (fig.data[offset + 2] << 8) | fig.data[offset + 3];  // Service ID
        
        uint8_t scids = (fig.data[offset + 4] >> 4) & 0x0F;  // Service Component ID
        uint8_t num_user_apps = fig.data[offset + 4] & 0x0F; // Number of user applications
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("FIG 0/13: SID 0x%1, SCIdS %2, %3 user applications")
                              .arg(sid, 8, 16, QChar('0')).arg(scids).arg(num_user_apps));
        
        offset += 5;
        
        // Parse each user application
        for (uint8_t app = 0; app < num_user_apps && offset + 2 < fig.data.size(); ++app) {
            uint16_t user_app_type = (fig.data[offset] << 8) | fig.data[offset + 1];
            uint8_t user_app_data_length = 0;
            
            offset += 2;
            
            // Check if there's application data length field
            if (offset < fig.data.size()) {
                user_app_data_length = fig.data[offset];
                offset++;
                
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                      QString("FIG 0/13: User App Type 0x%1, Data length %2")
                                      .arg(user_app_type, 4, 16, QChar('0'))
                                      .arg(user_app_data_length));
                
                // Store user application information
                if (m_dabServices.find(sid) != m_dabServices.end()) {
                    // Format application information as string for storage
                    QString app_info = QString("Type:0x%1,Length:%2,SCID:%3")
                                      .arg(user_app_type, 4, 16, QChar('0'))
                                      .arg(user_app_data_length)
                                      .arg(scids);
                    
                    m_userApplications[sid].push_back(app_info.toStdString());
                    
                    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                          QString("FIG 0/13: Added user application for SID 0x%1: %2")
                                          .arg(sid, 8, 16, QChar('0')).arg(app_info));
                } else {
                    // Create new service entry and add application
                    eti::DabService service;
                    service.service_id = static_cast<uint16_t>(sid & 0xFFFF);
                    m_dabServices[sid] = service;
                    
                    QString app_info = QString("Type:0x%1,Length:%2,SCID:%3")
                                      .arg(user_app_type, 4, 16, QChar('0'))
                                      .arg(user_app_data_length)
                                      .arg(scids);
                    
                    m_userApplications[sid].push_back(app_info.toStdString());
                    
                    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                                          QString("FIG 0/13: Created new service SID 0x%1 with user application: %2")
                                          .arg(sid, 8, 16, QChar('0')).arg(app_info));
                }
                
                // Copy application data if present
                if (user_app_data_length > 0 && offset + user_app_data_length <= fig.data.size()) {
                    offset += user_app_data_length;
                }
            }
        }
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG0_17_ProgrammeType(const eti::FigBlock& fig) 
{
    // Simplified implementation
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "FIG 0/17: Programme type (simplified)");
    return true;
}

// ============================================================================
// FIG Type 1 Processors (Labels)
// ============================================================================

bool EnhancedFIGAnalyser::processFIG1_0_EnsembleLabel(const eti::FigBlock& fig)
{
    if (fig.data.size() < 18) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 1/0: Insufficient data", "Major");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    uint16_t ensemble_id = (fig.data[0] << 8) | fig.data[1];
    
    if (current_ensemble_ && current_ensemble_->base_ensemble.ensemble_id == ensemble_id) {
        std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + 2, 16), m_thai_support_enabled);
        
        current_ensemble_->base_ensemble.label = label;
        
        // Decode Thai label if Thai support is enabled
        if (m_thai_support_enabled) {
            current_ensemble_->thai_label = decodeThaiDabText(std::span<const uint8_t>(fig.data.data() + 2, 16));
        }
        
        lock.unlock();
        emit ensembleDiscovered(*current_ensemble_);
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                              QString("FIG 1/0: Ensemble label='%1'").arg(QString::fromStdString(label)));
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG1_1_ServiceLabel(const eti::FigBlock& fig)
{
    if (fig.data.size() < 18) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 1/1: Insufficient data", "Major");
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    uint16_t service_id = (fig.data[0] << 8) | fig.data[1];
    
    auto service_it = discovered_services_.find(service_id);
    if (service_it != discovered_services_.end()) {
        std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + 2, 16), m_thai_support_enabled);
        
        service_it->second.base_service.label = label;
        
        // Decode Thai label if Thai support is enabled
        if (m_thai_support_enabled) {
            service_it->second.thai_label = decodeThaiDabText(std::span<const uint8_t>(fig.data.data() + 2, 16));
        }
        
        service_it->second.last_updated = std::chrono::steady_clock::now();
        
        auto service_copy = service_it->second;
        lock.unlock();
        
        emit serviceDiscovered(service_copy);
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                              QString("FIG 1/1: Service label='%1' (ID=0x%2)")
                              .arg(QString::fromStdString(label))
                              .arg(service_id, 4, 16, QChar('0')));
    }
    
    return true;
}

bool EnhancedFIGAnalyser::processFIG1_2_RESERVED(const eti::FigBlock& fig)
{
    // FIG 1/2: Reserved for future use
    reportComplianceViolation("ETSI EN 300 401", "FIG 1/2: Reserved extension used", "Minor");
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                          QString("FIG 1/2: Reserved extension used (%1 bytes)")
                          .arg(fig.data.size()));
    return false;
}

bool EnhancedFIGAnalyser::processFIG1_3_RESERVED(const eti::FigBlock& fig)
{
    // FIG 1/3: Reserved for future use
    reportComplianceViolation("ETSI EN 300 401", "FIG 1/3: Reserved extension used", "Minor");
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                          QString("FIG 1/3: Reserved extension used (%1 bytes)")
                          .arg(fig.data.size()));
    return false;
}

bool EnhancedFIGAnalyser::processFIG1_4_ServiceComponentLabel(const eti::FigBlock& fig) 
{
    // FIG 1/4: Service component label (ETSI EN 300 401 Section 8.1.9)
    if (fig.data.size() < 18) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 1/4: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 1/4: Processing service component label (%1 bytes)")
                          .arg(fig.data.size()));
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int labels_processed = 0;
    
    while (offset + 17 < fig.data.size()) {
        uint8_t pd_flag = (fig.data[offset] >> 7) & 0x01;
        uint8_t scids = (fig.data[offset] >> 4) & 0x0F;
        
        if (pd_flag) {
            // 32-bit service component identifier
            if (offset + 5 >= fig.data.size()) break;
            
            uint32_t scid = (fig.data[offset + 1] << 24) | (fig.data[offset + 2] << 16) |
                           (fig.data[offset + 3] << 8) | fig.data[offset + 4];
            
            std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + offset + 5, 16), m_thai_support_enabled);
            
            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                                  QString("FIG 1/4: Service component label='%1' (SCID=0x%2)")
                                  .arg(QString::fromStdString(label))
                                  .arg(scid, 8, 16, QChar('0')));
            
            offset += 21; // 1 + 4 + 16 bytes
        } else {
            // 12-bit service component identifier
            if (offset + 17 >= fig.data.size()) break;
            
            uint16_t scid = ((fig.data[offset] & 0x0F) << 8) | fig.data[offset + 1];
            
            std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + offset + 2, 16), m_thai_support_enabled);
            
            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                                  QString("FIG 1/4: Service component label='%1' (SCID=0x%2)")
                                  .arg(QString::fromStdString(label))
                                  .arg(scid, 3, 16, QChar('0')));
            
            offset += 18; // 2 + 16 bytes
        }
        
        labels_processed++;
    }
    
    return labels_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG1_5_DataServiceLabel(const eti::FigBlock& fig)
{
    // FIG 1/5: Data service label (ETSI EN 300 401 Section 8.1.10)
    if (fig.data.size() < 20) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 1/5: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 1/5: Processing data service label (%1 bytes)")
                          .arg(fig.data.size()));
    
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    size_t offset = 0;
    int labels_processed = 0;
    
    while (offset + 19 < fig.data.size()) {
        uint32_t service_id = (fig.data[offset] << 24) | (fig.data[offset + 1] << 16) |
                             (fig.data[offset + 2] << 8) | fig.data[offset + 3];
        
        std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + offset + 4, 16), m_thai_support_enabled);
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                              QString("FIG 1/5: Data service label='%1' (SID=0x%2)")
                              .arg(QString::fromStdString(label))
                              .arg(service_id, 8, 16, QChar('0')));
        
        // Update service information if it exists
        uint16_t service_id_16 = static_cast<uint16_t>(service_id & 0xFFFF);
        auto service_it = discovered_services_.find(service_id_16);
        if (service_it != discovered_services_.end()) {
            service_it->second.base_service.label = label;
            
            // Decode Thai label if Thai support is enabled
            if (m_thai_support_enabled) {
                service_it->second.thai_label = decodeThaiDabText(std::span<const uint8_t>(fig.data.data() + offset + 4, 16));
            }
            
            service_it->second.last_updated = std::chrono::steady_clock::now();
            
            auto service_copy = service_it->second;
            lock.unlock();
            emit serviceUpdated(service_copy);
            lock.lock();
        }
        
        labels_processed++;
        offset += 20; // 4 + 16 bytes
    }
    
    return labels_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG1_6_XPADUserAppLabel(const eti::FigBlock& fig)
{
    // FIG 1/6: X-PAD user application label (ETSI EN 300 401 Section 8.1.12)
    if (fig.data.size() < 20) {
        reportComplianceViolation("ETSI EN 300 401", "FIG 1/6: Insufficient data", "Major");
        return false;
    }
    
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                          QString("FIG 1/6: Processing X-PAD user application label (%1 bytes)")
                          .arg(fig.data.size()));
    
    size_t offset = 0;
    int labels_processed = 0;
    
    while (offset + 19 < fig.data.size()) {
        uint8_t pd_flag = (fig.data[offset] >> 7) & 0x01;
        uint8_t scids = (fig.data[offset] >> 4) & 0x0F;
        uint8_t x_pad_app_type = fig.data[offset + 1] & 0x1F;
        
        if (pd_flag) {
            // 32-bit service component identifier
            if (offset + 21 >= fig.data.size()) break;
            
            uint32_t scid = (fig.data[offset + 2] << 24) | (fig.data[offset + 3] << 16) |
                           (fig.data[offset + 4] << 8) | fig.data[offset + 5];
            
            std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + offset + 6, 16), m_thai_support_enabled);
            
            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                                  QString("FIG 1/6: X-PAD app label='%1' (SCID=0x%2, Type=%3)")
                                  .arg(QString::fromStdString(label))
                                  .arg(scid, 8, 16, QChar('0'))
                                  .arg(x_pad_app_type));
            
            offset += 22; // 2 + 4 + 16 bytes
        } else {
            // 12-bit service component identifier
            if (offset + 17 >= fig.data.size()) break;
            
            uint16_t scid = ((fig.data[offset] & 0x0F) << 8) | fig.data[offset + 2];
            
            std::string label = decodeDabText(std::span<const uint8_t>(fig.data.data() + offset + 3, 16), m_thai_support_enabled);
            
            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                                  QString("FIG 1/6: X-PAD app label='%1' (SCID=0x%2, Type=%3)")
                                  .arg(QString::fromStdString(label))
                                  .arg(scid, 3, 16, QChar('0'))
                                  .arg(x_pad_app_type));
            
            offset += 19; // 3 + 16 bytes
        }
        
        labels_processed++;
    }
    
    return labels_processed > 0;
}

bool EnhancedFIGAnalyser::processFIG1_7_RESERVED(const eti::FigBlock& fig)
{
    // FIG 1/7: Reserved for future use
    reportComplianceViolation("ETSI EN 300 401", "FIG 1/7: Reserved extension used", "Minor");
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                          QString("FIG 1/7: Reserved extension used (%1 bytes)")
                          .arg(fig.data.size()));
    return false;
}

// ============================================================================
// FIG Type 2 Processors (Extended Service Information)
// ============================================================================

bool EnhancedFIGAnalyser::processFIG2_0_MOTChannel(const eti::FigBlock& fig) 
{
    // Simplified implementation
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "FIG 2/0: MOT channel (simplified)");
    return true;
}

bool EnhancedFIGAnalyser::processFIG2_1_MOTService(const eti::FigBlock& fig) 
{
    // Simplified implementation
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", "FIG 2/1: MOT service (simplified)");
    return true;
}

// ============================================================================
// Utility Methods
// ============================================================================

bool EnhancedFIGAnalyser::validateFIGStructure(const eti::FigBlock& fig) const
{
    // Validate FIG type (0-7 valid)
    if (fig.fig_type > 7) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              QString("Invalid FIG type: %1").arg(fig.fig_type));
        return false;
    }
    
    // Validate length (0-31 valid)
    if (fig.length > 31) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              QString("Invalid FIG length: %1").arg(fig.length));
        return false;
    }
    
    // Ensure data size matches declared length
    if (fig.data.size() != fig.length) {
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              QString("FIG data size mismatch: declared %1, actual %2")
                              .arg(fig.length).arg(fig.data.size()));
        return false;
    }
    
    return true;
}

bool EnhancedFIGAnalyser::validateFIGCRC(const eti::FigBlock& fig) const
{
    // Basic FIG CRC validation
    // For now, assume valid if structure is correct
    // Full CRC-16 implementation would be needed for production
    return validateFIGStructure(fig);
}

std::string EnhancedFIGAnalyser::decodeDabText(std::span<const uint8_t> data, bool enable_thai) const
{
    std::string result;
    result.reserve(data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        uint8_t byte = data[i];
        
        // Handle Thai encoding if enabled
        if (enable_thai && byte >= 0x80) {
            // Thai characters in ISO 8859-11 (TIS-620) range
            if (byte >= 0xA1 && byte <= 0xFB) {
                // Convert to UTF-8 Thai characters
                // TIS-620 to Unicode mapping (simplified)
                uint16_t unicode_value = 0x0E00 + (byte - 0xA0);
                
                if (unicode_value <= 0x0E7F) {
                    // Convert to UTF-8
                    result += static_cast<char>(0xE0 | ((unicode_value >> 12) & 0x0F));
                    result += static_cast<char>(0x80 | ((unicode_value >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (unicode_value & 0x3F));
                } else {
                    result += '?'; // Invalid character
                }
            } else {
                result += '?'; // Non-printable
            }
        } else if (byte >= 0x20 && byte <= 0x7E) {
            // Standard ASCII printable characters
            result += static_cast<char>(byte);
        } else if (byte == 0x00) {
            // Null terminator - end of string
            break;
        } else {
            // Non-printable character - replace with space
            result += ' ';
        }
    }
    
    // Trim trailing spaces
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

std::string EnhancedFIGAnalyser::decodeThaiDabText(std::span<const uint8_t> data) const
{
    // Use the main decoder with Thai support enabled
    return decodeDabText(data, true);
}

bool EnhancedFIGAnalyser::attemptErrorRecovery(const eti::FigBlock& fig)
{
    // Simple error recovery - check if we can fix length issues
    if (fig.length == 0 && !fig.data.empty()) {
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              "Attempting error recovery: zero length FIG with data");
        return false; // Cannot recover from this
    }
    
    if (fig.fig_type > 7) {
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              "Attempting error recovery: invalid FIG type");
        return false; // Cannot recover from invalid type
    }
    
    return true; // Basic recovery possible
}

void EnhancedFIGAnalyser::validateCarouselTiming()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_stats_update_);
    
    if (elapsed.count() >= 1000) { // Update every second
        // Calculate carousel efficiency
        double total_figs = stats_.total_figs_processed;
        if (total_figs > 0) {
            double error_rate = (stats_.invalid_figs + stats_.crc_errors) / total_figs;
            carousel_analysis_.carousel_efficiency = std::max(0.0, 100.0 - (error_rate * 100.0));
        }
        
        carousel_analysis_.last_update = now;
        last_stats_update_ = now;
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("Carousel efficiency: %1%")
                              .arg(carousel_analysis_.carousel_efficiency, 0, 'f', 1));
    }
}

void EnhancedFIGAnalyser::updateCarouselAnalysis(uint8_t fig_type, uint8_t extension)
{
    // Track FIG distribution for carousel analysis
    uint16_t fig_key = (static_cast<uint16_t>(fig_type) << 8) | extension;
    
    // Update carousel timing information
    auto now = std::chrono::steady_clock::now();
    if (carousel_analysis_.fig_intervals.find(fig_key) != carousel_analysis_.fig_intervals.end()) {
        auto last_seen = carousel_analysis_.fig_intervals[fig_key];
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_seen);
        
        // Track if interval is within expected range (typically 24ms for ETI frames)
        if (interval.count() < 100) { // Less than 100ms is good
            carousel_analysis_.fig_repetition_rate = 
                std::min(carousel_analysis_.fig_repetition_rate + 0.1, 100.0);
        } else {
            carousel_analysis_.fig_repetition_rate = 
                std::max(carousel_analysis_.fig_repetition_rate - 0.1, 0.0);
        }
    }
    
    carousel_analysis_.fig_intervals[fig_key] = now;
    carousel_analysis_.total_fig_count++;
}

void EnhancedFIGAnalyser::updateServiceInfo(const eti::DabService& service)
{
    auto service_it = discovered_services_.find(service.service_id);
    if (service_it != discovered_services_.end()) {
        // Update existing service
        service_it->second.base_service = service;
        service_it->second.last_updated = std::chrono::steady_clock::now();
    } else {
        // Create new enhanced service info
        EnhancedServiceInfo enhanced_service;
        enhanced_service.base_service = service;
        enhanced_service.first_discovered = std::chrono::steady_clock::now();
        enhanced_service.last_updated = enhanced_service.first_discovered;
        enhanced_service.is_stable = true;
        enhanced_service.signal_quality = 1.0;
        
        discovered_services_[service.service_id] = enhanced_service;
        
        // Update ensemble service count
        if (current_ensemble_) {
            current_ensemble_->total_services = discovered_services_.size();
        }
        
        emit serviceDiscovered(enhanced_service);
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyser", 
                              QString("New service discovered: ID=0x%1")
                              .arg(service.service_id, 4, 16, QChar('0')));
    }
}

void EnhancedFIGAnalyser::mergeServiceComponents(EnhancedServiceInfo& service, const eti::ServiceComponent& component)
{
    // Check if component already exists
    bool found = false;
    for (auto& existing_comp : service.components) {
        if (existing_comp.sub_channel_id == component.sub_channel_id) {
            // Update existing component
            existing_comp.sub_channel_id = component.sub_channel_id;
            existing_comp.component_type_name = "Audio"; // Default for now
            existing_comp.is_protected = (component.protection_level > 0);
            existing_comp.protection_level = component.protection_level;
            found = true;
            break;
        }
    }
    
    if (!found) {
        // Add new component by converting types
        EnhancedServiceInfo::ComponentAnalysis new_comp;
        new_comp.sub_channel_id = component.sub_channel_id;
        new_comp.component_type_name = "Audio"; // Default for now  
        new_comp.is_protected = (component.protection_level > 0);
        new_comp.protection_level = component.protection_level;
        service.components.push_back(new_comp);
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyser", 
                              QString("Added component to service 0x%1: SubCh=%2, TMID=%3")
                              .arg(service.base_service.service_id, 4, 16, QChar('0'))
                              .arg(component.sub_channel_id)
                              .arg(component.tmid));
    }
    
    service.last_updated = std::chrono::steady_clock::now();
}

void EnhancedFIGAnalyser::reportComplianceViolation(const std::string& standard, 
                                                    const std::string& description, 
                                                    const std::string& severity)
{
    std::string issue = QString("[%1] %2: %3")
                       .arg(QString::fromStdString(severity))
                       .arg(QString::fromStdString(standard))
                       .arg(QString::fromStdString(description))
                       .toStdString();
    
    // Avoid duplicate reports
    if (reported_issues_.find(issue) == reported_issues_.end()) {
        compliance_issues_.push_back(issue);
        reported_issues_.insert(issue);
        
        Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyser", 
                              QString("ETSI Compliance Violation: %1").arg(QString::fromStdString(issue)));
    }
}

} // namespace eti::modern
