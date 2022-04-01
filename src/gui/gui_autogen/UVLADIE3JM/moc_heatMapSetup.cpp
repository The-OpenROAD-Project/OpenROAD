/****************************************************************************
** Meta object code from reading C++ file 'heatMapSetup.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/heatMapSetup.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'heatMapSetup.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__HeatMapSetup_t {
    QByteArrayData data[17];
    char stringdata0[213];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__HeatMapSetup_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__HeatMapSetup_t qt_meta_stringdata_gui__HeatMapSetup = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::HeatMapSetup"
QT_MOC_LITERAL(1, 18, 7), // "changed"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 17), // "updateShowNumbers"
QT_MOC_LITERAL(4, 45, 6), // "option"
QT_MOC_LITERAL(5, 52, 16), // "updateShowLegend"
QT_MOC_LITERAL(6, 69, 18), // "updateShowMinRange"
QT_MOC_LITERAL(7, 88, 4), // "show"
QT_MOC_LITERAL(8, 93, 18), // "updateShowMaxRange"
QT_MOC_LITERAL(9, 112, 11), // "updateScale"
QT_MOC_LITERAL(10, 124, 18), // "updateReverseScale"
QT_MOC_LITERAL(11, 143, 11), // "updateAlpha"
QT_MOC_LITERAL(12, 155, 5), // "alpha"
QT_MOC_LITERAL(13, 161, 11), // "updateRange"
QT_MOC_LITERAL(14, 173, 14), // "updateGridSize"
QT_MOC_LITERAL(15, 188, 13), // "updateWidgets"
QT_MOC_LITERAL(16, 202, 10) // "destroyMap"

    },
    "gui::HeatMapSetup\0changed\0\0updateShowNumbers\0"
    "option\0updateShowLegend\0updateShowMinRange\0"
    "show\0updateShowMaxRange\0updateScale\0"
    "updateReverseScale\0updateAlpha\0alpha\0"
    "updateRange\0updateGridSize\0updateWidgets\0"
    "destroyMap"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__HeatMapSetup[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   75,    2, 0x08 /* Private */,
       5,    1,   78,    2, 0x08 /* Private */,
       6,    1,   81,    2, 0x08 /* Private */,
       8,    1,   84,    2, 0x08 /* Private */,
       9,    1,   87,    2, 0x08 /* Private */,
      10,    1,   90,    2, 0x08 /* Private */,
      11,    1,   93,    2, 0x08 /* Private */,
      13,    0,   96,    2, 0x08 /* Private */,
      14,    0,   97,    2, 0x08 /* Private */,
      15,    0,   98,    2, 0x08 /* Private */,
      16,    0,   99,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::HeatMapSetup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        HeatMapSetup *_t = static_cast<HeatMapSetup *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->changed(); break;
        case 1: _t->updateShowNumbers((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->updateShowLegend((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->updateShowMinRange((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->updateShowMaxRange((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->updateScale((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->updateReverseScale((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->updateAlpha((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->updateRange(); break;
        case 9: _t->updateGridSize(); break;
        case 10: _t->updateWidgets(); break;
        case 11: _t->destroyMap(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (HeatMapSetup::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HeatMapSetup::changed)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject gui::HeatMapSetup::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_gui__HeatMapSetup.data,
      qt_meta_data_gui__HeatMapSetup,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::HeatMapSetup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::HeatMapSetup::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__HeatMapSetup.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int gui::HeatMapSetup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void gui::HeatMapSetup::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
