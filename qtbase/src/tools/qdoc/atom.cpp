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

#include <qregexp.h>
#include "atom.h"
#include "location.h"
#include "qdocdatabase.h"
#include <stdio.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*! \class Atom
    \brief The Atom class is the fundamental unit for representing
    documents internally.

  Atoms have a \i type and are completed by a \i string whose
  meaning depends on the \i type. For example, the string
  \quotation
      \i italic text looks nicer than \bold bold text
  \endquotation
  is represented by the following atoms:
  \quotation
      (FormattingLeft, ATOM_FORMATTING_ITALIC)
      (String, "italic")
      (FormattingRight, ATOM_FORMATTING_ITALIC)
      (String, " text is more attractive than ")
      (FormattingLeft, ATOM_FORMATTING_BOLD)
      (String, "bold")
      (FormattingRight, ATOM_FORMATTING_BOLD)
      (String, " text")
  \endquotation

  \also Text
*/

/*! \enum Atom::Type

  \value AbstractLeft
  \value AbstractRight
  \value AnnotatedList
  \value AutoLink
  \value BaseName
  \value BriefLeft
  \value BriefRight
  \value C
  \value CaptionLeft
  \value CaptionRight
  \value Code
  \value CodeBad
  \value CodeNew
  \value CodeOld
  \value CodeQuoteArgument
  \value CodeQuoteCommand
  \value DivLeft
  \value DivRight
  \value EndQmlText
  \value FormatElse
  \value FormatEndif
  \value FormatIf
  \value FootnoteLeft
  \value FootnoteRight
  \value FormattingLeft
  \value FormattingRight
  \value GeneratedList
  \value Image
  \value ImageText
  \value ImportantNote
  \value InlineImage
  \value LineBreak
  \value Link
  \value LinkNode
  \value ListLeft
  \value ListItemNumber
  \value ListTagLeft
  \value ListTagRight
  \value ListItemLeft
  \value ListItemRight
  \value ListRight
  \value Nop
  \value Note
  \value ParaLeft
  \value ParaRight
  \value Qml
  \value QmlText
  \value QuotationLeft
  \value QuotationRight
  \value RawString
  \value SectionLeft
  \value SectionRight
  \value SectionHeadingLeft
  \value SectionHeadingRight
  \value SidebarLeft
  \value SidebarRight
  \value SinceList
  \value String
  \value TableLeft
  \value TableRight
  \value TableHeaderLeft
  \value TableHeaderRight
  \value TableRowLeft
  \value TableRowRight
  \value TableItemLeft
  \value TableItemRight
  \value TableOfContents
  \value Target
  \value UnhandledFormat
  \value UnknownCommand
*/

QString Atom::noError_ = QString();

static const struct {
    const char *english;
    int no;
} atms[] = {
    { "AbstractLeft", Atom::AbstractLeft },
    { "AbstractRight", Atom::AbstractRight },
    { "AnnotatedList", Atom::AnnotatedList },
    { "AutoLink", Atom::AutoLink },
    { "BaseName", Atom::BaseName },
    { "br", Atom::BR},
    { "BriefLeft", Atom::BriefLeft },
    { "BriefRight", Atom::BriefRight },
    { "C", Atom::C },
    { "CaptionLeft", Atom::CaptionLeft },
    { "CaptionRight", Atom::CaptionRight },
    { "Code", Atom::Code },
    { "CodeBad", Atom::CodeBad },
    { "CodeNew", Atom::CodeNew },
    { "CodeOld", Atom::CodeOld },
    { "CodeQuoteArgument", Atom::CodeQuoteArgument },
    { "CodeQuoteCommand", Atom::CodeQuoteCommand },
    { "DivLeft", Atom::DivLeft },
    { "DivRight", Atom::DivRight },
    { "EndQmlText", Atom::EndQmlText },
    { "FootnoteLeft", Atom::FootnoteLeft },
    { "FootnoteRight", Atom::FootnoteRight },
    { "FormatElse", Atom::FormatElse },
    { "FormatEndif", Atom::FormatEndif },
    { "FormatIf", Atom::FormatIf },
    { "FormattingLeft", Atom::FormattingLeft },
    { "FormattingRight", Atom::FormattingRight },
    { "GeneratedList", Atom::GeneratedList },
    { "GuidLink", Atom::GuidLink},
    { "hr", Atom::HR},
    { "Image", Atom::Image },
    { "ImageText", Atom::ImageText },
    { "ImportantLeft", Atom::ImportantLeft },
    { "ImportantRight", Atom::ImportantRight },
    { "InlineImage", Atom::InlineImage },
    { "JavaScript", Atom::JavaScript },
    { "EndJavaScript", Atom::EndJavaScript },
    { "LegaleseLeft", Atom::LegaleseLeft },
    { "LegaleseRight", Atom::LegaleseRight },
    { "LineBreak", Atom::LineBreak },
    { "Link", Atom::Link },
    { "LinkNode", Atom::LinkNode },
    { "ListLeft", Atom::ListLeft },
    { "ListItemNumber", Atom::ListItemNumber },
    { "ListTagLeft", Atom::ListTagLeft },
    { "ListTagRight", Atom::ListTagRight },
    { "ListItemLeft", Atom::ListItemLeft },
    { "ListItemRight", Atom::ListItemRight },
    { "ListRight", Atom::ListRight },
    { "Nop", Atom::Nop },
    { "NoteLeft", Atom::NoteLeft },
    { "NoteRight", Atom::NoteRight },
    { "ParaLeft", Atom::ParaLeft },
    { "ParaRight", Atom::ParaRight },
    { "Qml", Atom::Qml},
    { "QmlText", Atom::QmlText },
    { "QuotationLeft", Atom::QuotationLeft },
    { "QuotationRight", Atom::QuotationRight },
    { "RawString", Atom::RawString },
    { "SectionLeft", Atom::SectionLeft },
    { "SectionRight", Atom::SectionRight },
    { "SectionHeadingLeft", Atom::SectionHeadingLeft },
    { "SectionHeadingRight", Atom::SectionHeadingRight },
    { "SidebarLeft", Atom::SidebarLeft },
    { "SidebarRight", Atom::SidebarRight },
    { "SinceList", Atom::SinceList },
    { "SnippetCommand", Atom::SnippetCommand },
    { "SnippetIdentifier", Atom::SnippetIdentifier },
    { "SnippetLocation", Atom::SnippetLocation },
    { "String", Atom::String },
    { "TableLeft", Atom::TableLeft },
    { "TableRight", Atom::TableRight },
    { "TableHeaderLeft", Atom::TableHeaderLeft },
    { "TableHeaderRight", Atom::TableHeaderRight },
    { "TableRowLeft", Atom::TableRowLeft },
    { "TableRowRight", Atom::TableRowRight },
    { "TableItemLeft", Atom::TableItemLeft },
    { "TableItemRight", Atom::TableItemRight },
    { "TableOfContents", Atom::TableOfContents },
    { "Target", Atom::Target },
    { "UnhandledFormat", Atom::UnhandledFormat },
    { "UnknownCommand", Atom::UnknownCommand },
    { 0, 0 }
};

/*! \fn Atom::Atom(Type type, const QString& string)

  Constructs an atom of the specified \a type with the single
  parameter \a string and does not put the new atom in a list.
*/

/*! \fn Atom::Atom(Type type, const QString& p1, const QString& p2)

  Constructs an atom of the specified \a type with the two
  parameters \a p1 and \a p2 and does not put the new atom
  in a list.
*/

/*! \fn Atom(Atom *previous, Type type, const QString& string)

  Constructs an atom of the specified \a type with the single
  parameter \a string and inserts the new atom into the list
  after the \a previous atom.
*/

/*! \fn Atom::Atom(Atom* previous, Type type, const QString& p1, const QString& p2)

  Constructs an atom of the specified \a type with the two
  parameters \a p1 and \a p2 and inserts the new atom into
  the list after the \a previous atom.
*/

/*! \fn void Atom::appendChar(QChar ch)

  Appends \a ch to the string parameter of this atom.

  \also string()
*/

/*! \fn void Atom::appendString(const QString& string)

  Appends \a string to the string parameter of this atom.

  \also string()
*/

/*! \fn void Atom::chopString()

  \also string()
*/

/*! \fn Atom *Atom::next()
  Return the next atom in the atom list.
  \also type(), string()
*/

/*!
  Return the next Atom in the list if it is of Type \a t.
  Otherwise return 0.
 */
const Atom* Atom::next(Type t) const
{
    return (next_ && (next_->type() == t)) ? next_ : 0;
}

/*!
  Return the next Atom in the list if it is of Type \a t
  and its string part is \a s. Otherwise return 0.
 */
const Atom* Atom::next(Type t, const QString& s) const
{
    return (next_ && (next_->type() == t) && (next_->string() == s)) ? next_ : 0;
}

/*! \fn const Atom *Atom::next() const
  Return the next atom in the atom list.
  \also type(), string()
*/

/*! \fn Type Atom::type() const
  Return the type of this atom.
  \also string(), next()
*/

/*!
  Return the type of this atom as a string. Return "Invalid" if
  type() returns an impossible value.

  This is only useful for debugging.

  \also type()
*/
QString Atom::typeString() const
{
    static bool deja = false;

    if (!deja) {
        int i = 0;
        while (atms[i].english != 0) {
            if (atms[i].no != i)
                Location::internalError(QCoreApplication::translate("QDoc::Atom", "atom %1 missing").arg(i));
            i++;
        }
        deja = true;
    }

    int i = (int) type();
    if (i < 0 || i > (int) Last)
        return QLatin1String("Invalid");
    return QLatin1String(atms[i].english);
}

/*! \fn const QString& Atom::string() const

  Returns the string parameter that together with the type
  characterizes this atom.

  \also type(), next()
*/

/*!
  Dumps this Atom to stderr in printer friendly form.
 */
void Atom::dump() const
{
    QString str = string();
    str.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    str.replace(QLatin1String("\""), QLatin1String("\\\""));
    str.replace(QLatin1String("\n"), QLatin1String("\\n"));
    str.replace(QRegExp(QLatin1String("[^\x20-\x7e]")), QLatin1String("?"));
    if (!str.isEmpty())
        str = QLatin1String(" \"") + str + QLatin1Char('"');
    fprintf(stderr,
            "    %-15s%s\n",
            typeString().toLatin1().data(),
            str.toLatin1().data());
}

/*!
  The only constructor for LinkAtom. It creates an Atom of
  type Atom::Link. \a p1 being the link target. \a p2 is the
  parameters in square brackets. Normally there is just one
  word in the square brackets, but there can be up to three
  words separated by spaces. The constructor splits \a p2 on
  the space character.
 */
LinkAtom::LinkAtom(const QString& p1, const QString& p2)
    : Atom(p1), genus_(Node::DontCare), goal_(Node::NoType), domain_(0)
{
    QStringList params = p2.toLower().split(QLatin1Char(' '));
    foreach (const QString& p, params) {
        if (!domain_) {
            domain_ = QDocDatabase::qdocDB()->findTree(p);
            if (domain_)
                continue;
        }
        if (goal_ == Node::NoType) {
            goal_ = Node::goal(p);
            if (goal_ != Node::NoType)
                continue;
        }
        if (p == "qml") {
            genus_ = Node::QML;
            continue;
        }
        if (p == "cpp") {
            genus_ = Node::CPP;
            continue;
        }
        error_ = p2;
        break;
    }
}

/*!
  Standard copy constructor of LinkAtom \a t.
 */
LinkAtom::LinkAtom(const LinkAtom& t)
    : Atom(Link, t.string()),
      genus_(t.genus_),
      goal_(t.goal_),
      domain_(t.domain_),
      error_(t.error_)
{
    // nothing
}

/*!
  Special copy constructor of LinkAtom \a t, where
  where the new LinkAtom will not be the first one
  in the list.
 */
LinkAtom::LinkAtom(Atom* previous, const LinkAtom& t)
    : Atom(previous, Link, t.string()),
      genus_(t.genus_),
      goal_(t.goal_),
      domain_(t.domain_),
      error_(t.error_)
{
    previous->next_ = this;
}

QT_END_NAMESPACE
