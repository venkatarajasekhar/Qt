/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  window.cpp

  A minimal subclass of QTableView with slots to allow the selection model
  to be monitored.
*/

#include <QAbstractItemModel>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

#include "model.h"
#include "window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Selected Items in a Table Model");

    model = new TableModel(8, 4, this);

    table = new QTableView(this);
    table->setModel(model);

    QMenu *actionMenu = new QMenu(tr("&Actions"), this);
    QAction *fillAction = actionMenu->addAction(tr("&Fill Selection"));
    QAction *clearAction = actionMenu->addAction(tr("&Clear Selection"));
    QAction *selectAllAction = actionMenu->addAction(tr("&Select All"));
    menuBar()->addMenu(actionMenu);

    connect(fillAction, SIGNAL(triggered()), this, SLOT(fillSelection()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clearSelection()));
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));

    selectionModel = table->selectionModel();

    statusBar();
    setCentralWidget(table);
}

void MainWindow::fillSelection()
{
//! [0]
    QModelIndexList indexes = selectionModel->selectedIndexes();
    QModelIndex index;

    foreach(index, indexes) {
        QString text = QString("(%1,%2)").arg(index.row()).arg(index.column());
        model->setData(index, text);
    }
//! [0]
}

void MainWindow::clearSelection()
{
    QModelIndexList indexes = selectionModel->selectedIndexes();
    QModelIndex index;

    foreach(index, indexes)
        model->setData(index, "");
}

void MainWindow::selectAll()
{
//! [1]
    QModelIndex parent = QModelIndex();
//! [1] //! [2]
    QModelIndex topLeft = model->index(0, 0, parent);
    QModelIndex bottomRight = model->index(model->rowCount(parent)-1,
        model->columnCount(parent)-1, parent);
//! [2]

//! [3]
    QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, QItemSelectionModel::Select);
//! [3]
}
