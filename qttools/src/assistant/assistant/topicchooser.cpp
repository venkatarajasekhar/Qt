/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Assistant of the Qt Toolkit.
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
#include "tracer.h"

#include "topicchooser.h"
#include "helpenginewrapper.h"

#include <QKeyEvent>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QUrl>

QT_BEGIN_NAMESPACE

TopicChooser::TopicChooser(QWidget *parent, const QString &keyword, const QMap<QString, QUrl> &links)
    : QDialog(parent)
    , m_filterModel(new QSortFilterProxyModel(this))
{
    TRACE_OBJ
    ui.setupUi(this);

    setFocusProxy(ui.lineEdit);
    ui.lineEdit->installEventFilter(this);
    ui.lineEdit->setPlaceholderText(tr("Filter"));
    ui.label->setText(tr("Choose a topic for <b>%1</b>:").arg(keyword));

    QStandardItemModel *model = new QStandardItemModel(this);
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QMap<QString, QUrl>::const_iterator it = links.constBegin();
    for (; it != links.constEnd(); ++it) {
        m_links.append(it.value());
        QStandardItem *item = new QStandardItem(it.key());
        item->setToolTip(it.value().toString());
        model->appendRow(item);
    }

    ui.listWidget->setModel(m_filterModel);
    ui.listWidget->setUniformItemSizes(true);
    ui.listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (m_filterModel->rowCount() != 0)
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));

    connect(ui.buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui.buttonDisplay, SIGNAL(clicked()), this, SLOT(acceptDialog()));
    connect(ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));
    connect(ui.listWidget, SIGNAL(activated(QModelIndex)), this, SLOT(activated(QModelIndex)));

    const QByteArray ba = HelpEngineWrapper::instance().topicChooserGeometry();
    if (!ba.isEmpty())
        restoreGeometry(ba);
}

TopicChooser::~TopicChooser()
{
    HelpEngineWrapper::instance().setTopicChooserGeometry(saveGeometry());
}

QUrl TopicChooser::link() const
{
    TRACE_OBJ
    if (m_activedIndex.isValid())
        return m_links.at(m_filterModel->mapToSource(m_activedIndex).row());
    return QUrl();
}

void TopicChooser::acceptDialog()
{
    TRACE_OBJ
    m_activedIndex = ui.listWidget->currentIndex();
    accept();
}

void TopicChooser::setFilter(const QString &pattern)
{
    TRACE_OBJ
    m_filterModel->setFilterFixedString(pattern);
    if (m_filterModel->rowCount() != 0 && !ui.listWidget->currentIndex().isValid())
        ui.listWidget->setCurrentIndex(m_filterModel->index(0, 0));
}

void TopicChooser::activated(const QModelIndex &index)
{
    TRACE_OBJ
    m_activedIndex = index;
    accept();
}

bool TopicChooser::eventFilter(QObject *object, QEvent *event)
{
    TRACE_OBJ
    if (object == ui.lineEdit && event->type() == QEvent::KeyPress) {
        QModelIndex idx = ui.listWidget->currentIndex();
        switch ((static_cast<QKeyEvent*>(event)->key())) {
            case Qt::Key_Up:
                idx = m_filterModel->index(idx.row() - 1, idx.column(),
                    idx.parent());
                if (idx.isValid())
                    ui.listWidget->setCurrentIndex(idx);
                break;

            case Qt::Key_Down:
                idx = m_filterModel->index(idx.row() + 1, idx.column(),
                    idx.parent());
                if (idx.isValid())
                    ui.listWidget->setCurrentIndex(idx);
                break;

            default: ;
        }
    } else if (ui.lineEdit && event->type() == QEvent::FocusIn
        && static_cast<QFocusEvent *>(event)->reason() != Qt::MouseFocusReason) {
            ui.lineEdit->selectAll();
            ui.lineEdit->setFocus();
    }
    return QDialog::eventFilter(object, event);
}

QT_END_NAMESPACE
