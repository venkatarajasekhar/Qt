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

#include "connectdialog_p.h"
#include "signalslot_utils_p.h"

#include <signalslotdialog_p.h>
#include <metadatabase_p.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerLanguageExtension>

#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

namespace {
    typedef QList<QListWidgetItem*> ListWidgetItems;
}

static QString realClassName(QDesignerFormEditorInterface *core, QWidget *widget)
{
    QString class_name = QLatin1String(widget->metaObject()->className());
    const QDesignerWidgetDataBaseInterface *wdb = core->widgetDataBase();
    const int idx = wdb->indexOfObject(widget);
    if (idx != -1)
        class_name = wdb->item(idx)->name();
    return class_name;
}

static QString widgetLabel(QDesignerFormEditorInterface *core, QWidget *widget)
{
    return QString::fromUtf8("%1 (%2)")
            .arg(qdesigner_internal::realObjectName(core, widget))
            .arg(realClassName(core, widget));
}

namespace qdesigner_internal {

ConnectDialog::ConnectDialog(QDesignerFormWindowInterface *formWindow,
                             QWidget *source, QWidget *destination,
                             QWidget *parent) :
    QDialog(parent),
    m_source(source),
    m_destination(destination),
    m_sourceMode(widgetMode(m_source, formWindow)),
    m_destinationMode(widgetMode(m_destination, formWindow)),
    m_formWindow(formWindow)
{
    m_ui.setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(m_ui.signalList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(selectSignal(QListWidgetItem*)));
    connect(m_ui.slotList, SIGNAL(itemClicked(QListWidgetItem*)),
            this,  SLOT(selectSlot(QListWidgetItem*)));
    m_ui.slotList->setEnabled(false);

    QPushButton *ok_button = okButton();
    ok_button->setDefault(true);
    ok_button->setEnabled(false);

    connect(m_ui.showAllCheckBox, SIGNAL(toggled(bool)), this, SLOT(populateLists()));

    QDesignerFormEditorInterface *core = m_formWindow->core();
    m_ui.signalGroupBox->setTitle(widgetLabel(core, source));
    m_ui.slotGroupBox->setTitle(widgetLabel(core, destination));

    m_ui.editSignalsButton->setEnabled(m_sourceMode != NormalWidget);
    connect(m_ui.editSignalsButton, SIGNAL(clicked()), this, SLOT(editSignals()));

    m_ui.editSlotsButton->setEnabled(m_destinationMode != NormalWidget);
    connect(m_ui.editSlotsButton,   SIGNAL(clicked()), this, SLOT(editSlots()));

    populateLists();
}

ConnectDialog::WidgetMode ConnectDialog::widgetMode(QWidget *w,  QDesignerFormWindowInterface *formWindow)
{
    QDesignerFormEditorInterface *core = formWindow->core();
    if (qt_extension<QDesignerLanguageExtension*>(core->extensionManager(), core))
        return NormalWidget;

    if (w == formWindow || formWindow->mainContainer() == w)
        return MainContainer;

    if (isPromoted(formWindow->core(), w))
        return PromotedWidget;

    return NormalWidget;
}

QPushButton *ConnectDialog::okButton()
{
    return m_ui.buttonBox->button(QDialogButtonBox::Ok);
}

void ConnectDialog::setOkButtonEnabled(bool e)
{
    okButton()->setEnabled(e);
}

void ConnectDialog::populateLists()
{
    populateSignalList();
}

void ConnectDialog::setSignalSlot(const QString &signal, const QString &slot)
{
    ListWidgetItems sigItems = m_ui.signalList->findItems(signal, Qt::MatchExactly);

    if (sigItems.empty()) {
        m_ui.showAllCheckBox->setChecked(true);
        sigItems = m_ui.signalList->findItems(signal, Qt::MatchExactly);
    }

    if (!sigItems.empty()) {
        selectSignal(sigItems.front());
        ListWidgetItems slotItems = m_ui.slotList->findItems(slot, Qt::MatchExactly);
        if (slotItems.empty()) {
            m_ui.showAllCheckBox->setChecked(true);
            slotItems = m_ui.slotList->findItems(slot, Qt::MatchExactly);
        }
        if (!slotItems.empty())
            selectSlot(slotItems.front());
    }
}

bool ConnectDialog::showAllSignalsSlots() const
{
    return m_ui.showAllCheckBox->isChecked();
}

void ConnectDialog::setShowAllSignalsSlots(bool showIt)
{
    m_ui.showAllCheckBox->setChecked(showIt);
}

void ConnectDialog::selectSignal(QListWidgetItem *item)
{
    if (item) {
        m_ui.signalList->setCurrentItem(item);
        populateSlotList(item->text());
        m_ui.slotList->setEnabled(true);
        setOkButtonEnabled(!m_ui.slotList->selectedItems().isEmpty());
    } else {
        m_ui.signalList->clearSelection();
        populateSlotList();
        m_ui.slotList->setEnabled(false);
        setOkButtonEnabled(false);
    }
}

void ConnectDialog::selectSlot(QListWidgetItem *item)
{
    if (item) {
        m_ui.slotList->setCurrentItem(item);
    } else {
        m_ui.slotList->clearSelection();
    }
    setOkButtonEnabled(true);
}

QString ConnectDialog::signal() const
{
    const ListWidgetItems item_list = m_ui.signalList->selectedItems();
    if (item_list.size() != 1)
        return QString();
    return item_list.at(0)->text();
}

QString ConnectDialog::slot() const
{
    const ListWidgetItems item_list = m_ui.slotList->selectedItems();
    if (item_list.size() != 1)
        return QString();
    return item_list.at(0)->text();
}

void ConnectDialog::populateSlotList(const QString &signal)
{
    enum { deprecatedSlot = 0 };
    QString selectedName;
    if (const QListWidgetItem * item = m_ui.slotList->currentItem())
        selectedName = item->text();

    m_ui.slotList->clear();

    QMap<QString, QString> memberToClassName = getMatchingSlots(m_formWindow->core(), m_destination, signal, showAllSignalsSlots());

    QFont font = QApplication::font();
    font.setItalic(true);
    QVariant variantFont = QVariant::fromValue(font);

    QListWidgetItem *curr = 0;
    QMap<QString, QString>::ConstIterator itMember = memberToClassName.constBegin();
    const QMap<QString, QString>::ConstIterator itMemberEnd = memberToClassName.constEnd();
    while (itMember != itMemberEnd) {
        const QString member = itMember.key();
        QListWidgetItem *item = new QListWidgetItem(m_ui.slotList);
        item->setText(member);
        if (member == selectedName)
            curr = item;

        // Mark deprecated slots red. Not currently in use (historically for Qt 3 slots in Qt 4),
        // but may be used again in the future.
        if (deprecatedSlot) {
            item->setData(Qt::FontRole, variantFont);
            item->setData(Qt::ForegroundRole, QColor(Qt::red));
        }
        ++itMember;
    }

    if (curr)
        m_ui.slotList->setCurrentItem(curr);

    if (m_ui.slotList->selectedItems().isEmpty())
        setOkButtonEnabled(false);
}

void ConnectDialog::populateSignalList()
{
    enum { deprecatedSignal = 0 };

    QString selectedName;
    if (const QListWidgetItem *item = m_ui.signalList->currentItem())
        selectedName = item->text();

    m_ui.signalList->clear();

    QMap<QString, QString> memberToClassName = getSignals(m_formWindow->core(), m_source, showAllSignalsSlots());

    QFont font = QApplication::font();
    font.setItalic(true);
    QVariant variantFont = QVariant::fromValue(font);

    QListWidgetItem *curr = 0;
    QMap<QString, QString>::ConstIterator itMember = memberToClassName.constBegin();
    const QMap<QString, QString>::ConstIterator itMemberEnd = memberToClassName.constEnd();
    while (itMember != itMemberEnd) {
        const QString member = itMember.key();

        QListWidgetItem *item = new QListWidgetItem(m_ui.signalList);
        item->setText(member);
        if (!selectedName.isEmpty() && member == selectedName)
            curr = item;

        // Mark deprecated signals red. Not currently in use (historically for Qt 3 slots in Qt 4),
        // but may be used again in the future.
        if (deprecatedSignal) {
            item->setData(Qt::FontRole, variantFont);
            item->setData(Qt::ForegroundRole, QColor(Qt::red));
        }
        ++itMember;
    }

    if (curr) {
        m_ui.signalList->setCurrentItem(curr);
    } else {
        selectedName.clear();
    }

    populateSlotList(selectedName);
    if (!curr)
        m_ui.slotList->setEnabled(false);
}

void ConnectDialog::editSignals()
{
    editSignalsSlots(m_source, m_sourceMode, SignalSlotDialog::FocusSignals);
}

void ConnectDialog::editSlots()
{
    editSignalsSlots(m_destination, m_destinationMode, SignalSlotDialog::FocusSlots);
}

void ConnectDialog::editSignalsSlots(QWidget *w, WidgetMode mode, int signalSlotDialogModeInt)
{
    const SignalSlotDialog::FocusMode signalSlotDialogMode = static_cast<SignalSlotDialog::FocusMode>(signalSlotDialogModeInt);
    switch (mode) {
    case  NormalWidget:
        break;
    case MainContainer:
        if (SignalSlotDialog::editMetaDataBase(m_formWindow, w, this, signalSlotDialogMode))
            populateLists();
        break;
    case PromotedWidget:
        if (SignalSlotDialog::editPromotedClass(m_formWindow->core(), w, this, signalSlotDialogMode))
            populateLists();
        break;
    }
}

}

QT_END_NAMESPACE
