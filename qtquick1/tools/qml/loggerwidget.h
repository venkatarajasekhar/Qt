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

#ifndef LOGGERWIDGET_H
#define LOGGERWIDGET_H

#include <QMainWindow>
#include <QMetaType>

QT_BEGIN_NAMESPACE

class QPlainTextEdit;
class QLabel;
class QMenu;
class QAction;

class LoggerWidget : public QMainWindow {
    Q_OBJECT
public:
    LoggerWidget(QWidget *parent=0);

    enum Visibility { ShowWarnings, HideWarnings, AutoShowWarnings };

    Visibility defaultVisibility() const;
    void setDefaultVisibility(Visibility visibility);

    QMenu *preferencesMenu();
    QAction *showAction();

public slots:
    void append(const QString &msg);
    void updateNoWarningsLabel();

private slots:
    void warningsPreferenceChanged(QAction *action);
    void readSettings();
    void saveSettings();

protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);

signals:
    void opened();
    void closed();

private:
    void setupPreferencesMenu();

    QMenu *m_preferencesMenu;
    QAction *m_showWidgetAction;
    QPlainTextEdit *m_plainTextEdit;
    QLabel *m_noWarningsLabel;
    enum ConfigOrigin { CommandLineOrigin, SettingsOrigin };
    ConfigOrigin m_visibilityOrigin;
    Visibility m_visibility;
    QStringList m_queuedMessages;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(LoggerWidget::Visibility)

#endif // LOGGERWIDGET_H
