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

#include "qdesigner_propertyeditor_p.h"
#include "pluginmanager_p.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerDynamicPropertySheetExtension>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionManager>
#include <widgetfactory_p.h>
#include <QtWidgets/QAction>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QAbstractButton>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {
typedef QDesignerPropertyEditor::StringPropertyParameters StringPropertyParameters;
// A map of property name to type
typedef QHash<QString, StringPropertyParameters> PropertyNameTypeMap;

// Compile a map of hard-coded string property types
static const PropertyNameTypeMap &stringPropertyTypes()
{
    static PropertyNameTypeMap propertyNameTypeMap;
    if (propertyNameTypeMap.empty()) {
        const StringPropertyParameters richtext(ValidationRichText, true);
        // Accessibility. Both are texts the narrator reads
        propertyNameTypeMap.insert(QStringLiteral("accessibleDescription"), richtext);
        propertyNameTypeMap.insert(QStringLiteral("accessibleName"), richtext);
        // object names
        const StringPropertyParameters objectName(ValidationObjectName, false);
        propertyNameTypeMap.insert(QStringLiteral("buddy"), objectName);
        propertyNameTypeMap.insert(QStringLiteral("currentItemName"), objectName);
        propertyNameTypeMap.insert(QStringLiteral("currentPageName"), objectName);
        propertyNameTypeMap.insert(QStringLiteral("currentTabName"), objectName);
        propertyNameTypeMap.insert(QStringLiteral("layoutName"), objectName);
        propertyNameTypeMap.insert(QStringLiteral("spacerName"), objectName);
        // Style sheet
        propertyNameTypeMap.insert(QStringLiteral("styleSheet"), StringPropertyParameters(ValidationStyleSheet, false));
        // Buttons/  QCommandLinkButton
        const StringPropertyParameters multiline(ValidationMultiLine, true);
        propertyNameTypeMap.insert(QStringLiteral("description"), multiline);
        propertyNameTypeMap.insert(QStringLiteral("iconText"), multiline);
        // Tooltips, etc.
        propertyNameTypeMap.insert(QStringLiteral("toolTip"), richtext);
        propertyNameTypeMap.insert(QStringLiteral("whatsThis"), richtext);
        propertyNameTypeMap.insert(QStringLiteral("windowIconText"), richtext);
        propertyNameTypeMap.insert(QStringLiteral("html"), richtext);
        //  A QWizard page id
        propertyNameTypeMap.insert(QStringLiteral("pageId"), StringPropertyParameters(ValidationSingleLine, false));
        // QPlainTextEdit
        propertyNameTypeMap.insert(QStringLiteral("plainText"), StringPropertyParameters(ValidationMultiLine, true));
    }
    return propertyNameTypeMap;
}

QDesignerPropertyEditor::QDesignerPropertyEditor(QWidget *parent, Qt::WindowFlags flags) :
    QDesignerPropertyEditorInterface(parent, flags),
    m_propertyChangedForwardingBlocked(false)
{
    // Make old signal work for  compatibility
    connect(this, SIGNAL(propertyChanged(QString,QVariant)), this, SLOT(slotPropertyChanged(QString,QVariant)));
}

static inline bool isDynamicProperty(QDesignerFormEditorInterface *core, QObject *object,
                                     const QString &propertyName)
{
    if (const QDesignerDynamicPropertySheetExtension *dynamicSheet = qt_extension<QDesignerDynamicPropertySheetExtension*>(core->extensionManager(), object)) {
        if (dynamicSheet->dynamicPropertiesAllowed()) {
            if (QDesignerPropertySheetExtension *propertySheet = qt_extension<QDesignerPropertySheetExtension*>(core->extensionManager(), object)) {
                const int index = propertySheet->indexOf(propertyName);
                return index >= 0 && dynamicSheet->isDynamicProperty(index);
            }
        }
    }
    return false;
}

QDesignerPropertyEditor::StringPropertyParameters QDesignerPropertyEditor::textPropertyValidationMode(
        QDesignerFormEditorInterface *core, const QObject *object,
        const QString &propertyName, bool isMainContainer)
{
    // object name - no comment
    if (propertyName == QStringLiteral("objectName")) {
        const TextPropertyValidationMode vm =  isMainContainer ? ValidationObjectNameScope : ValidationObjectName;
        return StringPropertyParameters(vm, false);
    }

    // Check custom widgets by class.
    const QString className = WidgetFactory::classNameOf(core, object);
    const QDesignerCustomWidgetData customData = core->pluginManager()->customWidgetData(className);
    if (!customData.isNull()) {
        StringPropertyParameters customType;
        if (customData.xmlStringPropertyType(propertyName, &customType))
            return customType;
    }

    if (isDynamicProperty(core, const_cast<QObject *>(object), propertyName))
        return StringPropertyParameters(ValidationMultiLine, true);

    // Check hardcoded property ames
   const PropertyNameTypeMap::const_iterator hit = stringPropertyTypes().constFind(propertyName);
   if (hit != stringPropertyTypes().constEnd())
       return hit.value();

    // text: Check according to widget type.
    if (propertyName == QStringLiteral("text")) {
        if (qobject_cast<const QAction *>(object) || qobject_cast<const QLineEdit *>(object))
            return StringPropertyParameters(ValidationSingleLine, true);
        if (qobject_cast<const QAbstractButton *>(object))
            return StringPropertyParameters(ValidationMultiLine, true);
        return StringPropertyParameters(ValidationRichText, true);
    }

   // Fuzzy matching
    if (propertyName.endsWith(QStringLiteral("Name")))
        return StringPropertyParameters(ValidationSingleLine, true);

    if (propertyName.endsWith(QStringLiteral("ToolTip")))
        return StringPropertyParameters(ValidationRichText, true);

#ifdef Q_OS_WIN // No translation for the active X "control" property
    if (propertyName == QStringLiteral("control") && className == QStringLiteral("QAxWidget"))
        return StringPropertyParameters(ValidationSingleLine, false);
#endif

    // default to single
    return StringPropertyParameters(ValidationSingleLine, true);
}

void QDesignerPropertyEditor::emitPropertyValueChanged(const QString &name, const QVariant &value, bool enableSubPropertyHandling)
{
    // Avoid duplicate signal emission - see below
    m_propertyChangedForwardingBlocked = true;
    emit propertyValueChanged(name, value, enableSubPropertyHandling);
    emit propertyChanged(name, value);
    m_propertyChangedForwardingBlocked = false;
}

void QDesignerPropertyEditor::slotPropertyChanged(const QString &name, const QVariant &value)
{
    // Forward signal from Integration using the old interfaces.
    if (!m_propertyChangedForwardingBlocked)
        emit propertyValueChanged(name, value, true);
}

}

QT_END_NAMESPACE
