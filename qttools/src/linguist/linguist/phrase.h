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

#ifndef PHRASE_H
#define PHRASE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QtCore/QLocale>

QT_BEGIN_NAMESPACE

class PhraseBook;

class Phrase
{
public:
    Phrase();
    Phrase(const QString &source, const QString &target,
            const QString &definition, int sc = -1);
    Phrase(const QString &source, const QString &target,
            const QString &definition, PhraseBook *phraseBook);

    QString source() const { return s; }
    void setSource(const QString &ns);
    QString target() const {return t;}
    void setTarget(const QString &nt);
    QString definition() const {return d;}
    void setDefinition (const QString &nd);
    int shortcut() const { return shrtc; }
    PhraseBook *phraseBook() const { return m_phraseBook; }
    void setPhraseBook(PhraseBook *book) { m_phraseBook = book; }

private:
    int shrtc;
    QString s;
    QString t;
    QString d;
    PhraseBook *m_phraseBook;
};

bool operator==(const Phrase &p, const Phrase &q);
inline bool operator!=(const Phrase &p, const Phrase &q) {
    return !(p == q);
}

class QphHandler;

class PhraseBook : public QObject
{
    Q_OBJECT

public:
    PhraseBook();
    ~PhraseBook();
    bool load(const QString &fileName, bool *langGuessed);
    bool save(const QString &fileName);
    QList<Phrase *> phrases() const { return m_phrases; }
    void append(Phrase *phrase);
    void remove(Phrase *phrase);
    QString fileName() const { return m_fileName; }
    QString friendlyPhraseBookName() const;
    bool isModified() const { return m_changed; }

    void setLanguageAndCountry(QLocale::Language lang, QLocale::Country country);
    QLocale::Language language() const { return m_language; }
    QLocale::Country country() const { return m_country; }
    void setSourceLanguageAndCountry(QLocale::Language lang, QLocale::Country country);
    QLocale::Language sourceLanguage() const { return m_sourceLanguage; }
    QLocale::Country sourceCountry() const { return m_sourceCountry; }

signals:
    void modifiedChanged(bool changed);
    void listChanged();

private:
    // Prevent copying
    PhraseBook(const PhraseBook &);
    PhraseBook& operator=(const PhraseBook &);

    void setModified(bool modified);
    void phraseChanged(Phrase *phrase);

    QList<Phrase *> m_phrases;
    QString m_fileName;
    bool m_changed;

    QLocale::Language m_language;
    QLocale::Language m_sourceLanguage;
    QLocale::Country m_country;
    QLocale::Country m_sourceCountry;

    friend class QphHandler;
    friend class Phrase;
};

QT_END_NAMESPACE

#endif
