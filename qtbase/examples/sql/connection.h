/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

/*
    This file defines a helper function to open a connection to an
    in-memory SQLITE database and to create a test table.

    If you want to use another database, simply modify the code
    below. All the examples in this directory use this function to
    connect to a database.
*/
//! [0]
static bool createConnection()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
            qApp->tr("Unable to establish a database connection.\n"
                     "This example needs SQLite support. Please read "
                     "the Qt SQL driver documentation for information how "
                     "to build it.\n\n"
                     "Click Cancel to exit."), QMessageBox::Cancel);
        return false;
    }

    QSqlQuery query;
    query.exec("create table person (id int primary key, "
               "firstname varchar(20), lastname varchar(20))");
    query.exec("insert into person values(101, 'Danny', 'Young')");
    query.exec("insert into person values(102, 'Christine', 'Holand')");
    query.exec("insert into person values(103, 'Lars', 'Gordon')");
    query.exec("insert into person values(104, 'Roberto', 'Robitaille')");
    query.exec("insert into person values(105, 'Maria', 'Papadopoulos')");

    query.exec("create table items (id int primary key,"
                                             "imagefile int,"
                                             "itemtype varchar(20),"
                                             "description varchar(100))");
    query.exec("insert into items "
               "values(0, 0, 'Qt',"
               "'Qt is a full development framework with tools designed to "
               "streamline the creation of stunning applications and  "
               "amazing user interfaces for desktop, embedded and mobile "
               "platforms.')");
    query.exec("insert into items "
               "values(1, 1, 'Qt Quick',"
               "'Qt Quick is a collection of techniques designed to help "
               "developers create intuitive, modern-looking, and fluid "
               "user interfaces using a CSS & JavaScript like language.')");
    query.exec("insert into items "
               "values(2, 2, 'Qt Creator',"
               "'Qt Creator is a powerful cross-platform integrated "
               "development environment (IDE), including UI design tools "
               "and on-device debugging.')");
    query.exec("insert into items "
               "values(3, 3, 'Qt Project',"
               "'The Qt Project governs the open source development of Qt, "
               "allowing anyone wanting to contribute to join the effort "
               "through a meritocratic structure of approvers and "
               "maintainers.')");

    query.exec("create table images (itemid int, file varchar(20))");
    query.exec("insert into images values(0, 'images/qt-logo.png')");
    query.exec("insert into images values(1, 'images/qt-quick.png')");
    query.exec("insert into images values(2, 'images/qt-creator.png')");
    query.exec("insert into images values(3, 'images/qt-project.png')");

    return true;
}
//! [0]

#endif
