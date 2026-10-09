#include "audio_monitoring_widget.h"
#include "core/eti_processor.hpp"
#include "service_browser.h"
#include <QApplication>
#include <QStyle>
#include <QSplitter>
#include <QFrame>
#include <QSpacerItem>
#include <QSlider>
#include <QSpinBox>
#include <QPalette>
#include <QDateTime>
#include <QDebug>
#include <QtMath>

AudioMonitoringWidget::AudioMonitoringWidget(QWidget *parent)
    : QWidget(parent)
    , m_etiProcessor(nullptr)
    , m_serviceBrowser(nullptr)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollContent(nullptr)
    , m_scrollLayout(nullptr)
    , m_headerGroup(nullptr)
    , m_serviceGroup(nullptr)
    , m_statisticsGroup(nullptr)
    , m_complianceGroup(nullptr)
    , m_selectedServiceId(0)
    , m_overallComplianceScore(0.0)
    , m_updateTimer(nullptr)
    , m_realTimeEnabled(true)
    , m_updateRate(DEFAULT_UPDATE_RATE)
    , m_totalServices(0)
    , m_dabPlusServices(0)
    , m_activeServices(0)
    , m_averageQuality(0.0)
{
    createUI();
    setupSignalConnections();
    applyBroadcastStyling();
    
    // Initialize update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AudioMonitoringWidget::handleAudioUpdate);
    
    // Start real-time updates
    if (m_realTimeEnabled) {
        setUpdateRate(m_updateRate);
    }
}

AudioMonitoringWidget::~AudioMonitoringWidget()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    clearAudioData();
}

bool AudioMonitoringWidget::initialize(EtiProcessor *processor)
{
    setEtiProcessor(processor);
    return validateState();
}

void AudioMonitoringWidget::setEtiProcessor(EtiProcessor *processor)
{
    if (m_etiProcessor == processor) {
        return;
    }
    
    // Disconnect old processor
    if (m_etiProcessor) {
        disconnect(m_etiProcessor, nullptr, this, nullptr);
    }
    
    m_etiProcessor = processor;
    
    // Connect new processor signals
    if (m_etiProcessor) {
        // Note: These signals would need to be implemented in EtiProcessor
        // For now, we'll implement the slots but connections will be made later
    }
}

void AudioMonitoringWidget::setServiceBrowser(ServiceBrowser *serviceBrowser)
{
    m_serviceBrowser = serviceBrowser;
}

void AudioMonitoringWidget::updateAudioLevels(const QMap<QString, int>& levels)
{
    // Performance optimization: Skip if widget not visible or no data
    if (!isVisible() || levels.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    // Performance optimization: Batch UI updates
    QList<QPair<quint32, int>> levelChanges;
    QList<QPair<quint32, QString>> styleUpdates;
    
    for (auto it = levels.begin(); it != levels.end(); ++it) {
        quint32 serviceId = it.key().toUInt();
        int level = qBound(0, it.value(), 100);
        
        // Update service audio info
        if (m_serviceAudioInfo.contains(serviceId)) {
            int oldLevel = m_serviceAudioInfo[serviceId].audioLevel;
            m_serviceAudioInfo[serviceId].audioLevel = level;
            m_serviceAudioInfo[serviceId].lastUpdate = QDateTime::currentDateTime();
            
            // Only update UI if level changed significantly (reduce flicker)
            if (qAbs(level - oldLevel) >= 2 || level == 0 || level == 100) {
                levelChanges.append({serviceId, level});
                
                // Prepare style update
                QColor meterColor = getAudioQualityColor(m_serviceAudioInfo[serviceId].quality);
                QString meterStyle = QString(
                    "QProgressBar {"
                    "    border: 1px solid #555555;"
                    "    border-radius: 3px;"
                    "    background-color: #2D2D30;"
                    "    text-align: center;"
                    "    color: white;"
                    "    font-size: 10px;"
                    "}"
                    "QProgressBar::chunk {"
                    "    background-color: %1;"
                    "    border-radius: 2px;"
                    "}"
                ).arg(meterColor.name());
                
                styleUpdates.append({serviceId, meterStyle});
            }
        }
    }
    
    // Unlock mutex before UI updates to prevent blocking
    locker.unlock();
    
    // Apply batched UI updates
    for (const auto& change : levelChanges) {
        quint32 serviceId = change.first;
        int level = change.second;
        
        if (m_audioMeters.contains(serviceId)) {
            m_audioMeters[serviceId]->setValue(level);
            emit audioLevelChanged(serviceId, level);
        }
    }
    
    // Apply style updates (less frequent)
    static int styleUpdateCounter = 0;
    if (++styleUpdateCounter % 10 == 0) { // Update styles every 10th call
        for (const auto& styleUpdate : styleUpdates) {
            if (m_audioMeters.contains(styleUpdate.first)) {
                m_audioMeters[styleUpdate.first]->setStyleSheet(styleUpdate.second);
            }
        }
    }
    
    // Update statistics less frequently for better performance
    static int statisticsUpdateCounter = 0;
    if (++statisticsUpdateCounter % 30 == 0) { // Update every 30th call (once per second at 30 FPS)
        updateStatisticsLabels();
    }
}

void AudioMonitoringWidget::updateServiceInfo(const QList<ServiceAudioInfo>& services)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Clear existing services not in the new list
    QSet<quint32> newServiceIds;
    for (const auto& service : services) {
        newServiceIds.insert(service.serviceId);
    }
    
    // Remove services no longer present
    auto it = m_serviceAudioInfo.begin();
    while (it != m_serviceAudioInfo.end()) {
        if (!newServiceIds.contains(it.key())) {
            removeServiceFromMonitoring(it.key());
            it = m_serviceAudioInfo.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update or add services
    for (const auto& service : services) {
        bool isNew = !m_serviceAudioInfo.contains(service.serviceId);
        m_serviceAudioInfo[service.serviceId] = service;
        
        if (isNew) {
            addServiceToMonitoring(service);
            
            // Emit DAB+ detection signal if applicable
            if (service.isDabPlus) {
                emit dabPlusServiceDetected(service.serviceId, service.codecDetails);
            }
        } else {
            updateServiceDisplay(service.serviceId, service);
        }
        
        // Update quality if changed
        AudioQuality newQuality = calculateAudioQuality(
            service.signalStrength, service.errorRate, service.bitRate
        );
        
        if (newQuality != m_serviceAudioInfo[service.serviceId].quality) {
            m_serviceAudioInfo[service.serviceId].quality = newQuality;
            emit audioQualityChanged(service.serviceId, newQuality);
        }
    }
    
    updateStatisticsLabels();
}

void AudioMonitoringWidget::updateEnsembleStatistics(const EnsembleStatistics& statistics)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_ensembleStatistics = statistics;
    m_totalServices = statistics.totalServices;
    m_dabPlusServices = statistics.dabPlusServices;
    m_activeServices = statistics.activeServices;
    
    updateStatisticsLabels();
}

void AudioMonitoringWidget::updateComplianceMetrics(double complianceScore, const QVariantMap& detailedMetrics)
{
    QMutexLocker locker(&m_dataMutex);
    
    m_overallComplianceScore = qBound(0.0, complianceScore, 100.0);
    m_complianceMetrics = detailedMetrics;
    
    // Update compliance display
    if (m_complianceProgress) {
        m_complianceProgress->setValue(static_cast<int>(m_overallComplianceScore));
        
        // Color-code compliance bar
        QColor complianceColor;
        if (m_overallComplianceScore >= 95.0) {
            complianceColor = QColor("#4CAF50"); // Green
        } else if (m_overallComplianceScore >= 80.0) {
            complianceColor = QColor("#8BC34A"); // Light Green
        } else if (m_overallComplianceScore >= 60.0) {
            complianceColor = QColor("#FF9800"); // Orange
        } else {
            complianceColor = QColor("#F44336"); // Red
        }
        
        QString complianceStyle = QString(
            "QProgressBar {"
            "    border: 1px solid #555555;"
            "    border-radius: 3px;"
            "    background-color: #2D2D30;"
            "    text-align: center;"
            "    color: white;"
            "    font-weight: bold;"
            "}"
            "QProgressBar::chunk {"
            "    background-color: %1;"
            "    border-radius: 2px;"
            "}"
        ).arg(complianceColor.name());
        
        m_complianceProgress->setStyleSheet(complianceStyle);
    }
    
    if (m_complianceLabel) {
        m_complianceLabel->setText(formatComplianceScore(m_overallComplianceScore));
    }
    
    emit etsiComplianceChanged(m_overallComplianceScore);
}

AudioMonitoringWidget::ServiceAudioInfo AudioMonitoringWidget::getServiceAudioInfo(quint32 serviceId) const
{
    QMutexLocker locker(&m_dataMutex);
    return m_serviceAudioInfo.value(serviceId, ServiceAudioInfo());
}

QList<quint32> AudioMonitoringWidget::getMonitoredServices() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_serviceAudioInfo.keys();
}

AudioMonitoringWidget::EnsembleStatistics AudioMonitoringWidget::getEnsembleStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_ensembleStatistics;
}

void AudioMonitoringWidget::setRealTimeEnabled(bool enabled)
{
    if (m_realTimeEnabled == enabled) {
        return;
    }
    
    m_realTimeEnabled = enabled;
    
    if (m_updateTimer) {
        if (enabled) {
            setUpdateRate(m_updateRate);
        } else {
            m_updateTimer->stop();
        }
    }
}

void AudioMonitoringWidget::setUpdateRate(int fps)
{
    m_updateRate = qBound(MIN_UPDATE_RATE, fps, MAX_UPDATE_RATE);
    
    if (m_updateTimer && m_realTimeEnabled) {
        int intervalMs = 1000 / m_updateRate;
        m_updateTimer->start(intervalMs);
    }
}

void AudioMonitoringWidget::clearAudioData()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Clear service data
    for (auto it = m_serviceAudioInfo.begin(); it != m_serviceAudioInfo.end(); ++it) {
        removeServiceFromMonitoring(it.key());
    }
    m_serviceAudioInfo.clear();
    
    // Reset statistics
    m_ensembleStatistics = EnsembleStatistics();
    m_totalServices = 0;
    m_dabPlusServices = 0;
    m_activeServices = 0;
    m_averageQuality = 0.0;
    m_overallComplianceScore = 0.0;
    m_complianceMetrics.clear();
    
    updateStatisticsLabels();
}

int AudioMonitoringWidget::getDabPlusServiceCount() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_dabPlusServices;
}

double AudioMonitoringWidget::getOverallQualityScore() const
{
    QMutexLocker locker(&m_dataMutex);
    
    if (m_serviceAudioInfo.isEmpty()) {
        return 0.0;
    }
    
    double totalQuality = 0.0;
    int validServices = 0;
    
    for (const auto& service : m_serviceAudioInfo) {
        if (service.isActive) {
            totalQuality += static_cast<double>(service.quality) * 25.0; // Convert enum to percentage
            validServices++;
        }
    }
    
    return validServices > 0 ? totalQuality / validServices : 0.0;
}

void AudioMonitoringWidget::refreshAudioLevels()
{
    if (!m_etiProcessor) {
        return;
    }
    
    // This would trigger the ETI processor to provide updated audio levels
    // For now, this is a placeholder for the actual implementation
    updateStatisticsLabels();
}

void AudioMonitoringWidget::selectService(quint32 serviceId)
{
    if (m_selectedServiceId == serviceId) {
        return;
    }
    
    m_selectedServiceId = serviceId;
    
    // Update visual selection
    for (auto it = m_serviceWidgets.begin(); it != m_serviceWidgets.end(); ++it) {
        QWidget* widget = it.value();
        if (widget) {
            if (it.key() == serviceId) {
                widget->setStyleSheet(widget->styleSheet() + " border: 2px solid #0078D4;");
            } else {
                QString style = widget->styleSheet();
                style.remove("border: 2px solid #0078D4;");
                widget->setStyleSheet(style);
            }
        }
    }
    
    emit serviceSelected(serviceId);
}

void AudioMonitoringWidget::updateComplianceStatistics()
{
    // Calculate compliance statistics from current services
    double totalCompliance = 0.0;
    int validServices = 0;
    
    QMutexLocker locker(&m_dataMutex);
    for (const auto& service : m_serviceAudioInfo) {
        if (service.isActive) {
            // Calculate service-level compliance based on quality metrics
            double serviceCompliance = 0.0;
            
            // Signal strength component (40%)
            if (service.signalStrength >= -60.0) {
                serviceCompliance += 40.0;
            } else if (service.signalStrength >= -80.0) {
                serviceCompliance += 20.0 + (service.signalStrength + 80.0) * 20.0 / 20.0;
            }
            
            // Error rate component (30%)
            if (service.errorRate <= 1.0) {
                serviceCompliance += 30.0;
            } else if (service.errorRate <= 5.0) {
                serviceCompliance += 30.0 - (service.errorRate - 1.0) * 7.5;
            }
            
            // DAB+ codec component (30%)
            if (service.isDabPlus) {
                serviceCompliance += 30.0;
            } else {
                serviceCompliance += 15.0; // Partial credit for legacy DAB
            }
            
            totalCompliance += serviceCompliance;
            validServices++;
        }
    }
    
    if (validServices > 0) {
        m_overallComplianceScore = totalCompliance / validServices;
        updateComplianceMetrics(m_overallComplianceScore, QVariantMap());
    }
}

void AudioMonitoringWidget::expandAll()
{
    // Expand all service groups (if using collapsible groups)
    for (auto widget : m_serviceWidgets) {
        if (widget) {
            widget->show();
        }
    }
}

void AudioMonitoringWidget::collapseAll()
{
    // Collapse all service groups (if using collapsible groups)
    for (auto widget : m_serviceWidgets) {
        if (widget) {
            widget->hide();
        }
    }
}

void AudioMonitoringWidget::handleAudioUpdate()
{
    // Performance optimization: Skip updates if widget is not visible
    if (!isVisible() || !m_realTimeEnabled) {
        return;
    }
    
    // Performance optimization: Throttle updates based on system performance
    static qint64 lastUpdateTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Adaptive update rate based on number of services
    int adaptiveInterval = 1000 / m_updateRate;
    if (m_serviceAudioInfo.size() > 10) {
        adaptiveInterval *= 2; // Halve update rate for many services
    }
    
    if (currentTime - lastUpdateTime < adaptiveInterval) {
        return; // Skip this update to maintain performance
    }
    lastUpdateTime = currentTime;
    
    // Update audio level meters and statistics
    refreshAudioLevels();
    
    // Performance optimization: Only update quality for visible services
    QMutexLocker locker(&m_dataMutex);
    
    // Batch quality updates to minimize UI redraws
    QList<QPair<quint32, AudioQuality>> qualityChanges;
    
    for (auto it = m_serviceAudioInfo.begin(); it != m_serviceAudioInfo.end(); ++it) {
        ServiceAudioInfo& service = it.value();
        
        // Only calculate quality for active services
        if (!service.isActive) {
            continue;
        }
        
        // Recalculate quality based on current metrics
        AudioQuality newQuality = calculateAudioQuality(
            service.signalStrength, service.errorRate, service.bitRate
        );
        
        if (newQuality != service.quality) {
            service.quality = newQuality;
            qualityChanges.append({service.serviceId, newQuality});
        }
    }
    
    // Unlock mutex before UI updates
    locker.unlock();
    
    // Batch UI updates for better performance
    for (const auto& change : qualityChanges) {
        updateServiceDisplay(change.first, m_serviceAudioInfo[change.first]);
        emit audioQualityChanged(change.first, change.second);
    }
}

void AudioMonitoringWidget::handleServiceSelectionChanged()
{
    // Handle service selection from UI components
    QObject* sender = this->sender();
    
    // Try to identify which service was selected
    for (auto it = m_serviceWidgets.begin(); it != m_serviceWidgets.end(); ++it) {
        if (it.value() == sender || it.value()->isAncestorOf(qobject_cast<QWidget*>(sender))) {
            selectService(it.key());
            break;
        }
    }
}

void AudioMonitoringWidget::handleAudioMeterClicked(quint32 serviceId)
{
    selectService(serviceId);
}

void AudioMonitoringWidget::refreshCodecDisplay()
{
    QMutexLocker locker(&m_dataMutex);
    
    for (auto it = m_serviceAudioInfo.begin(); it != m_serviceAudioInfo.end(); ++it) {
        const ServiceAudioInfo& service = it.value();
        
        if (m_codecLabels.contains(service.serviceId)) {
            QLabel* codecLabel = m_codecLabels[service.serviceId];
            
            QString codecText = getCodecDescription(service.audioCodec);
            if (!service.codecDetails.isEmpty()) {
                codecText += QString(" (%1)").arg(service.codecDetails);
            }
            
            codecLabel->setText(codecText);
            
            // Color-code based on codec type
            if (service.isDabPlus) {
                codecLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
            } else {
                codecLabel->setStyleSheet("color: #FF9800;");
            }
        }
    }
}

void AudioMonitoringWidget::createUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(6);
    
    createHeaderSection();
    createServiceMonitoringSection();
    createEnsembleStatisticsSection();
    createComplianceSection();
    
    // Add stretch to push everything to the top
    m_mainLayout->addStretch();
}

void AudioMonitoringWidget::createHeaderSection()
{
    m_headerGroup = new QGroupBox("🎵 DAB+ Audio Monitoring", this);
    m_headerLayout = new QHBoxLayout(m_headerGroup);
    
    m_titleLabel = new QLabel("Real-time Audio Monitoring", m_headerGroup);
    m_titleLabel->setStyleSheet("font-weight: bold; color: #0078D4;");
    
    m_refreshButton = new QPushButton("🔄 Refresh", m_headerGroup);
    m_refreshButton->setToolTip("Refresh audio monitoring data");
    connect(m_refreshButton, &QPushButton::clicked, this, &AudioMonitoringWidget::refreshAudioLevels);
    
    m_expandAllButton = new QPushButton("▼ Expand All", m_headerGroup);
    m_expandAllButton->setToolTip("Expand all service groups");
    connect(m_expandAllButton, &QPushButton::clicked, this, &AudioMonitoringWidget::expandAll);
    
    m_collapseAllButton = new QPushButton("▲ Collapse All", m_headerGroup);
    m_collapseAllButton->setToolTip("Collapse all service groups");
    connect(m_collapseAllButton, &QPushButton::clicked, this, &AudioMonitoringWidget::collapseAll);
    
    m_displayModeCombo = new QComboBox(m_headerGroup);
    m_displayModeCombo->addItem("All Services");
    m_displayModeCombo->addItem("DAB+ Only");
    m_displayModeCombo->addItem("Active Only");
    
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_displayModeCombo);
    m_headerLayout->addWidget(m_refreshButton);
    m_headerLayout->addWidget(m_expandAllButton);
    m_headerLayout->addWidget(m_collapseAllButton);
    
    m_mainLayout->addWidget(m_headerGroup);
}

void AudioMonitoringWidget::createServiceMonitoringSection()
{
    m_serviceGroup = new QGroupBox("Service Audio Levels", this);
    m_serviceLayout = new QVBoxLayout(m_serviceGroup);
    
    // Create scroll area for services
    m_scrollArea = new QScrollArea(m_serviceGroup);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_scrollContent = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setSpacing(4);
    
    m_scrollArea->setWidget(m_scrollContent);
    m_serviceLayout->addWidget(m_scrollArea);
    
    m_mainLayout->addWidget(m_serviceGroup);
}

void AudioMonitoringWidget::createEnsembleStatisticsSection()
{
    m_statisticsGroup = new QGroupBox("Ensemble Statistics", this);
    m_statisticsLayout = new QGridLayout(m_statisticsGroup);
    
    m_totalServicesLabel = new QLabel("Total Services: 0", m_statisticsGroup);
    m_dabPlusServicesLabel = new QLabel("DAB+ Services: 0", m_statisticsGroup);
    m_activeServicesLabel = new QLabel("Active Services: 0", m_statisticsGroup);
    m_totalBitrateLabel = new QLabel("Total Bitrate: 0.0 Mbps", m_statisticsGroup);
    
    m_statisticsLayout->addWidget(m_totalServicesLabel, 0, 0);
    m_statisticsLayout->addWidget(m_dabPlusServicesLabel, 0, 1);
    m_statisticsLayout->addWidget(m_activeServicesLabel, 1, 0);
    m_statisticsLayout->addWidget(m_totalBitrateLabel, 1, 1);
    
    m_mainLayout->addWidget(m_statisticsGroup);
}

void AudioMonitoringWidget::createComplianceSection()
{
    m_complianceGroup = new QGroupBox("ETSI Compliance", this);
    m_complianceLayout = new QVBoxLayout(m_complianceGroup);
    
    m_complianceProgress = new QProgressBar(m_complianceGroup);
    m_complianceProgress->setRange(0, 100);
    m_complianceProgress->setValue(0);
    m_complianceProgress->setFormat("%p% ETSI 300 799 Compliant");
    
    m_complianceLabel = new QLabel("Compliance Score: 0.0%", m_complianceGroup);
    m_complianceDetailsLabel = new QLabel("No compliance data available", m_complianceGroup);
    
    m_complianceLayout->addWidget(m_complianceProgress);
    m_complianceLayout->addWidget(m_complianceLabel);
    m_complianceLayout->addWidget(m_complianceDetailsLabel);
    
    m_mainLayout->addWidget(m_complianceGroup);
}

void AudioMonitoringWidget::setupAudioLevelMeters()
{
    // This method will be called when services are added
    // Individual meters are created in addServiceToMonitoring()
}

QWidget* AudioMonitoringWidget::createServiceDisplay(const ServiceAudioInfo& serviceInfo)
{
    QWidget* serviceWidget = new QWidget();
    serviceWidget->setFixedHeight(SERVICE_WIDGET_HEIGHT);
    
    QGridLayout* layout = new QGridLayout(serviceWidget);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);
    
    // Service name label
    QLabel* nameLabel = new QLabel(serviceInfo.serviceName);
    nameLabel->setStyleSheet("font-weight: bold; color: white;");
    nameLabel->setToolTip(QString("Service ID: %1").arg(serviceInfo.serviceId));
    
    // Audio level meter
    QProgressBar* audioMeter = new QProgressBar();
    audioMeter->setRange(0, 100);
    audioMeter->setValue(serviceInfo.audioLevel);
    audioMeter->setFixedHeight(AUDIO_METER_HEIGHT);
    audioMeter->setFormat("%p%");
    
    // Codec label
    QLabel* codecLabel = new QLabel(getCodecDescription(serviceInfo.audioCodec));
    codecLabel->setStyleSheet(serviceInfo.isDabPlus ? "color: #4CAF50; font-weight: bold;" : "color: #FF9800;");
    
    // Quality label
    QLabel* qualityLabel = new QLabel(getQualityDescription(serviceInfo.quality));
    qualityLabel->setStyleSheet(QString("color: %1;").arg(getAudioQualityColor(serviceInfo.quality).name()));
    
    // Bitrate label
    QLabel* bitrateLabel = new QLabel(formatBitrate(serviceInfo.bitRate));
    bitrateLabel->setStyleSheet("color: #CCCCCC; font-size: 10px;");
    
    // Layout arrangement
    layout->addWidget(nameLabel, 0, 0, 1, 2);
    layout->addWidget(audioMeter, 1, 0, 1, 2);
    layout->addWidget(codecLabel, 2, 0);
    layout->addWidget(qualityLabel, 2, 1);
    layout->addWidget(bitrateLabel, 0, 2);
    
    // Store references
    m_serviceLabels[serviceInfo.serviceId] = nameLabel;
    m_audioMeters[serviceInfo.serviceId] = audioMeter;
    m_codecLabels[serviceInfo.serviceId] = codecLabel;
    m_qualityLabels[serviceInfo.serviceId] = qualityLabel;
    m_bitrateLabels[serviceInfo.serviceId] = bitrateLabel;
    
    // Connect signals
    connect(audioMeter, &QProgressBar::valueChanged, [this, serviceInfo](int value) {
        Q_UNUSED(value)  // Parameter not used in current implementation
        handleAudioMeterClicked(serviceInfo.serviceId);
    });
    
    return serviceWidget;
}

void AudioMonitoringWidget::updateServiceDisplay(quint32 serviceId, const ServiceAudioInfo& serviceInfo)
{
    if (m_serviceLabels.contains(serviceId)) {
        m_serviceLabels[serviceId]->setText(serviceInfo.serviceName);
    }
    
    if (m_audioMeters.contains(serviceId)) {
        m_audioMeters[serviceId]->setValue(serviceInfo.audioLevel);
    }
    
    if (m_codecLabels.contains(serviceId)) {
        m_codecLabels[serviceId]->setText(getCodecDescription(serviceInfo.audioCodec));
        m_codecLabels[serviceId]->setStyleSheet(
            serviceInfo.isDabPlus ? "color: #4CAF50; font-weight: bold;" : "color: #FF9800;"
        );
    }
    
    if (m_qualityLabels.contains(serviceId)) {
        m_qualityLabels[serviceId]->setText(getQualityDescription(serviceInfo.quality));
        m_qualityLabels[serviceId]->setStyleSheet(
            QString("color: %1;").arg(getAudioQualityColor(serviceInfo.quality).name())
        );
    }
    
    if (m_bitrateLabels.contains(serviceId)) {
        m_bitrateLabels[serviceId]->setText(formatBitrate(serviceInfo.bitRate));
    }
}

void AudioMonitoringWidget::addServiceToMonitoring(const ServiceAudioInfo& serviceInfo)
{
    if (m_serviceWidgets.contains(serviceInfo.serviceId)) {
        return; // Already exists
    }
    
    QWidget* serviceWidget = createServiceDisplay(serviceInfo);
    m_serviceWidgets[serviceInfo.serviceId] = serviceWidget;
    m_scrollLayout->addWidget(serviceWidget);
    
    // Apply broadcast styling to the new widget
    serviceWidget->setStyleSheet(
        "QWidget {"
        "    background-color: #3C3C3C;"
        "    border: 1px solid #555555;"
        "    border-radius: 4px;"
        "    margin: 2px;"
        "}"
        "QWidget:hover {"
        "    background-color: #404040;"
        "    border-color: #0078D4;"
        "}"
    );
}

void AudioMonitoringWidget::removeServiceFromMonitoring(quint32 serviceId)
{
    if (m_serviceWidgets.contains(serviceId)) {
        QWidget* widget = m_serviceWidgets.take(serviceId);
        m_scrollLayout->removeWidget(widget);
        widget->deleteLater();
    }
    
    // Remove from all maps
    m_serviceLabels.remove(serviceId);
    m_audioMeters.remove(serviceId);
    m_codecLabels.remove(serviceId);
    m_qualityLabels.remove(serviceId);
    m_bitrateLabels.remove(serviceId);
}

QColor AudioMonitoringWidget::getAudioQualityColor(AudioQuality quality) const
{
    switch (quality) {
        case AudioQuality::Excellent:
            return QColor("#4CAF50"); // Green
        case AudioQuality::Good:
            return QColor("#8BC34A"); // Light Green
        case AudioQuality::Fair:
            return QColor("#FF9800"); // Orange
        case AudioQuality::Poor:
            return QColor("#F44336"); // Red
        case AudioQuality::Unknown:
        default:
            return QColor("#9E9E9E"); // Gray
    }
}

QIcon AudioMonitoringWidget::getCodecIcon(AudioCodec codec) const
{
    Q_UNUSED(codec)  // For future codec-specific icon implementation
    // Return appropriate icons based on codec type
    // For now, return null icons - would be implemented with actual icon resources
    return QIcon();
}

QString AudioMonitoringWidget::getCodecDescription(AudioCodec codec) const
{
    switch (codec) {
        case AudioCodec::DABPlus:
            return "DAB+ HE-AAC";
        case AudioCodec::DABPlusSBR:
            return "DAB+ HE-AAC+SBR";
        case AudioCodec::DABPlusPS:
            return "DAB+ HE-AAC+PS";
        case AudioCodec::DAB:
            return "DAB MPEG-1 L2";
        case AudioCodec::Unknown:
        default:
            return "Unknown";
    }
}

QString AudioMonitoringWidget::getQualityDescription(AudioQuality quality) const
{
    switch (quality) {
        case AudioQuality::Excellent:
            return "Excellent";
        case AudioQuality::Good:
            return "Good";
        case AudioQuality::Fair:
            return "Fair";
        case AudioQuality::Poor:
            return "Poor";
        case AudioQuality::Unknown:
        default:
            return "Unknown";
    }
}

QString AudioMonitoringWidget::formatBitrate(quint32 bitrate) const
{
    return QString("%1 kbps").arg(bitrate);
}

QString AudioMonitoringWidget::formatComplianceScore(double score) const
{
    return QString("Compliance Score: %1%").arg(score, 0, 'f', 1);
}

AudioMonitoringWidget::AudioQuality AudioMonitoringWidget::calculateAudioQuality(
    double signalStrength, double errorRate, quint32 bitrate) const
{
    // Calculate quality based on multiple factors
    double qualityScore = 0.0;
    
    // Signal strength component (0-40 points)
    if (signalStrength >= -60.0) {
        qualityScore += 40.0;
    } else if (signalStrength >= -80.0) {
        qualityScore += 20.0 + (signalStrength + 80.0) * 20.0 / 20.0;
    }
    
    // Error rate component (0-30 points)
    if (errorRate <= 1.0) {
        qualityScore += 30.0;
    } else if (errorRate <= 5.0) {
        qualityScore += 30.0 - (errorRate - 1.0) * 7.5;
    }
    
    // Bitrate component (0-30 points)
    if (bitrate >= 128) {
        qualityScore += 30.0;
    } else if (bitrate >= 64) {
        qualityScore += 15.0 + (bitrate - 64) * 15.0 / 64.0;
    } else if (bitrate >= 32) {
        qualityScore += (bitrate - 32) * 15.0 / 32.0;
    }
    
    // Convert score to quality enum
    if (qualityScore >= QUALITY_THRESHOLD_EXCELLENT) {
        return AudioQuality::Excellent;
    } else if (qualityScore >= QUALITY_THRESHOLD_GOOD) {
        return AudioQuality::Good;
    } else if (qualityScore >= QUALITY_THRESHOLD_FAIR) {
        return AudioQuality::Fair;
    } else if (qualityScore >= QUALITY_THRESHOLD_POOR) {
        return AudioQuality::Poor;
    } else {
        return AudioQuality::Poor;
    }
}

void AudioMonitoringWidget::updateStatisticsLabels()
{
    if (m_totalServicesLabel) {
        m_totalServicesLabel->setText(QString("Total Services: %1").arg(m_totalServices));
    }
    
    if (m_dabPlusServicesLabel) {
        m_dabPlusServicesLabel->setText(QString("DAB+ Services: %1").arg(m_dabPlusServices));
        
        // Color-code DAB+ count
        if (m_dabPlusServices > 0) {
            m_dabPlusServicesLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else {
            m_dabPlusServicesLabel->setStyleSheet("color: #FF9800;");
        }
    }
    
    if (m_activeServicesLabel) {
        m_activeServicesLabel->setText(QString("Active Services: %1").arg(m_activeServices));
    }
    
    if (m_totalBitrateLabel) {
        double totalBitrate = m_ensembleStatistics.totalBitrate;
        m_totalBitrateLabel->setText(QString("Total Bitrate: %1 Mbps").arg(totalBitrate, 0, 'f', 2));
    }
}

bool AudioMonitoringWidget::validateState() const
{
    // Basic validation of widget state
    return m_mainLayout != nullptr && 
           m_scrollArea != nullptr && 
           m_updateTimer != nullptr;
}

void AudioMonitoringWidget::applyBroadcastStyling()
{
    // Apply professional broadcast industry styling
    setStyleSheet(
        "AudioMonitoringWidget {"
        "    background-color: #2D2D30;"
        "    color: white;"
        "    font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 1px solid #555555;"
        "    border-radius: 5px;"
        "    margin-top: 8px;"
        "    padding-top: 4px;"
        "    background-color: #3C3C3C;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "    color: #0078D4;"
        "}"
        "QPushButton {"
        "    background-color: #0078D4;"
        "    border: 1px solid #005A9E;"
        "    color: white;"
        "    padding: 4px 8px;"
        "    border-radius: 3px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106EBE;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005A9E;"
        "}"
        "QComboBox {"
        "    background-color: #3C3C3C;"
        "    border: 1px solid #555555;"
        "    color: white;"
        "    padding: 4px;"
        "    border-radius: 3px;"
        "}"
        "QScrollArea {"
        "    background-color: #2D2D30;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "}"
    );
    
    // Set fonts
    m_headerFont = QFont("Segoe UI", 10, QFont::Bold);
    m_serviceFont = QFont("Segoe UI", 9);
    m_statisticsFont = QFont("Segoe UI", 8);
}

void AudioMonitoringWidget::setupSignalConnections()
{
    // Internal signal connections are set up in createUI methods
    // External connections (to ETI processor, etc.) are handled in setEtiProcessor
}