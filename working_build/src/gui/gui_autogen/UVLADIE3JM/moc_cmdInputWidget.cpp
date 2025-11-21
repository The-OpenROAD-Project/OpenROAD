/****************************************************************************
** Meta object code from reading C++ file 'cmdInputWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/cmdInputWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cmdInputWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__CmdInputWidget_t {
    QByteArrayData data[18];
    char stringdata0[211];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__CmdInputWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__CmdInputWidget_t qt_meta_stringdata_gui__CmdInputWidget = {
    {
QT_MOC_LITERAL(0, 0, 19), // "gui::CmdInputWidget"
QT_MOC_LITERAL(1, 20, 7), // "exiting"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 21), // "commandAboutToExecute"
QT_MOC_LITERAL(4, 51, 24), // "commandFinishedExecuting"
QT_MOC_LITERAL(5, 76, 5), // "is_ok"
QT_MOC_LITERAL(6, 82, 17), // "addResultToOutput"
QT_MOC_LITERAL(7, 100, 6), // "result"
QT_MOC_LITERAL(8, 107, 15), // "addTextToOutput"
QT_MOC_LITERAL(9, 123, 4), // "text"
QT_MOC_LITERAL(10, 128, 5), // "color"
QT_MOC_LITERAL(11, 134, 18), // "addCommandToOutput"
QT_MOC_LITERAL(12, 153, 3), // "cmd"
QT_MOC_LITERAL(13, 157, 14), // "executeCommand"
QT_MOC_LITERAL(14, 172, 4), // "echo"
QT_MOC_LITERAL(15, 177, 6), // "silent"
QT_MOC_LITERAL(16, 184, 10), // "updateSize"
QT_MOC_LITERAL(17, 195, 15) // "commandFinished"

    },
    "gui::CmdInputWidget\0exiting\0\0"
    "commandAboutToExecute\0commandFinishedExecuting\0"
    "is_ok\0addResultToOutput\0result\0"
    "addTextToOutput\0text\0color\0"
    "addCommandToOutput\0cmd\0executeCommand\0"
    "echo\0silent\0updateSize\0commandFinished"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__CmdInputWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   69,    2, 0x06 /* Public */,
       3,    0,   70,    2, 0x06 /* Public */,
       4,    1,   71,    2, 0x06 /* Public */,
       6,    2,   74,    2, 0x06 /* Public */,
       8,    2,   79,    2, 0x06 /* Public */,
      11,    1,   84,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    3,   87,    2, 0x0a /* Public */,
      13,    2,   94,    2, 0x2a /* Public | MethodCloned */,
      13,    1,   99,    2, 0x2a /* Public | MethodCloned */,
      16,    0,  102,    2, 0x08 /* Private */,
      17,    1,  103,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    7,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::QColor,    9,   10,
    QMetaType::Void, QMetaType::QString,   12,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Bool, QMetaType::Bool,   12,   14,   15,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   12,   14,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,

       0        // eod
};

void gui::CmdInputWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CmdInputWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->exiting(); break;
        case 1: _t->commandAboutToExecute(); break;
        case 2: _t->commandFinishedExecuting((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->addResultToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 4: _t->addTextToOutput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        case 5: _t->addCommandToOutput((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 7: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 8: _t->executeCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->updateSize(); break;
        case 10: _t->commandFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CmdInputWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CmdInputWidget::exiting)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CmdInputWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CmdInputWidget::commandAboutToExecute)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CmdInputWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CmdInputWidget::commandFinishedExecuting)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CmdInputWidget::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CmdInputWidget::addResultToOutput)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CmdInputWidget::*)(const QString & , const QColor & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CmdInputWidget::addTextToOutput)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CmdInputWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CmdInputWidget::addCommandToOutput)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::CmdInputWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QPlainTextEdit::staticMetaObject>(),
    qt_meta_stringdata_gui__CmdInputWidget.data,
    qt_meta_data_gui__CmdInputWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::CmdInputWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::CmdInputWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__CmdInputWidget.stringdata0))
        return static_cast<void*>(this);
    return QPlainTextEdit::qt_metacast(_clname);
}

int gui::CmdInputWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPlainTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void gui::CmdInputWidget::exiting()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void gui::CmdInputWidget::commandAboutToExecute()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::CmdInputWidget::commandFinishedExecuting(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void gui::CmdInputWidget::addResultToOutput(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void gui::CmdInputWidget::addTextToOutput(const QString & _t1, const QColor & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::CmdInputWidget::addCommandToOutput(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
