#include "context_menu_factory.h"
#include <QIcon>

ContextMenuFactory::ContextMenuFactory(QObject* parent)
    : QObject(parent)
{
}

std::unique_ptr<QMenu> ContextMenuFactory::createContextMenu(MenuType type, const QVariant& context)
{
    Q_UNUSED(context)
    
    auto menu = std::make_unique<QMenu>();
    
    switch (type) {
        case MenuType::ETIFrame:
        case MenuType::General:
        default:
            return createETIAnalysisMenu();
    }
}

std::unique_ptr<QMenu> ContextMenuFactory::createETIAnalysisMenu()
{
    auto menu = std::make_unique<QMenu>("ETI Analysis");
    
    // Extract action
    QAction* extractAction = createAction("Extract Audio", "extract", "Ctrl+E");
    connect(extractAction, &QAction::triggered, this, &ContextMenuFactory::onExtractTriggered);
    menu->addAction(extractAction);
    
    // Analyze action  
    QAction* analyzeAction = createAction("Analyze FIGs", "analyze", "Ctrl+A");
    connect(analyzeAction, &QAction::triggered, this, &ContextMenuFactory::onAnalyzeTriggered);
    menu->addAction(analyzeAction);
    
    // Export action
    QAction* exportAction = createAction("Export Service", "export", "Ctrl+S");
    connect(exportAction, &QAction::triggered, this, &ContextMenuFactory::onExportTriggered);
    menu->addAction(exportAction);
    
    return menu;
}

void ContextMenuFactory::onExtractTriggered()
{
    emit contextActionTriggered("extract", QVariant());
}

void ContextMenuFactory::onAnalyzeTriggered()
{
    emit contextActionTriggered("analyze", QVariant());
}

void ContextMenuFactory::onExportTriggered()
{
    emit contextActionTriggered("export", QVariant());
}

QAction* ContextMenuFactory::createAction(const QString& text, const QString& iconName, const QString& shortcut)
{
    QAction* action = new QAction(text, this);
    
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    
    // Icon could be loaded from resources if available
    // action->setIcon(QIcon(QString(":/icons/%1.png").arg(iconName)));
    
    return action;
}