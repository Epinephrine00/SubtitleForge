/****************************************************************************
** Meta object code from reading C++ file 'textinputpanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/widgets/textinputpanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'textinputpanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14TextInputPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto TextInputPanel::qt_create_metaobjectdata<qt_meta_tag_ZN14TextInputPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TextInputPanel",
        "addSubtitleRequested",
        "",
        "text",
        "QFont",
        "font",
        "QColor",
        "color",
        "Qt::Alignment",
        "align",
        "subtitleTextChanged",
        "id",
        "subtitleFontChanged",
        "subtitleColorChanged",
        "subtitleAlignmentChanged",
        "deleteSubtitleRequested",
        "onAddClicked",
        "onColorClicked",
        "onFontChanged",
        "onAlignChanged",
        "onDeleteClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'addSubtitleRequested'
        QtMocHelpers::SignalData<void(const QString &, const QFont &, const QColor &, Qt::Alignment)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { 0x80000000 | 4, 5 }, { 0x80000000 | 6, 7 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'subtitleTextChanged'
        QtMocHelpers::SignalData<void(int, const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 }, { QMetaType::QString, 3 },
        }}),
        // Signal 'subtitleFontChanged'
        QtMocHelpers::SignalData<void(int, const QFont &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 }, { 0x80000000 | 4, 5 },
        }}),
        // Signal 'subtitleColorChanged'
        QtMocHelpers::SignalData<void(int, const QColor &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 }, { 0x80000000 | 6, 7 },
        }}),
        // Signal 'subtitleAlignmentChanged'
        QtMocHelpers::SignalData<void(int, Qt::Alignment)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'deleteSubtitleRequested'
        QtMocHelpers::SignalData<void(int)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 11 },
        }}),
        // Slot 'onAddClicked'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onColorClicked'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFontChanged'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAlignChanged'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeleteClicked'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TextInputPanel, qt_meta_tag_ZN14TextInputPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TextInputPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TextInputPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TextInputPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14TextInputPanelE_t>.metaTypes,
    nullptr
} };

void TextInputPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TextInputPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->addSubtitleRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QFont>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QColor>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<Qt::Alignment>>(_a[4]))); break;
        case 1: _t->subtitleTextChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->subtitleFontChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QFont>>(_a[2]))); break;
        case 3: _t->subtitleColorChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QColor>>(_a[2]))); break;
        case 4: _t->subtitleAlignmentChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<Qt::Alignment>>(_a[2]))); break;
        case 5: _t->deleteSubtitleRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->onAddClicked(); break;
        case 7: _t->onColorClicked(); break;
        case 8: _t->onFontChanged(); break;
        case 9: _t->onAlignChanged(); break;
        case 10: _t->onDeleteClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TextInputPanel::*)(const QString & , const QFont & , const QColor & , Qt::Alignment )>(_a, &TextInputPanel::addSubtitleRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextInputPanel::*)(int , const QString & )>(_a, &TextInputPanel::subtitleTextChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextInputPanel::*)(int , const QFont & )>(_a, &TextInputPanel::subtitleFontChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextInputPanel::*)(int , const QColor & )>(_a, &TextInputPanel::subtitleColorChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextInputPanel::*)(int , Qt::Alignment )>(_a, &TextInputPanel::subtitleAlignmentChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextInputPanel::*)(int )>(_a, &TextInputPanel::deleteSubtitleRequested, 5))
            return;
    }
}

const QMetaObject *TextInputPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TextInputPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TextInputPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TextInputPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void TextInputPanel::addSubtitleRequested(const QString & _t1, const QFont & _t2, const QColor & _t3, Qt::Alignment _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 1
void TextInputPanel::subtitleTextChanged(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void TextInputPanel::subtitleFontChanged(int _t1, const QFont & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void TextInputPanel::subtitleColorChanged(int _t1, const QColor & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void TextInputPanel::subtitleAlignmentChanged(int _t1, Qt::Alignment _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void TextInputPanel::deleteSubtitleRequested(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}
QT_WARNING_POP
