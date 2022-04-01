/****************************************************************************
** Meta object code from reading C++ file 'timingWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/timingWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'timingWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__TimingWidget_t {
    QByteArrayData data[30];
    char stringdata0[407];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__TimingWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__TimingWidget_t qt_meta_stringdata_gui__TimingWidget = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::TimingWidget"
QT_MOC_LITERAL(1, 18, 19), // "highlightTimingPath"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 11), // "TimingPath*"
QT_MOC_LITERAL(4, 51, 11), // "timing_path"
QT_MOC_LITERAL(5, 63, 7), // "inspect"
QT_MOC_LITERAL(6, 71, 8), // "Selected"
QT_MOC_LITERAL(7, 80, 9), // "selection"
QT_MOC_LITERAL(8, 90, 15), // "showPathDetails"
QT_MOC_LITERAL(9, 106, 5), // "index"
QT_MOC_LITERAL(10, 112, 16), // "clearPathDetails"
QT_MOC_LITERAL(11, 129, 18), // "highlightPathStage"
QT_MOC_LITERAL(12, 148, 22), // "TimingPathDetailModel*"
QT_MOC_LITERAL(13, 171, 5), // "model"
QT_MOC_LITERAL(14, 177, 14), // "toggleRenderer"
QT_MOC_LITERAL(15, 192, 6), // "enable"
QT_MOC_LITERAL(16, 199, 13), // "populatePaths"
QT_MOC_LITERAL(17, 213, 13), // "modelWasReset"
QT_MOC_LITERAL(18, 227, 18), // "selectedRowChanged"
QT_MOC_LITERAL(19, 246, 14), // "QItemSelection"
QT_MOC_LITERAL(20, 261, 10), // "prev_index"
QT_MOC_LITERAL(21, 272, 10), // "curr_index"
QT_MOC_LITERAL(22, 283, 24), // "selectedDetailRowChanged"
QT_MOC_LITERAL(23, 308, 25), // "selectedCaptureRowChanged"
QT_MOC_LITERAL(24, 334, 14), // "handleDbChange"
QT_MOC_LITERAL(25, 349, 8), // "setBlock"
QT_MOC_LITERAL(26, 358, 13), // "odb::dbBlock*"
QT_MOC_LITERAL(27, 372, 5), // "block"
QT_MOC_LITERAL(28, 378, 15), // "updateClockRows"
QT_MOC_LITERAL(29, 394, 12) // "showSettings"

    },
    "gui::TimingWidget\0highlightTimingPath\0"
    "\0TimingPath*\0timing_path\0inspect\0"
    "Selected\0selection\0showPathDetails\0"
    "index\0clearPathDetails\0highlightPathStage\0"
    "TimingPathDetailModel*\0model\0"
    "toggleRenderer\0enable\0populatePaths\0"
    "modelWasReset\0selectedRowChanged\0"
    "QItemSelection\0prev_index\0curr_index\0"
    "selectedDetailRowChanged\0"
    "selectedCaptureRowChanged\0handleDbChange\0"
    "setBlock\0odb::dbBlock*\0block\0"
    "updateClockRows\0showSettings"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__TimingWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   89,    2, 0x06 /* Public */,
       5,    1,   92,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    1,   95,    2, 0x0a /* Public */,
      10,    0,   98,    2, 0x0a /* Public */,
      11,    2,   99,    2, 0x0a /* Public */,
      14,    1,  104,    2, 0x0a /* Public */,
      16,    0,  107,    2, 0x0a /* Public */,
      17,    0,  108,    2, 0x0a /* Public */,
      18,    2,  109,    2, 0x0a /* Public */,
      22,    2,  114,    2, 0x0a /* Public */,
      23,    2,  119,    2, 0x0a /* Public */,
      24,    0,  124,    2, 0x0a /* Public */,
      25,    1,  125,    2, 0x0a /* Public */,
      28,    0,  128,    2, 0x0a /* Public */,
      29,    0,  129,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::QModelIndex,    9,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 12, QMetaType::QModelIndex,   13,    9,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19, 0x80000000 | 19,   20,   21,
    QMetaType::Void, 0x80000000 | 19, 0x80000000 | 19,   20,   21,
    QMetaType::Void, 0x80000000 | 19, 0x80000000 | 19,   20,   21,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 26,   27,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::TimingWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TimingWidget *_t = static_cast<TimingWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->highlightTimingPath((*reinterpret_cast< TimingPath*(*)>(_a[1]))); break;
        case 1: _t->inspect((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 2: _t->showPathDetails((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->clearPathDetails(); break;
        case 4: _t->highlightPathStage((*reinterpret_cast< TimingPathDetailModel*(*)>(_a[1])),(*reinterpret_cast< const QModelIndex(*)>(_a[2]))); break;
        case 5: _t->toggleRenderer((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->populatePaths(); break;
        case 7: _t->modelWasReset(); break;
        case 8: _t->selectedRowChanged((*reinterpret_cast< const QItemSelection(*)>(_a[1])),(*reinterpret_cast< const QItemSelection(*)>(_a[2]))); break;
        case 9: _t->selectedDetailRowChanged((*reinterpret_cast< const QItemSelection(*)>(_a[1])),(*reinterpret_cast< const QItemSelection(*)>(_a[2]))); break;
        case 10: _t->selectedCaptureRowChanged((*reinterpret_cast< const QItemSelection(*)>(_a[1])),(*reinterpret_cast< const QItemSelection(*)>(_a[2]))); break;
        case 11: _t->handleDbChange(); break;
        case 12: _t->setBlock((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        case 13: _t->updateClockRows(); break;
        case 14: _t->showSettings(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QItemSelection >(); break;
            }
            break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QItemSelection >(); break;
            }
            break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QItemSelection >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (TimingWidget::*_t)(TimingPath * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TimingWidget::highlightTimingPath)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (TimingWidget::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TimingWidget::inspect)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject gui::TimingWidget::staticMetaObject = {
    { &QDockWidget::staticMetaObject, qt_meta_stringdata_gui__TimingWidget.data,
      qt_meta_data_gui__TimingWidget,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::TimingWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::TimingWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__TimingWidget.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int gui::TimingWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void gui::TimingWidget::highlightTimingPath(TimingPath * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::TimingWidget::inspect(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
