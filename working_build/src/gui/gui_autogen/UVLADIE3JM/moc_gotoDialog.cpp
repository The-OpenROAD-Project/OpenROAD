/****************************************************************************
** Meta object code from reading C++ file 'gotoDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/gotoDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gotoDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__GotoLocationDialog_t {
    QByteArrayData data[11];
    char stringdata0[117];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__GotoLocationDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__GotoLocationDialog_t qt_meta_stringdata_gui__GotoLocationDialog = {
    {
QT_MOC_LITERAL(0, 0, 23), // "gui::GotoLocationDialog"
QT_MOC_LITERAL(1, 24, 14), // "updateLocation"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 10), // "QLineEdit*"
QT_MOC_LITERAL(4, 51, 6), // "x_edit"
QT_MOC_LITERAL(5, 58, 6), // "y_edit"
QT_MOC_LITERAL(6, 65, 11), // "updateUnits"
QT_MOC_LITERAL(7, 77, 14), // "dbu_per_micron"
QT_MOC_LITERAL(8, 92, 7), // "use_dbu"
QT_MOC_LITERAL(9, 100, 9), // "show_init"
QT_MOC_LITERAL(10, 110, 6) // "accept"

    },
    "gui::GotoLocationDialog\0updateLocation\0"
    "\0QLineEdit*\0x_edit\0y_edit\0updateUnits\0"
    "dbu_per_micron\0use_dbu\0show_init\0"
    "accept"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__GotoLocationDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   34,    2, 0x0a /* Public */,
       6,    2,   39,    2, 0x0a /* Public */,
       9,    0,   44,    2, 0x0a /* Public */,
      10,    0,   45,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    4,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,    7,    8,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::GotoLocationDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GotoLocationDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateLocation((*reinterpret_cast< QLineEdit*(*)>(_a[1])),(*reinterpret_cast< QLineEdit*(*)>(_a[2]))); break;
        case 1: _t->updateUnits((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: _t->show_init(); break;
        case 3: _t->accept(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::GotoLocationDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_gui__GotoLocationDialog.data,
    qt_meta_data_gui__GotoLocationDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::GotoLocationDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::GotoLocationDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__GotoLocationDialog.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "Ui::GotoLocDialog"))
        return static_cast< Ui::GotoLocDialog*>(this);
    return QDialog::qt_metacast(_clname);
}

int gui::GotoLocationDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
