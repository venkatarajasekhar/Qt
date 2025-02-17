/********************************************************************************
** Form generated from reading UI file 'preferencesdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <previewconfigurationwidget_p.h>
#include "fontpanel.h"
#include "gridpanel_p.h"

QT_BEGIN_NAMESPACE

class Ui_PreferencesDialog
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout1;
    QGroupBox *m_uiModeGroupBox;
    QVBoxLayout *vboxLayout2;
    QComboBox *m_uiModeCombo;
    FontPanel *m_fontPanel;
    qdesigner_internal::PreviewConfigurationWidget *m_previewConfigurationWidget;
    QVBoxLayout *vboxLayout3;
    QGroupBox *m_templatePathGroupBox;
    QGridLayout *gridLayout;
    QListWidget *m_templatePathListWidget;
    QToolButton *m_addTemplatePathButton;
    QToolButton *m_removeTemplatePathButton;
    QSpacerItem *spacerItem;
    qdesigner_internal::GridPanel *m_gridPanel;
    QFrame *line;
    QDialogButtonBox *m_dialogButtonBox;

    void setupUi(QDialog *PreferencesDialog)
    {
        if (PreferencesDialog->objectName().isEmpty())
            PreferencesDialog->setObjectName(QStringLiteral("PreferencesDialog"));
        PreferencesDialog->resize(455, 359);
        PreferencesDialog->setModal(true);
        vboxLayout = new QVBoxLayout(PreferencesDialog);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        m_uiModeGroupBox = new QGroupBox(PreferencesDialog);
        m_uiModeGroupBox->setObjectName(QStringLiteral("m_uiModeGroupBox"));
        vboxLayout2 = new QVBoxLayout(m_uiModeGroupBox);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        m_uiModeCombo = new QComboBox(m_uiModeGroupBox);
        m_uiModeCombo->setObjectName(QStringLiteral("m_uiModeCombo"));

        vboxLayout2->addWidget(m_uiModeCombo);


        vboxLayout1->addWidget(m_uiModeGroupBox);

        m_fontPanel = new FontPanel(PreferencesDialog);
        m_fontPanel->setObjectName(QStringLiteral("m_fontPanel"));

        vboxLayout1->addWidget(m_fontPanel);

        m_previewConfigurationWidget = new qdesigner_internal::PreviewConfigurationWidget(PreferencesDialog);
        m_previewConfigurationWidget->setObjectName(QStringLiteral("m_previewConfigurationWidget"));

        vboxLayout1->addWidget(m_previewConfigurationWidget);


        hboxLayout->addLayout(vboxLayout1);

        vboxLayout3 = new QVBoxLayout();
        vboxLayout3->setObjectName(QStringLiteral("vboxLayout3"));
        m_templatePathGroupBox = new QGroupBox(PreferencesDialog);
        m_templatePathGroupBox->setObjectName(QStringLiteral("m_templatePathGroupBox"));
        gridLayout = new QGridLayout(m_templatePathGroupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        m_templatePathListWidget = new QListWidget(m_templatePathGroupBox);
        m_templatePathListWidget->setObjectName(QStringLiteral("m_templatePathListWidget"));

        gridLayout->addWidget(m_templatePathListWidget, 0, 0, 1, 3);

        m_addTemplatePathButton = new QToolButton(m_templatePathGroupBox);
        m_addTemplatePathButton->setObjectName(QStringLiteral("m_addTemplatePathButton"));

        gridLayout->addWidget(m_addTemplatePathButton, 1, 0, 1, 1);

        m_removeTemplatePathButton = new QToolButton(m_templatePathGroupBox);
        m_removeTemplatePathButton->setObjectName(QStringLiteral("m_removeTemplatePathButton"));

        gridLayout->addWidget(m_removeTemplatePathButton, 1, 1, 1, 1);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 2, 1, 1);


        vboxLayout3->addWidget(m_templatePathGroupBox);

        m_gridPanel = new qdesigner_internal::GridPanel(PreferencesDialog);
        m_gridPanel->setObjectName(QStringLiteral("m_gridPanel"));

        vboxLayout3->addWidget(m_gridPanel);


        hboxLayout->addLayout(vboxLayout3);


        vboxLayout->addLayout(hboxLayout);

        line = new QFrame(PreferencesDialog);
        line->setObjectName(QStringLiteral("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        vboxLayout->addWidget(line);

        m_dialogButtonBox = new QDialogButtonBox(PreferencesDialog);
        m_dialogButtonBox->setObjectName(QStringLiteral("m_dialogButtonBox"));
        m_dialogButtonBox->setOrientation(Qt::Horizontal);
        m_dialogButtonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(m_dialogButtonBox);


        retranslateUi(PreferencesDialog);
        QObject::connect(m_dialogButtonBox, SIGNAL(accepted()), PreferencesDialog, SLOT(accept()));
        QObject::connect(m_dialogButtonBox, SIGNAL(rejected()), PreferencesDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(PreferencesDialog);
    } // setupUi

    void retranslateUi(QDialog *PreferencesDialog)
    {
        PreferencesDialog->setWindowTitle(QApplication::translate("PreferencesDialog", "Preferences", 0));
        m_uiModeGroupBox->setTitle(QApplication::translate("PreferencesDialog", "User Interface Mode", 0));
        m_templatePathGroupBox->setTitle(QApplication::translate("PreferencesDialog", "Additional Template Paths", 0));
        m_addTemplatePathButton->setText(QApplication::translate("PreferencesDialog", "...", 0));
        m_removeTemplatePathButton->setText(QApplication::translate("PreferencesDialog", "...", 0));
    } // retranslateUi

};

namespace Ui {
    class PreferencesDialog: public Ui_PreferencesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PREFERENCESDIALOG_H
