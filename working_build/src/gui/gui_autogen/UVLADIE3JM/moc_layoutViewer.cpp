/****************************************************************************
** Meta object code from reading C++ file 'layoutViewer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/layoutViewer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'layoutViewer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__LayoutViewer_t {
    QByteArrayData data[67];
    char stringdata0[787];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__LayoutViewer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__LayoutViewer_t qt_meta_stringdata_gui__LayoutViewer = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::LayoutViewer"
QT_MOC_LITERAL(1, 18, 8), // "location"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 1), // "x"
QT_MOC_LITERAL(4, 30, 1), // "y"
QT_MOC_LITERAL(5, 32, 8), // "selected"
QT_MOC_LITERAL(6, 41, 8), // "Selected"
QT_MOC_LITERAL(7, 50, 17), // "show_connectivity"
QT_MOC_LITERAL(8, 68, 11), // "addSelected"
QT_MOC_LITERAL(9, 80, 12), // "SelectionSet"
QT_MOC_LITERAL(10, 93, 8), // "addRuler"
QT_MOC_LITERAL(11, 102, 2), // "x0"
QT_MOC_LITERAL(12, 105, 2), // "y0"
QT_MOC_LITERAL(13, 108, 2), // "x1"
QT_MOC_LITERAL(14, 111, 2), // "y1"
QT_MOC_LITERAL(15, 114, 16), // "focusNetsChanged"
QT_MOC_LITERAL(16, 131, 6), // "zoomIn"
QT_MOC_LITERAL(17, 138, 10), // "odb::Point"
QT_MOC_LITERAL(18, 149, 5), // "focus"
QT_MOC_LITERAL(19, 155, 14), // "do_delta_focus"
QT_MOC_LITERAL(20, 170, 7), // "zoomOut"
QT_MOC_LITERAL(21, 178, 6), // "zoomTo"
QT_MOC_LITERAL(22, 185, 9), // "odb::Rect"
QT_MOC_LITERAL(23, 195, 8), // "rect_dbu"
QT_MOC_LITERAL(24, 204, 11), // "blockLoaded"
QT_MOC_LITERAL(25, 216, 13), // "odb::dbBlock*"
QT_MOC_LITERAL(26, 230, 5), // "block"
QT_MOC_LITERAL(27, 236, 3), // "fit"
QT_MOC_LITERAL(28, 240, 8), // "centerAt"
QT_MOC_LITERAL(29, 249, 12), // "updateCenter"
QT_MOC_LITERAL(30, 262, 2), // "dx"
QT_MOC_LITERAL(31, 265, 2), // "dy"
QT_MOC_LITERAL(32, 268, 13), // "setResolution"
QT_MOC_LITERAL(33, 282, 14), // "pixels_per_dbu"
QT_MOC_LITERAL(34, 297, 15), // "viewportUpdated"
QT_MOC_LITERAL(35, 313, 11), // "fullRepaint"
QT_MOC_LITERAL(36, 325, 16), // "getVisibleCenter"
QT_MOC_LITERAL(37, 342, 28), // "selectHighlightConnectedInst"
QT_MOC_LITERAL(38, 371, 11), // "select_flag"
QT_MOC_LITERAL(39, 383, 28), // "selectHighlightConnectedNets"
QT_MOC_LITERAL(40, 412, 6), // "output"
QT_MOC_LITERAL(41, 419, 5), // "input"
QT_MOC_LITERAL(42, 425, 35), // "selectHighlightConnectedBuffe..."
QT_MOC_LITERAL(43, 461, 15), // "highlight_group"
QT_MOC_LITERAL(44, 477, 22), // "updateContextMenuItems"
QT_MOC_LITERAL(45, 500, 20), // "showLayoutCustomMenu"
QT_MOC_LITERAL(46, 521, 3), // "pos"
QT_MOC_LITERAL(47, 525, 15), // "startRulerBuild"
QT_MOC_LITERAL(48, 541, 16), // "cancelRulerBuild"
QT_MOC_LITERAL(49, 558, 10), // "selectArea"
QT_MOC_LITERAL(50, 569, 4), // "area"
QT_MOC_LITERAL(51, 574, 6), // "append"
QT_MOC_LITERAL(52, 581, 9), // "selection"
QT_MOC_LITERAL(53, 591, 14), // "selectionFocus"
QT_MOC_LITERAL(54, 606, 18), // "selectionAnimation"
QT_MOC_LITERAL(55, 625, 7), // "repeats"
QT_MOC_LITERAL(56, 633, 15), // "update_interval"
QT_MOC_LITERAL(57, 649, 4), // "exit"
QT_MOC_LITERAL(58, 654, 10), // "resetCache"
QT_MOC_LITERAL(59, 665, 21), // "commandAboutToExecute"
QT_MOC_LITERAL(60, 687, 24), // "commandFinishedExecuting"
QT_MOC_LITERAL(61, 712, 15), // "executionPaused"
QT_MOC_LITERAL(62, 728, 8), // "setBlock"
QT_MOC_LITERAL(63, 737, 12), // "updatePixmap"
QT_MOC_LITERAL(64, 750, 5), // "image"
QT_MOC_LITERAL(65, 756, 6), // "bounds"
QT_MOC_LITERAL(66, 763, 23) // "handleLoadingIndication"

    },
    "gui::LayoutViewer\0location\0\0x\0y\0"
    "selected\0Selected\0show_connectivity\0"
    "addSelected\0SelectionSet\0addRuler\0x0\0"
    "y0\0x1\0y1\0focusNetsChanged\0zoomIn\0"
    "odb::Point\0focus\0do_delta_focus\0zoomOut\0"
    "zoomTo\0odb::Rect\0rect_dbu\0blockLoaded\0"
    "odb::dbBlock*\0block\0fit\0centerAt\0"
    "updateCenter\0dx\0dy\0setResolution\0"
    "pixels_per_dbu\0viewportUpdated\0"
    "fullRepaint\0getVisibleCenter\0"
    "selectHighlightConnectedInst\0select_flag\0"
    "selectHighlightConnectedNets\0output\0"
    "input\0selectHighlightConnectedBufferTrees\0"
    "highlight_group\0updateContextMenuItems\0"
    "showLayoutCustomMenu\0pos\0startRulerBuild\0"
    "cancelRulerBuild\0selectArea\0area\0"
    "append\0selection\0selectionFocus\0"
    "selectionAnimation\0repeats\0update_interval\0"
    "exit\0resetCache\0commandAboutToExecute\0"
    "commandFinishedExecuting\0executionPaused\0"
    "setBlock\0updatePixmap\0image\0bounds\0"
    "handleLoadingIndication"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__LayoutViewer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      47,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,  249,    2, 0x06 /* Public */,
       5,    2,  254,    2, 0x06 /* Public */,
       5,    1,  259,    2, 0x26 /* Public | MethodCloned */,
       8,    1,  262,    2, 0x06 /* Public */,
       8,    1,  265,    2, 0x06 /* Public */,
      10,    4,  268,    2, 0x06 /* Public */,
      15,    0,  277,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      16,    0,  278,    2, 0x0a /* Public */,
      16,    2,  279,    2, 0x0a /* Public */,
      16,    1,  284,    2, 0x2a /* Public | MethodCloned */,
      20,    0,  287,    2, 0x0a /* Public */,
      20,    2,  288,    2, 0x0a /* Public */,
      20,    1,  293,    2, 0x2a /* Public | MethodCloned */,
      21,    1,  296,    2, 0x0a /* Public */,
      24,    1,  299,    2, 0x0a /* Public */,
      27,    0,  302,    2, 0x0a /* Public */,
      28,    1,  303,    2, 0x0a /* Public */,
      29,    2,  306,    2, 0x0a /* Public */,
      32,    1,  311,    2, 0x0a /* Public */,
      34,    0,  314,    2, 0x0a /* Public */,
      35,    0,  315,    2, 0x0a /* Public */,
      36,    0,  316,    2, 0x0a /* Public */,
      37,    1,  317,    2, 0x0a /* Public */,
      39,    3,  320,    2, 0x0a /* Public */,
      42,    2,  327,    2, 0x0a /* Public */,
      42,    1,  332,    2, 0x2a /* Public | MethodCloned */,
      44,    0,  335,    2, 0x0a /* Public */,
      45,    1,  336,    2, 0x0a /* Public */,
      47,    0,  339,    2, 0x0a /* Public */,
      48,    0,  340,    2, 0x0a /* Public */,
      49,    2,  341,    2, 0x0a /* Public */,
      52,    1,  346,    2, 0x0a /* Public */,
      53,    1,  349,    2, 0x0a /* Public */,
      54,    3,  352,    2, 0x0a /* Public */,
      54,    2,  359,    2, 0x2a /* Public | MethodCloned */,
      54,    1,  364,    2, 0x2a /* Public | MethodCloned */,
      54,    2,  367,    2, 0x0a /* Public */,
      54,    1,  372,    2, 0x2a /* Public | MethodCloned */,
      54,    0,  375,    2, 0x2a /* Public | MethodCloned */,
      57,    0,  376,    2, 0x0a /* Public */,
      58,    0,  377,    2, 0x0a /* Public */,
      59,    0,  378,    2, 0x0a /* Public */,
      60,    0,  379,    2, 0x0a /* Public */,
      61,    0,  380,    2, 0x0a /* Public */,
      62,    1,  381,    2, 0x08 /* Private */,
      63,    2,  384,    2, 0x08 /* Private */,
      66,    0,  389,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    3,    4,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Bool,    5,    7,
    QMetaType::Void, 0x80000000 | 6,    5,
    QMetaType::Void, 0x80000000 | 6,    5,
    QMetaType::Void, 0x80000000 | 9,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   11,   12,   13,   14,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17, QMetaType::Bool,   18,   19,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17, QMetaType::Bool,   18,   19,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, 0x80000000 | 25,   26,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   30,   31,
    QMetaType::Void, QMetaType::QReal,   33,
    QMetaType::Void,
    QMetaType::Void,
    0x80000000 | 17,
    QMetaType::Void, QMetaType::Bool,   38,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool,   38,   40,   41,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,   38,   43,
    QMetaType::Void, QMetaType::Bool,   38,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   46,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Int, 0x80000000 | 22, QMetaType::Bool,   50,   51,
    QMetaType::Void, 0x80000000 | 6,   52,
    QMetaType::Void, 0x80000000 | 6,   18,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Int, QMetaType::Int,   52,   55,   56,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Int,   52,   55,
    QMetaType::Void, 0x80000000 | 6,   52,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   55,   56,
    QMetaType::Void, QMetaType::Int,   55,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 25,   26,
    QMetaType::Void, QMetaType::QImage, QMetaType::QRect,   64,   65,
    QMetaType::Void,

       0        // eod
};

void gui::LayoutViewer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LayoutViewer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->location((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: _t->selected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 3: _t->addSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 4: _t->addSelected((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 5: _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 6: _t->focusNetsChanged(); break;
        case 7: _t->zoomIn(); break;
        case 8: _t->zoomIn((*reinterpret_cast< const odb::Point(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 9: _t->zoomIn((*reinterpret_cast< const odb::Point(*)>(_a[1]))); break;
        case 10: _t->zoomOut(); break;
        case 11: _t->zoomOut((*reinterpret_cast< const odb::Point(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 12: _t->zoomOut((*reinterpret_cast< const odb::Point(*)>(_a[1]))); break;
        case 13: _t->zoomTo((*reinterpret_cast< const odb::Rect(*)>(_a[1]))); break;
        case 14: _t->blockLoaded((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        case 15: _t->fit(); break;
        case 16: _t->centerAt((*reinterpret_cast< const odb::Point(*)>(_a[1]))); break;
        case 17: _t->updateCenter((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 18: _t->setResolution((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 19: _t->viewportUpdated(); break;
        case 20: _t->fullRepaint(); break;
        case 21: { odb::Point _r = _t->getVisibleCenter();
            if (_a[0]) *reinterpret_cast< odb::Point*>(_a[0]) = std::move(_r); }  break;
        case 22: _t->selectHighlightConnectedInst((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 23: _t->selectHighlightConnectedNets((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 24: _t->selectHighlightConnectedBufferTrees((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 25: _t->selectHighlightConnectedBufferTrees((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 26: _t->updateContextMenuItems(); break;
        case 27: _t->showLayoutCustomMenu((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 28: _t->startRulerBuild(); break;
        case 29: _t->cancelRulerBuild(); break;
        case 30: { int _r = _t->selectArea((*reinterpret_cast< const odb::Rect(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 31: _t->selection((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 32: _t->selectionFocus((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 33: _t->selectionAnimation((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 34: _t->selectionAnimation((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 35: _t->selectionAnimation((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 36: _t->selectionAnimation((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 37: _t->selectionAnimation((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 38: _t->selectionAnimation(); break;
        case 39: _t->exit(); break;
        case 40: _t->resetCache(); break;
        case 41: _t->commandAboutToExecute(); break;
        case 42: _t->commandFinishedExecuting(); break;
        case 43: _t->executionPaused(); break;
        case 44: _t->setBlock((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        case 45: _t->updatePixmap((*reinterpret_cast< const QImage(*)>(_a[1])),(*reinterpret_cast< const QRect(*)>(_a[2]))); break;
        case 46: _t->handleLoadingIndication(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LayoutViewer::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutViewer::location)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LayoutViewer::*)(const Selected & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutViewer::selected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LayoutViewer::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutViewer::addSelected)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LayoutViewer::*)(const SelectionSet & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutViewer::addSelected)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LayoutViewer::*)(int , int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutViewer::addRuler)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LayoutViewer::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutViewer::focusNetsChanged)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::LayoutViewer::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__LayoutViewer.data,
    qt_meta_data_gui__LayoutViewer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::LayoutViewer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::LayoutViewer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__LayoutViewer.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int gui::LayoutViewer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 47)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 47;
    }
    return _id;
}

// SIGNAL 0
void gui::LayoutViewer::location(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::LayoutViewer::selected(const Selected & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 3
void gui::LayoutViewer::addSelected(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void gui::LayoutViewer::addSelected(const SelectionSet & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::LayoutViewer::addRuler(int _t1, int _t2, int _t3, int _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void gui::LayoutViewer::focusNetsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
struct qt_meta_stringdata_gui__LayoutScroll_t {
    QByteArrayData data[6];
    char stringdata0[55];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__LayoutScroll_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__LayoutScroll_t qt_meta_stringdata_gui__LayoutScroll = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::LayoutScroll"
QT_MOC_LITERAL(1, 18, 15), // "viewportChanged"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 13), // "centerChanged"
QT_MOC_LITERAL(4, 49, 2), // "dx"
QT_MOC_LITERAL(5, 52, 2) // "dy"

    },
    "gui::LayoutScroll\0viewportChanged\0\0"
    "centerChanged\0dx\0dy"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__LayoutScroll[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x06 /* Public */,
       3,    2,   25,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    4,    5,

       0        // eod
};

void gui::LayoutScroll::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LayoutScroll *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->viewportChanged(); break;
        case 1: _t->centerChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LayoutScroll::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutScroll::viewportChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LayoutScroll::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LayoutScroll::centerChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::LayoutScroll::staticMetaObject = { {
    QMetaObject::SuperData::link<QScrollArea::staticMetaObject>(),
    qt_meta_stringdata_gui__LayoutScroll.data,
    qt_meta_data_gui__LayoutScroll,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::LayoutScroll::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::LayoutScroll::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__LayoutScroll.stringdata0))
        return static_cast<void*>(this);
    return QScrollArea::qt_metacast(_clname);
}

int gui::LayoutScroll::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QScrollArea::qt_metacall(_c, _id, _a);
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
void gui::LayoutScroll::viewportChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void gui::LayoutScroll::centerChanged(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
