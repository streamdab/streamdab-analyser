/**
 * QUICK INTEGRATION SNIPPET - Copy-Paste Ready Code
 *
 * File: /home/seksan/workspace/streamdab-analyzer/src/main.cpp
 *
 * Use this file for quick copy-paste integration.
 * See SERVICE_TREE_INTEGRATION_GUIDE.md for detailed instructions.
 */

// ============================================================================
// PART 1: CLASS DECLARATIONS
// Insert at line ~1411 (after existing onServiceDiscovered declaration)
// ============================================================================

    void onServiceDiscovered(const ServiceInfo& service);  // EXISTING

    // NEW METHOD DECLARATIONS - Add these
    void updateHierarchicalServiceTree();
    void onServicesUpdated();
    void onServiceTreeSelectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    void updateEnsembleTreeWithRealData();  // NEW HELPER METHOD

// ============================================================================
// PART 2: SIGNAL CONNECTIONS
// Insert at line ~2270 (after existing serviceDiscovered connection)
// ============================================================================

    // EXISTING CONNECTION
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::serviceDiscovered,
            this, &DABAnalyzerWindow::onServiceDiscovered);

    // NEW CONNECTIONS - Add these 3 connections
    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::servicesUpdated,
            this, &DABAnalyzerWindow::onServicesUpdated);

    connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::ensembleInfoUpdated,
            this, &DABAnalyzerWindow::updateHierarchicalServiceTree);

    connect(m_hierarchicalServiceTree, &QTreeWidget::currentItemChanged,
            this, &DABAnalyzerWindow::onServiceTreeSelectionChanged);

// ============================================================================
// PART 3: METHOD IMPLEMENTATIONS
// Insert after updateEnsembleTree() method (line ~2358)
// ============================================================================

// ----------------------------------------------------------------------------
// Main tree update method (270 lines) - Full Implementation
// ----------------------------------------------------------------------------
void DABAnalyzerWindow::updateHierarchicalServiceTree()
{
    if (!m_hierarchicalServiceTree || !m_figAnalyzer) {
        qWarning() << "updateHierarchicalServiceTree: Null widget or analyzer";
        return;
    }

    // Save expansion state
    QSet<QString> expandedItems;
    for (int i = 0; i < m_hierarchicalServiceTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_hierarchicalServiceTree->topLevelItem(i);
        if (item->isExpanded()) {
            expandedItems.insert(item->text(0));
        }
    }

    m_hierarchicalServiceTree->clear();

    // Get real data from FIG analyzer
    EnsembleInfo ensemble = m_figAnalyzer->getCurrentEnsemble();
    std::vector<DABService> services = m_figAnalyzer->getDABServices();

    qInfo() << "updateHierarchicalServiceTree: Ensemble ID" << Qt::hex << ensemble.ensembleId
            << "Services:" << services.size();

    // Create ensemble root
    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem(m_hierarchicalServiceTree);

    QString ensembleLabel = ensemble.ensembleLabel.isEmpty()
        ? QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
        : ensemble.ensembleLabel.trimmed();

    ensembleItem->setText(0, ensembleLabel);
    ensembleItem->setText(1, "Ensemble");
    ensembleItem->setText(2, QString("EID: 0x%1 | Country: %2")
        .arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
        .arg(ensemble.countryCode.isEmpty() ? "Unknown" : ensemble.countryCode));

    ensembleItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

    QFont ensembleFont = ensembleItem->font(0);
    ensembleFont.setBold(true);
    ensembleItem->setFont(0, ensembleFont);
    ensembleItem->setFont(1, ensembleFont);

    ensembleItem->setToolTip(0, QString(
        "Ensemble: %1\n"
        "EID: 0x%2\n"
        "Country: %3\n"
        "Services: %4"
    ).arg(ensembleLabel)
     .arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
     .arg(ensemble.countryCode.isEmpty() ? "Unknown" : ensemble.countryCode)
     .arg(services.size()));

    int audioServiceCount = 0;
    int dataServiceCount = 0;

    // Populate services
    for (const auto& service : services) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleItem);

        QString serviceLabel = service.service_label.isEmpty()
            ? QString("Service 0x%1").arg(service.service_id, 8, 16, QChar('0')).toUpper()
            : service.service_label.trimmed();

        QString serviceType;
        QStyle::StandardPixmap iconType;

        if (service.service_type == 0) {
            serviceType = service.is_dab_plus ? "DAB+ Audio" : "DAB Audio";
            iconType = QStyle::SP_MediaPlay;
            audioServiceCount++;
        } else {
            serviceType = "Data Service";
            iconType = QStyle::SP_FileIcon;
            dataServiceCount++;
        }

        serviceItem->setText(0, serviceLabel);
        serviceItem->setText(1, serviceType);
        serviceItem->setText(2, QString("SID: 0x%1 | %2 components")
            .arg(service.service_id, 8, 16, QChar('0')).toUpper()
            .arg(service.components.size()));

        serviceItem->setIcon(0, style()->standardIcon(iconType));

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

        if (service.is_dab_plus) {
            QFont dabPlusFont = serviceItem->font(0);
            dabPlusFont.setBold(true);
            serviceItem->setFont(0, dabPlusFont);
            serviceItem->setForeground(1, QBrush(QColor(0, 102, 204)));
        }

        // Add service components
        for (size_t i = 0; i < service.components.size(); ++i) {
            const auto& component = service.components[i];

            QTreeWidgetItem* componentItem = new QTreeWidgetItem(serviceItem);

            QString componentLabel = QString("Component %1").arg(i + 1);
            if (component.is_primary) {
                componentLabel += " (Primary)";
            }

            SubChannelInfo subchInfo = m_figAnalyzer->getSubChannelInfo(component.sub_channel_id);

            QString componentType;
            if (component.transport_mode == 0) {
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
                componentDetails = QString("SubCh %1 | Start: %2 CU | %3 | %4 kbps")
                    .arg(component.sub_channel_id)
                    .arg(subchInfo.start_address)
                    .arg(subchInfo.getProtectionLevel())
                    .arg(subchInfo.getBitrate());
            } else {
                componentDetails = QString("SubCh %1 | Details pending FIG 0/1")
                    .arg(component.sub_channel_id);
            }

            componentItem->setText(0, componentLabel);
            componentItem->setText(1, componentType);
            componentItem->setText(2, componentDetails);

            componentItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaVolume));

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

            QFont componentFont = componentItem->font(0);
            componentFont.setPointSize(componentFont.pointSize() - 1);
            componentItem->setFont(0, componentFont);
            componentItem->setFont(1, componentFont);
            componentItem->setFont(2, componentFont);
        }
    }

    // Restore expansion state
    if (expandedItems.contains(ensembleLabel)) {
        ensembleItem->setExpanded(true);
    } else {
        m_hierarchicalServiceTree->expandAll();
    }

    // Update extracted information panels
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

    qInfo() << "Hierarchical service tree updated: Ensemble"
            << ensembleLabel << "-" << services.size() << "services"
            << "(" << audioServiceCount << "audio," << dataServiceCount << "data)";

    if (m_systemMessages) {
        m_systemMessages->append(QString(
            "Service Tree Updated: %1 services (%2 audio, %3 data) in ensemble '%4'"
        ).arg(services.size()).arg(audioServiceCount).arg(dataServiceCount).arg(ensembleLabel));
    }
}

// ----------------------------------------------------------------------------
// Signal handler for batch service updates
// ----------------------------------------------------------------------------
void DABAnalyzerWindow::onServicesUpdated()
{
    qDebug() << "onServicesUpdated: FIG analyzer signals service discovery update";
    updateHierarchicalServiceTree();
    updateEnsembleTreeWithRealData();
}

// ----------------------------------------------------------------------------
// Selection change handler
// ----------------------------------------------------------------------------
void DABAnalyzerWindow::onServiceTreeSelectionChanged(
    QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (!current) {
        return;
    }

    QString itemName = current->text(0);
    QString itemType = current->text(1);
    QString itemDetails = current->text(2);

    if (m_systemMessages) {
        m_systemMessages->append(QString(
            "Selected: %1 | Type: %2 | %3"
        ).arg(itemName).arg(itemType).arg(itemDetails));
    }

    if (m_figAnalysisDetails) {
        QString detailsHtml = "<h3>Selected Item Details</h3>";
        detailsHtml += QString("<p><b>Name:</b> %1</p>").arg(itemName);
        detailsHtml += QString("<p><b>Type:</b> %1</p>").arg(itemType);
        detailsHtml += QString("<p><b>Details:</b> %1</p>").arg(itemDetails);

        QString tooltip = current->toolTip(0);
        if (!tooltip.isEmpty()) {
            detailsHtml += "<h4>Additional Information</h4>";
            detailsHtml += QString("<pre>%1</pre>").arg(tooltip.replace("\n", "<br>"));
        }

        m_figAnalysisDetails->setHtml(detailsHtml);
    }

    qDebug() << "Service tree selection:" << itemName << "-" << itemType;
}

// ----------------------------------------------------------------------------
// Legacy ensemble tree update helper
// ----------------------------------------------------------------------------
void DABAnalyzerWindow::updateEnsembleTreeWithRealData()
{
    if (!m_ensembleTree || !m_figAnalyzer) {
        return;
    }

    m_ensembleTree->clear();

    EnsembleInfo ensemble = m_figAnalyzer->getCurrentEnsemble();
    std::vector<DABService> services = m_figAnalyzer->getDABServices();

    QTreeWidgetItem* ensembleItem = new QTreeWidgetItem();

    QString ensembleLabel = ensemble.ensembleLabel.isEmpty()
        ? QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0'))
        : ensemble.ensembleLabel.trimmed();

    ensembleItem->setText(0, ensembleLabel);
    ensembleItem->setText(1, QString("EID: 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')));
    ensembleItem->setText(2, "Ensemble");
    ensembleItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

    m_ensembleTree->addTopLevelItem(ensembleItem);

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
// PART 4: UPDATE EXISTING onServiceDiscovered() METHOD
// Replace existing method at line ~1411
// ============================================================================

void DABAnalyzerWindow::onServiceDiscovered(const ServiceInfo& service)
{
    if (m_ensembleTree) {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem();
        serviceItem->setText(0, QString("Service 0x%1").arg(service.serviceId, 8, 16, QChar('0')));
        serviceItem->setText(1, service.label.isEmpty() ? "Unknown Service" : service.label);
        serviceItem->setText(2, service.isAudio ? "Audio" : "Data");

        if (service.isAudio) {
            serviceItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
        } else {
            serviceItem->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }

        m_ensembleTree->addTopLevelItem(serviceItem);
    }

    if (m_systemMessages) {
        m_systemMessages->append(QString("Discovered service: %1 (%2)")
                                 .arg(service.label.isEmpty() ? QString("SID 0x%1")
                                     .arg(service.serviceId, 8, 16, QChar('0')) : service.label)
                                 .arg(service.isAudio ? "Audio" : "Data"));
    }

    qInfo() << "Service discovered: SID" << Qt::hex << service.serviceId
            << "Label:" << service.label << "Type:" << (service.isAudio ? "Audio" : "Data");
}

// ============================================================================
// INTEGRATION COMPLETE
// ============================================================================
