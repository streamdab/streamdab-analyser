/**
 * @file report_generators.h
 * @brief ETSI Compliance Report Generators Implementation
 * 
 * Comprehensive implementation of professional report generators for ETSI
 * compliance documentation including HTML, PDF, XML, and regulatory formats
 * with professional styling, charts, and broadcast industry standards.
 * 
 * Features:
 * - Professional HTML reports with CSS styling and interactive charts
 * - PDF generation with professional layout and branding
 * - XML structured data export for system integration
 * - Regulatory format compliance for filing requirements
 * - Real-time dashboard generation
 * - Multi-language support for international deployment
 * - Accessibility compliance (WCAG 2.1 AA)
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#ifndef ETSI_REPORT_GENERATORS_H
#define ETSI_REPORT_GENERATORS_H

#include "compliance_reporter.h"
#include "compliance_engine.h"
#include "compliance_logger.h"
#include "alert_system.h"
#include "broadcast_standards.h"
#include "../eti_types.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace etsi {
namespace reporting {
namespace generators {

/**
 * @brief Chart and visualization types for reports
 */
enum class ChartType : uint8_t {
    LINE_CHART = 1,         // Time series line charts
    BAR_CHART = 2,          // Bar charts for comparisons
    PIE_CHART = 3,          // Pie charts for distributions
    GAUGE_CHART = 4,        // Gauge charts for metrics
    HEATMAP = 5,            // Heatmaps for correlation data
    SCATTER_PLOT = 6,       // Scatter plots for analysis
    HISTOGRAM = 7,          // Histograms for distributions
    TIMELINE = 8,           // Timeline for events
    DASHBOARD = 9           // Multi-widget dashboard
};

/**
 * @brief Chart configuration for report visualization
 */
struct ChartConfig {
    ChartType type;
    std::string title;
    std::string x_axis_label;
    std::string y_axis_label;
    std::vector<std::string> data_labels;
    std::vector<std::vector<double>> data_series;
    std::vector<std::string> colors;
    
    // Chart styling
    uint32_t width = 800;
    uint32_t height = 400;
    bool enable_legend = true;
    bool enable_grid = true;
    bool enable_tooltips = true;
    
    ChartConfig(ChartType chart_type, const std::string& chart_title)
        : type(chart_type), title(chart_title) {}
};

/**
 * @brief Base class for all report generators
 */
class ReportGenerator {
public:
    explicit ReportGenerator(const std::string& generator_name)
        : generator_name_(generator_name) {}
    
    virtual ~ReportGenerator() = default;
    
    /**
     * @brief Generate report content
     * @param metrics Compliance metrics to include
     * @param scope Report scope configuration
     * @param template_config Report template
     * @return Generated report content
     */
    virtual std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) = 0;
    
    /**
     * @brief Generate chart visualization
     * @param config Chart configuration
     * @return Chart markup/code for the specific format
     */
    virtual std::string generate_chart(const ChartConfig& config) = 0;
    
    /**
     * @brief Get supported file extension
     * @return File extension (including dot)
     */
    virtual std::string get_file_extension() const = 0;
    
    /**
     * @brief Get MIME type for the generated content
     * @return MIME type string
     */
    virtual std::string get_mime_type() const = 0;
    
    /**
     * @brief Validate generator configuration
     * @return true if configuration is valid
     */
    virtual bool validate_configuration() const = 0;

protected:
    std::string generator_name_;
    
    // Common utility methods
    std::string format_timestamp(std::chrono::system_clock::time_point timestamp) const;
    std::string format_duration(std::chrono::seconds duration) const;
    std::string format_percentage(double percentage, int decimals = 2) const;
    std::string format_number(double number, int decimals = 2) const;
    std::string escape_content(const std::string& content) const;
    
    // Data processing helpers
    std::vector<ChartConfig> create_compliance_charts(const ComplianceMetricsSummary& metrics) const;
    std::vector<ChartConfig> create_performance_charts(const ComplianceMetricsSummary& metrics) const;
    std::vector<ChartConfig> create_service_quality_charts(const ComplianceMetricsSummary& metrics) const;
};

/**
 * @brief HTML Report Generator with CSS styling and interactive charts
 */
class HtmlReportGenerator : public ReportGenerator {
public:
    HtmlReportGenerator() : ReportGenerator("HTML Generator") {}
    
    std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) override;
    
    std::string generate_chart(const ChartConfig& config) override;
    
    std::string get_file_extension() const override { return ".html"; }
    std::string get_mime_type() const override { return "text/html"; }
    bool validate_configuration() const override { return true; }
    
    /**
     * @brief Generate responsive HTML dashboard
     * @param metrics Compliance metrics
     * @param scope Report scope
     * @param template_config Template configuration
     * @return HTML dashboard content
     */
    std::string generate_dashboard(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    );
    
    /**
     * @brief Generate interactive compliance chart using Chart.js
     * @param config Chart configuration
     * @return Chart.js JavaScript code
     */
    std::string generate_interactive_chart(const ChartConfig& config);

private:
    std::string generate_html_header(const ReportTemplate& template_config) const;
    std::string generate_html_footer(const ReportTemplate& template_config) const;
    std::string generate_css_styles() const;
    std::string generate_javascript_libraries() const;
    std::string generate_executive_summary_html(const ComplianceMetricsSummary& metrics) const;
    std::string generate_compliance_overview_html(const ComplianceMetricsSummary& metrics) const;
    std::string generate_service_quality_html(const ComplianceMetricsSummary& metrics) const;
    std::string generate_alerts_summary_html(const ComplianceMetricsSummary& metrics) const;
    std::string generate_performance_metrics_html(const ComplianceMetricsSummary& metrics) const;
    std::string generate_navigation_menu() const;
    std::string generate_responsive_table(const std::vector<std::vector<std::string>>& data,
                                        const std::vector<std::string>& headers) const;
    
    // Chart generation helpers
    std::string generate_chartjs_line_chart(const ChartConfig& config) const;
    std::string generate_chartjs_bar_chart(const ChartConfig& config) const;
    std::string generate_chartjs_pie_chart(const ChartConfig& config) const;
    std::string generate_chartjs_gauge_chart(const ChartConfig& config) const;
};

/**
 * @brief PDF Report Generator with professional layout
 */
class PdfReportGenerator : public ReportGenerator {
public:
    PdfReportGenerator() : ReportGenerator("PDF Generator") {}
    
    std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) override;
    
    std::string generate_chart(const ChartConfig& config) override;
    
    std::string get_file_extension() const override { return ".pdf"; }
    std::string get_mime_type() const override { return "application/pdf"; }
    bool validate_configuration() const override;
    
    /**
     * @brief Generate professional PDF with custom layout
     * @param metrics Compliance metrics
     * @param scope Report scope
     * @param template_config Template configuration
     * @return PDF content as binary string
     */
    std::string generate_professional_pdf(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    );

private:
    std::string generate_pdf_header(const ReportTemplate& template_config) const;
    std::string generate_pdf_footer(const ReportTemplate& template_config) const;
    std::string generate_pdf_cover_page(const ReportTemplate& template_config,
                                      const ReportScope& scope) const;
    std::string generate_pdf_table_of_contents() const;
    std::string generate_pdf_executive_summary(const ComplianceMetricsSummary& metrics) const;
    std::string generate_pdf_detailed_analysis(const ComplianceMetricsSummary& metrics) const;
    std::string generate_pdf_charts_section(const ComplianceMetricsSummary& metrics) const;
    std::string generate_pdf_appendices(const ComplianceMetricsSummary& metrics) const;
    
    // PDF-specific chart generation (using LaTeX/TikZ or embedded images)
    std::string generate_pdf_chart_image(const ChartConfig& config) const;
    std::string generate_latex_table(const std::vector<std::vector<std::string>>& data,
                                   const std::vector<std::string>& headers) const;
};

/**
 * @brief XML Report Generator for structured data export
 */
class XmlReportGenerator : public ReportGenerator {
public:
    XmlReportGenerator() : ReportGenerator("XML Generator") {}
    
    std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) override;
    
    std::string generate_chart(const ChartConfig& config) override;
    
    std::string get_file_extension() const override { return ".xml"; }
    std::string get_mime_type() const override { return "application/xml"; }
    bool validate_configuration() const override { return true; }
    
    /**
     * @brief Generate XML with schema validation
     * @param metrics Compliance metrics
     * @param scope Report scope
     * @param schema_version XML schema version
     * @return XML content with schema declaration
     */
    std::string generate_schema_compliant_xml(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const std::string& schema_version = "1.0"
    );

private:
    std::string generate_xml_header(const std::string& schema_version) const;
    std::string generate_xml_metadata(const ReportScope& scope) const;
    std::string generate_xml_compliance_section(const ComplianceMetricsSummary& metrics) const;
    std::string generate_xml_service_quality_section(const ComplianceMetricsSummary& metrics) const;
    std::string generate_xml_alerts_section(const ComplianceMetricsSummary& metrics) const;
    std::string generate_xml_performance_section(const ComplianceMetricsSummary& metrics) const;
    std::string escape_xml_content(const std::string& content) const;
    std::string format_xml_element(const std::string& name, const std::string& value,
                                 const std::map<std::string, std::string>& attributes = {}) const;
};

/**
 * @brief CSV Report Generator for data analysis
 */
class CsvReportGenerator : public ReportGenerator {
public:
    CsvReportGenerator() : ReportGenerator("CSV Generator") {}
    
    std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) override;
    
    std::string generate_chart(const ChartConfig& config) override;
    
    std::string get_file_extension() const override { return ".csv"; }
    std::string get_mime_type() const override { return "text/csv"; }
    bool validate_configuration() const override { return true; }

private:
    std::string generate_csv_header() const;
    std::string generate_compliance_metrics_csv(const ComplianceMetricsSummary& metrics) const;
    std::string generate_service_quality_csv(const ComplianceMetricsSummary& metrics) const;
    std::string generate_alert_statistics_csv(const ComplianceMetricsSummary& metrics) const;
    std::string escape_csv_field(const std::string& field) const;
};

/**
 * @brief JSON Report Generator for API integration
 */
class JsonReportGenerator : public ReportGenerator {
public:
    JsonReportGenerator() : ReportGenerator("JSON Generator") {}
    
    std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) override;
    
    std::string generate_chart(const ChartConfig& config) override;
    
    std::string get_file_extension() const override { return ".json"; }
    std::string get_mime_type() const override { return "application/json"; }
    bool validate_configuration() const override { return true; }

private:
    std::string generate_json_metadata(const ReportScope& scope) const;
    std::string generate_compliance_metrics_json(const ComplianceMetricsSummary& metrics) const;
    std::string generate_service_quality_json(const ComplianceMetricsSummary& metrics) const;
    std::string generate_alert_statistics_json(const ComplianceMetricsSummary& metrics) const;
    std::string escape_json_string(const std::string& str) const;
    std::string format_json_number(double number) const;
};

/**
 * @brief Regulatory Format Generator for compliance filing
 */
class RegulatoryReportGenerator : public ReportGenerator {
public:
    explicit RegulatoryReportGenerator(const std::string& regulation_standard)
        : ReportGenerator("Regulatory Generator"), regulation_standard_(regulation_standard) {}
    
    std::string generate_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const ReportTemplate& template_config
    ) override;
    
    std::string generate_chart(const ChartConfig& config) override;
    
    std::string get_file_extension() const override;
    std::string get_mime_type() const override;
    bool validate_configuration() const override;
    
    /**
     * @brief Generate FCC-specific compliance report
     * @param metrics Compliance metrics
     * @param scope Report scope
     * @param license_number FCC license number
     * @return FCC-formatted report
     */
    std::string generate_fcc_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const std::string& license_number
    );
    
    /**
     * @brief Generate Ofcom-specific compliance report
     * @param metrics Compliance metrics
     * @param scope Report scope
     * @param license_number Ofcom license number
     * @return Ofcom-formatted report
     */
    std::string generate_ofcom_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const std::string& license_number
    );
    
    /**
     * @brief Generate CRTC-specific compliance report
     * @param metrics Compliance metrics
     * @param scope Report scope
     * @param license_number CRTC license number
     * @return CRTC-formatted report
     */
    std::string generate_crtc_report(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope,
        const std::string& license_number
    );

private:
    std::string regulation_standard_;
    
    std::string generate_regulatory_header(const std::string& authority,
                                         const std::string& license_number) const;
    std::string generate_regulatory_summary(const ComplianceMetricsSummary& metrics) const;
    std::string generate_regulatory_technical_details(const ComplianceMetricsSummary& metrics) const;
    std::string generate_regulatory_certification(const std::string& authority) const;
    std::string format_regulatory_compliance_table(const ComplianceMetricsSummary& metrics) const;
};

/**
 * @brief Dashboard Generator for real-time monitoring
 */
class DashboardGenerator {
public:
    /**
     * @brief Dashboard configuration
     */
    struct DashboardConfig {
        std::string title;
        uint32_t refresh_interval_seconds = 30;
        bool enable_real_time_updates = true;
        bool enable_responsive_design = true;
        std::vector<std::string> enabled_widgets;
        std::map<std::string, std::string> widget_configurations;
        
        DashboardConfig() {
            enabled_widgets = {
                "compliance_overview", "service_status", "alert_summary",
                "performance_metrics", "audio_quality", "error_statistics"
            };
        }
    };
    
    explicit DashboardGenerator(const DashboardConfig& config) : config_(config) {}
    
    /**
     * @brief Generate real-time dashboard HTML
     * @param metrics Current compliance metrics
     * @param scope Dashboard scope
     * @return HTML dashboard content
     */
    std::string generate_dashboard(
        const ComplianceMetricsSummary& metrics,
        const ReportScope& scope
    );
    
    /**
     * @brief Generate dashboard widget
     * @param widget_name Widget identifier
     * @param metrics Compliance metrics
     * @return Widget HTML content
     */
    std::string generate_widget(
        const std::string& widget_name,
        const ComplianceMetricsSummary& metrics
    );
    
    /**
     * @brief Generate real-time data API endpoint
     * @param metrics Current metrics
     * @return JSON data for dashboard updates
     */
    std::string generate_api_data(const ComplianceMetricsSummary& metrics);

private:
    DashboardConfig config_;
    
    std::string generate_dashboard_header() const;
    std::string generate_dashboard_navigation() const;
    std::string generate_dashboard_grid() const;
    std::string generate_dashboard_scripts() const;
    
    // Widget generators
    std::string generate_compliance_overview_widget(const ComplianceMetricsSummary& metrics);
    std::string generate_service_status_widget(const ComplianceMetricsSummary& metrics);
    std::string generate_alert_summary_widget(const ComplianceMetricsSummary& metrics);
    std::string generate_performance_metrics_widget(const ComplianceMetricsSummary& metrics);
    std::string generate_audio_quality_widget(const ComplianceMetricsSummary& metrics);
    std::string generate_error_statistics_widget(const ComplianceMetricsSummary& metrics);
};

/**
 * @brief Utility functions for report generation
 */
namespace utils {
    /**
     * @brief Generate compliance scorecard with color-coded grades
     * @param metrics Compliance metrics
     * @return Scorecard HTML/text content
     */
    std::string generate_compliance_scorecard(const ComplianceMetricsSummary& metrics);
    
    /**
     * @brief Create trend analysis from historical data
     * @param historical_metrics Vector of historical metrics
     * @param analysis_period Period for trend analysis
     * @return Trend analysis report section
     */
    std::string create_trend_analysis(
        const std::vector<ComplianceMetricsSummary>& historical_metrics,
        std::chrono::hours analysis_period
    );
    
    /**
     * @brief Generate executive summary with key insights
     * @param metrics Compliance metrics
     * @param max_length Maximum summary length
     * @return Executive summary text
     */
    std::string generate_executive_summary(
        const ComplianceMetricsSummary& metrics,
        size_t max_length = 500
    );
    
    /**
     * @brief Create compliance recommendations
     * @param metrics Compliance metrics
     * @return List of actionable recommendations
     */
    std::vector<std::string> create_compliance_recommendations(
        const ComplianceMetricsSummary& metrics
    );
    
    /**
     * @brief Format compliance grade (A-F scale)
     * @param compliance_percentage Compliance percentage
     * @return Grade letter with color coding
     */
    std::string format_compliance_grade(double compliance_percentage);
    
    /**
     * @brief Generate QR code for report verification
     * @param report_checksum Report checksum
     * @param verification_url Verification URL
     * @return QR code SVG/image data
     */
    std::string generate_verification_qr_code(
        const std::string& report_checksum,
        const std::string& verification_url
    );
    
    /**
     * @brief Create chart color palette for consistent styling
     * @param chart_type Type of chart
     * @param data_series_count Number of data series
     * @return Vector of color codes
     */
    std::vector<std::string> create_chart_color_palette(
        ChartType chart_type,
        size_t data_series_count
    );
    
    /**
     * @brief Validate report content for completeness
     * @param report_content Report content to validate
     * @param required_sections List of required sections
     * @return Validation result with missing sections
     */
    std::pair<bool, std::vector<std::string>> validate_report_content(
        const std::string& report_content,
        const std::vector<std::string>& required_sections
    );
}

} // namespace generators
} // namespace reporting
} // namespace etsi

#endif // ETSI_REPORT_GENERATORS_H