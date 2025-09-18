/****************************************************************************
** Meta object code from reading C++ file 'constellation_widget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/gui/constellation_widget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'constellation_widget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19ConstellationWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto ConstellationWidget::qt_create_metaobjectdata<qt_meta_tag_ZN19ConstellationWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ConstellationWidget",
        "displayModeChanged",
        "",
        "DisplayMode",
        "mode",
        "zoomLevelChanged",
        "zoom",
        "signalQualityChanged",
        "quality",
        "constellationPointClicked",
        "point",
        "updateDisplay",
        "clearDisplay",
        "takeScreenshot",
        "exportData",
        "handleRealTimeUpdate",
        "handleSignalDataUpdate"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'displayModeChanged'
        QtMocHelpers::SignalData<void(enum DisplayMode)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'zoomLevelChanged'
        QtMocHelpers::SignalData<void(double)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 6 },
        }}),
        // Signal 'signalQualityChanged'
        QtMocHelpers::SignalData<void(double)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 8 },
        }}),
        // Signal 'constellationPointClicked'
        QtMocHelpers::SignalData<void(const QPointF &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QPointF, 10 },
        }}),
        // Slot 'updateDisplay'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'clearDisplay'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'takeScreenshot'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'exportData'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleRealTimeUpdate'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSignalDataUpdate'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ConstellationWidget, qt_meta_tag_ZN19ConstellationWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ConstellationWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ConstellationWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ConstellationWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19ConstellationWidgetE_t>.metaTypes,
    nullptr
} };

void ConstellationWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ConstellationWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->displayModeChanged((*reinterpret_cast< std::add_pointer_t<enum DisplayMode>>(_a[1]))); break;
        case 1: _t->zoomLevelChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 2: _t->signalQualityChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 3: _t->constellationPointClicked((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 4: _t->updateDisplay(); break;
        case 5: _t->clearDisplay(); break;
        case 6: _t->takeScreenshot(); break;
        case 7: _t->exportData(); break;
        case 8: _t->handleRealTimeUpdate(); break;
        case 9: _t->handleSignalDataUpdate(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ConstellationWidget::*)(DisplayMode )>(_a, &ConstellationWidget::displayModeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ConstellationWidget::*)(double )>(_a, &ConstellationWidget::zoomLevelChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ConstellationWidget::*)(double )>(_a, &ConstellationWidget::signalQualityChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ConstellationWidget::*)(const QPointF & )>(_a, &ConstellationWidget::constellationPointClicked, 3))
            return;
    }
}

const QMetaObject *ConstellationWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ConstellationWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ConstellationWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ConstellationWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void ConstellationWidget::displayModeChanged(DisplayMode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ConstellationWidget::zoomLevelChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ConstellationWidget::signalQualityChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ConstellationWidget::constellationPointClicked(const QPointF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
