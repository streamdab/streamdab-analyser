/**
 * @file enhanced_fig_analyzer.cpp
 * @brief Enhanced FIG Analyzer Implementation
 */

#include "enhanced_fig_analyzer.h"
#include "utils/logger.h"
#include <algorithm>
#include <bitset>
#include <sstream>
#include <iomanip>
#include <shared_mutex>

namespace eti::modern {

EnhancedFIGAnalyzer::EnhancedFIGAnalyzer(QObject* parent)
    : QObject(parent)
    , analysis_start_time_(std::chrono::steady_clock::now())
    , last_stats_update_(analysis_start_time_)
{
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                          "Enhanced FIG Analyzer initialized with C++20 optimizations");
}

EnhancedFIGAnalyzer::~EnhancedFIGAnalyzer() {
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                          QString("FIG Analyzer destroyed - Stats: %1 FIGs processed, %.1f%% compliance")
                          .arg(stats_.total_figs_processed)
                          .arg(stats_.compliance_score));
}

bool EnhancedFIGAnalyzer::initialize(bool enable_carousel_analysis, bool enable_thai_support) {
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                          "Initializing Enhanced FIG Analyzer");
    
    try {
        carousel_analysis_enabled_ = enable_carousel_analysis;
        thai_support_enabled_ = enable_thai_support;
        error_recovery_enabled_ = true;
        
        // Initialize carousel analysis
        if (carousel_analysis_enabled_) {
            carousel_analysis_.analysis_start = std::chrono::steady_clock::now();
            carousel_analysis_.analysis_duration = std::chrono::milliseconds(0);
            carousel_analysis_.carousel_efficiency = 0.0;
            
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                                  "FIG carousel analysis enabled");
        }
        
        // Reset all statistics
        reset();
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                              QString("Enhanced FIG Analyzer initialized - Carousel: %1, Thai: %2")
                              .arg(carousel_analysis_enabled_ ? "ON" : "OFF")
                              .arg(thai_support_enabled_ ? "ON" : "OFF"));
        
        return true;
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EnhancedFIGAnalyzer", 
                              QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

std::vector<eti::FigBlock> EnhancedFIGAnalyzer::processFICData(const eti::EtiFicField& fic_data) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<eti::FigBlock> fig_blocks;
    
    try {
        // Decode FIG blocks from FIC data
        fig_blocks = fic_data.decode_fig_blocks();
        
        // Process each FIG block
        for (const auto& fig : fig_blocks) {
            if (validateFIGStructure(fig)) {
                if (analyzeFIGBlock(fig)) {
                    stats_.total_figs_processed++;
                    
                    // Update type-specific counters
                    switch (fig.fig_type) {
                        case 0: stats_.fig_type_0_count++; break;
                        case 1: stats_.fig_type_1_count++; break;
                        case 2: stats_.fig_type_2_count++; break;
                    }
                    
                    // Update carousel analysis
                    if (carousel_analysis_enabled_) {
                        updateCarouselAnalysis(fig.fig_type, fig.get_extension());
                    }
                } else {
                    stats_.parsing_errors++;
                    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyzer", 
                                          QString("Failed to parse FIG %1/%2")
                                          .arg(fig.fig_type).arg(fig.get_extension()));
                }
            } else {
                stats_.invalid_figs++;
                
                // Attempt error recovery if enabled
                if (error_recovery_enabled_ && attemptErrorRecovery(fig)) {
                    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                                          "Successfully recovered from FIG parsing error");
                }
            }
        }
        
        // Update performance statistics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        stats_.total_processing_time_ns += processing_time.count();
        
        if (processing_time.count() > stats_.peak_processing_time_ns) {
            stats_.peak_processing_time_ns = processing_time.count();
        }
        
        // Update average processing time
        if (stats_.total_figs_processed > 0) {
            stats_.average_processing_time_ns = 
                static_cast<double>(stats_.total_processing_time_ns) / stats_.total_figs_processed;
        }
        
        Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                              QString("Processed %1 FIG blocks in %2 ns")
                              .arg(fig_blocks.size())
                              .arg(processing_time.count()));
        
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EnhancedFIGAnalyzer", 
                              QString("FIC data processing failed: %1").arg(e.what()));
        stats_.parsing_errors++;
    }
    
    return fig_blocks;
}

bool EnhancedFIGAnalyzer::analyzeFIGBlock(const eti::FigBlock& fig_block) {
    try {
        switch (fig_block.fig_type) {
            case 0:
                return processFIG0(fig_block);
            case 1:
                return processFIG1(fig_block);
            case 2:
                return processFIG2(fig_block);
            default:
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                                      QString("Unsupported FIG type: %1").arg(fig_block.fig_type));
                return false;
        }
    }
    catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "EnhancedFIGAnalyzer", 
                              QString("FIG block analysis failed: %1").arg(e.what()));
        return false;
    }
}

std::optional<EnhancedEnsembleInfo> EnhancedFIGAnalyzer::getCurrentEnsemble() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return current_ensemble_;
}

FIGProcessingStats EnhancedFIGAnalyzer::getProcessingStats() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    // Create a copy with current values (protected by shared_mutex)
    FIGProcessingStats copy = stats_;
    
    return copy;
}

std::vector<EnhancedServiceInfo> EnhancedFIGAnalyzer::getDiscoveredServices() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    std::vector<EnhancedServiceInfo> services;
    services.reserve(discovered_services_.size());
    
    for (const auto& [service_id, service_info] : discovered_services_) {
        services.push_back(service_info);
    }
    
    return services;
}

std::optional<EnhancedServiceInfo> EnhancedFIGAnalyzer::getServiceById(uint16_t service_id) const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    auto it = discovered_services_.find(service_id);
    if (it != discovered_services_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

FIGCarouselAnalysis EnhancedFIGAnalyzer::getCarouselAnalysis() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    
    FIGCarouselAnalysis analysis = carousel_analysis_;
    analysis.analysis_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - analysis.analysis_start);
    
    return analysis;
}

void EnhancedFIGAnalyzer::reset() {
    std::unique_lock<std::shared_mutex> lock(data_mutex_);
    
    // Reset state
    current_ensemble_.reset();
    discovered_services_.clear();
    subchannels_.clear();
    compliance_issues_.clear();
    reported_issues_.clear();
    
    // Reset statistics
    stats_ = FIGProcessingStats{};
    
    // Reset carousel analysis
    if (carousel_analysis_enabled_) {
        carousel_analysis_ = FIGCarouselAnalysis{};
        carousel_analysis_.analysis_start = std::chrono::steady_clock::now();
    }
    
    analysis_start_time_ = std::chrono::steady_clock::now();
    last_stats_update_ = analysis_start_time_;
    
    Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                          "FIG Analyzer state reset");
}

double EnhancedFIGAnalyzer::getETSIComplianceScore() const {
    return stats_.compliance_score;
}

std::vector<std::string> EnhancedFIGAnalyzer::getComplianceIssues() const {
    std::shared_lock<std::shared_mutex> lock(data_mutex_);
    return compliance_issues_;
}

bool EnhancedFIGAnalyzer::validateFIGTiming(uint8_t fig_type, uint8_t extension, 
                                           std::chrono::milliseconds interval) const {
    switch (fig_type) {
        case 0:
            switch (extension) {
                case 0: return interval <= FIG_0_0_MAX_INTERVAL;
                case 1: return interval <= FIG_0_1_MAX_INTERVAL;
                case 2: return interval <= FIG_0_2_MAX_INTERVAL;
                default: return true; // No specific timing requirements
            }
        case 1:
            switch (extension) {
                case 0: return interval <= FIG_1_0_MAX_INTERVAL;
                case 1: return interval <= FIG_1_1_MAX_INTERVAL;
                default: return true;
            }
        default:
            return true;
    }
}

// Private implementation methods

bool EnhancedFIGAnalyzer::processFIG0(const eti::FigBlock& fig) {
    uint8_t extension = fig.get_extension();
    
    switch (extension) {
        case 0: return processFIG0_0_EnsembleInfo(fig);
        case 1: return processFIG0_1_SubchannelOrg(fig);
        case 2: return processFIG0_2_ServiceOrg(fig);
        case 3: return processFIG0_3_ServiceComponent(fig);
        case 5: return processFIG0_5_ServiceComponentLang(fig);
        case 8: return processFIG0_8_ServiceComponentGlobal(fig);
        case 9: return processFIG0_9_CountryLTOInt(fig);
        case 10: return processFIG0_10_DateTime(fig);
        case 13: return processFIG0_13_UserAppInfo(fig);
        case 17: return processFIG0_17_ProgrammeType(fig);
        default:
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                                  QString("Unsupported FIG 0/%1").arg(extension));
            return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG1(const eti::FigBlock& fig) {
    uint8_t extension = fig.get_extension();
    
    switch (extension) {
        case 0: return processFIG1_0_EnsembleLabel(fig);
        case 1: return processFIG1_1_ServiceLabel(fig);
        case 4: return processFIG1_4_ServiceComponentLabel(fig);
        default:
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                                  QString("Unsupported FIG 1/%1").arg(extension));
            return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG2(const eti::FigBlock& fig) {
    uint8_t extension = fig.get_extension();
    
    switch (extension) {
        case 0: return processFIG2_0_MOTChannel(fig);
        case 1: return processFIG2_1_MOTService(fig);
        default:
            Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                                  QString("Unsupported FIG 2/%1").arg(extension));
            return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG0_0_EnsembleInfo(const eti::FigBlock& fig) {
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 "FIG 0/0: Insufficient data length", "Error");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(data_mutex_);
        
        // Parse ensemble information
        uint16_t ensemble_id = (fig.data[0] << 8) | fig.data[1];
        uint8_t country_id = (fig.data[2] >> 4) & 0x0F;
        uint8_t extended_country_code = fig.data[2] & 0x0F;
        
        // Create or update ensemble info
        if (!current_ensemble_.has_value()) {
            current_ensemble_ = EnhancedEnsembleInfo{};
            current_ensemble_->first_discovered = std::chrono::steady_clock::now();
        }
        
        current_ensemble_->base_ensemble.ensemble_id = ensemble_id;
        current_ensemble_->base_ensemble.country_id = country_id;
        current_ensemble_->base_ensemble.extended_country_code = extended_country_code;
        current_ensemble_->last_updated = std::chrono::steady_clock::now();
        
        // Parse additional fields if available
        if (fig.data.size() >= 6) {
            current_ensemble_->base_ensemble.cif_count = fig.data[3];
            current_ensemble_->base_ensemble.occurrence_change = (fig.data[4] >> 4) & 0x0F;
            current_ensemble_->base_ensemble.alarm_flag = (fig.data[4] & 0x01) != 0;
        }
        
        lock.unlock();
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                              QString("FIG 0/0: Ensemble ID=0x%1, Country=0x%2, ECC=0x%3")
                              .arg(ensemble_id, 4, 16, QChar('0'))
                              .arg(country_id, 2, 16, QChar('0'))
                              .arg(extended_country_code, 2, 16, QChar('0')));
        
        emit ensembleDiscovered(current_ensemble_.value());
        return true;
        
    }
    catch (const std::exception& e) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 QString("FIG 0/0 parsing error: %1").arg(e.what()).toStdString(), 
                                 "Error");
        return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG0_1_SubchannelOrg(const eti::FigBlock& fig) {
    if (fig.data.size() < 3) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 "FIG 0/1: Insufficient data length", "Error");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(data_mutex_);
        
        size_t offset = 0;
        int subchannels_parsed = 0;
        
        while (offset + 2 < fig.data.size()) {
            eti::SubChannelInfo subchannel;
            
            // Parse subchannel ID and start address
            subchannel.sub_channel_id = (fig.data[offset] >> 2) & 0x3F;
            subchannel.start_address = ((fig.data[offset] & 0x03) << 8) | fig.data[offset + 1];
            
            if (offset + 2 >= fig.data.size()) break;
            
            // Check form type (short/long)
            if (fig.data[offset + 2] & 0x80) {
                // Long form (UEP)
                if (offset + 3 >= fig.data.size()) break;
                
                subchannel.size = ((fig.data[offset + 2] & 0x03) << 8) | fig.data[offset + 3];
                subchannel.protection_level = (fig.data[offset + 2] >> 2) & 0x1F;
                subchannel.uep_flag = true;
                offset += 4;
            } else {
                // Short form (EEP)
                uint8_t size_protection = fig.data[offset + 2];
                subchannel.size = (size_protection & 0x3F) * 8;
                subchannel.protection_level = ((size_protection >> 6) & 0x03) + 1;
                subchannel.uep_flag = false;
                offset += 3;
            }
            
            // Validate subchannel
            if (subchannel.validate_sub_channel_id() && subchannel.validate_boundaries()) {
                subchannels_[subchannel.sub_channel_id] = subchannel;
                subchannels_parsed++;
                
                Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                                      QString("FIG 0/1: SubCh %1, Start=%2, Size=%3 CUs, Protection=%4")
                                      .arg(subchannel.sub_channel_id)
                                      .arg(subchannel.start_address)
                                      .arg(subchannel.size)
                                      .arg(subchannel.protection_level));
            } else {
                reportComplianceViolation("ETSI EN 300 401", 
                                         QString("FIG 0/1: Invalid subchannel %1")
                                         .arg(subchannel.sub_channel_id).toStdString(), 
                                         "Warning");
            }
        }
        
        // Update ensemble subchannel information
        if (current_ensemble_.has_value()) {
            current_ensemble_->total_subchannels = subchannels_.size();
            
            // Calculate capacity utilization
            uint32_t total_capacity = 0;
            for (const auto& [id, subchannel] : subchannels_) {
                total_capacity += subchannel.size;
            }
            current_ensemble_->total_capacity_units = total_capacity;
            current_ensemble_->capacity_utilization = 
                static_cast<double>(total_capacity) / eti::MAX_CAPACITY_UNITS;
        }
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                              QString("FIG 0/1: Parsed %1 subchannels").arg(subchannels_parsed));
        
        return subchannels_parsed > 0;
        
    }
    catch (const std::exception& e) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 QString("FIG 0/1 parsing error: %1").arg(e.what()).toStdString(), 
                                 "Error");
        return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG0_2_ServiceOrg(const eti::FigBlock& fig) {
    if (fig.data.size() < 4) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 "FIG 0/2: Insufficient data length", "Error");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(data_mutex_);
        
        size_t offset = 0;
        int services_parsed = 0;
        
        while (offset + 3 < fig.data.size()) {
            eti::DabService service;
            
            // Parse service information
            service.service_id = (fig.data[offset] << 8) | fig.data[offset + 1];
            service.country_id = (fig.data[offset + 2] >> 4) & 0x0F;
            service.extended_country_code = fig.data[offset + 2] & 0x0F;
            
            if (offset + 3 < fig.data.size()) {
                service.is_programme = (fig.data[offset + 3] & 0x02) != 0;
            }
            
            // Validate service
            if (service.validate_service_id() && service.validate_country_code()) {
                updateServiceInfo(service);
                services_parsed++;
                
                Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                                      QString("FIG 0/2: Service ID=0x%1, Country=0x%2, Programme=%3")
                                      .arg(service.service_id, 4, 16, QChar('0'))
                                      .arg(service.country_id, 2, 16, QChar('0'))
                                      .arg(service.is_programme ? "Yes" : "No"));
            } else {
                reportComplianceViolation("ETSI EN 300 401", 
                                         QString("FIG 0/2: Invalid service ID 0x%1")
                                         .arg(service.service_id, 4, 16, QChar('0')).toStdString(), 
                                         "Warning");
            }
            
            offset += 4;
        }
        
        return services_parsed > 0;
        
    }
    catch (const std::exception& e) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 QString("FIG 0/2 parsing error: %1").arg(e.what()).toStdString(), 
                                 "Error");
        return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG0_3_ServiceComponent(const eti::FigBlock& fig) {
    // Implementation similar to other FIG processors
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", "Processing FIG 0/3: Service Component");
    return true;
}

bool EnhancedFIGAnalyzer::processFIG1_0_EnsembleLabel(const eti::FigBlock& fig) {
    if (fig.data.size() < 18) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 "FIG 1/0: Insufficient data length", "Error");
        return false;
    }
    
    try {
        std::unique_lock<std::shared_mutex> lock(data_mutex_);
        
        uint16_t ensemble_id = (fig.data[0] << 8) | fig.data[1];
        
        if (current_ensemble_.has_value() && 
            current_ensemble_->base_ensemble.ensemble_id == ensemble_id) {
            
            // Decode ensemble label
            std::span<const uint8_t> label_data(fig.data.data() + 2, 16);
            std::string label = decodeDabText(label_data, thai_support_enabled_);
            
            current_ensemble_->base_ensemble.label = label;
            
            // Decode Thai label if enabled
            if (thai_support_enabled_) {
                current_ensemble_->thai_label = decodeThaiDabText(label_data);
            }
            
            // Parse character flag if available
            if (fig.data.size() >= 20) {
                current_ensemble_->base_ensemble.character_flag = 
                    (fig.data[18] << 8) | fig.data[19];
            }
            
            Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                                  QString("FIG 1/0: Ensemble label='%1'")
                                  .arg(QString::fromStdString(label)));
            
            emit ensembleDiscovered(current_ensemble_.value());
            return true;
        }
        
        return false;
        
    }
    catch (const std::exception& e) {
        reportComplianceViolation("ETSI EN 300 401", 
                                 QString("FIG 1/0 parsing error: %1").arg(e.what()).toStdString(), 
                                 "Error");
        return false;
    }
}

bool EnhancedFIGAnalyzer::processFIG1_1_ServiceLabel(const eti::FigBlock& fig) {
    // Implementation for service labels
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", "Processing FIG 1/1: Service Label");
    return true;
}

// Additional FIG processors would be implemented similarly...

bool EnhancedFIGAnalyzer::validateFIGStructure(const eti::FigBlock& fig) const {
    return fig.is_valid() && fig.fig_type <= 7 && !fig.data.empty();
}

std::string EnhancedFIGAnalyzer::decodeDabText(std::span<const uint8_t> data, bool is_thai) const {
    std::string result;
    result.reserve(data.size());
    
    for (uint8_t byte : data) {
        if (byte == 0x00) break; // Null terminator
        if (byte >= 0x20 && byte <= 0x7E) { // Printable ASCII
            result += static_cast<char>(byte);
        } else if (is_thai && byte >= 0xA1 && byte <= 0xFE) {
            // Thai character range - simplified handling
            result += static_cast<char>(byte);
        } else if (byte == 0x20) {
            result += ' ';
        }
        // Add more character set handling as needed
    }
    
    // Remove trailing spaces
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

void EnhancedFIGAnalyzer::updateCarouselAnalysis(uint8_t fig_type, uint8_t extension) {
    if (!carousel_analysis_enabled_) return;
    
    uint16_t fig_key = (static_cast<uint16_t>(fig_type) << 8) | extension;
    auto now = std::chrono::steady_clock::now();
    
    auto& occurrence = carousel_analysis_.fig_occurrences[fig_key];
    
    if (occurrence.occurrence_count == 0) {
        // First occurrence
        occurrence.fig_type = fig_type;
        occurrence.extension = extension;
        occurrence.last_seen = now;
        occurrence.occurrence_count = 1;
        occurrence.meets_etsi_timing = true;
    } else {
        // Calculate interval
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - occurrence.last_seen);
        
        // Update statistics
        occurrence.occurrence_count++;
        occurrence.last_seen = now;
        
        if (occurrence.occurrence_count == 2) {
            occurrence.min_interval = interval;
            occurrence.max_interval = interval;
            occurrence.average_interval = interval;
        } else {
            occurrence.min_interval = std::min(occurrence.min_interval, interval);
            occurrence.max_interval = std::max(occurrence.max_interval, interval);
            
            // Update running average
            auto total_time = occurrence.average_interval * (occurrence.occurrence_count - 2) + interval;
            occurrence.average_interval = total_time / (occurrence.occurrence_count - 1);
        }
        
        // Validate timing
        occurrence.meets_etsi_timing = validateFIGTiming(fig_type, extension, interval);
        
        if (!occurrence.meets_etsi_timing) {
            std::string issue = QString("FIG %1/%2 timing violation: %3ms interval")
                               .arg(fig_type).arg(extension).arg(interval.count()).toStdString();
            carousel_analysis_.timing_violations.push_back(issue);
        }
    }
}

void EnhancedFIGAnalyzer::updateServiceInfo(const eti::DabService& service) {
    auto& enhanced_service = discovered_services_[service.service_id];
    
    if (enhanced_service.base_service.service_id == 0) {
        // New service
        enhanced_service.base_service = service;
        enhanced_service.first_discovered = std::chrono::steady_clock::now();
        
        Logger::instance().log(Logger::Info, "EnhancedFIGAnalyzer", 
                              QString("New service discovered: ID=0x%1")
                              .arg(service.service_id, 4, 16, QChar('0')));
        
        emit serviceDiscovered(enhanced_service);
    } else {
        // Update existing service
        enhanced_service.base_service = service;
        enhanced_service.last_updated = std::chrono::steady_clock::now();
        
        emit serviceUpdated(enhanced_service);
    }
}

void EnhancedFIGAnalyzer::reportComplianceViolation(const std::string& standard, 
                                                   const std::string& issue,
                                                   const std::string& severity) {
    // Avoid duplicate reports
    std::string issue_key = standard + ":" + issue;
    if (reported_issues_.find(issue_key) != reported_issues_.end()) {
        return;
    }
    
    reported_issues_.insert(issue_key);
    compliance_issues_.push_back(issue);
    
    stats_.etsi_violations++;
    
    // Update compliance score
    double penalty = (severity == "Error") ? 5.0 : 1.0;
    stats_.compliance_score = std::max(0.0, stats_.compliance_score - penalty);
    
    emit complianceIssue(QString::fromStdString(standard), 
                        QString::fromStdString(issue),
                        QString::fromStdString(severity));
    
    Logger::instance().log(Logger::Warning, "EnhancedFIGAnalyzer", 
                          QString("Compliance violation [%1]: %2 - %3")
                          .arg(QString::fromStdString(severity))
                          .arg(QString::fromStdString(standard))
                          .arg(QString::fromStdString(issue)));
}

bool EnhancedFIGAnalyzer::attemptErrorRecovery(const eti::FigBlock& fig) {
    // Simple error recovery - try to continue processing if possible
    Logger::instance().log(Logger::Debug, "EnhancedFIGAnalyzer", 
                          QString("Attempting error recovery for FIG %1/%2")
                          .arg(fig.fig_type).arg(fig.get_extension()));
    return false; // Basic implementation
}

// Placeholder implementations for remaining FIG processors
bool EnhancedFIGAnalyzer::processFIG0_5_ServiceComponentLang(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG0_8_ServiceComponentGlobal(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG0_9_CountryLTOInt(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG0_10_DateTime(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG0_13_UserAppInfo(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG0_17_ProgrammeType(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG1_4_ServiceComponentLabel(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG2_0_MOTChannel(const eti::FigBlock& fig) { return true; }
bool EnhancedFIGAnalyzer::processFIG2_1_MOTService(const eti::FigBlock& fig) { return true; }

std::string EnhancedFIGAnalyzer::decodeThaiDabText(std::span<const uint8_t> data) const {
    // Thai text decoding implementation would go here
    return decodeDabText(data, true);
}

bool EnhancedFIGAnalyzer::validateFIGCRC(const eti::FigBlock& fig) const {
    // CRC validation implementation
    return true; // Placeholder
}

bool FIGCarouselAnalysis::is_carousel_compliant() const {
    return timing_violations.empty();
}

std::vector<std::string> FIGCarouselAnalysis::get_compliance_issues() const {
    return timing_violations;
}

} // namespace eti::modern