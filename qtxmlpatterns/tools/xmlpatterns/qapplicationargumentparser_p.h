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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QApplicationArgumentParser_H
#define QApplicationArgumentParser_H

#include <QtCore/QVariant> /* Needed, because we can't forward declare QVariantList. */

QT_BEGIN_NAMESPACE

class QApplicationArgument;
class QApplicationArgumentParserPrivate;
class QStringList;
template<typename A, typename B> struct QPair;
template<typename T> class QList;
template<typename Value> class QList;

class QApplicationArgumentParser
{
public:
    enum ExitCode
    {
        Success = 0,
        ParseError = 1
    };

    QApplicationArgumentParser(int argc, char **argv);
    QApplicationArgumentParser(const QStringList &input);
    virtual ~QApplicationArgumentParser();
    void addArgument(const QApplicationArgument &argument);
    void setDeclaredArguments(const QList<QApplicationArgument> &arguments);
    QList<QApplicationArgument> declaredArguments() const;

    int count(const QApplicationArgument &argument) const;
    bool has(const QApplicationArgument &argument) const;

    virtual bool parse();
    ExitCode exitCode() const;
    QVariant value(const QApplicationArgument &argument) const;
    QVariantList values(const QApplicationArgument &argument) const;
    void setApplicationDescription(const QString &description);
    void setApplicationVersion(const QString &version);
    virtual void message(const QString &message) const;

protected:
    void setExitCode(ExitCode code);
    void setUsedArguments(const QList<QPair<QApplicationArgument, QVariant> > &arguments);
    QList<QPair<QApplicationArgument, QVariant> > usedArguments() const;
    QStringList input() const;
    virtual QVariant convertToValue(const QApplicationArgument &argument,
                                    const QString &value) const;
    virtual QString typeToName(const QApplicationArgument &argument) const;
    virtual QVariant defaultValue(const QApplicationArgument &argument) const;

private:
    friend class QApplicationArgumentParserPrivate;
    QApplicationArgumentParserPrivate *d;
    Q_DISABLE_COPY(QApplicationArgumentParser)
};

QT_END_NAMESPACE

#endif
