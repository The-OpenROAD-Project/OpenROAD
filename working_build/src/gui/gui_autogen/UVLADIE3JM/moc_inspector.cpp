/****************************************************************************
** Meta object code from reading C++ file 'inspector.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/inspector.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'inspector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
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
       8,       // revision
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
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject gui::EditorItemDelegate::staticMetaObject = { {
    QMetaObject::SuperData::link<QItemDelegate::staticMetaObject>(),
    qt_meta_stringdata_gui__EditorItemDelegate.data,
    qt_meta_data_gui__EditorItemDelegate,
    qt_static_metacall,
    nullptr,
    nullptr
} };


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
    QByteArrayData data[6];
    char stringdata0[75];
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
QT_MOC_LITERAL(3, 44, 11), // "QModelIndex"
QT_MOC_LITERAL(4, 56, 5), // "index"
QT_MOC_LITERAL(5, 62, 12) // "updateObject"

    },
    "gui::SelectedItemModel\0selectedItemChanged\0"
    "\0QModelIndex\0index\0updateObject"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__SelectedItemModel[] = {

 // content:
       8,       // revision
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
       5,    0,   27,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void gui::SelectedItemModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SelectedItemModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selectedItemChanged((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 1: _t->updateObject(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SelectedItemModel::*)(const QModelIndex & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SelectedItemModel::selectedItemChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::SelectedItemModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QStandardItemModel::staticMetaObject>(),
    qt_meta_stringdata_gui__SelectedItemModel.data,
    qt_meta_data_gui__SelectedItemModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


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
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
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
       8,       // revision
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
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject gui::ActionLayout::staticMetaObject = { {
    QMetaObject::SuperData::link<QLayout::staticMetaObject>(),
    qt_meta_stringdata_gui__ActionLayout.data,
    qt_meta_data_gui__ActionLayout,
    qt_static_metacall,
    nullptr,
    nullptr
} };


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
       8,       // revision
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
        auto *_t = static_cast<ObjectTree *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->mouseExited(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ObjectTree::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ObjectTree::mouseExited)) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject gui::ObjectTree::staticMetaObject = { {
    QMetaObject::SuperData::link<QTreeView::staticMetaObject>(),
    qt_meta_stringdata_gui__ObjectTree.data,
    qt_meta_data_gui__ObjectTree,
    qt_static_metacall,
    nullptr,
    nullptr
} };


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
    QByteArrayData data[36];
    char stringdata0[425];
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
QT_MOC_LITERAL(6, 61, 17), // "show_connectivity"
QT_MOC_LITERAL(7, 79, 19), // "selectedItemChanged"
QT_MOC_LITERAL(8, 99, 9), // "selection"
QT_MOC_LITERAL(9, 109, 5), // "focus"
QT_MOC_LITERAL(10, 115, 12), // "addHighlight"
QT_MOC_LITERAL(11, 128, 12), // "SelectionSet"
QT_MOC_LITERAL(12, 141, 15), // "removeHighlight"
QT_MOC_LITERAL(13, 157, 22), // "QList<const Selected*>"
QT_MOC_LITERAL(14, 180, 10), // "setCommand"
QT_MOC_LITERAL(15, 191, 7), // "command"
QT_MOC_LITERAL(16, 199, 7), // "inspect"
QT_MOC_LITERAL(17, 207, 6), // "object"
QT_MOC_LITERAL(18, 214, 7), // "clicked"
QT_MOC_LITERAL(19, 222, 11), // "QModelIndex"
QT_MOC_LITERAL(20, 234, 5), // "index"
QT_MOC_LITERAL(21, 240, 13), // "doubleClicked"
QT_MOC_LITERAL(22, 254, 6), // "update"
QT_MOC_LITERAL(23, 261, 10), // "selectNext"
QT_MOC_LITERAL(24, 272, 14), // "selectPrevious"
QT_MOC_LITERAL(25, 287, 20), // "updateSelectedFields"
QT_MOC_LITERAL(26, 308, 11), // "setReadOnly"
QT_MOC_LITERAL(27, 320, 13), // "unsetReadOnly"
QT_MOC_LITERAL(28, 334, 6), // "reload"
QT_MOC_LITERAL(29, 341, 11), // "loadActions"
QT_MOC_LITERAL(30, 353, 10), // "focusIndex"
QT_MOC_LITERAL(31, 364, 7), // "defocus"
QT_MOC_LITERAL(32, 372, 12), // "indexClicked"
QT_MOC_LITERAL(33, 385, 18), // "indexDoubleClicked"
QT_MOC_LITERAL(34, 404, 16), // "showCommandsMenu"
QT_MOC_LITERAL(35, 421, 3) // "pos"

    },
    "gui::Inspector\0addSelected\0\0Selected\0"
    "selected\0removeSelected\0show_connectivity\0"
    "selectedItemChanged\0selection\0focus\0"
    "addHighlight\0SelectionSet\0removeHighlight\0"
    "QList<const Selected*>\0setCommand\0"
    "command\0inspect\0object\0clicked\0"
    "QModelIndex\0index\0doubleClicked\0update\0"
    "selectNext\0selectPrevious\0"
    "updateSelectedFields\0setReadOnly\0"
    "unsetReadOnly\0reload\0loadActions\0"
    "focusIndex\0defocus\0indexClicked\0"
    "indexDoubleClicked\0showCommandsMenu\0"
    "pos"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__Inspector[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  149,    2, 0x06 /* Public */,
       5,    1,  152,    2, 0x06 /* Public */,
       4,    2,  155,    2, 0x06 /* Public */,
       4,    1,  160,    2, 0x26 /* Public | MethodCloned */,
       7,    1,  163,    2, 0x06 /* Public */,
       8,    1,  166,    2, 0x06 /* Public */,
       9,    1,  169,    2, 0x06 /* Public */,
      10,    1,  172,    2, 0x06 /* Public */,
      12,    1,  175,    2, 0x06 /* Public */,
      14,    1,  178,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      16,    1,  181,    2, 0x0a /* Public */,
      18,    1,  184,    2, 0x0a /* Public */,
      21,    1,  187,    2, 0x0a /* Public */,
      22,    1,  190,    2, 0x0a /* Public */,
      22,    0,  193,    2, 0x2a /* Public | MethodCloned */,
      23,    0,  194,    2, 0x0a /* Public */,
      24,    0,  195,    2, 0x0a /* Public */,
      25,    1,  196,    2, 0x0a /* Public */,
      26,    0,  199,    2, 0x0a /* Public */,
      27,    0,  200,    2, 0x0a /* Public */,
      28,    0,  201,    2, 0x0a /* Public */,
      29,    0,  202,    2, 0x0a /* Public */,
      30,    1,  203,    2, 0x08 /* Private */,
      31,    0,  206,    2, 0x08 /* Private */,
      32,    0,  207,    2, 0x08 /* Private */,
      33,    1,  208,    2, 0x08 /* Private */,
      34,    1,  211,    2, 0x08 /* Private */,

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
    QMetaType::Void, QMetaType::QString,   15,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,   17,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, 0x80000000 | 3,   17,
    QMetaType::Void,
    QMetaType::Int,
    QMetaType::Int,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, QMetaType::QPoint,   35,

       0        // eod
};

void gui::Inspector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Inspector *>(_o);
        (void)_t;
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
        case 9: _t->setCommand((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->inspect((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 11: _t->clicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 12: _t->doubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 13: _t->update((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 14: _t->update(); break;
        case 15: { int _r = _t->selectNext();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 16: { int _r = _t->selectPrevious();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->updateSelectedFields((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 18: _t->setReadOnly(); break;
        case 19: _t->unsetReadOnly(); break;
        case 20: _t->reload(); break;
        case 21: _t->loadActions(); break;
        case 22: _t->focusIndex((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 23: _t->defocus(); break;
        case 24: _t->indexClicked(); break;
        case 25: _t->indexDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 26: _t->showCommandsMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Inspector::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::addSelected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::removeSelected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const Selected & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::selected)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::selectedItemChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::selection)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::focus)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const SelectionSet & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::addHighlight)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const QList<const Selected*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::removeHighlight)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (Inspector::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Inspector::setCommand)) {
                *result = 9;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::Inspector::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__Inspector.data,
    qt_meta_data_gui__Inspector,
    qt_static_metacall,
    nullptr,
    nullptr
} };


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
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 27;
    }
    return _id;
}

// SIGNAL 0
void gui::Inspector::addSelected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::Inspector::removeSelected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void gui::Inspector::selected(const Selected & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void gui::Inspector::selectedItemChanged(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::Inspector::selection(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void gui::Inspector::focus(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void gui::Inspector::addHighlight(const SelectionSet & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void gui::Inspector::removeHighlight(const QList<const Selected*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void gui::Inspector::setCommand(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
