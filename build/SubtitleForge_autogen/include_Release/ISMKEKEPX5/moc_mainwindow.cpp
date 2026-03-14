/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/app/mainwindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "onNewProject",
        "",
        "onOpenProject",
        "onSaveProject",
        "onSaveProjectAs",
        "onImportVideo",
        "onImportTxt",
        "onExportVideo",
        "onExportPremiereXml",
        "onFrameChanged",
        "frame",
        "onPlayPause",
        "onStepForward",
        "onStepBackward",
        "onStepForwardLarge",
        "onStepBackwardLarge",
        "onSubtitleSelected",
        "id",
        "onSubtitleDeselected",
        "onSubtitleMoved",
        "newKeyFrame",
        "onDeleteSubtitle",
        "onGlobalSubtitleFontChanged",
        "onGlobalSubtitleColorChanged",
        "onVideoTitleChanged",
        "onPreviewFrameDecoded",
        "frameIndex",
        "QImage",
        "image",
        "onSetTrimStart",
        "onSetTrimEnd",
        "onAudioPositionChanged",
        "onAudioDurationChanged",
        "ms",
        "onRefreshTick"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onNewProject'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onOpenProject'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSaveProject'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSaveProjectAs'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onImportVideo'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onImportTxt'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportVideo'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportPremiereXml'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFrameChanged'
        QtMocHelpers::SlotData<void(qint64)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 11 },
        }}),
        // Slot 'onPlayPause'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepForward'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepBackward'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepForwardLarge'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepBackwardLarge'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSubtitleSelected'
        QtMocHelpers::SlotData<void(int)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Slot 'onSubtitleDeselected'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSubtitleMoved'
        QtMocHelpers::SlotData<void(int, qint64)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 }, { QMetaType::LongLong, 21 },
        }}),
        // Slot 'onDeleteSubtitle'
        QtMocHelpers::SlotData<void(int)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Slot 'onGlobalSubtitleFontChanged'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGlobalSubtitleColorChanged'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onVideoTitleChanged'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPreviewFrameDecoded'
        QtMocHelpers::SlotData<void(qint64, QImage)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 27 }, { 0x80000000 | 28, 29 },
        }}),
        // Slot 'onSetTrimStart'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSetTrimEnd'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAudioPositionChanged'
        QtMocHelpers::SlotData<void(qint64)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 11 },
        }}),
        // Slot 'onAudioDurationChanged'
        QtMocHelpers::SlotData<void(qint64)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 34 },
        }}),
        // Slot 'onRefreshTick'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onNewProject(); break;
        case 1: _t->onOpenProject(); break;
        case 2: _t->onSaveProject(); break;
        case 3: _t->onSaveProjectAs(); break;
        case 4: _t->onImportVideo(); break;
        case 5: _t->onImportTxt(); break;
        case 6: _t->onExportVideo(); break;
        case 7: _t->onExportPremiereXml(); break;
        case 8: _t->onFrameChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 9: _t->onPlayPause(); break;
        case 10: _t->onStepForward(); break;
        case 11: _t->onStepBackward(); break;
        case 12: _t->onStepForwardLarge(); break;
        case 13: _t->onStepBackwardLarge(); break;
        case 14: _t->onSubtitleSelected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->onSubtitleDeselected(); break;
        case 16: _t->onSubtitleMoved((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[2]))); break;
        case 17: _t->onDeleteSubtitle((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 18: _t->onGlobalSubtitleFontChanged(); break;
        case 19: _t->onGlobalSubtitleColorChanged(); break;
        case 20: _t->onVideoTitleChanged(); break;
        case 21: _t->onPreviewFrameDecoded((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QImage>>(_a[2]))); break;
        case 22: _t->onSetTrimStart(); break;
        case 23: _t->onSetTrimEnd(); break;
        case 24: _t->onAudioPositionChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 25: _t->onAudioDurationChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 26: _t->onRefreshTick(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 27;
    }
    return _id;
}
QT_WARNING_POP
