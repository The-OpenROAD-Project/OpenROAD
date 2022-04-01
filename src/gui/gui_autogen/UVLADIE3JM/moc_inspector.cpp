/****************************************************************************
** Meta object code from reading C++ file 'inspector.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/inspector.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'inspector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__EditorItemDelegate_t {
    QByteArrayData data[1];
    char stringdata0[24];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__EditorItemDelegate_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__EditorItemDelegate_t qt_meta_stringdata_gui__EditorItemDelegate = {
    {
QT_MOC_LITERAL(0, 0, 23) // "gui::EditorItemDelegate"

    },
    "gui::EditorItemDelegate"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__EditorItemDelegate[] = {

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

void gui::EditorItemDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject gui::EditorItemDelegate::staticMetaObject = {
    { &QItemDelegate::staticMetaObject, qt_meta_stringdata_gui__EditorItemDelegate.data,
      qt_meta_data_gui__EditorItemDelegate,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::EditorItemDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::EditorItemDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__EditorItemDelegate.stringdata0))
        return static_cast<void*>(this);
    return QItemDelegate::qt_metacast(_clname);
}

int gui::EditorItemDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QItemDelegate::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_gui__SelectedItemModel_t {
    QByteArrayData data[5];
    char stringdata0[63];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__SelectedItemModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__SelectedItemModel_t qt_meta_stringdata_gui__SelectedItemModel = {
    {
QT_MOC_LITERAL(0, 0, 22), // "gui::SelectedItemModel"
QT_MOC_LITERAL(1, 23, 19), // "selectedItemChanged"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 5), // "index"
QT_MOC_LITERAL(4, 50, 12) // "updateObject"

    },
    "gui::SelectedItemModel\0selectedItemChanged\0"
    "\0index\0updateObject"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__SelectedItemModel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   27,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QModelIndex,    3,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void gui::SelectedItemModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        SelectedItemModel *_t = static_cast<SelectedItemModel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->selectedItemChanged((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 1: _t->updateObject(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (SelectedItemModel::*_t)(const QModelIndex & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectedItemModel::selectedItemChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject gui::SelectedItemModel::staticMetaObject = {
    { &QStandardItemModel::staticMetaObject, qt_meta_stringdata_gui__SelectedItemModel.data,
      qt_meta_data_gui__SelectedItemModel,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::SelectedItemModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::SelectedItemModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__SelectedItemModel.stringdata0))
        return static_cast<void*>(this);
    return QStandardItemModel::qt_metacast(_clname);
}

int gui::SelectedItemModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStandardItemModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void gui::SelectedItemModel::selectedItemChanged(const QModelIndex & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_gui__ActionLayout_t {
    QByteArrayData data[1];
    char stringdata0[18];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ActionLayout_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ActionLayout_t qt_meta_stringdata_gui__ActionLayout = {
    {
QT_MOC_LITERAL(0, 0, 17) // "gui::ActionLayout"

    },
    "gui::ActionLayout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ActionLayout[] = {

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

void gui::ActionLayout::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject gui::ActionLayout::staticMetaObject = {
    { &QLayout::staticMetaObject, qt_meta_stringdata_gui__ActionLayout.data,
      qt_meta_data_gui__ActionLayout,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::ActionLayout::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ActionLayout::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ActionLayout.stringdata0))
        return static_cast<void*>(this);
    return QLayout::qt_metacast(_clname);
}

int gui::ActionLayout::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLayout::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_gui__ObjectTree_t {
    QByteArrayData data[3];
    char stringdata0[29];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ObjectTree_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ObjectTree_t qt_meta_stringdata_gui__ObjectTree = {
    {
QT_MOC_LITERAL(0, 0, 15), // "gui::ObjectTree"
QT_MOC_LITERAL(1, 16, 11), // "mouseExited"
QT_MOC_LITERAL(2, 28, 0) // ""

    },
    "gui::ObjectTree\0mouseExited\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ObjectTree[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

void gui::ObjectTree::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ObjectTree *_t = static_cast<ObjectTree *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->mouseExited(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (ObjectTree::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ObjectTree::mouseExited)) {
                *result = 0;
                return;
            }
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject gui::ObjectTree::staticMetaObject = {
    { &QTreeView::staticMetaObject, qt_meta_stringdata_gui__ObjectTree.data,
      qt_meta_data_gui__ObjectTree,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::ObjectTree::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ObjectTree::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ObjectTree.stringdata0))
        return static_cast<void*>(this);
    return QTreeView::qt_metacast(_clname);
}

int gui::ObjectTree::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void gui::ObjectTree::mouseExited()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_gui__Inspector_t {
    QByteArrayData data[29];
    char stringdata0[354];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__Inspector_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__Inspector_t qt_meta_stringdata_gui__Inspector = {
    {
QT_MOC_LITERAL(0, 0, 14), // "gui::Inspector"
QT_MOC_LITERAL(1, 15, 11), // "addSelected"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 8), // "Selected"
QT_MOC_LITERAL(4, 37, 8), // "selected"
QT_MOC_LITERAL(5, 46, 14), // "removeSelected"
QT_MOC_LITERAL(6, 61, 16), // "showConnectivity"
QT_MOC_LITERAL(7, 78, 19), // "selectedItemChanged"
QT_MOC_LITERAL(8, 98, 9), // "selection"
QT_MOC_LITERAL(9, 108, 5), // "focus"
QT_MOC_LITERAL(10, 114, 12), // "addHighlight"
QT_MOC_LITERAL(11, 127, 12), // "SelectionSet"
QT_MOC_LITERAL(12, 140, 15), // "removeHighlight"
QT_MOC_LITERAL(13, 156, 22), // "QList<const Selected*>"
QT_MOC_LITERAL(14, 179, 7), // "inspect"
QT_MOC_LITERAL(15, 187, 6), // "object"
QT_MOC_LITERAL(16, 194, 7), // "clicked"
QT_MOC_LITERAL(17, 202, 5), // "index"
QT_MOC_LITERAL(18, 208, 6), // "update"
QT_MOC_LITERAL(19, 215, 16), // "highlightChanged"
QT_MOC_LITERAL(20, 232, 16), // "focusNetsChanged"
QT_MOC_LITERAL(21, 249, 10), // "selectNext"
QT_MOC_LITERAL(22, 260, 14), // "selectPrevious"
QT_MOC_LITERAL(23, 275, 20), // "updateSelectedFields"
QT_MOC_LITERAL(24, 296, 6), // "reload"
QT_MOC_LITERAL(25, 303, 10), // "focusIndex"
QT_MOC_LITERAL(26, 314, 7), // "defocus"
QT_MOC_LITERAL(27, 322, 12), // "indexClicked"
QT_MOC_LITERAL(28, 335, 18) // "indexDoubleClicked"

    },
    "gui::Inspector\0addSelected\0\0Selected\0"
    "selected\0removeSelected\0showConnectivity\0"
    "selectedItemChanged\0selection\0focus\0"
    "addHighlight\0SelectionSet\0removeHighlight\0"
    "QList<const Selected*>\0inspect\0object\0"
    "clicked\0index\0update\0highlightChanged\0"
    "focusNetsChanged\0selectNext\0selectPrevious\0"
    "updateSelectedFields\0reload\0focusIndex\0"
    "defocus\0indexClicked\0indexDoubleClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__Inspector[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  129,    2, 0x06 /* Public */,
       5,    1,  132,    2, 0x06 /* Public */,
       4,    2,  135,    2, 0x06 /* Public */,
       4,    1,  140,    2, 0x26 /* Public | MethodCloned */,
       7,    1,  143,    2, 0x06 /* Public */,
       8,    1,  146,    2, 0x06 /* Public */,
       9,    1,  149,    2, 0x06 /* Public */,
      10,    1,  152,    2, 0x06 /* Public */,
      12,    1,  155,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    1,  158,    2, 0x0a /* Public */,
      16,    1,  161,    2, 0x0a /* Public */,
      18,    1,  164,    2, 0x0a /* Public */,
      18,    0,  167,    2, 0x2a /* Public | MethodCloned */,
      19,    0,  168,    2, 0x0a /* Public */,
      20,    0,  169,    2, 0x0a /* Public */,
      21,    0,  170,    2, 0x0a /* Public */,
      22,    0,  171,    2, 0x0a /* Public */,
      23,    1,  172,    2, 0x0a /* Public */,
      24,    0,  175,    2, 0x0a /* Public */,
      25,    1,  176,    2, 0x08 /* Private */,
      26,    0,  179,    2, 0x08 /* Private */,
      27,    0,  180,    2, 0x08 /* Private */,
      28,    1,  181,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Bool,    4,    6,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 11,    8,
    QMetaType::Void, 0x80000000 | 13,    4,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,   15,
    QMetaType::Void, QMetaType::QModelIndex,   17,
    QMetaType::Void, 0x80000000 | 3,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Int,
    QMetaType::Int,
    QMetaType::Void, QMetaType::QModelIndex,   17,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QModelIndex,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QModelIndex,   17,

       0        // eod
};

void gui::Inspector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Inspector *_t = static_cast<Inspector *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->addSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 1: _t->removeSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 2: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 4: _t->selectedItemChanged((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 5: _t->selection((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 6: _t->focus((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 7: _t->addHighlight((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 8: _t->removeHighlight((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 9: _t->inspect((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 10: _t->clicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 11: _t->update((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 12: _t->update(); break;
        case 13: _t->highlightChanged(); break;
        case 14: _t->focusNetsChanged(); break;
        case 15: { int _r = _t->selectNext();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 16: { int _r = _t->selectPrevious();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->updateSelectedFields((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 18: _t->reload(); break;
        case 19: _t->focusIndex((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 20: _t->defocus(); break;
        case 21: _t->indexClicked(); break;
        case 22: _t->indexDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (Inspector::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::addSelected)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::removeSelected)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const Selected & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::selected)) {
                *result = 2;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::selectedItemChanged)) {
                *result = 4;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::selection)) {
                *result = 5;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::focus)) {
                *result = 6;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const SelectionSet & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::addHighlight)) {
                *result = 7;
                return;
            }
        }
        {
            typedef void (Inspector::*_t)(const QList<const Selected*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::removeHighlight)) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject gui::Inspector::staticMetaObject = {
    { &QDockWidget::staticMetaObject, qt_meta_stringdata_gui__Inspector.data,
      qt_meta_data_gui__Inspector,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::Inspector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::Inspector::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__Inspector.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int gui::Inspector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 23;
    }
    return _id;
}

// SIGNAL 0
void gui::Inspector::addSelected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::Inspector::removeSelected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void gui::Inspector::selected(const Selected & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void gui::Inspector::selectedItemChanged(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::Inspector::selection(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void gui::Inspector::focus(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void gui::Inspector::addHighlight(const SelectionSet & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void gui::Inspector::removeHighlight(const QList<const Selected*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
