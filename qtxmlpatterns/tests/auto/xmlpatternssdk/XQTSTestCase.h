/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef PatternistSDK_XQTSTestCase_H
#define PatternistSDK_XQTSTestCase_H

#include <QDate>
#include <QString>
#include <QUrl>

#include <private/qexternalvariableloader_p.h>

#include "TestBaseLine.h"
#include "TestCase.h"

QT_BEGIN_NAMESPACE

namespace QPatternistSDK
{
    /**
     * @short Represents a test case in a test suite in the XML Query Test Suite.
     *
     * TestCase is a memory representation of a test case, and maps
     * to the @c test-case element in the XQuery Test Suite test
     * case catalog.
     *
     * @ingroup PatternistSDK
     * @author Frans Englich <frans.englich@nokia.com>
     */
    class Q_PATTERNISTSDK_EXPORT XQTSTestCase : public TestCase
    {
    public:
        XQTSTestCase(const Scenario scen, TreeItem *parent,
                     const QXmlQuery::QueryLanguage lang = QXmlQuery::XQuery10);
        virtual ~XQTSTestCase();

        /**
         * The identifier, the name of the test. For example, "Literals034".
         * The name of a test case must be unique.
         */
        virtual QString name() const;
        virtual QString creator() const;
        virtual QString description() const;
        /**
         * @returns the query inside the file, specified by testCasePath(). Loading
         * of the file is not cached in order to avoid complications.
         * @param ok is set to @c false if loading the query file fails
         */
        virtual QString sourceCode(bool &ok) const;
        virtual QUrl testCasePath() const;
        virtual QDate lastModified() const;

        bool isXPath() const;

        /**
         * What kind of test case this is, what kind of scenario it takes part
         * of. For example, whether the test case should evaluate normally or fail.
         */
        Scenario scenario() const;

        void setCreator(const QString &creator);
        void setLastModified(const QDate &date);
        void setDescription(const QString &description);
        void setIsXPath(const bool isXPath);
        void setName(const QString &name);
        void setQueryPath(const QUrl &uri);
        void setContextItemSource(const QUrl &uri);
        void addBaseLine(TestBaseLine *lines);
        void setInitialTemplateName(const QXmlName &name);

        virtual TreeItem *parent() const;

        virtual QVariant data(const Qt::ItemDataRole role, int column) const;

        virtual QString title() const;
        virtual TestBaseLine::List baseLines() const;

        virtual int columnCount() const;

        void setExternalVariableLoader(const QPatternist::ExternalVariableLoader::Ptr &loader);
        virtual QPatternist::ExternalVariableLoader::Ptr externalVariableLoader() const;
        virtual QUrl contextItemSource() const;
        virtual QXmlQuery::QueryLanguage language() const;
        void setParent(TreeItem *const parent);
        virtual QXmlName initialTemplateName() const;

    private:
        QString                                     m_name;
        QString                                     m_creator;
        QString                                     m_description;
        QUrl                                        m_queryPath;
        bool                                        m_isXPath;
        QDate                                       m_lastModified;
        const Scenario                              m_scenario;
        TreeItem *                                  m_parent;
        TestBaseLine::List                          m_baseLines;
        QPatternist::ExternalVariableLoader::Ptr    m_externalVariableLoader;
        QUrl                                        m_contextItemSource;
        QXmlQuery::QueryLanguage                    m_lang;
        QXmlName                                    m_initialTemplateName;
    };
}

QT_END_NAMESPACE

#endif
// vim: et:ts=4:sw=4:sts=4
