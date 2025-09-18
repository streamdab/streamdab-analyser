/****************************************************************************
** Meta object code from reading C++ file 'service_browser.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/gui/service_browser.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'service_browser.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14ServiceBrowserE_t {};
} // unnamed namespace

template <> constexpr inline auto ServiceBrowser::qt_create_metaobjectdata<qt_meta_tag_ZN14ServiceBrowserE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ServiceBrowser",
        "serviceSelectionChanged",
        "",
        "serviceId",
        "serviceActivated",
        "serviceUpdated",
        "serviceDiscovered",
        "serviceLost",
        "updateServices",
        "clearServices",
        "refreshServices",
        "selectService",
        "expandAll",
        "collapseAll",
        "filterServices",
        "filterText",
        "handleTreeSelectionChanged",
        "handleTreeItemActivated",
        "QTreeWidgetItem*",
        "item",
        "column",
        "handleTableSelectionChanged",
        "handleTableItemActivated",
        "QTableWidgetItem*",
        "handleListSelectionChanged",
        "handleListItemActivated",
        "QListWidgetItem*",
        "handleViewModeChanged",
        "handleFilterTextChanged",
        "handleRealTimeUpdate",
        "handleSelectService",
        "handleServiceProperties",
        "handleExportServices"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'serviceSelectionChanged'
        QtMocHelpers::SignalData<void(quint32)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Signal 'serviceActivated'
        QtMocHelpers::SignalData<void(quint32)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Signal 'serviceUpdated'
        QtMocHelpers::SignalData<void(quint32)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Signal 'serviceDiscovered'
        QtMocHelpers::SignalData<void(quint32)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Signal 'serviceLost'
        QtMocHelpers::SignalData<void(quint32)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Slot 'updateServices'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'clearServices'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'refreshServices'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectService'
        QtMocHelpers::SlotData<void(quint32)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Slot 'expandAll'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'collapseAll'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'filterServices'
        QtMocHelpers::SlotData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Slot 'handleTreeSelectionChanged'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleTreeItemActivated'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, int)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 18, 19 }, { QMetaType::Int, 20 },
        }}),
        // Slot 'handleTableSelectionChanged'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleTableItemActivated'
        QtMocHelpers::SlotData<void(QTableWidgetItem *)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 23, 19 },
        }}),
        // Slot 'handleListSelectionChanged'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleListItemActivated'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 26, 19 },
        }}),
        // Slot 'handleViewModeChanged'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleFilterTextChanged'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleRealTimeUpdate'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSelectService'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleServiceProperties'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleExportServices'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ServiceBrowser, qt_meta_tag_ZN14ServiceBrowserE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ServiceBrowser::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ServiceBrowserE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ServiceBrowserE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14ServiceBrowserE_t>.metaTypes,
    nullptr
} };

void ServiceBrowser::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ServiceBrowser *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->serviceSelectionChanged((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 1: _t->serviceActivated((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 2: _t->serviceUpdated((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 3: _t->serviceDiscovered((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 4: _t->serviceLost((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 5: _t->updateServices(); break;
        case 6: _t->clearServices(); break;
        case 7: _t->refreshServices(); break;
        case 8: _t->selectService((*reinterpret_cast< std::add_pointer_t<quint32>>(_a[1]))); break;
        case 9: _t->expandAll(); break;
        case 10: _t->collapseAll(); break;
        case 11: _t->filterServices((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->handleTreeSelectionChanged(); break;
        case 13: _t->handleTreeItemActivated((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 14: _t->handleTableSelectionChanged(); break;
        case 15: _t->handleTableItemActivated((*reinterpret_cast< std::add_pointer_t<QTableWidgetItem*>>(_a[1]))); break;
        case 16: _t->handleListSelectionChanged(); break;
        case 17: _t->handleListItemActivated((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 18: _t->handleViewModeChanged(); break;
        case 19: _t->handleFilterTextChanged(); break;
        case 20: _t->handleRealTimeUpdate(); break;
        case 21: _t->handleSelectService(); break;
        case 22: _t->handleServiceProperties(); break;
        case 23: _t->handleExportServices(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ServiceBrowser::*)(quint32 )>(_a, &ServiceBrowser::serviceSelectionChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ServiceBrowser::*)(quint32 )>(_a, &ServiceBrowser::serviceActivated, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ServiceBrowser::*)(quint32 )>(_a, &ServiceBrowser::serviceUpdated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ServiceBrowser::*)(quint32 )>(_a, &ServiceBrowser::serviceDiscovered, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ServiceBrowser::*)(quint32 )>(_a, &ServiceBrowser::serviceLost, 4))
            return;
    }
}

const QMetaObject *ServiceBrowser::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ServiceBrowser::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ServiceBrowserE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ServiceBrowser::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 24)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 24;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 24)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 24;
    }
    return _id;
}

// SIGNAL 0
void ServiceBrowser::serviceSelectionChanged(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ServiceBrowser::serviceActivated(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ServiceBrowser::serviceUpdated(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ServiceBrowser::serviceDiscovered(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ServiceBrowser::serviceLost(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
