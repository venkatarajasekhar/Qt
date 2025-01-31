/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include <qglobal.h>
#include <QDebug>
#include <QSettings>
#include <QActionGroup>
#include <QMenu>
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>

#include "loggerwidget.h"

QT_BEGIN_NAMESPACE

LoggerWidget::LoggerWidget(QWidget *parent) :
    QMainWindow(parent),
    m_visibilityOrigin(SettingsOrigin)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle(tr("Warnings"));

    m_plainTextEdit = new QPlainTextEdit();
    setCentralWidget(m_plainTextEdit);

    m_noWarningsLabel = new QLabel(m_plainTextEdit);
    m_noWarningsLabel->setText(tr("(No warnings)"));
    m_noWarningsLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_noWarningsLabel);
    m_plainTextEdit->setLayout(layout);
    connect(m_plainTextEdit, SIGNAL(textChanged()), this, SLOT(updateNoWarningsLabel()));

    readSettings();
    setupPreferencesMenu();
}

void LoggerWidget::append(const QString &msg)
{
    if (!isVisible()) {
        // appending the message for real can take a couple of milliseconds, so if we don't have to...
        m_queuedMessages.append(msg);
        return;
    }
    m_plainTextEdit->appendPlainText(msg);

    if (!isVisible() && (defaultVisibility() == AutoShowWarnings))
        setVisible(true);
}

LoggerWidget::Visibility LoggerWidget::defaultVisibility() const
{
    return m_visibility;
}

void LoggerWidget::setDefaultVisibility(LoggerWidget::Visibility visibility)
{
    if (m_visibility == visibility)
        return;

    m_visibility = visibility;
    m_visibilityOrigin = CommandLineOrigin;

    m_preferencesMenu->setEnabled(m_visibilityOrigin == SettingsOrigin);
}

QMenu *LoggerWidget::preferencesMenu()
{
    return m_preferencesMenu;
}

QAction *LoggerWidget::showAction()
{
    return m_showWidgetAction;
}

void LoggerWidget::readSettings()
{
    QSettings settings;
    QString warningsPreferences = settings.value(QLatin1String("warnings"), QLatin1String("hide")).toString();
    if (warningsPreferences == QLatin1String("show")) {
        m_visibility = ShowWarnings;
    } else if (warningsPreferences == QLatin1String("hide")) {
        m_visibility = HideWarnings;
    } else {
        m_visibility = AutoShowWarnings;
    }
}

void LoggerWidget::saveSettings()
{
    if (m_visibilityOrigin != SettingsOrigin)
        return;

    QString value = QLatin1String("autoShow");
    if (defaultVisibility() == ShowWarnings) {
        value = QLatin1String("show");
    } else if (defaultVisibility() == HideWarnings) {
        value = QLatin1String("hide");
    }

    QSettings settings;
    settings.setValue(QLatin1String("warnings"), value);
}

void LoggerWidget::warningsPreferenceChanged(QAction *action)
{
    Visibility newSetting = static_cast<Visibility>(action->data().toInt());
    m_visibility = newSetting;
    saveSettings();
}

void LoggerWidget::showEvent(QShowEvent *event)
{
    foreach (const QString &msg, m_queuedMessages)
        m_plainTextEdit->appendPlainText(msg);
    m_queuedMessages.clear();
    QWidget::showEvent(event);
    emit opened();
}

void LoggerWidget::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    emit closed();
}

void LoggerWidget::setupPreferencesMenu()
{
    m_preferencesMenu = new QMenu(tr("Warnings"));
    QActionGroup *warnings = new QActionGroup(m_preferencesMenu);
    warnings->setExclusive(true);

    connect(warnings, SIGNAL(triggered(QAction*)), this, SLOT(warningsPreferenceChanged(QAction*)));

    QAction *showWarningsPreference = new QAction(tr("Show by default"), m_preferencesMenu);
    showWarningsPreference->setCheckable(true);
    showWarningsPreference->setData(LoggerWidget::ShowWarnings);
    warnings->addAction(showWarningsPreference);
    m_preferencesMenu->addAction(showWarningsPreference);

    QAction *hideWarningsPreference = new QAction(tr("Hide by default"), m_preferencesMenu);
    hideWarningsPreference->setCheckable(true);
    hideWarningsPreference->setData(LoggerWidget::HideWarnings);
    warnings->addAction(hideWarningsPreference);
    m_preferencesMenu->addAction(hideWarningsPreference);

    QAction *autoWarningsPreference = new QAction(tr("Show for first warning"), m_preferencesMenu);
    autoWarningsPreference->setCheckable(true);
    autoWarningsPreference->setData(LoggerWidget::AutoShowWarnings);
    warnings->addAction(autoWarningsPreference);
    m_preferencesMenu->addAction(autoWarningsPreference);

    switch (defaultVisibility()) {
    case LoggerWidget::ShowWarnings:
        showWarningsPreference->setChecked(true);
        break;
    case LoggerWidget::HideWarnings:
        hideWarningsPreference->setChecked(true);
        break;
    default:
        autoWarningsPreference->setChecked(true);
    }
}

void LoggerWidget::updateNoWarningsLabel()
{
    m_noWarningsLabel->setVisible(m_plainTextEdit->toPlainText().length() == 0);
}

QT_END_NAMESPACE
