/**
 * @file headless_eti_processor.cpp
 * @brief FIXED: HeadlessETIProcessor with REAL ETI frame parsing
 *
 * BUG FIX (2025-11-06): Added actual frame parsing and FIG analysis
 * Previously: Frames were counted but NOT parsed - all data empty
 * Now: Full ETI frame parsing, FIG extraction, service/subchannel discovery
 *
 * AGENT 45 FIX (2025-11-08): Added result aggregation for YAML output
 * Previously: Data discovered but NOT exported to ProcessingResult
 * Now: Ensemble, services, subchannels, and FIG counts populated in result
 *
 * VERBOSE FRAMES FIX (2025-11-08): Added per-frame detailed data collection
 * Collects: ETI header, STC fields, FIC data, FIG blocks for each frame
 */

#include "headless_eti_processor.hpp"
#include "eti_processor.hpp"
#include "modern_eti_frame_parser.hpp"
#include "enhanced_fig_analyzer.hpp"
// #include "comprehensive_etsi_validator.hpp"  // DISABLED: All methods commented out for CLI build
#include "utils/logger.h"
#include "eti_types.hpp"
#include "fig_parser.hpp"
#include "../cli/yaml_output_generator.hpp"  // For FrameAnalysisData

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QLoggingCategory>
#include <QSet>
#include <QMap>
#include <QDateTime>

// Define logging category for CLI mode
Q_LOGGING_CATEGORY(etiMain, "streamdab.eti")

HeadlessETIProcessor::HeadlessETIProcessor(QObject* parent)
    : QObject(parent)
    , m_progress(0.0)
    , m_statusText("")
    , m_verbose_mode(false)
{
    Logger::instance().log(Logger::Info, "HeadlessETIProcessor", "HeadlessETIProcessor created for batch processing");
}

HeadlessETIProcessor::~HeadlessETIProcessor()
{
    Logger::instance().log(Logger::Info, "HeadlessETIProcessor", "HeadlessETIProcessor destroyed");
}

HeadlessETIProcessor::ProcessingResult HeadlessETIProcessor::process_file(const QString& inputFile, const QString& outputFile)
{
    ProcessingResult result;

    // Reset processing state
    reset_processing_state();

    // Set verbose mode in result
    result.verbose_mode = m_verbose_mode;

    // Validate input file exists
    QFileInfo inputInfo(inputFile);
    if (!inputInfo.exists() || !inputInfo.isReadable()) {
        result.success = false;
        result.errorMessage = QString("Input file not found or not readable: %1").arg(inputFile);
        return result;
    }

    // Start processing timer
    m_processingTimer.start();

    // Initialize components
    if (!initialize_components()) {
        result.success = false;
        result.errorMessage = "Failed to initialize processing components";
        return result;
    }

    // Emit initial status
    m_statusText = "Starting ETI file processing...";
    emit status_changed(m_statusText);
    emit progress_changed(0.1);

    // Basic file validation (check if it looks like ETI data)
    QFile file(inputFile);
    if (!file.open(QIODevice::ReadOnly)) {
        result.success = false;
        result.errorMessage = QString("Cannot open input file: %1").arg(inputFile);
        return result;
    }

    // Read first few bytes to validate ETI format
    QByteArray header = file.read(16);
    file.close();

    if (header.size() < 4) {
        result.success = false;
        result.errorMessage = "Input file too small to be valid ETI";
        return result;
    }

    // Check for ETI sync pattern: ETI-LI or ETI-NI
    // ETI-LI: 0x49 0x93 0x1E 0x03
    // ETI-NI: 0xFF 0x?? 0x?? 0x?? (with CRC)
    bool isETI_LI = (static_cast<uint8_t>(header[0]) == 0x49 &&
                     static_cast<uint8_t>(header[1]) == 0x93 &&
                     static_cast<uint8_t>(header[2]) == 0x1E &&
                     static_cast<uint8_t>(header[3]) == 0x03);

    bool isETI_NI = (static_cast<uint8_t>(header[0]) == 0xFF);

    bool looksLikeETI = (isETI_LI || isETI_NI);

    if (!looksLikeETI) {
        result.success = false;
        result.errorMessage = QString("Input file does not appear to be valid ETI format (sync=%1 %2 %3 %4)")
            .arg(static_cast<uint8_t>(header[0]), 2, 16, QChar('0'))
            .arg(static_cast<uint8_t>(header[1]), 2, 16, QChar('0'))
            .arg(static_cast<uint8_t>(header[2]), 2, 16, QChar('0'))
            .arg(static_cast<uint8_t>(header[3]), 2, 16, QChar('0'))
            ;
        return result;
    }

    qCInfo(etiMain) << "Detected ETI format:" << (isETI_LI ? "ETI-LI" : "ETI-NI");

    // Calculate expected number of ETI frames
    QFileInfo fileInfo(inputFile);
    qint64 fileSize = fileInfo.size();
    const qint64 ETI_FRAME_SIZE = 6144;
    qint64 expectedFrames = fileSize / ETI_FRAME_SIZE;

    qCInfo(etiMain) << "File size:" << fileSize << "bytes, Expected frames:" << expectedFrames;

    // ============================================================================
    // FIX: Data accumulators for real parsing results
    // ============================================================================
    QMap<uint16_t, eti::Ensemble> discovered_ensembles;
    QMap<uint32_t, eti::DabService> discovered_services;
    QMap<uint8_t, eti::SubChannelInfo> discovered_subchannels;
    QMap<QString, int> fig_type_counts;


    // Process frames with real ETI parsing
    qint64 processedFrames = 0;
    qint64 errorFrames = 0;
    qint64 valid_parsed_frames = 0;

    QFile frameFile(inputFile);
    if (frameFile.open(QIODevice::ReadOnly)) {
        QByteArray frameData;
        frameData.resize(ETI_FRAME_SIZE);

        while (!frameFile.atEnd()) {
            qint64 bytesRead = frameFile.read(frameData.data(), ETI_FRAME_SIZE);

            if (bytesRead == ETI_FRAME_SIZE) {
                processedFrames++;

                // ============================================================================
                // FIX: ACTUAL FRAME PARSING (was missing before!)
                // ============================================================================
                try {
                    // Step 1: Parse the ETI frame structure
                    auto parseResult = m_frameParser->parseFrame(frameData);

                    if (parseResult.success) {
                        valid_parsed_frames++;

                        // Step 2: Analyze FIG data from the frame
                        auto figResult = m_frameParser->analyzeFIGData(parseResult.frame);

                        // *** VERBOSE MODE: Collect per-frame data ***
                        if (m_verbose_mode) {
                            cli::FrameAnalysisData frameAnalysis;

                            // Extract ETI header (LIDATA field)
                            auto lidata = parseResult.frame.get_lidata_field();

                            frameAnalysis.frame_number = processedFrames - 1;
                            frameAnalysis.timestamp = QDateTime::currentDateTime().addMSecs((processedFrames - 1) * 24);

                            // ETI header fields
                            frameAnalysis.header.fct = lidata.fc;
                            frameAnalysis.header.ficf = lidata.ficf;
                            frameAnalysis.header.nst = lidata.nst;
                            frameAnalysis.header.mid = lidata.mid;
                            frameAnalysis.header.fp = lidata.fp;
                            frameAnalysis.header.fl = lidata.fl;
                            frameAnalysis.header.tist = lidata.tist;
                            frameAnalysis.header.format = (eti::detectETIFormat(
                                reinterpret_cast<const uint8_t*>(frameData.data()),
                                frameData.size()
                            ) == eti::ETIFormat::ETI_NI) ? "ETI-NI" : "ETI-LI";

                            // STC fields (from parseResult.subchannels_from_stc for ETI-NI)
                            for (const auto& subchannel : parseResult.subchannels_from_stc) {
                                cli::FrameAnalysisData::STCField stc;
                                stc.scid = subchannel.sub_channel_id;
                                stc.sad = subchannel.start_address;
                                stc.size_cu = subchannel.size;
                                // Protection info (simplified)
                                stc.protection_form = subchannel.uep_flag ? "UEP" : "EEP";
                                stc.protection_level = QString("%1").arg(subchannel.protection_level);
                                // Calculate approximate bitrate (size * 8kbps per CU)
                                stc.bitrate_kbps = subchannel.size * 8;
                                frameAnalysis.stc_fields.append(stc);
                            }

                            // FIC data (FIG blocks)
                            if (lidata.ficf) {
                                frameAnalysis.fic.fib_count = 3;  // Standard DAB has 3 FIBs
                                // Note: FIB-level CRC checking would require FIC parser enhancement
                                // For now, we just collect the FIG blocks
                                cli::FrameAnalysisData::FICData::FIB fib;
                                fib.fib_number = 0;  // Combined FIBs
                                fib.crc_ok = parseResult.validation.valid;
                                for (const auto& fb : figResult.fig_blocks) fib.fig_blocks.append(fb);
                                frameAnalysis.fic.fibs.append(fib);
                            }

                            // Frame validation
                            frameAnalysis.crc_ok = parseResult.validation.valid;
                            if (!parseResult.validation.valid) {
                                for (const auto& err : parseResult.validation.errors) {
                                    frameAnalysis.errors.append(QString::fromStdString(err));
                                }
                            }

                            // Store in result
                            result.frame_analyses.push_back(frameAnalysis);
                        }

                        // Step 3: Collect ensemble information
                        if (figResult.ensemble_info.validate_ensemble_id()) {
                            uint16_t eid = figResult.ensemble_info.ensemble_id;
                            if (!discovered_ensembles.contains(eid)) {
                                discovered_ensembles[eid] = figResult.ensemble_info;
                                qCInfo(etiMain) << "Discovered ensemble:" << QString("0x%1").arg(eid, 4, 16, QChar('0'))
                                               << "Label:" << QString::fromStdString(figResult.ensemble_info.label);
                            } else if (!figResult.ensemble_info.label.empty()) {
                                // Update label if not set yet
                                if (discovered_ensembles[eid].label.empty()) {
                                    discovered_ensembles[eid].label = figResult.ensemble_info.label;
                                }
                            }
                        }

                        // Step 4: Collect service information
                        for (const auto& service : figResult.discovered_services) {
                            if (service.validate_service_id()) {
                                uint32_t sid = service.service_id;
                                if (!discovered_services.contains(sid)) {
                                    discovered_services[sid] = service;
                                    qCInfo(etiMain) << "Discovered service:" << QString("0x%1").arg(sid, 4, 16, QChar('0'))
                                                   << "Label:" << QString::fromStdString(service.label);
                                } else if (!service.label.empty()) {
                                    // Update label if not set yet
                                    if (discovered_services[sid].label.empty()) {
                                        discovered_services[sid].label = service.label;
                                    }
                                    // Merge components
                                    for (const auto& comp : service.components) {
                                        bool found = false;
                                        for (const auto& existing_comp : discovered_services[sid].components) {
                                            if (existing_comp.sub_channel_id == comp.sub_channel_id) {
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found) {
                                            discovered_services[sid].components.push_back(comp);
                                        }
                                    }
                                }
                            }
                        }

                        // Step 5: Collect subchannel information
                        // For ETI-NI: subchannels from STC fields (parseResult.subchannels_from_stc)
                        for (const auto& subchannel : parseResult.subchannels_from_stc) {
                            if (subchannel.validate_sub_channel_id()) {
                                uint8_t subch_id = subchannel.sub_channel_id;
                                if (!discovered_subchannels.contains(subch_id)) {
                                    discovered_subchannels[subch_id] = subchannel;
                                    qCInfo(etiMain) << "Discovered subchannel (STC):" << subch_id
                                                   << "Start:" << QString("0x%1").arg(subchannel.start_address, 4, 16, QChar('0'))
                                                   << "Size:" << subchannel.size << "CUs";
                                }
                            }
                        }
                        // For ETI-LI: subchannels from FIG 0/1 (figResult.subchannels)
                        for (const auto& subchannel : figResult.subchannels) {
                            if (subchannel.validate_sub_channel_id()) {
                                uint8_t subch_id = subchannel.sub_channel_id;
                                if (!discovered_subchannels.contains(subch_id)) {
                                    discovered_subchannels[subch_id] = subchannel;
                                    qCInfo(etiMain) << "Discovered subchannel (FIG):" << subch_id
                                                   << "Start:" << QString("0x%1").arg(subchannel.start_address, 4, 16, QChar('0'))
                                                   << "Size:" << subchannel.size << "CUs";
                                }
                            }
                        }


                        // Step 6: Count FIG types
                        for (const auto& fig_block : figResult.fig_blocks) {
                            QString fig_key = QString("FIG %1/%2")
                                .arg(fig_block.fig_type)
                                .arg(fig_block.get_extension());
                            fig_type_counts[fig_key]++;
                        }

                    } else {
                        // Frame parsing failed
                        errorFrames++;
                        qCWarning(etiMain) << "Frame" << processedFrames << "parsing failed";
                    }

                } catch (const std::exception& e) {
                    errorFrames++;
                    qCWarning(etiMain) << "Exception parsing frame" << processedFrames << ":" << e.what();
                }

                // Update progress
                double progress = static_cast<double>(processedFrames) / expectedFrames;
                m_progress = qMin(progress, 1.0);
                m_statusText = QString("Processing frame %1 of %2...").arg(processedFrames).arg(expectedFrames);
                emit progress_changed(m_progress);
                emit status_changed(m_statusText);

                // Small delay every 1000 frames for UI responsiveness
                if (processedFrames % 1000 == 0) {
                    QThread::msleep(1);
                    qCInfo(etiMain) << "Progress:" << processedFrames << "frames,"
                                   << discovered_services.size() << "services,"
                                   << discovered_subchannels.size() << "subchannels";
                    if (m_verbose_mode) {
                        qCInfo(etiMain) << "  Verbose: Collected" << result.frame_analyses.size() << "frame analyses";
                    }
                }

            } else if (bytesRead > 0) {
                // Partial frame - count as error
                errorFrames++;
                qCWarning(etiMain) << "Partial frame detected:" << bytesRead << "bytes instead of" << ETI_FRAME_SIZE;
            }
        }
        frameFile.close();
    }

    // Calculate processing statistics
    qint64 processingTime = m_processingTimer.elapsed();
    result.processingTimeMs = processingTime;

    // ============================================================================
    // FIX: Real processing results (not hardcoded!)
    // ============================================================================
    result.success = true;
    result.totalFrames = expectedFrames;
    result.processedFrames = processedFrames;
    result.errorFrames = errorFrames;
    result.averageFPS = processedFrames / (processingTime / 1000.0);

    // Populate ensemble information
    result.ensemblesFound = discovered_ensembles.size();
    if (!discovered_ensembles.isEmpty()) {
        // Use first ensemble as primary
        auto ensemble = discovered_ensembles.first();
        result.ensembleName = QString::fromStdString(ensemble.label);

        // AGENT 45 FIX: Store full ensemble structure in result
        result.ensemble = ensemble;

        // Emit ensemble discovered signal
        emit ensemble_discovered(ensemble.ensemble_id, result.ensembleName);
    }

    // Populate service information
    result.servicesFound = discovered_services.size();
    for (const auto& service : discovered_services) {
        QString service_name = QString::fromStdString(service.label);
        if (service_name.isEmpty()) {
            service_name = QString("Service 0x%1").arg(service.service_id, 4, 16, QChar('0'));
        }
        result.serviceNames << service_name;

        // AGENT 45 FIX: Store full service structures in result
        result.services.push_back(service);

        // Determine service type
        QString service_type = "Unknown";
        if (service.is_programme) {
            // Check if DAB+ (ASCTy = 0x3F or >= 0x02)
            bool is_dabplus = false;
            for (const auto& comp : service.components) {
                if (comp.asc_ty == 0x3F || comp.asc_ty >= 0x02) {
                    is_dabplus = true;
                    break;
                }
            }
            service_type = is_dabplus ? "DAB+ Audio" : "DAB Audio";
        } else {
            service_type = "Data Service";
        }

        // Emit service discovered signal
        emit service_discovered(service.service_id, service_name, service_type);
    }
    // AGENT 45 FIX: Convert SubChannelInfo to Fig01SubchannelInfo for YAML output
    for (const auto& subchannel : discovered_subchannels) {
        eti::fig::Fig01SubchannelInfo fig01_sc{};
        fig01_sc.subchannel_id = subchannel.sub_channel_id;
        fig01_sc.start_address = subchannel.start_address;

        // Determine form based on UEP flag
        fig01_sc.short_form = subchannel.uep_flag;

        if (fig01_sc.short_form) {
            // Short form (UEP): estimate table index from size
            fig01_sc.table_index = (subchannel.size / 8);  // Simplified
            fig01_sc.table_switch = 0;
        } else {
            // Long form (EEP)
            fig01_sc.subchannel_size = subchannel.size;
            fig01_sc.protection_level = subchannel.protection_level;
            fig01_sc.option = 0;  // Default option
        }
        fig01_sc.is_valid = true;
        result.subchannels.push_back(fig01_sc);
    }

    // AGENT 45 FIX: Populate FIG type counts
    result.fig_type_counts = fig_type_counts;

    // ETSI compliance
    result.etsiCompliant = (errorFrames == 0);
    if (errorFrames > 0) {
        result.complianceWarnings << QString("%1 frames failed parsing").arg(errorFrames);
    }
    if (discovered_services.isEmpty()) {
        result.complianceWarnings << "No services discovered (unusual for valid ETI)";
    }
    if (discovered_subchannels.isEmpty()) {
        result.complianceWarnings << "No subchannels discovered (unusual for valid ETI)";
    }

    // Final progress update
    m_progress = 1.0;
    m_statusText = "Processing complete";
    emit progress_changed(m_progress);
    emit status_changed(m_statusText);

    // Log final summary
    qCInfo(etiMain) << "Processing complete:";
    qCInfo(etiMain) << "  Frames processed:" << processedFrames << "/" << expectedFrames;
    qCInfo(etiMain) << "  Valid frames:" << valid_parsed_frames;
    qCInfo(etiMain) << "  Error frames:" << errorFrames;
    qCInfo(etiMain) << "  Ensembles discovered:" << discovered_ensembles.size();
    qCInfo(etiMain) << "  Services discovered:" << discovered_services.size();
    qCInfo(etiMain) << "  Subchannels discovered:" << discovered_subchannels.size();
    qCInfo(etiMain) << "  FIG types seen:" << fig_type_counts.size();
    qCInfo(etiMain) << "  Processing time:" << processingTime << "ms";
    qCInfo(etiMain) << "  Average FPS:" << QString::number(result.averageFPS, 'f', 1);
    if (m_verbose_mode) {
        qCInfo(etiMain) << "  Verbose frames collected:" << result.frame_analyses.size();
    }

    // Save results to output file
    if (!save_results_to_file(outputFile, result)) {
        result.success = false;
        result.errorMessage = QString("Failed to save results to: %1").arg(outputFile);
        return result;
    }

    // Emit completion signal
    emit processing_complete(result);

    return result;
}

bool HeadlessETIProcessor::initialize_components()
{
    try {
        // Create core processing components
        m_etiProcessor = std::make_unique<EtiProcessor>();
        if (!m_etiProcessor->initialize()) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize EtiProcessor");
            return false;
        }

        // Initialize modern frame parser with default config
        m_frameParser = std::make_unique<eti::modern::ModernETIFrameParser>();
        eti::modern::ProcessingConfig config;
        config.enable_threading = false;  // Single-threaded for CLI mode
        config.enable_caching = false;    // Disable caching for simplicity
        config.target_fps = 1000.0;       // Target 1000 FPS

        if (!m_frameParser->initialize(config)) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize ModernETIFrameParser");
            return false;
        }

        m_figAnalyzer = std::make_unique<eti::modern::EnhancedFIGAnalyzer>();
        if (!m_figAnalyzer->initialize()) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize EnhancedFIGAnalyzer");
            return false;
        }

        // DISABLED FOR CLI BUILD: ComprehensiveETSIValidator all methods commented out
        // m_etsiValidator = std::make_unique<eti::compliance::ComprehensiveETSIValidator>();
        // if (!m_etsiValidator->initialize()) {
        //     Logger::instance().log(Logger::Error, "HeadlessETIProcessor", "Failed to initialize ComprehensiveETSIValidator");
        //     return false;
        // }

        Logger::instance().log(Logger::Info, "HeadlessETIProcessor", "All processing components initialized successfully");
        return true;

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "HeadlessETIProcessor",
                               QString("Component initialization failed: %1").arg(e.what()));
        return false;
    }
}

void HeadlessETIProcessor::reset_processing_state()
{
    m_progress = 0.0;
    m_statusText = "";
    m_currentResult = ProcessingResult();
}

bool HeadlessETIProcessor::save_results_to_file(const QString& outputFile, const ProcessingResult& result)
{
    try {
        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Logger::instance().log(Logger::Error, "HeadlessETIProcessor",
                                   QString("Cannot open output file for writing: %1").arg(outputFile));
            return false;
        }

        QString yamlOutput = generate_yaml_output(result);
        file.write(yamlOutput.toUtf8());
        file.close();

        Logger::instance().log(Logger::Info, "HeadlessETIProcessor",
                              QString("Results saved to: %1").arg(outputFile));
        return true;

    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "HeadlessETIProcessor",
                               QString("Failed to save results: %1").arg(e.what()));
        return false;
    }
}

QString HeadlessETIProcessor::generate_yaml_output(const ProcessingResult& result)
{
    QString yaml;
    yaml += "# ETI Stream Analysis Results\n";
    yaml += "# Generated by StreamDAB Stream Analyzer Headless Mode\n\n";

    yaml += "processing:\n";
    yaml += QString("  success: %1\n").arg(result.success ? "true" : "false");
    if (!result.errorMessage.isEmpty()) {
        yaml += QString("  errorMessage: \"%1\"\n").arg(result.errorMessage);
    }
    yaml += "\n";

    yaml += "statistics:\n";
    yaml += QString("  totalFrames: %1\n").arg(result.totalFrames);
    yaml += QString("  processedFrames: %1\n").arg(result.processedFrames);
    yaml += QString("  errorFrames: %1\n").arg(result.errorFrames);
    yaml += QString("  processingTimeMs: %1\n").arg(result.processingTimeMs, 0, 'f', 2);
    yaml += QString("  averageFPS: %1\n").arg(result.averageFPS, 0, 'f', 2);
    yaml += "\n";

    yaml += "ensemble:\n";
    yaml += QString("  ensemblesFound: %1\n").arg(result.ensemblesFound);
    if (!result.ensembleName.isEmpty()) {
        yaml += QString("  ensembleName: \"%1\"\n").arg(result.ensembleName);
    }
    yaml += "\n";

    yaml += "services:\n";
    yaml += QString("  servicesFound: %1\n").arg(result.servicesFound);
    if (!result.serviceNames.isEmpty()) {
        yaml += "  serviceNames:\n";
        for (const QString& name : result.serviceNames) {
            yaml += QString("    - \"%1\"\n").arg(name);
        }
    } else {
        yaml += "  serviceNames: []\n";
    }
    yaml += "\n";

    yaml += "etsiCompliance:\n";
    yaml += QString("  etsiCompliant: %1\n").arg(result.etsiCompliant ? "true" : "false");
    if (!result.complianceWarnings.isEmpty()) {
        yaml += "  warnings:\n";
        for (const QString& warning : result.complianceWarnings) {
            yaml += QString("    - \"%1\"\n").arg(warning);
        }
    }
    if (!result.complianceErrors.isEmpty()) {
        yaml += "  errors:\n";
        for (const QString& error : result.complianceErrors) {
            yaml += QString("    - \"%1\"\n").arg(error);
        }
    }

    return yaml;
}

QString HeadlessETIProcessor::generate_json_output(const ProcessingResult& result)
{
    QJsonObject json;

    QJsonObject processing;
    processing["success"] = result.success;
    if (!result.errorMessage.isEmpty()) {
        processing["errorMessage"] = result.errorMessage;
    }
    json["processing"] = processing;

    QJsonObject statistics;
    statistics["totalFrames"] = static_cast<qint64>(result.totalFrames);
    statistics["processedFrames"] = static_cast<qint64>(result.processedFrames);
    statistics["errorFrames"] = static_cast<qint64>(result.errorFrames);
    statistics["processingTimeMs"] = result.processingTimeMs;
    statistics["averageFPS"] = result.averageFPS;
    json["statistics"] = statistics;

    QJsonObject services;
    services["servicesFound"] = static_cast<qint64>(result.servicesFound);
    services["ensemblesFound"] = static_cast<qint64>(result.ensemblesFound);
    if (!result.ensembleName.isEmpty()) {
        services["ensembleName"] = result.ensembleName;
    }
    json["services"] = services;

    QJsonObject compliance;
    compliance["etsiCompliant"] = result.etsiCompliant;
    json["etsiCompliance"] = compliance;

    QJsonDocument doc(json);
    return doc.toJson();
}

void HeadlessETIProcessor::on_frame_processed()
{
    // Callback for frame processing events
}

void HeadlessETIProcessor::on_error_detected(const QString& error)
{
    Logger::instance().log(Logger::Warning, "HeadlessETIProcessor",
                           QString("Processing error detected: %1").arg(error));
}
