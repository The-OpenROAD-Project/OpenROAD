/****************************************************************************
** Meta object code from reading C++ file 'mainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../src/mainWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__MainWindow_t {
    QByteArrayData data[93];
    char stringdata0[1092];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__MainWindow_t qt_meta_stringdata_gui__MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 15), // "gui::MainWindow"
QT_MOC_LITERAL(1, 16, 12), // "designLoaded"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 13), // "odb::dbBlock*"
QT_MOC_LITERAL(4, 44, 5), // "block"
QT_MOC_LITERAL(5, 50, 4), // "exit"
QT_MOC_LITERAL(6, 55, 4), // "hide"
QT_MOC_LITERAL(7, 60, 6), // "redraw"
QT_MOC_LITERAL(8, 67, 5), // "pause"
QT_MOC_LITERAL(9, 73, 7), // "timeout"
QT_MOC_LITERAL(10, 81, 16), // "selectionChanged"
QT_MOC_LITERAL(11, 98, 8), // "Selected"
QT_MOC_LITERAL(12, 107, 9), // "selection"
QT_MOC_LITERAL(13, 117, 16), // "highlightChanged"
QT_MOC_LITERAL(14, 134, 13), // "rulersChanged"
QT_MOC_LITERAL(15, 148, 12), // "saveSettings"
QT_MOC_LITERAL(16, 161, 11), // "setLocation"
QT_MOC_LITERAL(17, 173, 1), // "x"
QT_MOC_LITERAL(18, 175, 1), // "y"
QT_MOC_LITERAL(19, 177, 20), // "updateSelectedStatus"
QT_MOC_LITERAL(20, 198, 11), // "addSelected"
QT_MOC_LITERAL(21, 210, 12), // "SelectionSet"
QT_MOC_LITERAL(22, 223, 10), // "selections"
QT_MOC_LITERAL(23, 234, 14), // "removeSelected"
QT_MOC_LITERAL(24, 249, 20), // "removeSelectedByType"
QT_MOC_LITERAL(25, 270, 11), // "std::string"
QT_MOC_LITERAL(26, 282, 4), // "type"
QT_MOC_LITERAL(27, 287, 11), // "setSelected"
QT_MOC_LITERAL(28, 299, 17), // "show_connectivity"
QT_MOC_LITERAL(29, 317, 14), // "addHighlighted"
QT_MOC_LITERAL(30, 332, 15), // "highlight_group"
QT_MOC_LITERAL(31, 348, 8), // "addRuler"
QT_MOC_LITERAL(32, 357, 2), // "x0"
QT_MOC_LITERAL(33, 360, 2), // "y0"
QT_MOC_LITERAL(34, 363, 2), // "x1"
QT_MOC_LITERAL(35, 366, 2), // "y1"
QT_MOC_LITERAL(36, 369, 5), // "label"
QT_MOC_LITERAL(37, 375, 4), // "name"
QT_MOC_LITERAL(38, 380, 11), // "deleteRuler"
QT_MOC_LITERAL(39, 392, 20), // "updateHighlightedSet"
QT_MOC_LITERAL(40, 413, 22), // "QList<const Selected*>"
QT_MOC_LITERAL(41, 436, 18), // "items_to_highlight"
QT_MOC_LITERAL(42, 455, 16), // "clearHighlighted"
QT_MOC_LITERAL(43, 472, 11), // "clearRulers"
QT_MOC_LITERAL(44, 484, 18), // "removeFromSelected"
QT_MOC_LITERAL(45, 503, 5), // "items"
QT_MOC_LITERAL(46, 509, 21), // "removeFromHighlighted"
QT_MOC_LITERAL(47, 531, 6), // "zoomTo"
QT_MOC_LITERAL(48, 538, 9), // "odb::Rect"
QT_MOC_LITERAL(49, 548, 8), // "rect_dbu"
QT_MOC_LITERAL(50, 557, 13), // "zoomInToItems"
QT_MOC_LITERAL(51, 571, 6), // "status"
QT_MOC_LITERAL(52, 578, 7), // "message"
QT_MOC_LITERAL(53, 586, 14), // "showFindDialog"
QT_MOC_LITERAL(54, 601, 8), // "showHelp"
QT_MOC_LITERAL(55, 610, 16), // "addToolbarButton"
QT_MOC_LITERAL(56, 627, 4), // "text"
QT_MOC_LITERAL(57, 632, 6), // "script"
QT_MOC_LITERAL(58, 639, 4), // "echo"
QT_MOC_LITERAL(59, 644, 19), // "removeToolbarButton"
QT_MOC_LITERAL(60, 664, 11), // "addMenuItem"
QT_MOC_LITERAL(61, 676, 4), // "path"
QT_MOC_LITERAL(62, 681, 8), // "shortcut"
QT_MOC_LITERAL(63, 690, 14), // "removeMenuItem"
QT_MOC_LITERAL(64, 705, 16), // "requestUserInput"
QT_MOC_LITERAL(65, 722, 5), // "title"
QT_MOC_LITERAL(66, 728, 8), // "question"
QT_MOC_LITERAL(67, 737, 14), // "anyObjectInSet"
QT_MOC_LITERAL(68, 752, 13), // "selection_set"
QT_MOC_LITERAL(69, 766, 17), // "odb::dbObjectType"
QT_MOC_LITERAL(70, 784, 8), // "obj_type"
QT_MOC_LITERAL(71, 793, 29), // "selectHighlightConnectedInsts"
QT_MOC_LITERAL(72, 823, 11), // "select_flag"
QT_MOC_LITERAL(73, 835, 28), // "selectHighlightConnectedNets"
QT_MOC_LITERAL(74, 864, 6), // "output"
QT_MOC_LITERAL(75, 871, 5), // "input"
QT_MOC_LITERAL(76, 877, 10), // "timingCone"
QT_MOC_LITERAL(77, 888, 12), // "Gui::odbTerm"
QT_MOC_LITERAL(78, 901, 4), // "term"
QT_MOC_LITERAL(79, 906, 5), // "fanin"
QT_MOC_LITERAL(80, 912, 6), // "fanout"
QT_MOC_LITERAL(81, 919, 18), // "timingPathsThrough"
QT_MOC_LITERAL(82, 938, 22), // "std::set<Gui::odbTerm>"
QT_MOC_LITERAL(83, 961, 5), // "terms"
QT_MOC_LITERAL(84, 967, 15), // "registerHeatMap"
QT_MOC_LITERAL(85, 983, 18), // "HeatMapDataSource*"
QT_MOC_LITERAL(86, 1002, 7), // "heatmap"
QT_MOC_LITERAL(87, 1010, 17), // "unregisterHeatMap"
QT_MOC_LITERAL(88, 1028, 9), // "setUseDBU"
QT_MOC_LITERAL(89, 1038, 7), // "use_dbu"
QT_MOC_LITERAL(90, 1046, 16), // "setClearLocation"
QT_MOC_LITERAL(91, 1063, 19), // "showApplicationFont"
QT_MOC_LITERAL(92, 1083, 8) // "setBlock"

    },
    "gui::MainWindow\0designLoaded\0\0"
    "odb::dbBlock*\0block\0exit\0hide\0redraw\0"
    "pause\0timeout\0selectionChanged\0Selected\0"
    "selection\0highlightChanged\0rulersChanged\0"
    "saveSettings\0setLocation\0x\0y\0"
    "updateSelectedStatus\0addSelected\0"
    "SelectionSet\0selections\0removeSelected\0"
    "removeSelectedByType\0std::string\0type\0"
    "setSelected\0show_connectivity\0"
    "addHighlighted\0highlight_group\0addRuler\0"
    "x0\0y0\0x1\0y1\0label\0name\0deleteRuler\0"
    "updateHighlightedSet\0QList<const Selected*>\0"
    "items_to_highlight\0clearHighlighted\0"
    "clearRulers\0removeFromSelected\0items\0"
    "removeFromHighlighted\0zoomTo\0odb::Rect\0"
    "rect_dbu\0zoomInToItems\0status\0message\0"
    "showFindDialog\0showHelp\0addToolbarButton\0"
    "text\0script\0echo\0removeToolbarButton\0"
    "addMenuItem\0path\0shortcut\0removeMenuItem\0"
    "requestUserInput\0title\0question\0"
    "anyObjectInSet\0selection_set\0"
    "odb::dbObjectType\0obj_type\0"
    "selectHighlightConnectedInsts\0select_flag\0"
    "selectHighlightConnectedNets\0output\0"
    "input\0timingCone\0Gui::odbTerm\0term\0"
    "fanin\0fanout\0timingPathsThrough\0"
    "std::set<Gui::odbTerm>\0terms\0"
    "registerHeatMap\0HeatMapDataSource*\0"
    "heatmap\0unregisterHeatMap\0setUseDBU\0"
    "use_dbu\0setClearLocation\0showApplicationFont\0"
    "setBlock"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__MainWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      55,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  289,    2, 0x06 /* Public */,
       5,    0,  292,    2, 0x06 /* Public */,
       6,    0,  293,    2, 0x06 /* Public */,
       7,    0,  294,    2, 0x06 /* Public */,
       8,    1,  295,    2, 0x06 /* Public */,
      10,    1,  298,    2, 0x06 /* Public */,
      10,    0,  301,    2, 0x26 /* Public | MethodCloned */,
      13,    0,  302,    2, 0x06 /* Public */,
      14,    0,  303,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    0,  304,    2, 0x0a /* Public */,
      16,    2,  305,    2, 0x0a /* Public */,
      19,    1,  310,    2, 0x0a /* Public */,
      20,    1,  313,    2, 0x0a /* Public */,
      20,    1,  316,    2, 0x0a /* Public */,
      23,    1,  319,    2, 0x0a /* Public */,
      24,    1,  322,    2, 0x0a /* Public */,
      27,    2,  325,    2, 0x0a /* Public */,
      27,    1,  330,    2, 0x2a /* Public | MethodCloned */,
      29,    2,  333,    2, 0x0a /* Public */,
      29,    1,  338,    2, 0x2a /* Public | MethodCloned */,
      31,    6,  341,    2, 0x0a /* Public */,
      31,    5,  354,    2, 0x2a /* Public | MethodCloned */,
      31,    4,  365,    2, 0x2a /* Public | MethodCloned */,
      38,    1,  374,    2, 0x0a /* Public */,
      39,    2,  377,    2, 0x0a /* Public */,
      39,    1,  382,    2, 0x2a /* Public | MethodCloned */,
      42,    1,  385,    2, 0x0a /* Public */,
      42,    0,  388,    2, 0x2a /* Public | MethodCloned */,
      43,    0,  389,    2, 0x0a /* Public */,
      44,    1,  390,    2, 0x0a /* Public */,
      46,    2,  393,    2, 0x0a /* Public */,
      46,    1,  398,    2, 0x2a /* Public | MethodCloned */,
      47,    1,  401,    2, 0x0a /* Public */,
      50,    1,  404,    2, 0x0a /* Public */,
      51,    1,  407,    2, 0x0a /* Public */,
      53,    0,  410,    2, 0x0a /* Public */,
      54,    0,  411,    2, 0x0a /* Public */,
      55,    4,  412,    2, 0x0a /* Public */,
      59,    1,  421,    2, 0x0a /* Public */,
      60,    6,  424,    2, 0x0a /* Public */,
      63,    1,  437,    2, 0x0a /* Public */,
      64,    2,  440,    2, 0x0a /* Public */,
      67,    2,  445,    2, 0x0a /* Public */,
      71,    2,  450,    2, 0x0a /* Public */,
      71,    1,  455,    2, 0x2a /* Public | MethodCloned */,
      73,    4,  458,    2, 0x0a /* Public */,
      73,    3,  467,    2, 0x2a /* Public | MethodCloned */,
      76,    3,  474,    2, 0x0a /* Public */,
      81,    1,  481,    2, 0x0a /* Public */,
      84,    1,  484,    2, 0x0a /* Public */,
      87,    1,  487,    2, 0x0a /* Public */,
      88,    1,  490,    2, 0x08 /* Private */,
      90,    0,  493,    2, 0x08 /* Private */,
      91,    0,  494,    2, 0x08 /* Private */,
      92,    1,  495,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   17,   18,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 21,   22,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 25,   26,
    QMetaType::Void, 0x80000000 | 11, QMetaType::Bool,   12,   28,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 21, QMetaType::Int,   12,   30,
    QMetaType::Void, 0x80000000 | 21,   12,
    0x80000000 | 25, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 25, 0x80000000 | 25,   32,   33,   34,   35,   36,   37,
    0x80000000 | 25, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 25,   32,   33,   34,   35,   36,
    0x80000000 | 25, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   32,   33,   34,   35,
    QMetaType::Void, 0x80000000 | 25,   37,
    QMetaType::Void, 0x80000000 | 40, QMetaType::Int,   41,   30,
    QMetaType::Void, 0x80000000 | 40,   41,
    QMetaType::Void, QMetaType::Int,   30,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 40,   45,
    QMetaType::Void, 0x80000000 | 40, QMetaType::Int,   45,   30,
    QMetaType::Void, 0x80000000 | 40,   45,
    QMetaType::Void, 0x80000000 | 48,   49,
    QMetaType::Void, 0x80000000 | 40,   45,
    QMetaType::Void, 0x80000000 | 25,   52,
    QMetaType::Void,
    QMetaType::Void,
    0x80000000 | 25, 0x80000000 | 25, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   37,   56,   57,   58,
    QMetaType::Void, 0x80000000 | 25,   37,
    0x80000000 | 25, 0x80000000 | 25, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   37,   61,   56,   57,   62,   58,
    QMetaType::Void, 0x80000000 | 25,   37,
    0x80000000 | 25, QMetaType::QString, QMetaType::QString,   65,   66,
    QMetaType::Bool, QMetaType::Bool, 0x80000000 | 69,   68,   70,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,   72,   30,
    QMetaType::Void, QMetaType::Bool,   72,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool, QMetaType::Int,   72,   74,   75,   30,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool,   72,   74,   75,
    QMetaType::Void, 0x80000000 | 77, QMetaType::Bool, QMetaType::Bool,   78,   79,   80,
    QMetaType::Void, 0x80000000 | 82,   83,
    QMetaType::Void, 0x80000000 | 85,   86,
    QMetaType::Void, 0x80000000 | 85,   86,
    QMetaType::Void, QMetaType::Bool,   89,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,    4,

       0        // eod
};

void gui::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        MainWindow *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->designLoaded((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        case 1: _t->exit(); break;
        case 2: _t->hide(); break;
        case 3: _t->redraw(); break;
        case 4: _t->pause((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->selectionChanged((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 6: _t->selectionChanged(); break;
        case 7: _t->highlightChanged(); break;
        case 8: _t->rulersChanged(); break;
        case 9: _t->saveSettings(); break;
        case 10: _t->setLocation((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 11: _t->updateSelectedStatus((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 12: _t->addSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 13: _t->addSelected((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 14: _t->removeSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 15: _t->removeSelectedByType((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 16: _t->setSelected((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 17: _t->setSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 18: _t->addHighlighted((*reinterpret_cast< const SelectionSet(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 19: _t->addHighlighted((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 20: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< const std::string(*)>(_a[5])),(*reinterpret_cast< const std::string(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 21: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< const std::string(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 22: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 23: _t->deleteRuler((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 24: _t->updateHighlightedSet((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 25: _t->updateHighlightedSet((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 26: _t->clearHighlighted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 27: _t->clearHighlighted(); break;
        case 28: _t->clearRulers(); break;
        case 29: _t->removeFromSelected((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 30: _t->removeFromHighlighted((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 31: _t->removeFromHighlighted((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 32: _t->zoomTo((*reinterpret_cast< const odb::Rect(*)>(_a[1]))); break;
        case 33: _t->zoomInToItems((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 34: _t->status((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 35: _t->showFindDialog(); break;
        case 36: _t->showHelp(); break;
        case 37: { std::string _r = _t->addToolbarButton((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 38: _t->removeToolbarButton((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 39: { std::string _r = _t->addMenuItem((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const QString(*)>(_a[5])),(*reinterpret_cast< bool(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 40: _t->removeMenuItem((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 41: { std::string _r = _t->requestUserInput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 42: { bool _r = _t->anyObjectInSet((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< odb::dbObjectType(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 43: _t->selectHighlightConnectedInsts((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 44: _t->selectHighlightConnectedInsts((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 45: _t->selectHighlightConnectedNets((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 46: _t->selectHighlightConnectedNets((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 47: _t->timingCone((*reinterpret_cast< Gui::odbTerm(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 48: _t->timingPathsThrough((*reinterpret_cast< const std::set<Gui::odbTerm>(*)>(_a[1]))); break;
        case 49: _t->registerHeatMap((*reinterpret_cast< HeatMapDataSource*(*)>(_a[1]))); break;
        case 50: _t->unregisterHeatMap((*reinterpret_cast< HeatMapDataSource*(*)>(_a[1]))); break;
        case 51: _t->setUseDBU((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 52: _t->setClearLocation(); break;
        case 53: _t->showApplicationFont(); break;
        case 54: _t->setBlock((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            typedef void (MainWindow::*_t)(odb::dbBlock * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::designLoaded)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::exit)) {
                *result = 1;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::hide)) {
                *result = 2;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::redraw)) {
                *result = 3;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::pause)) {
                *result = 4;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::selectionChanged)) {
                *result = 5;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::highlightChanged)) {
                *result = 7;
                return;
            }
        }
        {
            typedef void (MainWindow::*_t)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::rulersChanged)) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject gui::MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_gui__MainWindow.data,
      qt_meta_data_gui__MainWindow,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *gui::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__MainWindow.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "ord::OpenRoad::Observer"))
        return static_cast< ord::OpenRoad::Observer*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int gui::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 55)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 55;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 55)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 55;
    }
    return _id;
}

// SIGNAL 0
void gui::MainWindow::designLoaded(odb::dbBlock * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui::MainWindow::exit()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void gui::MainWindow::hide()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void gui::MainWindow::redraw()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void gui::MainWindow::pause(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::MainWindow::selectionChanged(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 7
void gui::MainWindow::highlightChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void gui::MainWindow::rulersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
