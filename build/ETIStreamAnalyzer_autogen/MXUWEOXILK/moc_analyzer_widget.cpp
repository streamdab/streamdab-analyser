/****************************************************************************
** Meta object code from reading C++ file 'analyzer_widget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/gui/analyzer_widget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'analyzer_widget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14AnalyzerWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto AnalyzerWidget::qt_create_metaobjectdata<qt_meta_tag_ZN14AnalyzerWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AnalyzerWidget",
        "analysisStarted",
        "",
        "analysisStopped",
        "errorDetected",
        "error",
        "frameRateChanged",
        "frameRate",
        "statisticsUpdated",
        "frames",
        "errors",
        "updateDisplay",
        "clearDisplay",
        "resetStatistics",
        "exportData",
        "takeScreenshot",
        "handleRealTimeUpdate",
        "handleAnalysisModeChanged",
        "handleDisplayModeChanged",
        "handleEtiData",
        "handleServiceSelectionChanged",
        "serviceId"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'analysisStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analysisStopped'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorDetected'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'frameRateChanged'
        QtMocHelpers::SignalData<void(double)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 7 },
        }}),
        // Signal 'statisticsUpdated'
        QtMocHelpers::SignalData<void(quint64, quint32)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::ULongLong, 9 }, { QMetaType::UInt, 10 },
        }}),
        // Slot 'updateDisplay'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'clearDisplay'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'resetStatistics'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'exportData'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'takeScreenshot'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleRealTimeUpdate'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleAnalysisModeChanged'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleDisplayModeChanged'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleEtiData'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleServiceSelectionChanged'
        QtMocHelpers::SlotData<void(int)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 21 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AnalyzerWidget, qt_meta_tag_ZN14AnalyzerWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AnalyzerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14AnalyzerWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14AnalyzerWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14AnalyzerWidgetE_t>.metaTypes,
    nullptr
} };

void AnalyzerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AnalyzerWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->analysisStarted(); break;
        case 1: _t->analysisStopped(); break;
        case 2: _t->errorDetected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->frameRateChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 4: _t->statisticsUpdated((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[2]))); break;
        case 5: _t->updateDisplay(); break;
        case 6: _t->clearDisplay(); break;
        case 7: _t->resetStatistics(); break;
        case 8: _t->exportData(); break;
        case 9: _t->takeScreenshot(); break;
        case 10: _t->handleRealTimeUpdate(); break;
        case 11: _t->handleAnalysisModeChanged(); break;
        case 12: _t->handleDisplayModeChanged(); break;
        case 13: _t->handleEtiData(); break;
        case 14: _t->handleServiceSelectionChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AnalyzerWidget::*)()>(_a, &AnalyzerWidget::analysisStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalyzerWidget::*)()>(_a, &AnalyzerWidget::analysisStopped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalyzerWidget::*)(const QString & )>(_a, &AnalyzerWidget::errorDetected, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalyzerWidget::*)(double )>(_a, &AnalyzerWidget::frameRateChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalyzerWidget::*)(quint64 , quint32 )>(_a, &AnalyzerWidget::statisticsUpdated, 4))
            return;
    }
}

const QMetaObject *AnalyzerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AnalyzerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14AnalyzerWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int AnalyzerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void AnalyzerWidget::analysisStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AnalyzerWidget::analysisStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AnalyzerWidget::errorDetected(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void AnalyzerWidget::frameRateChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void AnalyzerWidget::statisticsUpdated(quint64 _t1, quint32 _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
QT_WARNING_POP
