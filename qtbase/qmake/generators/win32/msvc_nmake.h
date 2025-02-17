/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef MSVC_NMAKE_H
#define MSVC_NMAKE_H

#include "winmakefile.h"

QT_BEGIN_NAMESPACE

class NmakeMakefileGenerator : public Win32MakefileGenerator
{
    bool init_flag;
    void writeNmakeParts(QTextStream &);
    bool writeMakefile(QTextStream &);
    void writeImplicitRulesPart(QTextStream &t);
    void writeBuildRulesPart(QTextStream &t);
    void writeLinkCommand(QTextStream &t, const QString &extraFlags = QString(), const QString &extraInlineFileContent = QString());
    int msvcVersion() const;
    void init();

protected:
    virtual void writeSubMakeCall(QTextStream &t, const QString &callPrefix,
                                  const QString &makeArguments);
    virtual QString getPdbTarget();
    virtual QString defaultInstall(const QString &t);
    virtual QStringList &findDependencies(const QString &file);
    QString var(const ProKey &value) const;
    QString precompH, precompObj, precompPch;
    bool usePCH;

public:
    NmakeMakefileGenerator();
    ~NmakeMakefileGenerator();

};

inline NmakeMakefileGenerator::~NmakeMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // MSVC_NMAKE_H
