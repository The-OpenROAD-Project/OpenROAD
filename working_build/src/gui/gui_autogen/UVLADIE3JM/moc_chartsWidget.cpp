/****************************************************************************
** Meta object code from reading C++ file 'chartsWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/gui/src/chartsWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chartsWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_gui__HistogramView_t {
    QByteArrayData data[10];
    char stringdata0[142];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__HistogramView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__HistogramView_t qt_meta_stringdata_gui__HistogramView = {
    {
QT_MOC_LITERAL(0, 0, 18), // "gui::HistogramView"
QT_MOC_LITERAL(1, 19, 17), // "endPointsToReport"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 25), // "std::set<const sta::Pin*>"
QT_MOC_LITERAL(4, 64, 11), // "report_pins"
QT_MOC_LITERAL(5, 76, 9), // "saveImage"
QT_MOC_LITERAL(6, 86, 11), // "showToolTip"
QT_MOC_LITERAL(7, 98, 11), // "is_hovering"
QT_MOC_LITERAL(8, 110, 9), // "bar_index"
QT_MOC_LITERAL(9, 120, 21) // "emitEndPointsInBucket"

    },
    "gui::HistogramView\0endPointsToReport\0"
    "\0std::set<const sta::Pin*>\0report_pins\0"
    "saveImage\0showToolTip\0is_hovering\0"
    "bar_index\0emitEndPointsInBucket"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__HistogramView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   37,    2, 0x0a /* Public */,
       6,    2,   38,    2, 0x08 /* Private */,
       9,    1,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,    7,    8,
    QMetaType::Void, QMetaType::Int,    8,

       0        // eod
};

void gui::HistogramView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HistogramView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->endPointsToReport((*reinterpret_cast< const std::set<const sta::Pin*>(*)>(_a[1]))); break;
        case 1: _t->saveImage(); break;
        case 2: _t->showToolTip((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->emitEndPointsInBucket((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (HistogramView::*)(const std::set<const sta::Pin*> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HistogramView::endPointsToReport)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::HistogramView::staticMetaObject = { {
    QMetaObject::SuperData::link<QChartView::staticMetaObject>(),
    qt_meta_stringdata_gui__HistogramView.data,
    qt_meta_data_gui__HistogramView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::HistogramView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::HistogramView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__HistogramView.stringdata0))
        return static_cast<void*>(this);
    return QChartView::qt_metacast(_clname);
}

int gui::HistogramView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QChartView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void gui::HistogramView::endPointsToReport(const std::set<const sta::Pin*> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_gui__ChartsWidget_t {
    QByteArrayData data[11];
    char stringdata0[179];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_gui__ChartsWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_gui__ChartsWidget_t qt_meta_stringdata_gui__ChartsWidget = {
    {
QT_MOC_LITERAL(0, 0, 17), // "gui::ChartsWidget"
QT_MOC_LITERAL(1, 18, 17), // "endPointsToReport"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 25), // "std::set<const sta::Pin*>"
QT_MOC_LITERAL(4, 63, 11), // "report_pins"
QT_MOC_LITERAL(5, 75, 11), // "std::string"
QT_MOC_LITERAL(6, 87, 15), // "path_group_name"
QT_MOC_LITERAL(7, 103, 15), // "reportEndPoints"
QT_MOC_LITERAL(8, 119, 10), // "changeMode"
QT_MOC_LITERAL(9, 130, 26), // "updatePathGroupMenuIndexes"
QT_MOC_LITERAL(10, 157, 21) // "changePathGroupFilter"

    },
    "gui::ChartsWidget\0endPointsToReport\0"
    "\0std::set<const sta::Pin*>\0report_pins\0"
    "std::string\0path_group_name\0reportEndPoints\0"
    "changeMode\0updatePathGroupMenuIndexes\0"
    "changePathGroupFilter"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_gui__ChartsWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   44,    2, 0x0a /* Public */,
       8,    0,   47,    2, 0x08 /* Private */,
       9,    0,   48,    2, 0x08 /* Private */,
      10,    0,   49,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void gui::ChartsWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartsWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->endPointsToReport((*reinterpret_cast< const std::set<const sta::Pin*>(*)>(_a[1])),(*reinterpret_cast< const std::string(*)>(_a[2]))); break;
        case 1: _t->reportEndPoints((*reinterpret_cast< const std::set<const sta::Pin*>(*)>(_a[1]))); break;
        case 2: _t->changeMode(); break;
        case 3: _t->updatePathGroupMenuIndexes(); break;
        case 4: _t->changePathGroupFilter(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartsWidget::*)(const std::set<const sta::Pin*> & , const std::string & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartsWidget::endPointsToReport)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject gui::ChartsWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_gui__ChartsWidget.data,
    qt_meta_data_gui__ChartsWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *gui::ChartsWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *gui::ChartsWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_gui__ChartsWidget.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int gui::ChartsWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void gui::ChartsWidget::endPointsToReport(const std::set<const sta::Pin*> & _t1, const std::string & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
