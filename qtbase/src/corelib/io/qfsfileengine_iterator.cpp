/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qfsfileengine_iterator_p.h"
#include "qfileinfo_p.h"
#include "qvariant.h"

#ifndef QT_NO_FILESYSTEMITERATOR

QT_BEGIN_NAMESPACE

QFSFileEngineIterator::QFSFileEngineIterator(QDir::Filters filters, const QStringList &filterNames)
    : QAbstractFileEngineIterator(filters, filterNames)
    , done(false)
{
}

QFSFileEngineIterator::~QFSFileEngineIterator()
{
}

bool QFSFileEngineIterator::hasNext() const
{
    if (!done && !nativeIterator) {
        nativeIterator.reset(new QFileSystemIterator(QFileSystemEntry(path()),
                    filters(), nameFilters()));
        advance();
    }

    return !done;
}

QString QFSFileEngineIterator::next()
{
    if (!hasNext())
        return QString();

    advance();
    return currentFilePath();
}

void QFSFileEngineIterator::advance() const
{
    currentInfo = nextInfo;

    QFileSystemEntry entry;
    QFileSystemMetaData data;
    if (nativeIterator->advance(entry, data)) {
        nextInfo = QFileInfo(new QFileInfoPrivate(entry, data));
    } else {
        done = true;
        nativeIterator.reset();
    }
}

QString QFSFileEngineIterator::currentFileName() const
{
    return currentInfo.fileName();
}

QFileInfo QFSFileEngineIterator::currentFileInfo() const
{
    return currentInfo;
}

QT_END_NAMESPACE

#endif // QT_NO_FILESYSTEMITERATOR
