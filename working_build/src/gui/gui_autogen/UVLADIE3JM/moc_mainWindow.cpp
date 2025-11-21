/****************************************************************************
** Meta object code from reading C++ file 'mainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/mainWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__MainWindow_t {
    QByteArrayData data[122];
    char stringdata0[1564];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__MainWindow_t qt_meta_stringdata_gui__MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 15), // "gui::MainWindow"
QT_MOC_LITERAL(1, 16, 11), // "blockLoaded"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 13), // "odb::dbBlock*"
QT_MOC_LITERAL(4, 43, 5), // "block"
QT_MOC_LITERAL(5, 49, 4), // "exit"
QT_MOC_LITERAL(6, 54, 4), // "hide"
QT_MOC_LITERAL(7, 59, 6), // "redraw"
QT_MOC_LITERAL(8, 66, 5), // "pause"
QT_MOC_LITERAL(9, 72, 7), // "timeout"
QT_MOC_LITERAL(10, 80, 16), // "selectionChanged"
QT_MOC_LITERAL(11, 97, 8), // "Selected"
QT_MOC_LITERAL(12, 106, 9), // "selection"
QT_MOC_LITERAL(13, 116, 16), // "highlightChanged"
QT_MOC_LITERAL(14, 133, 13), // "rulersChanged"
QT_MOC_LITERAL(15, 147, 13), // "labelsChanged"
QT_MOC_LITERAL(16, 161, 19), // "displayUnitsChanged"
QT_MOC_LITERAL(17, 181, 14), // "dbu_per_micron"
QT_MOC_LITERAL(18, 196, 7), // "use_dbu"
QT_MOC_LITERAL(19, 204, 9), // "findInCts"
QT_MOC_LITERAL(20, 214, 12), // "SelectionSet"
QT_MOC_LITERAL(21, 227, 12), // "saveSettings"
QT_MOC_LITERAL(22, 240, 11), // "setLocation"
QT_MOC_LITERAL(23, 252, 1), // "x"
QT_MOC_LITERAL(24, 254, 1), // "y"
QT_MOC_LITERAL(25, 256, 20), // "updateSelectedStatus"
QT_MOC_LITERAL(26, 277, 11), // "addSelected"
QT_MOC_LITERAL(27, 289, 11), // "find_in_cts"
QT_MOC_LITERAL(28, 301, 10), // "selections"
QT_MOC_LITERAL(29, 312, 11), // "setSelected"
QT_MOC_LITERAL(30, 324, 14), // "removeSelected"
QT_MOC_LITERAL(31, 339, 20), // "removeSelectedByType"
QT_MOC_LITERAL(32, 360, 11), // "std::string"
QT_MOC_LITERAL(33, 372, 4), // "type"
QT_MOC_LITERAL(34, 377, 17), // "show_connectivity"
QT_MOC_LITERAL(35, 395, 14), // "addHighlighted"
QT_MOC_LITERAL(36, 410, 10), // "highlights"
QT_MOC_LITERAL(37, 421, 15), // "highlight_group"
QT_MOC_LITERAL(38, 437, 17), // "removeHighlighted"
QT_MOC_LITERAL(39, 455, 8), // "addLabel"
QT_MOC_LITERAL(40, 464, 4), // "text"
QT_MOC_LITERAL(41, 469, 29), // "std::optional<Painter::Color>"
QT_MOC_LITERAL(42, 499, 5), // "color"
QT_MOC_LITERAL(43, 505, 18), // "std::optional<int>"
QT_MOC_LITERAL(44, 524, 4), // "size"
QT_MOC_LITERAL(45, 529, 30), // "std::optional<Painter::Anchor>"
QT_MOC_LITERAL(46, 560, 6), // "anchor"
QT_MOC_LITERAL(47, 567, 26), // "std::optional<std::string>"
QT_MOC_LITERAL(48, 594, 4), // "name"
QT_MOC_LITERAL(49, 599, 11), // "deleteLabel"
QT_MOC_LITERAL(50, 611, 11), // "clearLabels"
QT_MOC_LITERAL(51, 623, 8), // "addRuler"
QT_MOC_LITERAL(52, 632, 2), // "x0"
QT_MOC_LITERAL(53, 635, 2), // "y0"
QT_MOC_LITERAL(54, 638, 2), // "x1"
QT_MOC_LITERAL(55, 641, 2), // "y1"
QT_MOC_LITERAL(56, 644, 5), // "label"
QT_MOC_LITERAL(57, 650, 9), // "euclidian"
QT_MOC_LITERAL(58, 660, 11), // "deleteRuler"
QT_MOC_LITERAL(59, 672, 20), // "updateHighlightedSet"
QT_MOC_LITERAL(60, 693, 22), // "QList<const Selected*>"
QT_MOC_LITERAL(61, 716, 18), // "items_to_highlight"
QT_MOC_LITERAL(62, 735, 16), // "clearHighlighted"
QT_MOC_LITERAL(63, 752, 11), // "clearRulers"
QT_MOC_LITERAL(64, 764, 18), // "removeFromSelected"
QT_MOC_LITERAL(65, 783, 5), // "items"
QT_MOC_LITERAL(66, 789, 21), // "removeFromHighlighted"
QT_MOC_LITERAL(67, 811, 6), // "zoomTo"
QT_MOC_LITERAL(68, 818, 9), // "odb::Rect"
QT_MOC_LITERAL(69, 828, 8), // "rect_dbu"
QT_MOC_LITERAL(70, 837, 13), // "zoomInToItems"
QT_MOC_LITERAL(71, 851, 6), // "status"
QT_MOC_LITERAL(72, 858, 7), // "message"
QT_MOC_LITERAL(73, 866, 14), // "showFindDialog"
QT_MOC_LITERAL(74, 881, 14), // "showGotoDialog"
QT_MOC_LITERAL(75, 896, 8), // "showHelp"
QT_MOC_LITERAL(76, 905, 16), // "addToolbarButton"
QT_MOC_LITERAL(77, 922, 6), // "script"
QT_MOC_LITERAL(78, 929, 4), // "echo"
QT_MOC_LITERAL(79, 934, 19), // "removeToolbarButton"
QT_MOC_LITERAL(80, 954, 11), // "addMenuItem"
QT_MOC_LITERAL(81, 966, 4), // "path"
QT_MOC_LITERAL(82, 971, 8), // "shortcut"
QT_MOC_LITERAL(83, 980, 14), // "removeMenuItem"
QT_MOC_LITERAL(84, 995, 16), // "requestUserInput"
QT_MOC_LITERAL(85, 1012, 5), // "title"
QT_MOC_LITERAL(86, 1018, 8), // "question"
QT_MOC_LITERAL(87, 1027, 14), // "anyObjectInSet"
QT_MOC_LITERAL(88, 1042, 13), // "selection_set"
QT_MOC_LITERAL(89, 1056, 17), // "odb::dbObjectType"
QT_MOC_LITERAL(90, 1074, 8), // "obj_type"
QT_MOC_LITERAL(91, 1083, 29), // "selectHighlightConnectedInsts"
QT_MOC_LITERAL(92, 1113, 11), // "select_flag"
QT_MOC_LITERAL(93, 1125, 28), // "selectHighlightConnectedNets"
QT_MOC_LITERAL(94, 1154, 6), // "output"
QT_MOC_LITERAL(95, 1161, 5), // "input"
QT_MOC_LITERAL(96, 1167, 35), // "selectHighlightConnectedBuffe..."
QT_MOC_LITERAL(97, 1203, 10), // "timingCone"
QT_MOC_LITERAL(98, 1214, 9), // "Gui::Term"
QT_MOC_LITERAL(99, 1224, 4), // "term"
QT_MOC_LITERAL(100, 1229, 5), // "fanin"
QT_MOC_LITERAL(101, 1235, 6), // "fanout"
QT_MOC_LITERAL(102, 1242, 18), // "timingPathsThrough"
QT_MOC_LITERAL(103, 1261, 19), // "std::set<Gui::Term>"
QT_MOC_LITERAL(104, 1281, 5), // "terms"
QT_MOC_LITERAL(105, 1287, 15), // "registerHeatMap"
QT_MOC_LITERAL(106, 1303, 18), // "HeatMapDataSource*"
QT_MOC_LITERAL(107, 1322, 7), // "heatmap"
QT_MOC_LITERAL(108, 1330, 17), // "unregisterHeatMap"
QT_MOC_LITERAL(109, 1348, 9), // "setUseDBU"
QT_MOC_LITERAL(110, 1358, 16), // "setClearLocation"
QT_MOC_LITERAL(111, 1375, 19), // "showApplicationFont"
QT_MOC_LITERAL(112, 1395, 23), // "showArrowKeysScrollStep"
QT_MOC_LITERAL(113, 1419, 17), // "showGlobalConnect"
QT_MOC_LITERAL(114, 1437, 10), // "openDesign"
QT_MOC_LITERAL(115, 1448, 10), // "saveDesign"
QT_MOC_LITERAL(116, 1459, 25), // "reportSlackHistogramPaths"
QT_MOC_LITERAL(117, 1485, 25), // "std::set<const sta::Pin*>"
QT_MOC_LITERAL(118, 1511, 11), // "report_pins"
QT_MOC_LITERAL(119, 1523, 15), // "path_group_name"
QT_MOC_LITERAL(120, 1539, 15), // "enableDeveloper"
QT_MOC_LITERAL(121, 1555, 8) // "setBlock"

    },
    "gui::MainWindow\0blockLoaded\0\0odb::dbBlock*\0"
    "block\0exit\0hide\0redraw\0pause\0timeout\0"
    "selectionChanged\0Selected\0selection\0"
    "highlightChanged\0rulersChanged\0"
    "labelsChanged\0displayUnitsChanged\0"
    "dbu_per_micron\0use_dbu\0findInCts\0"
    "SelectionSet\0saveSettings\0setLocation\0"
    "x\0y\0updateSelectedStatus\0addSelected\0"
    "find_in_cts\0selections\0setSelected\0"
    "removeSelected\0removeSelectedByType\0"
    "std::string\0type\0show_connectivity\0"
    "addHighlighted\0highlights\0highlight_group\0"
    "removeHighlighted\0addLabel\0text\0"
    "std::optional<Painter::Color>\0color\0"
    "std::optional<int>\0size\0"
    "std::optional<Painter::Anchor>\0anchor\0"
    "std::optional<std::string>\0name\0"
    "deleteLabel\0clearLabels\0addRuler\0x0\0"
    "y0\0x1\0y1\0label\0euclidian\0deleteRuler\0"
    "updateHighlightedSet\0QList<const Selected*>\0"
    "items_to_highlight\0clearHighlighted\0"
    "clearRulers\0removeFromSelected\0items\0"
    "removeFromHighlighted\0zoomTo\0odb::Rect\0"
    "rect_dbu\0zoomInToItems\0status\0message\0"
    "showFindDialog\0showGotoDialog\0showHelp\0"
    "addToolbarButton\0script\0echo\0"
    "removeToolbarButton\0addMenuItem\0path\0"
    "shortcut\0removeMenuItem\0requestUserInput\0"
    "title\0question\0anyObjectInSet\0"
    "selection_set\0odb::dbObjectType\0"
    "obj_type\0selectHighlightConnectedInsts\0"
    "select_flag\0selectHighlightConnectedNets\0"
    "output\0input\0selectHighlightConnectedBufferTrees\0"
    "timingCone\0Gui::Term\0term\0fanin\0fanout\0"
    "timingPathsThrough\0std::set<Gui::Term>\0"
    "terms\0registerHeatMap\0HeatMapDataSource*\0"
    "heatmap\0unregisterHeatMap\0setUseDBU\0"
    "setClearLocation\0showApplicationFont\0"
    "showArrowKeysScrollStep\0showGlobalConnect\0"
    "openDesign\0saveDesign\0reportSlackHistogramPaths\0"
    "std::set<const sta::Pin*>\0report_pins\0"
    "path_group_name\0enableDeveloper\0"
    "setBlock"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      80,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  414,    2, 0x06 /* Public */,
       5,    0,  417,    2, 0x06 /* Public */,
       6,    0,  418,    2, 0x06 /* Public */,
       7,    0,  419,    2, 0x06 /* Public */,
       8,    1,  420,    2, 0x06 /* Public */,
      10,    1,  423,    2, 0x06 /* Public */,
      10,    0,  426,    2, 0x26 /* Public | MethodCloned */,
      13,    0,  427,    2, 0x06 /* Public */,
      14,    0,  428,    2, 0x06 /* Public */,
      15,    0,  429,    2, 0x06 /* Public */,
      16,    2,  430,    2, 0x06 /* Public */,
      19,    1,  435,    2, 0x06 /* Public */,
      19,    1,  438,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      21,    0,  441,    2, 0x0a /* Public */,
      22,    2,  442,    2, 0x0a /* Public */,
      25,    1,  447,    2, 0x0a /* Public */,
      26,    2,  450,    2, 0x0a /* Public */,
      26,    1,  455,    2, 0x2a /* Public | MethodCloned */,
      26,    2,  458,    2, 0x0a /* Public */,
      26,    1,  463,    2, 0x2a /* Public | MethodCloned */,
      29,    1,  466,    2, 0x0a /* Public */,
      30,    1,  469,    2, 0x0a /* Public */,
      31,    1,  472,    2, 0x0a /* Public */,
      29,    2,  475,    2, 0x0a /* Public */,
      29,    1,  480,    2, 0x2a /* Public | MethodCloned */,
      35,    2,  483,    2, 0x0a /* Public */,
      35,    1,  488,    2, 0x2a /* Public | MethodCloned */,
      38,    1,  491,    2, 0x0a /* Public */,
      39,    7,  494,    2, 0x0a /* Public */,
      39,    6,  509,    2, 0x2a /* Public | MethodCloned */,
      39,    5,  522,    2, 0x2a /* Public | MethodCloned */,
      39,    4,  533,    2, 0x2a /* Public | MethodCloned */,
      39,    3,  542,    2, 0x2a /* Public | MethodCloned */,
      49,    1,  549,    2, 0x0a /* Public */,
      50,    0,  552,    2, 0x0a /* Public */,
      51,    7,  553,    2, 0x0a /* Public */,
      51,    6,  568,    2, 0x2a /* Public | MethodCloned */,
      51,    5,  581,    2, 0x2a /* Public | MethodCloned */,
      51,    4,  592,    2, 0x2a /* Public | MethodCloned */,
      58,    1,  601,    2, 0x0a /* Public */,
      59,    2,  604,    2, 0x0a /* Public */,
      59,    1,  609,    2, 0x2a /* Public | MethodCloned */,
      62,    1,  612,    2, 0x0a /* Public */,
      62,    0,  615,    2, 0x2a /* Public | MethodCloned */,
      63,    0,  616,    2, 0x0a /* Public */,
      64,    1,  617,    2, 0x0a /* Public */,
      66,    2,  620,    2, 0x0a /* Public */,
      66,    1,  625,    2, 0x2a /* Public | MethodCloned */,
      67,    1,  628,    2, 0x0a /* Public */,
      70,    1,  631,    2, 0x0a /* Public */,
      71,    1,  634,    2, 0x0a /* Public */,
      73,    0,  637,    2, 0x0a /* Public */,
      74,    0,  638,    2, 0x0a /* Public */,
      75,    0,  639,    2, 0x0a /* Public */,
      76,    4,  640,    2, 0x0a /* Public */,
      79,    1,  649,    2, 0x0a /* Public */,
      80,    6,  652,    2, 0x0a /* Public */,
      83,    1,  665,    2, 0x0a /* Public */,
      84,    2,  668,    2, 0x0a /* Public */,
      87,    2,  673,    2, 0x0a /* Public */,
      91,    2,  678,    2, 0x0a /* Public */,
      91,    1,  683,    2, 0x2a /* Public | MethodCloned */,
      93,    4,  686,    2, 0x0a /* Public */,
      93,    3,  695,    2, 0x2a /* Public | MethodCloned */,
      96,    2,  702,    2, 0x0a /* Public */,
      96,    1,  707,    2, 0x2a /* Public | MethodCloned */,
      97,    3,  710,    2, 0x0a /* Public */,
     102,    1,  717,    2, 0x0a /* Public */,
     105,    1,  720,    2, 0x0a /* Public */,
     108,    1,  723,    2, 0x0a /* Public */,
     109,    1,  726,    2, 0x08 /* Private */,
     110,    0,  729,    2, 0x08 /* Private */,
     111,    0,  730,    2, 0x08 /* Private */,
     112,    0,  731,    2, 0x08 /* Private */,
     113,    0,  732,    2, 0x08 /* Private */,
     114,    0,  733,    2, 0x08 /* Private */,
     115,    0,  734,    2, 0x08 /* Private */,
     116,    2,  735,    2, 0x08 /* Private */,
     120,    0,  740,    2, 0x08 /* Private */,
     121,    1,  741,    2, 0x08 /* Private */,

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
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,   17,   18,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 20,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   23,   24,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 11, QMetaType::Bool,   12,   27,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 20, QMetaType::Bool,   28,   27,
    QMetaType::Void, 0x80000000 | 20,   28,
    QMetaType::Void, 0x80000000 | 20,   28,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 32,   33,
    QMetaType::Void, 0x80000000 | 11, QMetaType::Bool,   12,   34,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, 0x80000000 | 20, QMetaType::Int,   36,   37,
    QMetaType::Void, 0x80000000 | 20,   36,
    QMetaType::Void, 0x80000000 | 11,   12,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, 0x80000000 | 32, 0x80000000 | 41, 0x80000000 | 43, 0x80000000 | 45, 0x80000000 | 47,   23,   24,   40,   42,   44,   46,   48,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, 0x80000000 | 32, 0x80000000 | 41, 0x80000000 | 43, 0x80000000 | 45,   23,   24,   40,   42,   44,   46,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, 0x80000000 | 32, 0x80000000 | 41, 0x80000000 | 43,   23,   24,   40,   42,   44,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, 0x80000000 | 32, 0x80000000 | 41,   23,   24,   40,   42,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, 0x80000000 | 32,   23,   24,   40,
    QMetaType::Void, 0x80000000 | 32,   48,
    QMetaType::Void,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 32, 0x80000000 | 32, QMetaType::Bool,   52,   53,   54,   55,   56,   48,   57,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 32, 0x80000000 | 32,   52,   53,   54,   55,   56,   48,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 32,   52,   53,   54,   55,   56,
    0x80000000 | 32, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   52,   53,   54,   55,
    QMetaType::Void, 0x80000000 | 32,   48,
    QMetaType::Void, 0x80000000 | 60, QMetaType::Int,   61,   37,
    QMetaType::Void, 0x80000000 | 60,   61,
    QMetaType::Void, QMetaType::Int,   37,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 60,   65,
    QMetaType::Void, 0x80000000 | 60, QMetaType::Int,   65,   37,
    QMetaType::Void, 0x80000000 | 60,   65,
    QMetaType::Void, 0x80000000 | 68,   69,
    QMetaType::Void, 0x80000000 | 60,   65,
    QMetaType::Void, 0x80000000 | 32,   72,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    0x80000000 | 32, 0x80000000 | 32, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   48,   40,   77,   78,
    QMetaType::Void, 0x80000000 | 32,   48,
    0x80000000 | 32, 0x80000000 | 32, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   48,   81,   40,   77,   82,   78,
    QMetaType::Void, 0x80000000 | 32,   48,
    0x80000000 | 32, QMetaType::QString, QMetaType::QString,   85,   86,
    QMetaType::Bool, QMetaType::Bool, 0x80000000 | 89,   88,   90,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,   92,   37,
    QMetaType::Void, QMetaType::Bool,   92,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool, QMetaType::Int,   92,   94,   95,   37,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool,   92,   94,   95,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,   92,   37,
    QMetaType::Void, QMetaType::Bool,   92,
    QMetaType::Void, 0x80000000 | 98, QMetaType::Bool, QMetaType::Bool,   99,  100,  101,
    QMetaType::Void, 0x80000000 | 103,  104,
    QMetaType::Void, 0x80000000 | 106,  107,
    QMetaType::Void, 0x80000000 | 106,  107,
    QMetaType::Void, QMetaType::Bool,   18,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 117, 0x80000000 | 32,  118,  119,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,    4,

       0        // eod
};

void gui::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->blockLoaded((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        case 1: _t->exit(); break;
        case 2: _t->hide(); break;
        case 3: _t->redraw(); break;
        case 4: _t->pause((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->selectionChanged((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 6: _t->selectionChanged(); break;
        case 7: _t->highlightChanged(); break;
        case 8: _t->rulersChanged(); break;
        case 9: _t->labelsChanged(); break;
        case 10: _t->displayUnitsChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 11: _t->findInCts((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 12: _t->findInCts((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 13: _t->saveSettings(); break;
        case 14: _t->setLocation((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 15: _t->updateSelectedStatus((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 16: _t->addSelected((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 17: _t->addSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 18: _t->addSelected((*reinterpret_cast< const SelectionSet(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 19: _t->addSelected((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 20: _t->setSelected((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 21: _t->removeSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 22: _t->removeSelectedByType((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 23: _t->setSelected((*reinterpret_cast< const Selected(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 24: _t->setSelected((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 25: _t->addHighlighted((*reinterpret_cast< const SelectionSet(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 26: _t->addHighlighted((*reinterpret_cast< const SelectionSet(*)>(_a[1]))); break;
        case 27: _t->removeHighlighted((*reinterpret_cast< const Selected(*)>(_a[1]))); break;
        case 28: { std::string _r = _t->addLabel((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const std::string(*)>(_a[3])),(*reinterpret_cast< std::optional<Painter::Color>(*)>(_a[4])),(*reinterpret_cast< std::optional<int>(*)>(_a[5])),(*reinterpret_cast< std::optional<Painter::Anchor>(*)>(_a[6])),(*reinterpret_cast< std::optional<std::string>(*)>(_a[7])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 29: { std::string _r = _t->addLabel((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const std::string(*)>(_a[3])),(*reinterpret_cast< std::optional<Painter::Color>(*)>(_a[4])),(*reinterpret_cast< std::optional<int>(*)>(_a[5])),(*reinterpret_cast< std::optional<Painter::Anchor>(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 30: { std::string _r = _t->addLabel((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const std::string(*)>(_a[3])),(*reinterpret_cast< std::optional<Painter::Color>(*)>(_a[4])),(*reinterpret_cast< std::optional<int>(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 31: { std::string _r = _t->addLabel((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const std::string(*)>(_a[3])),(*reinterpret_cast< std::optional<Painter::Color>(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 32: { std::string _r = _t->addLabel((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const std::string(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 33: _t->deleteLabel((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 34: _t->clearLabels(); break;
        case 35: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< const std::string(*)>(_a[5])),(*reinterpret_cast< const std::string(*)>(_a[6])),(*reinterpret_cast< bool(*)>(_a[7])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 36: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< const std::string(*)>(_a[5])),(*reinterpret_cast< const std::string(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 37: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< const std::string(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 38: { std::string _r = _t->addRuler((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 39: _t->deleteRuler((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 40: _t->updateHighlightedSet((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 41: _t->updateHighlightedSet((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 42: _t->clearHighlighted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 43: _t->clearHighlighted(); break;
        case 44: _t->clearRulers(); break;
        case 45: _t->removeFromSelected((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 46: _t->removeFromHighlighted((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 47: _t->removeFromHighlighted((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 48: _t->zoomTo((*reinterpret_cast< const odb::Rect(*)>(_a[1]))); break;
        case 49: _t->zoomInToItems((*reinterpret_cast< const QList<const Selected*>(*)>(_a[1]))); break;
        case 50: _t->status((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 51: _t->showFindDialog(); break;
        case 52: _t->showGotoDialog(); break;
        case 53: _t->showHelp(); break;
        case 54: { std::string _r = _t->addToolbarButton((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 55: _t->removeToolbarButton((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 56: { std::string _r = _t->addMenuItem((*reinterpret_cast< const std::string(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const QString(*)>(_a[5])),(*reinterpret_cast< bool(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 57: _t->removeMenuItem((*reinterpret_cast< const std::string(*)>(_a[1]))); break;
        case 58: { std::string _r = _t->requestUserInput((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< std::string*>(_a[0]) = std::move(_r); }  break;
        case 59: { bool _r = _t->anyObjectInSet((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< odb::dbObjectType(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 60: _t->selectHighlightConnectedInsts((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 61: _t->selectHighlightConnectedInsts((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 62: _t->selectHighlightConnectedNets((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 63: _t->selectHighlightConnectedNets((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 64: _t->selectHighlightConnectedBufferTrees((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 65: _t->selectHighlightConnectedBufferTrees((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 66: _t->timingCone((*reinterpret_cast< Gui::Term(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 67: _t->timingPathsThrough((*reinterpret_cast< const std::set<Gui::Term>(*)>(_a[1]))); break;
        case 68: _t->registerHeatMap((*reinterpret_cast< HeatMapDataSource*(*)>(_a[1]))); break;
        case 69: _t->unregisterHeatMap((*reinterpret_cast< HeatMapDataSource*(*)>(_a[1]))); break;
        case 70: _t->setUseDBU((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 71: _t->setClearLocation(); break;
        case 72: _t->showApplicationFont(); break;
        case 73: _t->showArrowKeysScrollStep(); break;
        case 74: _t->showGlobalConnect(); break;
        case 75: _t->openDesign(); break;
        case 76: _t->saveDesign(); break;
        case 77: _t->reportSlackHistogramPaths((*reinterpret_cast< const std::set<const sta::Pin*>(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 78: _t->enableDeveloper(); break;
        case 79: _t->setBlock((*reinterpret_cast< odb::dbBlock*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MainWindow::*)(odb::dbBlock * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::blockLoaded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::exit)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::hide)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::redraw)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::pause)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::selectionChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::highlightChanged)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::rulersChanged)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::labelsChanged)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(int , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::displayUnitsChanged)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(const Selected & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::findInCts)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(const SelectionSet & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::findInCts)) {
                *result = 12;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_gui__MainWindow.data,
    qt_meta_data_gui__MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__MainWindow.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "odb::dbDatabaseObserver"))
        return static_cast< odb::dbDatabaseObserver*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int gui::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 80)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 80;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 80)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 80;
    }
    return _id;
}

// SIGNAL 0
void gui::MainWindow::blockLoaded(odb::dbBlock * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
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
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void gui::MainWindow::selectionChanged(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
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

// SIGNAL 9
void gui::MainWindow::labelsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void gui::MainWindow::displayUnitsChanged(int _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void gui::MainWindow::findInCts(const Selected & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void gui::MainWindow::findInCts(const SelectionSet & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
