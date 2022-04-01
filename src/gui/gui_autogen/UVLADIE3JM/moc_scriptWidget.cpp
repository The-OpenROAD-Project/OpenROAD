/****************************************************************************
** Meta object code from reading C++ file 'scriptWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/scriptWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scriptWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__ScriptWidget_t {
    QByteArrayData data[21];
    char stringdata0[246];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ScriptWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ScriptWidget_t qt_meta_stringdata_gui__ScriptWidget = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::ScriptWidget"
QT_MOC_LITERAL(1, 18, 15), // "commandExecuted"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 11), // "return_code"
QT_MOC_LITERAL(4, 47, 10), // "tclExiting"
QT_MOC_LITERAL(5, 58, 11), // "addToOutput"
QT_MOC_LITERAL(6, 70, 4), // "text"
QT_MOC_LITERAL(7, 75, 5), // "color"
QT_MOC_LITERAL(8, 81, 14), // "executeCommand"
QT_MOC_LITERAL(9, 96, 7), // "command"
QT_MOC_LITERAL(10, 104, 4), // "echo"
QT_MOC_LITERAL(11, 109, 20), // "executeSilentCommand"
QT_MOC_LITERAL(12, 130, 13), // "outputChanged"
QT_MOC_LITERAL(13, 144, 5), // "pause"
QT_MOC_LITERAL(14, 150, 7), // "timeout"
QT_MOC_LITERAL(15, 158, 7), // "unpause"
QT_MOC_LITERAL(16, 166, 13), // "pauserClicked"
QT_MOC_LITERAL(17, 180, 13), // "goBackHistory"
QT_MOC_LITERAL(18, 194, 16), // "goForwardHistory"
QT_MOC_LITERAL(19, 211, 18), // "updatePauseTimeout"
QT_MOC_LITERAL(20, 230, 15) // "addTextToOutput"

    },
    "gui::ScriptWidget\0commandExecuted\0\0"
    "return_code\0tclExiting\0addToOutput\0"
    "text\0color\0executeCommand\0command\0"
    "echo\0executeSilentCommand\0outputChanged\0"
    "pause\0timeout\0unpause\0pauserClicked\0"
    "goBackHistory\0goForwardHistory\0"
    "updatePauseTimeout\0addTextToOutput"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ScriptWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   84,    2, 0x06 /* Public */,
       4,    0,   87,    2, 0x06 /* Public */,
       5,    2,   88,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    2,   93,    2, 0x0a /* Public */,
       8,    1,   98,    2, 0x2a /* Public | MethodCloned */,
      11,    1,  101,    2, 0x0a /* Public */,
      12,    0,  104,    2, 0x08 /* Private */,
      13,    1,  105,    2, 0x08 /* Private */,
      15,    0,  108,    2, 0x08 /* Private */,
      16,    0,  109,    2, 0x08 /* Private */,
      17,    0,  110,    2, 0x08 /* Private */,
      18,    0,  111,    2, 0x08 /* Private */,
      19,    0,  112,    2, 0x08 /* Private */,
      20,    2,  113,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QColor,    6,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    9,   10,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QColor,    6,    7,

       0        // eod
};

void gui::ScriptWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ScriptWidget *_t = static_cast<ScriptWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->commandExecuted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->tclExiting(); break;
        case 2: _t->addToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        case 3: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 4: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->executeSilentCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->outputChanged(); break;
        case 7: _t->pause((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->unpause(); break;
        case 9: _t->pauserClicked(); break;
        case 10: _t->goBackHistory(); break;
        case 11: _t->goForwardHistory(); break;
        case 12: _t->updatePauseTimeout(); break;
        case 13: _t->addTextToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (ScriptWidget::*_t)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::commandExecuted)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (ScriptWidget::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::tclExiting)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (ScriptWidget::*_t)(const QString & , const QColor & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::addToOutput)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject gui::ScriptWidget::staticMetaObject = {
    { &QDockWidget::staticMetaObject, qt_meta_stringdata_gui__ScriptWidget.data,
      qt_meta_data_gui__ScriptWidget,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::ScriptWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ScriptWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ScriptWidget.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int gui::ScriptWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void gui::ScriptWidget::commandExecuted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::ScriptWidget::tclExiting()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::ScriptWidget::addToOutput(const QString & _t1, const QColor & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
