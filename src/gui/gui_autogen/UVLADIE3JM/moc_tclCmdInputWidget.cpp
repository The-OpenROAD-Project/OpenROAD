/****************************************************************************
** Meta object code from reading C++ file 'tclCmdInputWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/tclCmdInputWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tclCmdInputWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__TclCmdInputWidget_t {
    QByteArrayData data[13];
    char stringdata0[176];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__TclCmdInputWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__TclCmdInputWidget_t qt_meta_stringdata_gui__TclCmdInputWidget = {
    {
QT_MOC_LITERAL(0, 0, 22), // "gui::TclCmdInputWidget"
QT_MOC_LITERAL(1, 23, 15), // "completeCommand"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 7), // "command"
QT_MOC_LITERAL(4, 48, 13), // "historyGoBack"
QT_MOC_LITERAL(5, 62, 16), // "historyGoForward"
QT_MOC_LITERAL(6, 79, 15), // "commandExecuted"
QT_MOC_LITERAL(7, 95, 11), // "return_code"
QT_MOC_LITERAL(8, 107, 10), // "updateSize"
QT_MOC_LITERAL(9, 118, 18), // "updateHighlighting"
QT_MOC_LITERAL(10, 137, 16), // "updateCompletion"
QT_MOC_LITERAL(11, 154, 16), // "insertCompletion"
QT_MOC_LITERAL(12, 171, 4) // "text"

    },
    "gui::TclCmdInputWidget\0completeCommand\0"
    "\0command\0historyGoBack\0historyGoForward\0"
    "commandExecuted\0return_code\0updateSize\0"
    "updateHighlighting\0updateCompletion\0"
    "insertCompletion\0text"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__TclCmdInputWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       4,    0,   57,    2, 0x06 /* Public */,
       5,    0,   58,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    1,   59,    2, 0x0a /* Public */,
       8,    0,   62,    2, 0x08 /* Private */,
       9,    0,   63,    2, 0x08 /* Private */,
      10,    0,   64,    2, 0x08 /* Private */,
      11,    1,   65,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   12,

       0        // eod
};

void gui::TclCmdInputWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TclCmdInputWidget *_t = static_cast<TclCmdInputWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->completeCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->historyGoBack(); break;
        case 2: _t->historyGoForward(); break;
        case 3: _t->commandExecuted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->updateSize(); break;
        case 5: _t->updateHighlighting(); break;
        case 6: _t->updateCompletion(); break;
        case 7: _t->insertCompletion((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (TclCmdInputWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TclCmdInputWidget::completeCommand)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (TclCmdInputWidget::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TclCmdInputWidget::historyGoBack)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (TclCmdInputWidget::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TclCmdInputWidget::historyGoForward)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject gui::TclCmdInputWidget::staticMetaObject = {
    { &QPlainTextEdit::staticMetaObject, qt_meta_stringdata_gui__TclCmdInputWidget.data,
      qt_meta_data_gui__TclCmdInputWidget,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::TclCmdInputWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::TclCmdInputWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__TclCmdInputWidget.stringdata0))
        return static_cast<void*>(this);
    return QPlainTextEdit::qt_metacast(_clname);
}

int gui::TclCmdInputWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPlainTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void gui::TclCmdInputWidget::completeCommand(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::TclCmdInputWidget::historyGoBack()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::TclCmdInputWidget::historyGoForward()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
