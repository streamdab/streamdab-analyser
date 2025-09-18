/****************************************************************************
** Meta object code from reading C++ file 'mock_gui_components.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../tests/mocks/mock_gui_components.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mock_gui_components.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14MockMainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MockMainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN14MockMainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MockMainWindow",
        "etiFileOpened",
        "",
        "filename",
        "etiFileClosed",
        "realTimeModeChanged",
        "enabled",
        "settingsChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'etiFileOpened'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'etiFileClosed'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'realTimeModeChanged'
        QtMocHelpers::SignalData<void(bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'settingsChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MockMainWindow, qt_meta_tag_ZN14MockMainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MockMainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MockMainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MockMainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14MockMainWindowE_t>.metaTypes,
    nullptr
} };

void MockMainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MockMainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->etiFileOpened((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->etiFileClosed(); break;
        case 2: _t->realTimeModeChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->settingsChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MockMainWindow::*)(const QString & )>(_a, &MockMainWindow::etiFileOpened, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockMainWindow::*)()>(_a, &MockMainWindow::etiFileClosed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockMainWindow::*)(bool )>(_a, &MockMainWindow::realTimeModeChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockMainWindow::*)()>(_a, &MockMainWindow::settingsChanged, 3))
            return;
    }
}

const QMetaObject *MockMainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MockMainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MockMainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MockMainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void MockMainWindow::etiFileOpened(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void MockMainWindow::etiFileClosed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MockMainWindow::realTimeModeChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void MockMainWindow::settingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
namespace {
struct qt_meta_tag_ZN18MockAnalyzerWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto MockAnalyzerWidget::qt_create_metaobjectdata<qt_meta_tag_ZN18MockAnalyzerWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MockAnalyzerWidget",
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
        "handleUpdateTimer"
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
        // Slot 'handleUpdateTimer'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MockAnalyzerWidget, qt_meta_tag_ZN18MockAnalyzerWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MockAnalyzerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MockAnalyzerWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MockAnalyzerWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18MockAnalyzerWidgetE_t>.metaTypes,
    nullptr
} };

void MockAnalyzerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MockAnalyzerWidget *>(_o);
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
        case 8: _t->handleUpdateTimer(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MockAnalyzerWidget::*)()>(_a, &MockAnalyzerWidget::analysisStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockAnalyzerWidget::*)()>(_a, &MockAnalyzerWidget::analysisStopped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockAnalyzerWidget::*)(const QString & )>(_a, &MockAnalyzerWidget::errorDetected, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockAnalyzerWidget::*)(double )>(_a, &MockAnalyzerWidget::frameRateChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockAnalyzerWidget::*)(quint64 , quint32 )>(_a, &MockAnalyzerWidget::statisticsUpdated, 4))
            return;
    }
}

const QMetaObject *MockAnalyzerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MockAnalyzerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MockAnalyzerWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MockAnalyzerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void MockAnalyzerWidget::analysisStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MockAnalyzerWidget::analysisStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MockAnalyzerWidget::errorDetected(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void MockAnalyzerWidget::frameRateChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void MockAnalyzerWidget::statisticsUpdated(quint64 _t1, quint32 _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
namespace {
struct qt_meta_tag_ZN23MockConstellationWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto MockConstellationWidget::qt_create_metaobjectdata<qt_meta_tag_ZN23MockConstellationWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MockConstellationWidget",
        "realTimeUpdatesStarted",
        "",
        "realTimeUpdatesStopped",
        "zoomChanged",
        "zoom",
        "panChanged",
        "offset"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'realTimeUpdatesStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'realTimeUpdatesStopped'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'zoomChanged'
        QtMocHelpers::SignalData<void(double)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Double, 5 },
        }}),
        // Signal 'panChanged'
        QtMocHelpers::SignalData<void(const QPointF &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QPointF, 7 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MockConstellationWidget, qt_meta_tag_ZN23MockConstellationWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MockConstellationWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23MockConstellationWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23MockConstellationWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN23MockConstellationWidgetE_t>.metaTypes,
    nullptr
} };

void MockConstellationWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MockConstellationWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->realTimeUpdatesStarted(); break;
        case 1: _t->realTimeUpdatesStopped(); break;
        case 2: _t->zoomChanged((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 3: _t->panChanged((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MockConstellationWidget::*)()>(_a, &MockConstellationWidget::realTimeUpdatesStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockConstellationWidget::*)()>(_a, &MockConstellationWidget::realTimeUpdatesStopped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockConstellationWidget::*)(double )>(_a, &MockConstellationWidget::zoomChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockConstellationWidget::*)(const QPointF & )>(_a, &MockConstellationWidget::panChanged, 3))
            return;
    }
}

const QMetaObject *MockConstellationWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MockConstellationWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23MockConstellationWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MockConstellationWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void MockConstellationWidget::realTimeUpdatesStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MockConstellationWidget::realTimeUpdatesStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MockConstellationWidget::zoomChanged(double _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void MockConstellationWidget::panChanged(const QPointF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
namespace {
struct qt_meta_tag_ZN18MockServiceBrowserE_t {};
} // unnamed namespace

template <> constexpr inline auto MockServiceBrowser::qt_create_metaobjectdata<qt_meta_tag_ZN18MockServiceBrowserE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MockServiceBrowser",
        "serviceSelected",
        "",
        "serviceId",
        "serviceAdded",
        "serviceRemoved",
        "servicesCleared",
        "selectService",
        "refreshServices"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'serviceSelected'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'serviceAdded'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'serviceRemoved'
        QtMocHelpers::SignalData<void(int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'servicesCleared'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectService'
        QtMocHelpers::SlotData<void(int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'refreshServices'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MockServiceBrowser, qt_meta_tag_ZN18MockServiceBrowserE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MockServiceBrowser::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MockServiceBrowserE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MockServiceBrowserE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18MockServiceBrowserE_t>.metaTypes,
    nullptr
} };

void MockServiceBrowser::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MockServiceBrowser *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->serviceSelected((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->serviceAdded((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->serviceRemoved((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->servicesCleared(); break;
        case 4: _t->selectService((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->refreshServices(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MockServiceBrowser::*)(int )>(_a, &MockServiceBrowser::serviceSelected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockServiceBrowser::*)(int )>(_a, &MockServiceBrowser::serviceAdded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockServiceBrowser::*)(int )>(_a, &MockServiceBrowser::serviceRemoved, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MockServiceBrowser::*)()>(_a, &MockServiceBrowser::servicesCleared, 3))
            return;
    }
}

const QMetaObject *MockServiceBrowser::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MockServiceBrowser::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MockServiceBrowserE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MockServiceBrowser::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void MockServiceBrowser::serviceSelected(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void MockServiceBrowser::serviceAdded(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void MockServiceBrowser::serviceRemoved(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void MockServiceBrowser::servicesCleared()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
