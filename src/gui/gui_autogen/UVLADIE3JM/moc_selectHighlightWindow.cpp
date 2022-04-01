/****************************************************************************
** Meta object code from reading C++ file 'selectHighlightWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/selectHighlightWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'selectHighlightWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__SelectionModel_t {
    QByteArrayData data[1];
    char stringdata0[20];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__SelectionModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__SelectionModel_t qt_meta_stringdata_gui__SelectionModel = {
    {
QT_MOC_LITERAL(0, 0, 19) // "gui::SelectionModel"

    },
    "gui::SelectionModel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__SelectionModel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void gui::SelectionModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject gui::SelectionModel::staticMetaObject = {
    { &QAbstractTableModel::staticMetaObject, qt_meta_stringdata_gui__SelectionModel.data,
      qt_meta_data_gui__SelectionModel,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::SelectionModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::SelectionModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__SelectionModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractTableModel::qt_metacast(_clname);
}

int gui::SelectionModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractTableModel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_gui__HighlightModel_t {
    QByteArrayData data[1];
    char stringdata0[20];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__HighlightModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__HighlightModel_t qt_meta_stringdata_gui__HighlightModel = {
    {
QT_MOC_LITERAL(0, 0, 19) // "gui::HighlightModel"

    },
    "gui::HighlightModel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__HighlightModel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void gui::HighlightModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject gui::HighlightModel::staticMetaObject = {
    { &QAbstractTableModel::staticMetaObject, qt_meta_stringdata_gui__HighlightModel.data,
      qt_meta_data_gui__HighlightModel,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::HighlightModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::HighlightModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__HighlightModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractTableModel::qt_metacast(_clname);
}

int gui::HighlightModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractTableModel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_gui__SelectHighlightWindow_t {
    QByteArrayData data[25];
    char stringdata0[421];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__SelectHighlightWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__SelectHighlightWindow_t qt_meta_stringdata_gui__SelectHighlightWindow = {
    {
QT_MOC_LITERAL(0, 0, 26), // "gui::SelectHighlightWindow"
QT_MOC_LITERAL(1, 27, 18), // "clearAllSelections"
QT_MOC_LITERAL(2, 46, 0), // ""
QT_MOC_LITERAL(3, 47, 18), // "clearAllHighlights"
QT_MOC_LITERAL(4, 66, 8), // "selected"
QT_MOC_LITERAL(5, 75, 8), // "Selected"
QT_MOC_LITERAL(6, 84, 9), // "selection"
QT_MOC_LITERAL(7, 94, 18), // "clearSelectedItems"
QT_MOC_LITERAL(8, 113, 22), // "QList<const Selected*>"
QT_MOC_LITERAL(9, 136, 5), // "items"
QT_MOC_LITERAL(10, 142, 21), // "clearHighlightedItems"
QT_MOC_LITERAL(11, 164, 13), // "zoomInToItems"
QT_MOC_LITERAL(12, 178, 25), // "highlightSelectedItemsSig"
QT_MOC_LITERAL(13, 204, 20), // "updateSelectionModel"
QT_MOC_LITERAL(14, 225, 20), // "updateHighlightModel"
QT_MOC_LITERAL(15, 246, 12), // "updateModels"
QT_MOC_LITERAL(16, 259, 20), // "showSelectCustomMenu"
QT_MOC_LITERAL(17, 280, 3), // "pos"
QT_MOC_LITERAL(18, 284, 23), // "showHighlightCustomMenu"
QT_MOC_LITERAL(19, 308, 15), // "changeHighlight"
QT_MOC_LITERAL(20, 324, 13), // "deselectItems"
QT_MOC_LITERAL(21, 338, 22), // "highlightSelectedItems"
QT_MOC_LITERAL(22, 361, 19), // "zoomInSelectedItems"
QT_MOC_LITERAL(23, 381, 16), // "dehighlightItems"
QT_MOC_LITERAL(24, 398, 22) // "zoomInHighlightedItems"

    },
    "gui::SelectHighlightWindow\0"
    "clearAllSelections\0\0clearAllHighlights\0"
    "selected\0Selected\0selection\0"
    "clearSelectedItems\0QList<const Selected*>\0"
    "items\0clearHighlightedItems\0zoomInToItems\0"
    "highlightSelectedItemsSig\0"
    "updateSelectionModel\0updateHighlightModel\0"
    "updateModels\0showSelectCustomMenu\0pos\0"
    "showHighlightCustomMenu\0changeHighlight\0"
    "deselectItems\0highlightSelectedItems\0"
    "zoomInSelectedItems\0dehighlightItems\0"
    "zoomInHighlightedItems"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__SelectHighlightWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  104,    2, 0x06 /* Public */,
       3,    0,  105,    2, 0x06 /* Public */,
       4,    1,  106,    2, 0x06 /* Public */,
       7,    1,  109,    2, 0x06 /* Public */,
      10,    1,  112,    2, 0x06 /* Public */,
      11,    1,  115,    2, 0x06 /* Public */,
      12,    1,  118,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    0,  121,    2, 0x0a /* Public */,
      14,    0,  122,    2, 0x0a /* Public */,
      15,    0,  123,    2, 0x0a /* Public */,
      16,    1,  124,    2, 0x0a /* Public */,
      18,    1,  127,    2, 0x0a /* Public */,
      19,    0,  130,    2, 0x0a /* Public */,
      20,    0,  131,    2, 0x0a /* Public */,
      21,    0,  132,    2, 0x0a /* Public */,
      22,    0,  133,    2, 0x0a /* Public */,
      23,    0,  134,    2, 0x0a /* Public */,
      24,    0,  135,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 8,    9,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   17,
    QMetaType::Void, QMetaType::QPoint,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::SelectHighlightWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SelectHighlightWindow *_t = static_cast<SelectHighlightWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->clearAllSelections(); break;
        case 1: _t->clearAllHighlights(); break;
        case 2: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 3: _t->clearSelectedItems((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 4: _t->clearHighlightedItems((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 5: _t->zoomInToItems((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 6: _t->highlightSelectedItemsSig((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 7: _t->updateSelectionModel(); break;
        case 8: _t->updateHighlightModel(); break;
        case 9: _t->updateModels(); break;
        case 10: _t->showSelectCustomMenu((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 11: _t->showHighlightCustomMenu((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 12: _t->changeHighlight(); break;
        case 13: _t->deselectItems(); break;
        case 14: _t->highlightSelectedItems(); break;
        case 15: _t->zoomInSelectedItems(); break;
        case 16: _t->dehighlightItems(); break;
        case 17: _t->zoomInHighlightedItems(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (SelectHighlightWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::clearAllSelections)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (SelectHighlightWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::clearAllHighlights)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (SelectHighlightWindow::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::selected)) {
                *result = 2;
                return;
            }
        }
        {
            typedef void (SelectHighlightWindow::*_t)(const QList<const Selected*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::clearSelectedItems)) {
                *result = 3;
                return;
            }
        }
        {
            typedef void (SelectHighlightWindow::*_t)(const QList<const Selected*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::clearHighlightedItems)) {
                *result = 4;
                return;
            }
        }
        {
            typedef void (SelectHighlightWindow::*_t)(const QList<const Selected*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::zoomInToItems)) {
                *result = 5;
                return;
            }
        }
        {
            typedef void (SelectHighlightWindow::*_t)(const QList<const Selected*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectHighlightWindow::highlightSelectedItemsSig)) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject gui::SelectHighlightWindow::staticMetaObject = {
    { &QDockWidget::staticMetaObject, qt_meta_stringdata_gui__SelectHighlightWindow.data,
      qt_meta_data_gui__SelectHighlightWindow,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::SelectHighlightWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::SelectHighlightWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__SelectHighlightWindow.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int gui::SelectHighlightWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void gui::SelectHighlightWindow::clearAllSelections()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void gui::SelectHighlightWindow::clearAllHighlights()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::SelectHighlightWindow::selected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void gui::SelectHighlightWindow::clearSelectedItems(const QList<const Selected*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void gui::SelectHighlightWindow::clearHighlightedItems(const QList<const Selected*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::SelectHighlightWindow::zoomInToItems(const QList<const Selected*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void gui::SelectHighlightWindow::highlightSelectedItemsSig(const QList<const Selected*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
