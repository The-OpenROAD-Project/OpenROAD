/****************************************************************************
** Meta object code from reading C++ file 'tclCmdInputWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/tclCmdInputWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tclCmdInputWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__TclCmdInputWidget_t {
    QByteArrayData data[10];
    char stringdata0[113];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__TclCmdInputWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__TclCmdInputWidget_t qt_meta_stringdata_gui__TclCmdInputWidget = {
    {
QT_MOC_LITERAL(0, 0, 22), // "gui::TclCmdInputWidget"
QT_MOC_LITERAL(1, 23, 14), // "executeCommand"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 3), // "cmd"
QT_MOC_LITERAL(4, 43, 4), // "echo"
QT_MOC_LITERAL(5, 48, 6), // "silent"
QT_MOC_LITERAL(6, 55, 18), // "updateHighlighting"
QT_MOC_LITERAL(7, 74, 16), // "updateCompletion"
QT_MOC_LITERAL(8, 91, 16), // "insertCompletion"
QT_MOC_LITERAL(9, 108, 4) // "text"

    },
    "gui::TclCmdInputWidget\0executeCommand\0"
    "\0cmd\0echo\0silent\0updateHighlighting\0"
    "updateCompletion\0insertCompletion\0"
    "text"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__TclCmdInputWidget[] = {

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
       1,    3,   44,    2, 0x0a /* Public */,
       1,    2,   51,    2, 0x2a /* Public | MethodCloned */,
       1,    1,   56,    2, 0x2a /* Public | MethodCloned */,
       6,    0,   59,    2, 0x08 /* Private */,
       7,    0,   60,    2, 0x08 /* Private */,
       8,    1,   61,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Bool, QMetaType::Bool,    3,    4,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,

       0        // eod
};

void gui::TclCmdInputWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TclCmdInputWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 1: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->updateHighlighting(); break;
        case 4: _t->updateCompletion(); break;
        case 5: _t->insertCompletion((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::TclCmdInputWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<CmdInputWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__TclCmdInputWidget.data,
    qt_meta_data_gui__TclCmdInputWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::TclCmdInputWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::TclCmdInputWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__TclCmdInputWidget.stringdata0))
        return static_cast<void*>(this);
    return CmdInputWidget::qt_metacast(_clname);
}

int gui::TclCmdInputWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = CmdInputWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
