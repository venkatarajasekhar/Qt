/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Designer of the Qt Toolkit.
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

#ifndef DEFAULT_CONTAINER_H
#define DEFAULT_CONTAINER_H

#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/extension.h>
#include <extensionfactory_p.h>

#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QDockWidget>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

// ------------ QStackedWidgetContainer
class QStackedWidgetContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QStackedWidgetContainer(QStackedWidget *widget, QObject *parent = 0);

    virtual int count() const { return m_widget->count(); }
    virtual QWidget *widget(int index) const { return m_widget->widget(index); }

    virtual int currentIndex() const { return m_widget->currentIndex(); }
    virtual void setCurrentIndex(int index);

    virtual void addWidget(QWidget *widget);
    virtual void insertWidget(int index, QWidget *widget);
    virtual void remove(int index);

private:
    QStackedWidget *m_widget;
};

// ------------ QTabWidgetContainer
class QTabWidgetContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QTabWidgetContainer(QTabWidget *widget, QObject *parent = 0);

    virtual int count() const { return m_widget->count(); }
    virtual QWidget *widget(int index) const { return m_widget->widget(index); }

    virtual int currentIndex() const { return m_widget->currentIndex(); }
    virtual void setCurrentIndex(int index);

    virtual void addWidget(QWidget *widget);
    virtual void insertWidget(int index, QWidget *widget);
    virtual void remove(int index);

private:
    QTabWidget *m_widget;
};

// ------------  QToolBoxContainer
class QToolBoxContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QToolBoxContainer(QToolBox *widget, QObject *parent = 0);

    virtual int count() const { return m_widget->count(); }
    virtual QWidget *widget(int index) const { return m_widget->widget(index); }

    virtual int currentIndex() const { return m_widget->currentIndex(); }
    virtual void setCurrentIndex(int index);

    virtual void addWidget(QWidget *widget);
    virtual void insertWidget(int index, QWidget *widget);
    virtual void remove(int index);

private:
    QToolBox *m_widget;
};

// ------------ SingleChildContainer:
//  Template for containers that have a single child widget using widget()/setWidget().

template <class Container>
class SingleChildContainer: public QDesignerContainerExtension
{
protected:
    explicit SingleChildContainer(Container *widget, bool active = true);
public:
    virtual int count() const;
    virtual QWidget *widget(int index) const;
    virtual int currentIndex() const;
    virtual void setCurrentIndex(int /*index*/) {}
    virtual void addWidget(QWidget *widget);
    virtual void insertWidget(int index, QWidget *widget);
    virtual void remove(int /*index*/) {}

    virtual bool canAddWidget() const { return false; }
    virtual bool canRemove(int) const { return false; }

private:
    const bool m_active;
    Container *m_container;
};

template <class Container>
SingleChildContainer<Container>::SingleChildContainer(Container *widget, bool active) :
    m_active(active),
    m_container(widget)
{
}

template <class Container>
int SingleChildContainer<Container>::count() const
{
    return  m_active && m_container->widget() ? 1 : 0;
}

template <class Container>
QWidget *SingleChildContainer<Container>::widget(int /* index */) const
{
    return m_container->widget();
}

template <class Container>
int SingleChildContainer<Container>::currentIndex() const
{
    return m_active && m_container->widget() ? 0 : -1;
}

template <class Container>
void SingleChildContainer<Container>::addWidget(QWidget *widget)
{
    Q_ASSERT(m_container->widget() == 0);
    widget->setParent(m_container);
    m_container->setWidget(widget);
}

template <class Container>
void SingleChildContainer<Container>::insertWidget(int /* index */, QWidget *widget)
{
    addWidget(widget);
}

// ------------  QScrollAreaContainer
class QScrollAreaContainer: public QObject, public SingleChildContainer<QScrollArea>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QScrollAreaContainer(QScrollArea *widget, QObject *parent = 0);
};

// --------------- QDockWidgetContainer
class QDockWidgetContainer: public QObject, public SingleChildContainer<QDockWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QDockWidgetContainer(QDockWidget *widget, QObject *parent = 0);
};

typedef ExtensionFactory<QDesignerContainerExtension, QStackedWidget, QStackedWidgetContainer> QDesignerStackedWidgetContainerFactory;
typedef ExtensionFactory<QDesignerContainerExtension, QTabWidget, QTabWidgetContainer> QDesignerTabWidgetContainerFactory;
typedef ExtensionFactory<QDesignerContainerExtension, QToolBox, QToolBoxContainer> QDesignerToolBoxContainerFactory;
typedef ExtensionFactory<QDesignerContainerExtension, QScrollArea, QScrollAreaContainer> QScrollAreaContainerFactory;
typedef ExtensionFactory<QDesignerContainerExtension,  QDockWidget, QDockWidgetContainer> QDockWidgetContainerFactory;
}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // DEFAULT_CONTAINER_H
