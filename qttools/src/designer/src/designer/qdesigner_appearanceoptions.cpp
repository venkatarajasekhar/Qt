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

#include "qdesigner_appearanceoptions.h"
#include "ui_qdesigner_appearanceoptions.h"

#include "qdesigner_settings.h"
#include "qdesigner_toolwindow.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

// ---------------- AppearanceOptions
AppearanceOptions::AppearanceOptions() :
     uiMode(DockedMode)
{
}

bool AppearanceOptions::equals(const AppearanceOptions &rhs) const
{
    return uiMode == rhs.uiMode && toolWindowFontSettings == rhs.toolWindowFontSettings;
}

void AppearanceOptions::toSettings(QDesignerSettings &settings) const
{
    settings.setUiMode(uiMode);
    settings.setToolWindowFont(toolWindowFontSettings);
}

void AppearanceOptions::fromSettings(const QDesignerSettings &settings)
{
    uiMode = settings.uiMode();
    toolWindowFontSettings = settings.toolWindowFont();
}

// ---------------- QDesignerAppearanceOptionsWidget
QDesignerAppearanceOptionsWidget::QDesignerAppearanceOptionsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::AppearanceOptionsWidget),
    m_initialUIMode(NeutralMode)
{
    m_ui->setupUi(this);

    m_ui->m_uiModeCombo->addItem(tr("Docked Window"), QVariant(DockedMode));
    m_ui->m_uiModeCombo->addItem(tr("Multiple Top-Level Windows"), QVariant(TopLevelMode));
    connect(m_ui->m_uiModeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotUiModeComboChanged()));

    m_ui->m_fontPanel->setCheckable(true);
    m_ui->m_fontPanel->setTitle(tr("Toolwindow Font"));

}

QDesignerAppearanceOptionsWidget::~QDesignerAppearanceOptionsWidget()
{
    delete m_ui;
}

UIMode QDesignerAppearanceOptionsWidget::uiMode() const
{
    return static_cast<UIMode>(m_ui->m_uiModeCombo->itemData(m_ui->m_uiModeCombo->currentIndex()).toInt());
}

AppearanceOptions QDesignerAppearanceOptionsWidget::appearanceOptions() const
{
    AppearanceOptions rc;
    rc.uiMode = uiMode();
    rc.toolWindowFontSettings.m_font = m_ui->m_fontPanel->selectedFont();
    rc.toolWindowFontSettings.m_useFont = m_ui->m_fontPanel->isChecked();
    rc.toolWindowFontSettings.m_writingSystem = m_ui->m_fontPanel->writingSystem();
    return rc;
}

void QDesignerAppearanceOptionsWidget::setAppearanceOptions(const AppearanceOptions &ao)
{
    m_initialUIMode = ao.uiMode;
    m_ui->m_uiModeCombo->setCurrentIndex(m_ui->m_uiModeCombo->findData(QVariant(ao.uiMode)));
    m_ui->m_fontPanel->setWritingSystem(ao.toolWindowFontSettings.m_writingSystem);
    m_ui->m_fontPanel->setSelectedFont(ao.toolWindowFontSettings.m_font);
    m_ui->m_fontPanel->setChecked(ao.toolWindowFontSettings.m_useFont);
}

void QDesignerAppearanceOptionsWidget::slotUiModeComboChanged()
{
    emit uiModeChanged(m_initialUIMode != uiMode());
}

// ----------- QDesignerAppearanceOptionsPage
QDesignerAppearanceOptionsPage::QDesignerAppearanceOptionsPage(QDesignerFormEditorInterface *core) :
    m_core(core)
{
}

QString QDesignerAppearanceOptionsPage::name() const
{
    //: Tab in preferences dialog
    return QCoreApplication::translate("QDesignerAppearanceOptionsPage", "Appearance");
}

QWidget *QDesignerAppearanceOptionsPage::createPage(QWidget *parent)
{
    m_widget = new QDesignerAppearanceOptionsWidget(parent);
    m_initialOptions.fromSettings(QDesignerSettings(m_core));
    m_widget->setAppearanceOptions(m_initialOptions);
    return m_widget;
}

void QDesignerAppearanceOptionsPage::apply()
{
    if (m_widget) {
        const AppearanceOptions newOptions = m_widget->appearanceOptions();
        if (newOptions != m_initialOptions) {
            QDesignerSettings settings(m_core);
            newOptions.toSettings(settings);
            m_initialOptions = newOptions;
            emit settingsChanged();
        }
    }
}

void QDesignerAppearanceOptionsPage::finish()
{
}

QT_END_NAMESPACE

