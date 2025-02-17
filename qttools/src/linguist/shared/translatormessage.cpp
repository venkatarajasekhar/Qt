/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

#include "translatormessage.h"

#include <qplatformdefs.h>

#include <QDataStream>
#include <QDebug>

#include <stdlib.h>


QT_BEGIN_NAMESPACE

TranslatorMessage::TranslatorMessage()
  : m_lineNumber(-1), m_type(Unfinished), m_plural(false)
{
}

TranslatorMessage::TranslatorMessage(const QString &context,
    const QString &sourceText, const QString &comment,
    const QString &userData,
    const QString &fileName, int lineNumber, const QStringList &translations,
    Type type, bool plural)
  : m_context(context), m_sourcetext(sourceText), m_comment(comment),
    m_userData(userData),
    m_translations(translations), m_fileName(fileName), m_lineNumber(lineNumber),
    m_type(type), m_plural(plural)
{
}

void TranslatorMessage::addReference(const QString &fileName, int lineNumber)
{
    if (m_fileName.isEmpty()) {
        m_fileName = fileName;
        m_lineNumber = lineNumber;
    } else {
        m_extraRefs.append(Reference(fileName, lineNumber));
    }
}

void TranslatorMessage::addReferenceUniq(const QString &fileName, int lineNumber)
{
    if (m_fileName.isEmpty()) {
        m_fileName = fileName;
        m_lineNumber = lineNumber;
    } else {
        if (fileName == m_fileName && lineNumber == m_lineNumber)
            return;
        if (!m_extraRefs.isEmpty()) // Rather common case, so special-case it
            foreach (const Reference &ref, m_extraRefs)
                if (fileName == ref.fileName() && lineNumber == ref.lineNumber())
                    return;
        m_extraRefs.append(Reference(fileName, lineNumber));
    }
}

void TranslatorMessage::clearReferences()
{
    m_fileName.clear();
    m_lineNumber = -1;
    m_extraRefs.clear();
}

void TranslatorMessage::setReferences(const TranslatorMessage::References &refs0)
{
    if (!refs0.isEmpty()) {
        References refs = refs0;
        const Reference &ref = refs.takeFirst();
        m_fileName = ref.fileName();
        m_lineNumber = ref.lineNumber();
        m_extraRefs = refs;
    } else {
        clearReferences();
    }
}

TranslatorMessage::References TranslatorMessage::allReferences() const
{
    References refs;
    if (!m_fileName.isEmpty()) {
        refs.append(Reference(m_fileName, m_lineNumber));
        refs += m_extraRefs;
    }
    return refs;
}


bool TranslatorMessage::hasExtra(const QString &key) const
{
    return m_extra.contains(key);
}

QString TranslatorMessage::extra(const QString &key) const
{
    return m_extra[key];
}

void TranslatorMessage::setExtra(const QString &key, const QString &value)
{
    m_extra[key] = value;
}

void TranslatorMessage::unsetExtra(const QString &key)
{
    m_extra.remove(key);
}

void TranslatorMessage::dump() const
{
    qDebug()
        << "\nId                : " << m_id
        << "\nContext           : " << m_context
        << "\nSource            : " << m_sourcetext
        << "\nComment           : " << m_comment
        << "\nUserData          : " << m_userData
        << "\nExtraComment      : " << m_extraComment
        << "\nTranslatorComment : " << m_translatorComment
        << "\nTranslations      : " << m_translations
        << "\nFileName          : " << m_fileName
        << "\nLineNumber        : " << m_lineNumber
        << "\nType              : " << m_type
        << "\nPlural            : " << m_plural
        << "\nExtra             : " << m_extra;
}


QT_END_NAMESPACE
