/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "eventfilter.h"
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#if QT_VERSION >= 0x050000
#  if defined(QT_WIDGETS_LIB)
#    define HAVE_APPLICATION
#  endif
#  if defined(QT_GUI_LIB)
#    define HAVE_GUI_APPLICATION
#  endif
#else // Qt 5
#  if defined(QT_GUI_LIB)
#    define HAVE_APPLICATION
#  endif
#endif

#ifdef HAVE_APPLICATION
#  include <QApplication>
#  include <QWidget>
#endif
#ifdef HAVE_GUI_APPLICATION
#  include <QtGui/QGuiApplication>
#  include <QtGui/QWindow>
#endif

namespace QtDiag {

EventFilter::EventFilter(EventCategories eventCategories, QObject *p)
    : QObject(p)
{
    init(eventCategories);
}

EventFilter::EventFilter(QObject *p)
    : QObject(p)
{
    init(EventCategories(0xFFFFFFF));
}

void EventFilter::init(EventCategories eventCategories)
{
    m_objectTypes = OtherType | QWidgetType | QWindowType;

    if (eventCategories & MouseEvents) {
        m_eventTypes << QEvent::MouseButtonPress << QEvent::MouseButtonRelease
            << QEvent::MouseButtonDblClick << QEvent::NonClientAreaMouseButtonPress
            << QEvent::NonClientAreaMouseButtonRelease
            << QEvent::NonClientAreaMouseButtonDblClick
            << QEvent::Wheel << QEvent::Enter << QEvent::Leave;
    }
    if (eventCategories & MouseMoveEvents)
        m_eventTypes << QEvent::MouseMove << QEvent::NonClientAreaMouseMove;
    if (eventCategories & TouchEvents) {
        m_eventTypes << QEvent::TouchBegin << QEvent::TouchUpdate << QEvent::TouchEnd;
#if QT_VERSION >= 0x050000
        m_eventTypes << QEvent::TouchCancel;
#endif
    }
    if (eventCategories & TabletEvents) {
        m_eventTypes << QEvent::TabletEnterProximity << QEvent::TabletLeaveProximity
            << QEvent::TabletMove << QEvent::TabletPress << QEvent::TabletRelease;
    }
    if (eventCategories & DragAndDropEvents) {
        m_eventTypes << QEvent::DragEnter << QEvent::DragMove << QEvent::DragLeave
            << QEvent::Drop << QEvent::DragResponse;
    }
    if (eventCategories & KeyEvents) {
        m_eventTypes << QEvent::KeyPress << QEvent::KeyRelease << QEvent::ShortcutOverride
            << QEvent::Shortcut;
    }
    if (eventCategories & FocusEvents) {
        m_eventTypes
#if QT_VERSION >= 0x050000
            << QEvent::FocusAboutToChange
#endif
            << QEvent::FocusIn << QEvent::FocusOut;
    }
    if (eventCategories & GeometryEvents)
        m_eventTypes << QEvent::Move << QEvent::Resize;
    if (eventCategories & PaintEvents) {
        m_eventTypes << QEvent::UpdateRequest << QEvent::Paint
            << QEvent::Show << QEvent::Hide;
#if QT_VERSION >= 0x050000
        m_eventTypes << QEvent::Expose;
#endif
    }
    if (eventCategories & StateChangeEvents) {
        m_eventTypes
            << QEvent::WindowStateChange
            << QEvent::WindowBlocked << QEvent::WindowUnblocked
#if QT_VERSION >= 0x050000
            << QEvent::ApplicationStateChange
#endif
            << QEvent::ApplicationActivate << QEvent::ApplicationDeactivate;
    }
    if (eventCategories & TimerEvents)
        m_eventTypes << QEvent::Timer << QEvent::ZeroTimerEvent;
    if (eventCategories & ObjectEvents) {
        m_eventTypes << QEvent::ChildAdded << QEvent::ChildPolished
            << QEvent::ChildRemoved << QEvent::Create << QEvent::Destroy;
    }
    if (eventCategories & InputMethodEvents) {
        m_eventTypes << QEvent::InputMethod;
#if QT_VERSION >= 0x050000
        m_eventTypes << QEvent::InputMethodQuery;
#endif
    }
}

static inline bool matchesType(const QObject *o, EventFilter::ObjectTypes types)
{
    if (o->isWidgetType())
        return types & EventFilter::QWidgetType;
#if QT_VERSION >= 0x050000
    if (o->isWindowType())
        return types & EventFilter::QWindowType;
#endif
    return types & EventFilter::OtherType;
}

static void formatObject(const QObject *o, QDebug debug)
{
    if (o) {
        debug << o->metaObject()->className();
        const QString on = o->objectName();
        if (!on.isEmpty())
            debug << '/' << on;
    } else {
        debug << "null";
    }
}

static void formatApplicationState(QDebug debug)
{
#if defined(HAVE_APPLICATION)
    if (const QWidget *mw = QApplication::activeModalWidget()) {
        debug << "\n  QApplication::activeModalWidget = ";
        formatObject(mw, debug);
    }
    if (const QWidget *pw = QApplication::activePopupWidget()) {
        debug << "\n  QApplication::activePopupWidget = ";
        formatObject(pw, debug);
    }
    debug << "\n  QApplication::activeWindow      = ";
    formatObject(QApplication::activeWindow(), debug);
#endif // HAVE_APPLICATION
#if defined(HAVE_GUI_APPLICATION)
    if (const QWindow *mw = QGuiApplication::modalWindow()) {
        debug << "\n  QGuiApplication::modalWindow    = ";
        formatObject(mw, debug);
    }
    debug << "\n  QGuiApplication::focusWindow    = ";
    formatObject(QGuiApplication::focusWindow(), debug);
#endif // HAVE_GUI_APPLICATION
}

bool EventFilter::eventFilter(QObject *o, QEvent *e)
{
    static int n = 0;
    if (matchesType(o, m_objectTypes) && m_eventTypes.contains(e->type())) {
        QDebug debug = qDebug().nospace();
        debug << '#' << n++ << ' ';
        formatObject(o, debug);
        debug << ' ' << e;
        switch (e->type()) {
#if QT_VERSION >= 0x050000
        case QEvent::FocusAboutToChange:
#endif
        case QEvent::FocusIn:
            formatApplicationState(debug);
            break;
        default:
            break;
        }
    }
    return false;
}

} // namespace QtDiag
