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

#include "qdesigner_widgetbox_p.h"
#include "qdesigner_utils_p.h"

#include <QtDesigner/private/ui4_p.h>

#include <QtCore/QRegExp>
#include <QtCore/QDebug>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QSharedData>

QT_BEGIN_NAMESPACE

class QDesignerWidgetBoxWidgetData : public QSharedData
{
public:
    QDesignerWidgetBoxWidgetData(const QString &aname, const QString &xml,
                                 const QString &icon_name,
                                 QDesignerWidgetBoxInterface::Widget::Type atype);
    QString m_name;
    QString m_xml;
    QString m_icon_name;
    QDesignerWidgetBoxInterface::Widget::Type m_type;
};

QDesignerWidgetBoxWidgetData::QDesignerWidgetBoxWidgetData(const QString &aname,
                                                           const QString &xml,
                                                           const QString &icon_name,
                                                           QDesignerWidgetBoxInterface::Widget::Type atype) :
    m_name(aname), m_xml(xml), m_icon_name(icon_name), m_type(atype)
{
}

QDesignerWidgetBoxInterface::Widget::Widget(const QString &aname, const QString &xml,
                                            const QString &icon_name, Type atype) :
    m_data(new QDesignerWidgetBoxWidgetData(aname, xml, icon_name, atype))
{
}

QDesignerWidgetBoxInterface::Widget::~Widget()
{
}

QDesignerWidgetBoxInterface::Widget::Widget(const Widget &w) :
    m_data(w.m_data)
{
}

QDesignerWidgetBoxInterface::Widget &QDesignerWidgetBoxInterface::Widget::operator=(const Widget &rhs)
{
    if (this != &rhs) {
        m_data = rhs.m_data;
    }
    return *this;
}

QString QDesignerWidgetBoxInterface::Widget::name() const
{
    return m_data->m_name;
}

void QDesignerWidgetBoxInterface::Widget::setName(const QString &aname)
{
    m_data->m_name = aname;
}

QString QDesignerWidgetBoxInterface::Widget::domXml() const
{
    return m_data->m_xml;
}

void QDesignerWidgetBoxInterface::Widget::setDomXml(const QString &xml)
{
    m_data->m_xml = xml;
}

QString QDesignerWidgetBoxInterface::Widget::iconName() const
{
    return m_data->m_icon_name;
}

void QDesignerWidgetBoxInterface::Widget::setIconName(const QString &icon_name)
{
    m_data->m_icon_name = icon_name;
}

QDesignerWidgetBoxInterface::Widget::Type QDesignerWidgetBoxInterface::Widget::type() const
{
    return m_data->m_type;
}

void QDesignerWidgetBoxInterface::Widget::setType(Type atype)
{
    m_data->m_type = atype;
}

bool QDesignerWidgetBoxInterface::Widget::isNull() const
{
    return m_data->m_name.isEmpty();
}

namespace qdesigner_internal {
QDesignerWidgetBox::QDesignerWidgetBox(QWidget *parent, Qt::WindowFlags flags)
    : QDesignerWidgetBoxInterface(parent, flags),
      m_loadMode(LoadMerge)
{

}

QDesignerWidgetBox::LoadMode QDesignerWidgetBox::loadMode() const
{
    return m_loadMode;
}

void QDesignerWidgetBox::setLoadMode(LoadMode lm)
{
     m_loadMode = lm;
}

// Convenience to find a widget by class name
bool QDesignerWidgetBox::findWidget(const QDesignerWidgetBoxInterface *wbox,
                                    const QString &className,
                                    const QString &category,
                                    Widget *widgetData)
{
    // Note that entry names do not necessarily match the class name
    // (at least, not for the standard widgets), so,
    // look in the XML for the class name of the first widget to appear
    const QString widgetTag = QStringLiteral("<widget");
    QString pattern = QStringLiteral("^<widget\\s+class\\s*=\\s*\"");
    pattern += className;
    pattern += QStringLiteral("\".*$");
    QRegExp regexp(pattern);
    Q_ASSERT(regexp.isValid());
    const int catCount = wbox->categoryCount();
    for (int c = 0; c < catCount; c++) {
        const Category cat = wbox->category(c);
        if (category.isEmpty() || cat.name() == category) {
            const int widgetCount =  cat.widgetCount();
            for (int w = 0; w < widgetCount; w++) {
                const Widget widget = cat.widget(w);
                QString xml = widget.domXml(); // Erase the <ui> tag that can be present starting from 4.4
                const int widgetTagIndex = xml.indexOf(widgetTag);
                if (widgetTagIndex != -1) {
                    xml.remove(0, widgetTagIndex);
                    if (regexp.exactMatch(xml)) {
                        *widgetData = widget;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// Convenience to create a Dom Widget from widget box xml code.
DomUI *QDesignerWidgetBox::xmlToUi(const QString &name, const QString &xml, bool insertFakeTopLevel,
                                   QString *errorMessage)
{
    QXmlStreamReader reader(xml);
    DomUI *ui = 0;

    // The xml description must either contain a root element "ui" with a child element "widget"
    // or "widget" as the root element (4.3 legacy)
    const QString widgetTag = QStringLiteral("widget");

    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            const QStringRef name = reader.name();
            if (ui) {
                reader.raiseError(tr("Unexpected element <%1>").arg(name.toString()));
                continue;
            }

            if (name.compare(QStringLiteral("widget"), Qt::CaseInsensitive) == 0) { // 4.3 legacy, wrap into DomUI
                ui = new DomUI;
                DomWidget *widget = new DomWidget;
                widget->read(reader);
                ui->setElementWidget(widget);
            } else if (name.compare(QStringLiteral("ui"), Qt::CaseInsensitive) == 0) { // 4.4
                ui = new DomUI;
                ui->read(reader);
            } else {
                reader.raiseError(tr("Unexpected element <%1>").arg(name.toString()));
            }
        }
   }

    if (reader.hasError()) {
        delete ui;
        *errorMessage = tr("A parse error occurred at line %1, column %2 of the XML code "
                           "specified for the widget %3: %4\n%5")
                           .arg(reader.lineNumber()).arg(reader.columnNumber()).arg(name)
                           .arg(reader.errorString()).arg(xml);
        return 0;
    }

    if (!ui || !ui->elementWidget()) {
        delete ui;
        *errorMessage = tr("The XML code specified for the widget %1 does not contain "
                           "any widget elements.\n%2").arg(name).arg(xml);
        return 0;
    }

    if (insertFakeTopLevel)  {
        DomWidget *fakeTopLevel = new DomWidget;
        fakeTopLevel->setAttributeClass(QStringLiteral("QWidget"));
        QList<DomWidget *> children;
        children.push_back(ui->takeElementWidget());
        fakeTopLevel->setElementWidget(children);
        ui->setElementWidget(fakeTopLevel);
    }

    return ui;
}

// Convenience to create a Dom Widget from widget box xml code.
DomUI *QDesignerWidgetBox::xmlToUi(const QString &name, const QString &xml, bool insertFakeTopLevel)
{
    QString errorMessage;
    DomUI *rc = xmlToUi(name, xml, insertFakeTopLevel, &errorMessage);
    if (!rc)
        qdesigner_internal::designerWarning(errorMessage);
    return rc;
}

}  // namespace qdesigner_internal

QT_END_NAMESPACE
