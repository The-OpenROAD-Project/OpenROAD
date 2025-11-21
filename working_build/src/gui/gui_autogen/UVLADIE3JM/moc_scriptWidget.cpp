/****************************************************************************
** Meta object code from reading C++ file 'scriptWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/scriptWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scriptWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__ScriptWidget_t {
    QByteArrayData data[29];
    char stringdata0[360];
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
QT_MOC_LITERAL(3, 35, 5), // "is_ok"
QT_MOC_LITERAL(4, 41, 21), // "commandAboutToExecute"
QT_MOC_LITERAL(5, 63, 15), // "executionPaused"
QT_MOC_LITERAL(6, 79, 7), // "exiting"
QT_MOC_LITERAL(7, 87, 11), // "addToOutput"
QT_MOC_LITERAL(8, 99, 4), // "text"
QT_MOC_LITERAL(9, 104, 5), // "color"
QT_MOC_LITERAL(10, 110, 14), // "executeCommand"
QT_MOC_LITERAL(11, 125, 7), // "command"
QT_MOC_LITERAL(12, 133, 4), // "echo"
QT_MOC_LITERAL(13, 138, 20), // "executeSilentCommand"
QT_MOC_LITERAL(14, 159, 17), // "addResultToOutput"
QT_MOC_LITERAL(15, 177, 6), // "result"
QT_MOC_LITERAL(16, 184, 18), // "addCommandToOutput"
QT_MOC_LITERAL(17, 203, 3), // "cmd"
QT_MOC_LITERAL(18, 207, 5), // "pause"
QT_MOC_LITERAL(19, 213, 7), // "timeout"
QT_MOC_LITERAL(20, 221, 10), // "setCommand"
QT_MOC_LITERAL(21, 232, 13), // "outputChanged"
QT_MOC_LITERAL(22, 246, 7), // "unpause"
QT_MOC_LITERAL(23, 254, 13), // "pauserClicked"
QT_MOC_LITERAL(24, 268, 18), // "updatePauseTimeout"
QT_MOC_LITERAL(25, 287, 15), // "addTextToOutput"
QT_MOC_LITERAL(26, 303, 18), // "setPauserToRunning"
QT_MOC_LITERAL(27, 322, 11), // "resetPauser"
QT_MOC_LITERAL(28, 334, 25) // "flushReportBufferToOutput"

    },
    "gui::ScriptWidget\0commandExecuted\0\0"
    "is_ok\0commandAboutToExecute\0executionPaused\0"
    "exiting\0addToOutput\0text\0color\0"
    "executeCommand\0command\0echo\0"
    "executeSilentCommand\0addResultToOutput\0"
    "result\0addCommandToOutput\0cmd\0pause\0"
    "timeout\0setCommand\0outputChanged\0"
    "unpause\0pauserClicked\0updatePauseTimeout\0"
    "addTextToOutput\0setPauserToRunning\0"
    "resetPauser\0flushReportBufferToOutput"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ScriptWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  114,    2, 0x06 /* Public */,
       4,    0,  117,    2, 0x06 /* Public */,
       5,    0,  118,    2, 0x06 /* Public */,
       6,    0,  119,    2, 0x06 /* Public */,
       7,    2,  120,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    2,  125,    2, 0x0a /* Public */,
      10,    1,  130,    2, 0x2a /* Public | MethodCloned */,
      13,    1,  133,    2, 0x0a /* Public */,
      14,    2,  136,    2, 0x0a /* Public */,
      16,    1,  141,    2, 0x0a /* Public */,
      18,    1,  144,    2, 0x0a /* Public */,
      20,    1,  147,    2, 0x0a /* Public */,
      21,    0,  150,    2, 0x08 /* Private */,
      22,    0,  151,    2, 0x08 /* Private */,
      23,    0,  152,    2, 0x08 /* Private */,
      24,    0,  153,    2, 0x08 /* Private */,
      25,    2,  154,    2, 0x08 /* Private */,
      26,    0,  159,    2, 0x08 /* Private */,
      27,    0,  160,    2, 0x08 /* Private */,
      28,    0,  161,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QColor,    8,    9,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   11,   12,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   15,    3,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QColor,    8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::ScriptWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScriptWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->commandExecuted((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->commandAboutToExecute(); break;
        case 2: _t->executionPaused(); break;
        case 3: _t->exiting(); break;
        case 4: _t->addToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        case 5: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->executeSilentCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->addResultToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 9: _t->addCommandToOutput((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->pause((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->setCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->outputChanged(); break;
        case 13: _t->unpause(); break;
        case 14: _t->pauserClicked(); break;
        case 15: _t->updatePauseTimeout(); break;
        case 16: _t->addTextToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        case 17: _t->setPauserToRunning(); break;
        case 18: _t->resetPauser(); break;
        case 19: _t->flushReportBufferToOutput(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScriptWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::commandExecuted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScriptWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::commandAboutToExecute)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ScriptWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::executionPaused)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ScriptWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::exiting)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ScriptWidget::*)(const QString & , const QColor & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScriptWidget::addToOutput)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::ScriptWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__ScriptWidget.data,
    qt_meta_data_gui__ScriptWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


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
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void gui::ScriptWidget::commandExecuted(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::ScriptWidget::commandAboutToExecute()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::ScriptWidget::executionPaused()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void gui::ScriptWidget::exiting()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void gui::ScriptWidget::addToOutput(const QString & _t1, const QColor & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
