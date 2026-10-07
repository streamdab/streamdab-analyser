/**
 * @file SERVICE_TREE_UPDATE_IMPLEMENTATION.cpp
 * @brief Qt6 Service Tree Population with Real DAB Services
 *
 * Implementation code for updating the hierarchical service tree widget
 * with real discovered DAB services from AdvancedFIGAnalyser.
 *
 * Integration point: /home/seksan/workspace/streamdab-analyser/src/main.cpp
 *
 * Components to modify:
 * 1. Add updateHierarchicalServiceTree() method (replaces hardcoded simulation)
 * 2. Connect to servicesUpdated() signal from AdvancedFIGAnalyser
 * 3. Add service item icons and formatting
 * 4. Update onServiceDiscovered() to use hierarchical tree
 *
 * @see AdvancedFIGAnalyser::getDABServices()
 * @see AdvancedFIGAnalyser::getCurrentEnsemble()
 */

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QIcon>
#include <QPixmap>
#include <QStyle>
#include <QtCore/QDebug>

// ============================================================================
// STEP 1: Add new method declarations to DABAnalyserWindow class
// ============================================================================

// INSERT THESE DECLARATIONS in the private slots section of DABAnalyserWindow class
// (around line 1411, after onServiceDiscovered)

private slots:
    /**
     * @brief Update hierarchical service tree with real discovered DAB services
     *
     * Called when FIG analyser completes service discovery via servicesUpdated() signal.
     * Populates both m_hierarchicalServiceTree and m_ensembleTree with real data.
     *
     * Tree structure:
     * - Ensemble (root)
     *   - Service 1 (label, type, ID)
     *     - Component 1 (subchannel, bitrate, protection)
     *     - Component 2 (if multi-component)
     *   - Service 2
     *     - Component 1
     */
    void updateHierarchicalServiceTree();

    /**
     * @brief Handle real-time service updates from FIG processing
     *
     * Connected to AdvancedFIGAnalyser::servicesUpdated() signal.
     * Triggers immediate tree refresh with new service data.
     */
    void onServicesUpdated();

    /**
     * @brief Handle service tree item selection
     *
     * Displays detailed information about selected service/component
     * in the extracted information panel.
     *
     * @param current Currently selected tree item
     * @param previous Previously selected tree item
     */
    void onServiceTreeSelectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

// ============================================================================
// STEP 2: Add signal connection in setupSignalConnections()
// ============================================================================

// INSERT THIS CODE in setupSignalConnections() method (around line 2268-2270)
// Add after the existing serviceDiscovered connection:

void DABAnalyserWindow::setupSignalConnections()
{
    // ... existing connections ...

    // FIG Analyser signals for service discovery
    connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::serviceDiscovered,
            this, &DABAnalyserWindow::onServiceDiscovered);

    // NEW: Connect to servicesUpdated signal for batch updates
    connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::servicesUpdated,
            this, &DABAnalyserWindow::onServicesUpdated);

    // NEW: Connect to ensembleInfoUpdated signal for ensemble metadata
    connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::ensembleInfoUpdated,
            this, &DABAnalyserWindow::updateHierarchicalServiceTree);

    // NEW: Connect service tree selection handler
    connect(m_hierarchicalServiceTree, &QTreeWidget::currentItemChanged,
            this, &DABAnalyserWindow::onServiceTreeSelectionChanged);

    // ... existing connections ...
}

// ============================================================================
// STEP 3: Implementation of updateHierarchicalServiceTree()
// ============================================================================

/**
 * @brief Update hierarchical service tree with real discovered DAB services
 *
 * Replaces simulation data with real DAB services from FIG processing.
 * Creates professional tree structure with ensemble -> services -> components hierarchy.
 */
void DABAnalyserWindow::updateHierarchicalServiceTree()
{
    if (!m_hierarchicalServiceTree || !m_figAnalyser) {
        qWarning() << "updateHierarchicalServiceTree: Null widget or analyser";
        return;
    }

    // Save current expansion state
    QSet<QString> expandedItems;
    for (int i = 0; i < m_hierarchicalServiceTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_hierarchicalServiceTree->topLevelItem(i);
        if (item->isExpanded()) {
            expandedItems.insert(item->text(0));
        }
    }

    // Clear existing tree (replace simulation data)
    m_hierarchicalServiceTree->clear();

    // Get real ensemble information from FIG analyser
    EnsembleInfo ensemble = m_figAnalyser->getCurrentEnsemble();
    std::vector<DABService> services = m_figAnalyser->getDABServices();

    qInfo() << "updateHierarchicalServiceTree: Ensemble ID" << Qt::hex << ensemble.ensembleId
            << "Services:" << services.size();

    // Create ensemble root item
    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem(m_hierarchicalServiceTree);

    // Ensemble label (from FIG 1/0) or fallback to ID
    QString ensembleLabel = ensemble.ensembleLabel.isEmpty()
        ? QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
        : ensemble.ensembleLabel.trimmed();

    ensembleItem->setText(0, ensembleLabel);
    ensembleItem->setText(1, "Ensemble");
    ensembleItem->setText(2, QString("EID: 0x%1 | Country: %2")
        .arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
        .arg(ensemble.countryCode.isEmpty() ? "Unknown" : ensemble.countryCode));

    // Set ensemble icon (folder icon)
    ensembleItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

    // Make ensemble bold
    QFont ensembleFont = ensembleItem->font(0);
    ensembleFont.setBold(true);
    ensembleItem->setFont(0, ensembleFont);
    ensembleItem->setFont(1, ensembleFont);

    // Add tooltip with detailed ensemble info
    ensembleItem->setToolTip(0, QString(
        "Ensemble: %1\n"
        "EID: 0x%2\n"
        "Country: %3\n"
        "Services: %4"
    ).arg(ensembleLabel)
     .arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
     .arg(ensemble.countryCode.isEmpty() ? "Unknown" : ensemble.countryCode)
     .arg(services.size()));

    // Populate services
    int audioServiceCount = 0;
    int dataServiceCount = 0;

    for (const auto& service : services) {
        // Create service item
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);

        // Service label (from FIG 1/1) or fallback to ID
        QString serviceLabel = service.service_label.isEmpty()
            ? QString("Service 0x%1").arg(service.service_id, 8, 16, QChar('0')).toUpper()
            : service.service_label.trimmed();

        // Determine service type and icon
        QString serviceType;
        QStyle::StandardPixmap iconType;

        if (service.service_type == 0) {
            // Audio service
            serviceType = service.is_dab_plus ? "DAB+ Audio" : "DAB Audio";
            iconType = QStyle::SP_MediaPlay;
            audioServiceCount++;
        } else {
            // Data service
            serviceType = "Data Service";
            iconType = QStyle::SP_FileIcon;
            dataServiceCount++;
        }

        serviceItem->setText(0, serviceLabel);
        serviceItem->setText(1, serviceType);
        serviceItem->setText(2, QString("SID: 0x%1 | %2 components")
            .arg(service.service_id, 8, 16, QChar('0')).toUpper()
            .arg(service.components.size()));

        // Set service icon
        serviceItem->setIcon(0, style()->standardIcon(iconType));

        // Add tooltip with detailed service info
        QString shortLabel = service.short_label.isEmpty() ? "N/A" : service.short_label;
        serviceItem->setToolTip(0, QString(
            "Service: %1\n"
            "Short Label: %2\n"
            "SID: 0x%3\n"
            "Type: %4\n"
            "Components: %5\n"
            "Conditional Access: %6"
        ).arg(serviceLabel)
         .arg(shortLabel)
         .arg(service.service_id, 8, 16, QChar('0')).toUpper()
         .arg(serviceType)
         .arg(service.components.size())
         .arg(service.ca_flag ? "Yes (Encrypted)" : "No (Free)"));

        // Highlight DAB+ services
        if (service.is_dab_plus) {
            QFont dabPlusFont = serviceItem->font(0);
            dabPlusFont.setBold(true);
            serviceItem->setFont(0, dabPlusFont);

            // Set blue color for DAB+ services
            serviceItem->setForeground(1, QBrush(QColor(0, 102, 204)));
        }

        // Add service components (subchannels)
        for (size_t i = 0; i < service.components.size(); ++i) {
            const auto& component = service.components[i];

            QTreeWidgetItem* componentItem = new QTreeWidgetItem(serviceItem);

            // Component label
            QString componentLabel = QString("Component %1").arg(i + 1);
            if (component.is_primary) {
                componentLabel += " (Primary)";
            }

            // Get subchannel details from FIG 0/1
            SubChannelInfo subchInfo = m_figAnalyser->getSubChannelInfo(component.sub_channel_id);

            QString componentType;
            if (component.transport_mode == 0) {
                // MSC Stream Audio
                if (component.service_component_type == 0x00) {
                    componentType = "DAB Audio";
                } else if (component.service_component_type == 0x3F) {
                    componentType = "DAB+ Audio (HE-AAC)";
                } else {
                    componentType = QString("Audio Type 0x%1")
                        .arg(component.service_component_type, 2, 16, QChar('0'));
                }
            } else {
                componentType = QString("TMId %1").arg(component.transport_mode);
            }

            QString componentDetails;
            if (subchInfo.sub_channel_id == component.sub_channel_id) {
                // Found subchannel info
                componentDetails = QString("SubCh %1 | Start: %2 CU | %3 | %4 kbps")
                    .arg(component.sub_channel_id)
                    .arg(subchInfo.start_address)
                    .arg(subchInfo.getProtectionLevel())
                    .arg(subchInfo.getBitrate());
            } else {
                // Subchannel info not available yet (FIG 0/1 not processed)
                componentDetails = QString("SubCh %1 | Details pending FIG 0/1")
                    .arg(component.sub_channel_id);
            }

            componentItem->setText(0, componentLabel);
            componentItem->setText(1, componentType);
            componentItem->setText(2, componentDetails);

            // Set component icon (smaller media icon)
            componentItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaVolume));

            // Add tooltip with component details
            componentItem->setToolTip(0, QString(
                "Component ID: %1\n"
                "Subchannel: %2\n"
                "Type: %3\n"
                "Transport Mode: %4\n"
                "Primary Component: %5\n"
                "Conditional Access: %6\n"
                "Protection: %7\n"
                "Bitrate: %8 kbps"
            ).arg(component.component_id)
             .arg(component.sub_channel_id)
             .arg(componentType)
             .arg(component.transport_mode)
             .arg(component.is_primary ? "Yes" : "No")
             .arg(component.ca_flag ? "Yes (Encrypted)" : "No (Free)")
             .arg(subchInfo.getProtectionLevel())
             .arg(subchInfo.getBitrate()));

            // Indent component items
            QFont componentFont = componentItem->font(0);
            componentFont.setPointSize(componentFont.pointSize() - 1);
            componentItem->setFont(0, componentFont);
            componentItem->setFont(1, componentFont);
            componentItem->setFont(2, componentFont);
        }
    }

    // Restore expansion state or expand all by default
    if (expandedItems.contains(ensembleLabel)) {
        ensembleItem->setExpanded(true);
    } else {
        // Expand by default for better UX
        m_hierarchicalServiceTree->expandAll();
    }

    // Update extracted information panel
    if (m_extractedServiceLabels) {
        QString labelsText;
        for (const auto& service : services) {
            if (!service.service_label.isEmpty()) {
                labelsText += QString("• 0x%1: %2")
                    .arg(service.service_id, 8, 16, QChar('0')).toUpper()
                    .arg(service.service_label.trimmed());

                if (!service.short_label.isEmpty()) {
                    labelsText += QString(" (%1)").arg(service.short_label);
                }

                labelsText += "\n";
            }
        }

        if (labelsText.isEmpty()) {
            labelsText = "No service labels discovered yet.\nWaiting for FIG 1/1 processing...";
        }

        m_extractedServiceLabels->setPlainText(labelsText);
    }

    // Update programme information panel
    if (m_extractedProgrammeInfo) {
        QString programmeText;
        programmeText += QString("Total Services: %1\n").arg(services.size());
        programmeText += QString("Audio Services: %1\n").arg(audioServiceCount);
        programmeText += QString("Data Services: %1\n").arg(dataServiceCount);
        programmeText += QString("\nEnsemble: %1\n").arg(ensembleLabel);
        programmeText += QString("Country: %1\n").arg(
            ensemble.countryCode.isEmpty() ? "Unknown" : ensemble.countryCode);

        m_extractedProgrammeInfo->setPlainText(programmeText);
    }

    // Log update completion
    qInfo() << "Hierarchical service tree updated: Ensemble"
            << ensembleLabel << "-" << services.size() << "services"
            << "(" << audioServiceCount << "audio," << dataServiceCount << "data)";

    // Update system messages
    if (m_systemMessages) {
        m_systemMessages->append(QString(
            "🌳 Service Tree Updated: %1 services (%2 audio, %3 data) in ensemble '%4'"
        ).arg(services.size()).arg(audioServiceCount).arg(dataServiceCount).arg(ensembleLabel));
    }
}

// ============================================================================
// STEP 4: Implementation of onServicesUpdated()
// ============================================================================

/**
 * @brief Handle real-time service updates from FIG processing
 *
 * Slot connected to AdvancedFIGAnalyser::servicesUpdated() signal.
 * Triggered when FIG 0/2 discovers new services or updates existing ones.
 */
void DABAnalyserWindow::onServicesUpdated()
{
    qDebug() << "onServicesUpdated: FIG analyser signals service discovery update";

    // Trigger full tree refresh with new service data
    updateHierarchicalServiceTree();

    // Also update legacy ensemble tree for backward compatibility
    updateEnsembleTreeWithRealData();
}

// ============================================================================
// STEP 5: Implementation of onServiceTreeSelectionChanged()
// ============================================================================

/**
 * @brief Handle service tree item selection
 *
 * Displays detailed information about selected service/component
 * in the extracted information panel or details viewer.
 */
void DABAnalyserWindow::onServiceTreeSelectionChanged(
    QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (!current) {
        return;
    }

    // Get item text and type
    QString itemName = current->text(0);
    QString itemType = current->text(1);
    QString itemDetails = current->text(2);

    // Display detailed information in system messages
    if (m_systemMessages) {
        m_systemMessages->append(QString(
            "🔍 Selected: %1 | Type: %2 | %3"
        ).arg(itemName).arg(itemType).arg(itemDetails));
    }

    // Update FIG analysis details panel with service info
    if (m_figAnalysisDetails) {
        QString detailsHtml = "<h3>Selected Item Details</h3>";
        detailsHtml += QString("<p><b>Name:</b> %1</p>").arg(itemName);
        detailsHtml += QString("<p><b>Type:</b> %1</p>").arg(itemType);
        detailsHtml += QString("<p><b>Details:</b> %1</p>").arg(itemDetails);

        // Add tooltip content as additional details
        QString tooltip = current->toolTip(0);
        if (!tooltip.isEmpty()) {
            detailsHtml += "<h4>Additional Information</h4>";
            detailsHtml += QString("<pre>%1</pre>").arg(tooltip.replace("\n", "<br>"));
        }

        m_figAnalysisDetails->setHtml(detailsHtml);
    }

    qDebug() << "Service tree selection:" << itemName << "-" << itemType;
}

// ============================================================================
// STEP 6: Update legacy onServiceDiscovered() for compatibility
// ============================================================================

/**
 * @brief Handle individual service discovery (legacy handler)
 *
 * Updates m_ensembleTree with individual services as they're discovered.
 * Kept for backward compatibility with incremental discovery workflow.
 *
 * NOTE: Modern workflow uses onServicesUpdated() for batch updates.
 */
void DABAnalyserWindow::onServiceDiscovered(const ServiceInfo& service)
{
    // Legacy: Add service to m_ensembleTree (for backward compatibility)
    if (m_ensembleTree) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem();
        serviceItem->setText(0, QString("Service 0x%1").arg(service.serviceId, 8, 16, QChar('0')));
        serviceItem->setText(1, service.label.isEmpty() ? "Unknown Service" : service.label);
        serviceItem->setText(2, service.isAudio ? "Audio" : "Data");

        // Set icon based on type
        if (service.isAudio) {
            serviceItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
        } else {
            serviceItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }

        m_ensembleTree->addTopLevelItem(serviceItem);
    }

    // Log discovery
    if (m_systemMessages) {
        m_systemMessages->append(QString("🎵 Discovered service: %1 (%2)")
                                 .arg(service.label.isEmpty() ? QString("SID 0x%1")
                                     .arg(service.serviceId, 8, 16, QChar('0')) : service.label)
                                 .arg(service.isAudio ? "Audio" : "Data"));
    }

    qInfo() << "Service discovered: SID" << Qt::hex << service.serviceId
            << "Label:" << service.label << "Type:" << (service.isAudio ? "Audio" : "Data");
}

// ============================================================================
// STEP 7: Add updateEnsembleTreeWithRealData() helper method
// ============================================================================

/**
 * @brief Update legacy ensemble tree with real data
 *
 * Helper method to populate m_ensembleTree (legacy widget) with real services.
 * Maintains backward compatibility while transitioning to hierarchical tree.
 */
void DABAnalyserWindow::updateEnsembleTreeWithRealData()
{
    if (!m_ensembleTree || !m_figAnalyser) {
        return;
    }

    // Clear existing items
    m_ensembleTree->clear();

    // Get real data from FIG analyser
    EnsembleInfo ensemble = m_figAnalyser->getCurrentEnsemble();
    std::vector<DABService> services = m_figAnalyser->getDABServices();

    // Create ensemble root
    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem();

    QString ensembleLabel = ensemble.ensembleLabel.isEmpty()
        ? QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0'))
        : ensemble.ensembleLabel.trimmed();

    ensembleItem->setText(0, ensembleLabel);
    ensembleItem->setText(1, QString("EID: 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')));
    ensembleItem->setText(2, "Ensemble");
    ensembleItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

    m_ensembleTree->addTopLevelItem(ensembleItem);

    // Add services as children
    for (const auto& service : services) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);

        QString serviceLabel = service.service_label.isEmpty()
            ? QString("Service 0x%1").arg(service.service_id, 8, 16, QChar('0'))
            : service.service_label.trimmed();

        serviceItem->setText(0, serviceLabel);
        serviceItem->setText(1, QString("SID: 0x%1").arg(service.service_id, 8, 16, QChar('0')));
        serviceItem->setText(2, service.service_type == 0 ? "Audio" : "Data");

        if (service.service_type == 0) {
            serviceItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
        } else {
            serviceItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }
    }

    m_ensembleTree->expandAll();
}

// ============================================================================
// STEP 8: Add helper method declaration to class
// ============================================================================

// INSERT THIS DECLARATION in private methods section:
private:
    /**
     * @brief Update legacy ensemble tree with real data
     *
     * Populates m_ensembleTree widget with real services for backward compatibility.
     */
    void updateEnsembleTreeWithRealData();

// ============================================================================
// INTEGRATION SUMMARY
// ============================================================================

/*
 * INTEGRATION CHECKLIST:
 *
 * □ 1. Add method declarations to DABAnalyserWindow class (private slots section)
 *      - updateHierarchicalServiceTree()
 *      - onServicesUpdated()
 *      - onServiceTreeSelectionChanged()
 *      - updateEnsembleTreeWithRealData() (private helper)
 *
 * □ 2. Add signal connections in setupSignalConnections() method:
 *      - connect(m_figAnalyser, &AdvancedFIGAnalyser::servicesUpdated, ...)
 *      - connect(m_figAnalyser, &AdvancedFIGAnalyser::ensembleInfoUpdated, ...)
 *      - connect(m_hierarchicalServiceTree, &QTreeWidget::currentItemChanged, ...)
 *
 * □ 3. Copy method implementations to main.cpp:
 *      - updateHierarchicalServiceTree() - Full implementation
 *      - onServicesUpdated() - Signal handler
 *      - onServiceTreeSelectionChanged() - Selection handler
 *      - updateEnsembleTreeWithRealData() - Legacy compatibility
 *
 * □ 4. Update existing onServiceDiscovered() method:
 *      - Add icon setting based on service type
 *      - Enhance logging messages
 *
 * □ 5. Test with real ETI file:
 *      - Load ETI file via file dialog
 *      - Verify services appear in hierarchical tree
 *      - Check ensemble metadata (label, EID, country)
 *      - Verify service components with subchannel details
 *      - Test tree expansion/collapse state preservation
 *
 * □ 6. Verify signal flow:
 *      - FIG 0/0 → ensembleInfoUpdated → updateHierarchicalServiceTree
 *      - FIG 0/2 → servicesUpdated → onServicesUpdated → updateHierarchicalServiceTree
 *      - FIG 1/1 → serviceLabelUpdated → (optional) incremental update
 *
 * FILE LOCATIONS:
 * - Implementation target: /home/seksan/workspace/streamdab-analyser/src/main.cpp
 * - AdvancedFIGAnalyser header: /home/seksan/workspace/streamdab-analyser/src/core/advanced_fig_analyser.h
 * - AdvancedFIGAnalyser source: /home/seksan/workspace/streamdab-analyser/src/core/advanced_fig_analyser.cpp
 */

// ============================================================================
// END OF IMPLEMENTATION
// ============================================================================
