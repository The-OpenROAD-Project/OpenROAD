/****************************************************************************
** Meta object code from reading C++ file 'staGui.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/staGui.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'staGui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__TimingPathsModel_t {
    QByteArrayData data[6];
    char stringdata0[63];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__TimingPathsModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__TimingPathsModel_t qt_meta_stringdata_gui__TimingPathsModel = {
    {
QT_MOC_LITERAL(0, 0, 21), // "gui::TimingPathsModel"
QT_MOC_LITERAL(1, 22, 4), // "sort"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 9), // "col_index"
QT_MOC_LITERAL(4, 38, 13), // "Qt::SortOrder"
QT_MOC_LITERAL(5, 52, 10) // "sort_order"

    },
    "gui::TimingPathsModel\0sort\0\0col_index\0"
    "Qt::SortOrder\0sort_order"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__TimingPathsModel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   19,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, 0x80000000 | 4,    3,    5,

       0        // eod
};

void gui::TimingPathsModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TimingPathsModel *_t = static_cast<TimingPathsModel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sort((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< Qt::SortOrder(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject gui::TimingPathsModel::staticMetaObject = {
    { &QAbstractTableModel::staticMetaObject, qt_meta_stringdata_gui__TimingPathsModel.data,
      qt_meta_data_gui__TimingPathsModel,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::TimingPathsModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::TimingPathsModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__TimingPathsModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractTableModel::qt_metacast(_clname);
}

int gui::TimingPathsModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractTableModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_gui__GuiDBChangeListener_t {
    QByteArrayData data[4];
    char stringdata0[42];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__GuiDBChangeListener_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__GuiDBChangeListener_t qt_meta_stringdata_gui__GuiDBChangeListener = {
    {
QT_MOC_LITERAL(0, 0, 24), // "gui::GuiDBChangeListener"
QT_MOC_LITERAL(1, 25, 9), // "dbUpdated"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 5) // "reset"

    },
    "gui::GuiDBChangeListener\0dbUpdated\0\0"
    "reset"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__GuiDBChangeListener[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   25,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void gui::GuiDBChangeListener::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        GuiDBChangeListener *_t = static_cast<GuiDBChangeListener *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->dbUpdated(); break;
        case 1: _t->reset(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (GuiDBChangeListener::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GuiDBChangeListener::dbUpdated)) {
                *result = 0;
                return;
            }
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject gui::GuiDBChangeListener::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_gui__GuiDBChangeListener.data,
      qt_meta_data_gui__GuiDBChangeListener,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::GuiDBChangeListener::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::GuiDBChangeListener::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__GuiDBChangeListener.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "odb::dbBlockCallBackObj"))
        return static_cast< odb::dbBlockCallBackObj*>(this);
    return QObject::qt_metacast(_clname);
}

int gui::GuiDBChangeListener::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void gui::GuiDBChangeListener::dbUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_gui__PinSetWidget_t {
    QByteArrayData data[11];
    char stringdata0[111];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__PinSetWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__PinSetWidget_t qt_meta_stringdata_gui__PinSetWidget = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::PinSetWidget"
QT_MOC_LITERAL(1, 18, 18), // "addRemoveTriggered"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 13), // "PinSetWidget*"
QT_MOC_LITERAL(4, 52, 7), // "inspect"
QT_MOC_LITERAL(5, 60, 8), // "Selected"
QT_MOC_LITERAL(6, 69, 8), // "selected"
QT_MOC_LITERAL(7, 78, 9), // "clearPins"
QT_MOC_LITERAL(8, 88, 7), // "findPin"
QT_MOC_LITERAL(9, 96, 8), // "showMenu"
QT_MOC_LITERAL(10, 105, 5) // "point"

    },
    "gui::PinSetWidget\0addRemoveTriggered\0"
    "\0PinSetWidget*\0inspect\0Selected\0"
    "selected\0clearPins\0findPin\0showMenu\0"
    "point"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__PinSetWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,
       4,    1,   42,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   45,    2, 0x0a /* Public */,
       8,    0,   46,    2, 0x08 /* Private */,
       9,    1,   47,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    2,
    QMetaType::Void, 0x80000000 | 5,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   10,

       0        // eod
};

void gui::PinSetWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        PinSetWidget *_t = static_cast<PinSetWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->addRemoveTriggered((*reinterpret_cast< PinSetWidget*(*)>(_a[1]))); break;
        case 1: _t->inspect((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 2: _t->clearPins(); break;
        case 3: _t->findPin(); break;
        case 4: _t->showMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< PinSetWidget* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (PinSetWidget::*_t)(PinSetWidget * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PinSetWidget::addRemoveTriggered)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (PinSetWidget::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PinSetWidget::inspect)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject gui::PinSetWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_gui__PinSetWidget.data,
      qt_meta_data_gui__PinSetWidget,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::PinSetWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::PinSetWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__PinSetWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int gui::PinSetWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void gui::PinSetWidget::addRemoveTriggered(PinSetWidget * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::PinSetWidget::inspect(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_gui__TimingControlsDialog_t {
    QByteArrayData data[11];
    char stringdata0[113];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__TimingControlsDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__TimingControlsDialog_t qt_meta_stringdata_gui__TimingControlsDialog = {
    {
QT_MOC_LITERAL(0, 0, 25), // "gui::TimingControlsDialog"
QT_MOC_LITERAL(1, 26, 7), // "inspect"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 8), // "Selected"
QT_MOC_LITERAL(4, 44, 8), // "selected"
QT_MOC_LITERAL(5, 53, 11), // "expandClock"
QT_MOC_LITERAL(6, 65, 6), // "expand"
QT_MOC_LITERAL(7, 72, 8), // "populate"
QT_MOC_LITERAL(8, 81, 13), // "addRemoveThru"
QT_MOC_LITERAL(9, 95, 13), // "PinSetWidget*"
QT_MOC_LITERAL(10, 109, 3) // "row"

    },
    "gui::TimingControlsDialog\0inspect\0\0"
    "Selected\0selected\0expandClock\0expand\0"
    "populate\0addRemoveThru\0PinSetWidget*\0"
    "row"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__TimingControlsDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       5,    1,   37,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   40,    2, 0x0a /* Public */,
       8,    1,   41,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Bool,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,

       0        // eod
};

void gui::TimingControlsDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TimingControlsDialog *_t = static_cast<TimingControlsDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->inspect((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 1: _t->expandClock((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->populate(); break;
        case 3: _t->addRemoveThru((*reinterpret_cast< PinSetWidget*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< PinSetWidget* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (TimingControlsDialog::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TimingControlsDialog::inspect)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (TimingControlsDialog::*_t)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TimingControlsDialog::expandClock)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject gui::TimingControlsDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_gui__TimingControlsDialog.data,
      qt_meta_data_gui__TimingControlsDialog,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::TimingControlsDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::TimingControlsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__TimingControlsDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int gui::TimingControlsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void gui::TimingControlsDialog::inspect(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::TimingControlsDialog::expandClock(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
