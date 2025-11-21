/****************************************************************************
** Meta object code from reading C++ file 'globalConnectDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/globalConnectDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'globalConnectDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__GlobalConnectDialog_t {
    QByteArrayData data[13];
    char stringdata0[170];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__GlobalConnectDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__GlobalConnectDialog_t qt_meta_stringdata_gui__GlobalConnectDialog = {
    {
QT_MOC_LITERAL(0, 0, 24), // "gui::GlobalConnectDialog"
QT_MOC_LITERAL(1, 25, 15), // "connectionsMade"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 11), // "connections"
QT_MOC_LITERAL(4, 54, 8), // "runRules"
QT_MOC_LITERAL(5, 63, 10), // "clearRules"
QT_MOC_LITERAL(6, 74, 10), // "deleteRule"
QT_MOC_LITERAL(7, 85, 21), // "odb::dbGlobalConnect*"
QT_MOC_LITERAL(8, 107, 8), // "gconnect"
QT_MOC_LITERAL(9, 116, 8), // "makeRule"
QT_MOC_LITERAL(10, 125, 19), // "announceConnections"
QT_MOC_LITERAL(11, 145, 19), // "addRegexTextChanged"
QT_MOC_LITERAL(12, 165, 4) // "text"

    },
    "gui::GlobalConnectDialog\0connectionsMade\0"
    "\0connections\0runRules\0clearRules\0"
    "deleteRule\0odb::dbGlobalConnect*\0"
    "gconnect\0makeRule\0announceConnections\0"
    "addRegexTextChanged\0text"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__GlobalConnectDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   52,    2, 0x08 /* Private */,
       5,    0,   53,    2, 0x08 /* Private */,
       6,    1,   54,    2, 0x08 /* Private */,
       9,    0,   57,    2, 0x08 /* Private */,
      10,    1,   58,    2, 0x08 /* Private */,
      11,    1,   61,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::QString,   12,

       0        // eod
};

void gui::GlobalConnectDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GlobalConnectDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connectionsMade((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->runRules(); break;
        case 2: _t->clearRules(); break;
        case 3: _t->deleteRule((*reinterpret_cast< odb::dbGlobalConnect*(*)>(_a[1]))); break;
        case 4: _t->makeRule(); break;
        case 5: _t->announceConnections((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->addRegexTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GlobalConnectDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalConnectDialog::connectionsMade)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::GlobalConnectDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_gui__GlobalConnectDialog.data,
    qt_meta_data_gui__GlobalConnectDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::GlobalConnectDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::GlobalConnectDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__GlobalConnectDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int gui::GlobalConnectDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void gui::GlobalConnectDialog::connectionsMade(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
