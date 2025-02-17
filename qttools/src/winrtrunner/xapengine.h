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

#ifndef XAPENGINE_H
#define XAPENGINE_H

#include "runnerengine.h"
#include "runner.h"

#include <QtCore/QScopedPointer>

QT_USE_NAMESPACE

class XapEnginePrivate;
class XapEngine : public RunnerEngine
{
public:
    static bool canHandle(Runner *runner);
    static RunnerEngine *create(Runner *runner);
    static QStringList deviceNames();

    bool install(bool removeFirst = false) Q_DECL_OVERRIDE;
    bool remove() Q_DECL_OVERRIDE;
    bool start() Q_DECL_OVERRIDE;
    bool enableDebugging(const QString &debuggerExecutable,
                        const QString &debuggerArguments) Q_DECL_OVERRIDE;
    bool disableDebugging() Q_DECL_OVERRIDE;
    bool suspend() Q_DECL_OVERRIDE;
    bool waitForFinished(int secs) Q_DECL_OVERRIDE;
    bool stop() Q_DECL_OVERRIDE;
    qint64 pid() const Q_DECL_OVERRIDE;
    int exitCode() const Q_DECL_OVERRIDE;

    QString executable() const Q_DECL_OVERRIDE;
    QString devicePath(const QString &relativePath) const Q_DECL_OVERRIDE;
    bool sendFile(const QString &localFile, const QString &deviceFile) Q_DECL_OVERRIDE;
    bool receiveFile(const QString &deviceFile, const QString &localFile) Q_DECL_OVERRIDE;

private:
    explicit XapEngine(Runner *runner);
    ~XapEngine();

    uint fetchPid();
    bool connect();

    QScopedPointer<XapEnginePrivate> d_ptr;
    Q_DECLARE_PRIVATE(XapEngine)
};

#endif // XAPENGINE_H
