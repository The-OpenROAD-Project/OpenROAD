/****************************************************************************
** Meta object code from reading C++ file 'GUIProgress.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/GUIProgress.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GUIProgress.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__ProgressWidget_t {
    QByteArrayData data[7];
    char stringdata0[65];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ProgressWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ProgressWidget_t qt_meta_stringdata_gui__ProgressWidget = {
    {
QT_MOC_LITERAL(0, 0, 19), // "gui::ProgressWidget"
QT_MOC_LITERAL(1, 20, 14), // "updateProgress"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 8), // "progress"
QT_MOC_LITERAL(4, 45, 11), // "updateRange"
QT_MOC_LITERAL(5, 57, 3), // "min"
QT_MOC_LITERAL(6, 61, 3) // "max"

    },
    "gui::ProgressWidget\0updateProgress\0\0"
    "progress\0updateRange\0min\0max"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ProgressWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,
       4,    2,   27,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    5,    6,

       0        // eod
};

void gui::ProgressWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ProgressWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->updateRange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ProgressWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProgressWidget::updateProgress)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ProgressWidget::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProgressWidget::updateRange)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::ProgressWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__ProgressWidget.data,
    qt_meta_data_gui__ProgressWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ProgressWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ProgressWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ProgressWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int gui::ProgressWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void gui::ProgressWidget::updateProgress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::ProgressWidget::updateRange(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_gui__ProgressReporterWidget_t {
    QByteArrayData data[1];
    char stringdata0[28];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ProgressReporterWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ProgressReporterWidget_t qt_meta_stringdata_gui__ProgressReporterWidget = {
    {
QT_MOC_LITERAL(0, 0, 27) // "gui::ProgressReporterWidget"

    },
    "gui::ProgressReporterWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ProgressReporterWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void gui::ProgressReporterWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject gui::ProgressReporterWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<ProgressWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__ProgressReporterWidget.data,
    qt_meta_data_gui__ProgressReporterWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ProgressReporterWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ProgressReporterWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ProgressReporterWidget.stringdata0))
        return static_cast<void*>(this);
    return ProgressWidget::qt_metacast(_clname);
}

int gui::ProgressReporterWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProgressWidget::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_gui__CombinedProgressWidget_t {
    QByteArrayData data[10];
    char stringdata0[134];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__CombinedProgressWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__CombinedProgressWidget_t qt_meta_stringdata_gui__CombinedProgressWidget = {
    {
QT_MOC_LITERAL(0, 0, 27), // "gui::CombinedProgressWidget"
QT_MOC_LITERAL(1, 28, 9), // "interrupt"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 14), // "toggleProgress"
QT_MOC_LITERAL(4, 54, 12), // "showProgress"
QT_MOC_LITERAL(5, 67, 12), // "hideProgress"
QT_MOC_LITERAL(6, 80, 9), // "addWidget"
QT_MOC_LITERAL(7, 90, 23), // "ProgressReporterWidget*"
QT_MOC_LITERAL(8, 114, 6), // "widget"
QT_MOC_LITERAL(9, 121, 12) // "removeWidget"

    },
    "gui::CombinedProgressWidget\0interrupt\0"
    "\0toggleProgress\0showProgress\0hideProgress\0"
    "addWidget\0ProgressReporterWidget*\0"
    "widget\0removeWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__CombinedProgressWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x0a /* Public */,
       3,    0,   45,    2, 0x0a /* Public */,
       4,    0,   46,    2, 0x0a /* Public */,
       5,    0,   47,    2, 0x0a /* Public */,
       6,    1,   48,    2, 0x0a /* Public */,
       9,    1,   51,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

void gui::CombinedProgressWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CombinedProgressWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->interrupt(); break;
        case 1: _t->toggleProgress(); break;
        case 2: _t->showProgress(); break;
        case 3: _t->hideProgress(); break;
        case 4: _t->addWidget((*reinterpret_cast< ProgressReporterWidget*(*)>(_a[1]))); break;
        case 5: _t->removeWidget((*reinterpret_cast< ProgressReporterWidget*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ProgressReporterWidget* >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ProgressReporterWidget* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::CombinedProgressWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<ProgressWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__CombinedProgressWidget.data,
    qt_meta_data_gui__CombinedProgressWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::CombinedProgressWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::CombinedProgressWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__CombinedProgressWidget.stringdata0))
        return static_cast<void*>(this);
    return ProgressWidget::qt_metacast(_clname);
}

int gui::CombinedProgressWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProgressWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
