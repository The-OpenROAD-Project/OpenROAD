/****************************************************************************
** Meta object code from reading C++ file 'clockWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/clockWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'clockWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__ClockNodeGraphicsViewItem_t {
    QByteArrayData data[3];
    char stringdata0[43];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ClockNodeGraphicsViewItem_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ClockNodeGraphicsViewItem_t qt_meta_stringdata_gui__ClockNodeGraphicsViewItem = {
    {
QT_MOC_LITERAL(0, 0, 30), // "gui::ClockNodeGraphicsViewItem"
QT_MOC_LITERAL(1, 31, 10), // "updateView"
QT_MOC_LITERAL(2, 42, 0) // ""

    },
    "gui::ClockNodeGraphicsViewItem\0"
    "updateView\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ClockNodeGraphicsViewItem[] = {

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

void gui::ClockNodeGraphicsViewItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClockNodeGraphicsViewItem *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateView(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ClockNodeGraphicsViewItem::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockNodeGraphicsViewItem::updateView)) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject gui::ClockNodeGraphicsViewItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_meta_stringdata_gui__ClockNodeGraphicsViewItem.data,
    qt_meta_data_gui__ClockNodeGraphicsViewItem,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ClockNodeGraphicsViewItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ClockNodeGraphicsViewItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ClockNodeGraphicsViewItem.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int gui::ClockNodeGraphicsViewItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
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
void gui::ClockNodeGraphicsViewItem::updateView()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_gui__ClockTreeScene_t {
    QByteArrayData data[12];
    char stringdata0[136];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ClockTreeScene_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ClockTreeScene_t qt_meta_stringdata_gui__ClockTreeScene = {
    {
QT_MOC_LITERAL(0, 0, 19), // "gui::ClockTreeScene"
QT_MOC_LITERAL(1, 20, 19), // "changeRendererState"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 13), // "RendererState"
QT_MOC_LITERAL(4, 55, 5), // "state"
QT_MOC_LITERAL(5, 61, 3), // "fit"
QT_MOC_LITERAL(6, 65, 9), // "clearPath"
QT_MOC_LITERAL(7, 75, 4), // "save"
QT_MOC_LITERAL(8, 80, 10), // "colorDepth"
QT_MOC_LITERAL(9, 91, 5), // "depth"
QT_MOC_LITERAL(10, 97, 19), // "updateRendererState"
QT_MOC_LITERAL(11, 117, 18) // "triggeredClearPath"

    },
    "gui::ClockTreeScene\0changeRendererState\0"
    "\0RendererState\0state\0fit\0clearPath\0"
    "save\0colorDepth\0depth\0updateRendererState\0"
    "triggeredClearPath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ClockTreeScene[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       5,    0,   52,    2, 0x06 /* Public */,
       6,    0,   53,    2, 0x06 /* Public */,
       7,    0,   54,    2, 0x06 /* Public */,
       8,    1,   55,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    0,   58,    2, 0x08 /* Private */,
      11,    0,   59,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::ClockTreeScene::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClockTreeScene *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->changeRendererState((*reinterpret_cast< RendererState(*)>(_a[1]))); break;
        case 1: _t->fit(); break;
        case 2: _t->clearPath(); break;
        case 3: _t->save(); break;
        case 4: _t->colorDepth((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->updateRendererState(); break;
        case 6: _t->triggeredClearPath(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ClockTreeScene::*)(RendererState );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockTreeScene::changeRendererState)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ClockTreeScene::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockTreeScene::fit)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ClockTreeScene::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockTreeScene::clearPath)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ClockTreeScene::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockTreeScene::save)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ClockTreeScene::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockTreeScene::colorDepth)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::ClockTreeScene::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsScene::staticMetaObject>(),
    qt_meta_stringdata_gui__ClockTreeScene.data,
    qt_meta_data_gui__ClockTreeScene,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ClockTreeScene::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ClockTreeScene::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ClockTreeScene.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsScene::qt_metacast(_clname);
}

int gui::ClockTreeScene::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsScene::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void gui::ClockTreeScene::changeRendererState(RendererState _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::ClockTreeScene::fit()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::ClockTreeScene::clearPath()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void gui::ClockTreeScene::save()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void gui::ClockTreeScene::colorDepth(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
struct qt_meta_stringdata_gui__ClockTreeView_t {
    QByteArrayData data[18];
    char stringdata0[183];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ClockTreeView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ClockTreeView_t qt_meta_stringdata_gui__ClockTreeView = {
    {
QT_MOC_LITERAL(0, 0, 18), // "gui::ClockTreeView"
QT_MOC_LITERAL(1, 19, 8), // "selected"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 8), // "Selected"
QT_MOC_LITERAL(4, 38, 5), // "build"
QT_MOC_LITERAL(5, 44, 16), // "setRendererState"
QT_MOC_LITERAL(6, 61, 13), // "RendererState"
QT_MOC_LITERAL(7, 75, 5), // "state"
QT_MOC_LITERAL(8, 81, 3), // "fit"
QT_MOC_LITERAL(9, 85, 4), // "save"
QT_MOC_LITERAL(10, 90, 4), // "path"
QT_MOC_LITERAL(11, 95, 16), // "updateColorDepth"
QT_MOC_LITERAL(12, 112, 5), // "depth"
QT_MOC_LITERAL(13, 118, 16), // "selectionChanged"
QT_MOC_LITERAL(14, 135, 11), // "highlightTo"
QT_MOC_LITERAL(15, 147, 13), // "odb::dbITerm*"
QT_MOC_LITERAL(16, 161, 4), // "term"
QT_MOC_LITERAL(17, 166, 16) // "clearHighlightTo"

    },
    "gui::ClockTreeView\0selected\0\0Selected\0"
    "build\0setRendererState\0RendererState\0"
    "state\0fit\0save\0path\0updateColorDepth\0"
    "depth\0selectionChanged\0highlightTo\0"
    "odb::dbITerm*\0term\0clearHighlightTo"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ClockTreeView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   67,    2, 0x0a /* Public */,
       5,    1,   68,    2, 0x0a /* Public */,
       8,    0,   71,    2, 0x0a /* Public */,
       9,    1,   72,    2, 0x0a /* Public */,
       9,    0,   75,    2, 0x2a /* Public | MethodCloned */,
      11,    1,   76,    2, 0x0a /* Public */,
      13,    0,   79,    2, 0x08 /* Private */,
      14,    1,   80,    2, 0x08 /* Private */,
      17,    0,   83,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    1,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,

       0        // eod
};

void gui::ClockTreeView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClockTreeView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 1: _t->build(); break;
        case 2: _t->setRendererState((*reinterpret_cast< RendererState(*)>(_a[1]))); break;
        case 3: _t->fit(); break;
        case 4: _t->save((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->save(); break;
        case 6: _t->updateColorDepth((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->selectionChanged(); break;
        case 8: _t->highlightTo((*reinterpret_cast< odb::dbITerm*(*)>(_a[1]))); break;
        case 9: _t->clearHighlightTo(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ClockTreeView::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockTreeView::selected)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::ClockTreeView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_meta_stringdata_gui__ClockTreeView.data,
    qt_meta_data_gui__ClockTreeView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ClockTreeView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ClockTreeView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ClockTreeView.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int gui::ClockTreeView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void gui::ClockTreeView::selected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_gui__ClockWidget_t {
    QByteArrayData data[17];
    char stringdata0[168];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ClockWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ClockWidget_t qt_meta_stringdata_gui__ClockWidget = {
    {
QT_MOC_LITERAL(0, 0, 16), // "gui::ClockWidget"
QT_MOC_LITERAL(1, 17, 8), // "selected"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 8), // "Selected"
QT_MOC_LITERAL(4, 36, 8), // "setBlock"
QT_MOC_LITERAL(5, 45, 13), // "odb::dbBlock*"
QT_MOC_LITERAL(6, 59, 5), // "block"
QT_MOC_LITERAL(7, 65, 8), // "populate"
QT_MOC_LITERAL(8, 74, 12), // "sta::Corner*"
QT_MOC_LITERAL(9, 87, 6), // "corner"
QT_MOC_LITERAL(10, 94, 3), // "fit"
QT_MOC_LITERAL(11, 98, 9), // "findInCts"
QT_MOC_LITERAL(12, 108, 9), // "selection"
QT_MOC_LITERAL(13, 118, 12), // "SelectionSet"
QT_MOC_LITERAL(14, 131, 10), // "selections"
QT_MOC_LITERAL(15, 142, 19), // "currentClockChanged"
QT_MOC_LITERAL(16, 162, 5) // "index"

    },
    "gui::ClockWidget\0selected\0\0Selected\0"
    "setBlock\0odb::dbBlock*\0block\0populate\0"
    "sta::Corner*\0corner\0fit\0findInCts\0"
    "selection\0SelectionSet\0selections\0"
    "currentClockChanged\0index"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ClockWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   57,    2, 0x0a /* Public */,
       7,    1,   60,    2, 0x0a /* Public */,
       7,    0,   63,    2, 0x2a /* Public | MethodCloned */,
      10,    0,   64,    2, 0x0a /* Public */,
      11,    1,   65,    2, 0x0a /* Public */,
      11,    1,   68,    2, 0x0a /* Public */,
      15,    1,   71,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    1,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,   12,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, QMetaType::Int,   16,

       0        // eod
};

void gui::ClockWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ClockWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 1: _t->setBlock((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        case 2: _t->populate((*reinterpret_cast< sta::Corner*(*)>(_a[1]))); break;
        case 3: _t->populate(); break;
        case 4: _t->fit(); break;
        case 5: _t->findInCts((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 6: _t->findInCts((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 7: _t->currentClockChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ClockWidget::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ClockWidget::selected)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::ClockWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__ClockWidget.data,
    qt_meta_data_gui__ClockWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ClockWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ClockWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ClockWidget.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "sta::dbNetworkObserver"))
        return static_cast< sta::dbNetworkObserver*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int gui::ClockWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
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
void gui::ClockWidget::selected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
