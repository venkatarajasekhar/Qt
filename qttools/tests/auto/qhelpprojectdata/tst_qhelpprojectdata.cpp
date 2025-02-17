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
#include <QtTest/QtTest>

#include <QtCore/QFileInfo>

#include <QtHelp/private/qhelpprojectdata_p.h>

class tst_QHelpProjectData : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void readData();
    void namespaceName();
    void virtualFolder();
    void customFilters();
    void filterSections();
    void metaData();
    void rootPath();

private:
    QString m_inputFile;
};

void tst_QHelpProjectData::initTestCase()
{
    const QString path = QLatin1String(SRCDIR);
    m_inputFile = path + QLatin1String("/data/test.qhp");
}

void tst_QHelpProjectData::readData()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot parse qhp file!");
}

void tst_QHelpProjectData::namespaceName()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot read qhp file!");
    QCOMPARE(data.namespaceName(), QString("trolltech.com.1.0.0.test"));
}

void tst_QHelpProjectData::virtualFolder()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot read qhp file!");
    QCOMPARE(data.virtualFolder(), QString("testFolder"));
}

void tst_QHelpProjectData::customFilters()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot read qhp file!");

    QList<QHelpDataCustomFilter> filters = data.customFilters();
    QCOMPARE(filters.count(), 2);

    foreach (QHelpDataCustomFilter f, filters) {
        if (f.name == QLatin1String("Custom Filter 1")) {
            foreach (QString id, f.filterAttributes) {
                if (id != QLatin1String("test")
                    && id != QLatin1String("filter1"))
                    QFAIL("Wrong filter attribute!");
            }
        } else if (f.name == QLatin1String("Custom Filter 2")) {
            foreach (QString id, f.filterAttributes) {
                if (id != QLatin1String("test")
                    && id != QLatin1String("filter2"))
                    QFAIL("Wrong filter attribute!");
            }
        } else {
            QFAIL("Unexpected filter name!");
        }
    }
}

void tst_QHelpProjectData::filterSections()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot read qhp file!");

    QList<QHelpDataFilterSection> sections = data.filterSections();
    QCOMPARE(sections.count(), 2);

    foreach (QHelpDataFilterSection s, sections) {
        if (s.filterAttributes().contains("filter1")) {
            QCOMPARE(s.indices().count(), 5);
            foreach (QHelpDataIndexItem idx, s.indices()) {
                if (idx.name == QLatin1String("foo")) {
                    QCOMPARE(idx.identifier, QString("Test::foo"));
                } else if (idx.name == QLatin1String("bar")) {
                    QCOMPARE(idx.reference, QString("test.html#bar"));
                } else if (idx.name == QLatin1String("bla")) {
                    QCOMPARE(idx.identifier, QString("Test::bla"));
                } else if (idx.name == QLatin1String("einstein")) {
                    QCOMPARE(idx.reference, QString("people.html#einstein"));
                } else if (idx.name == QLatin1String("newton")) {
                    QCOMPARE(idx.identifier, QString("People::newton"));
                } else {
                    QFAIL("Unexpected index!");
                }
            }
            QCOMPARE(s.contents().count(), 1);
            QCOMPARE(s.contents().first()->children().count(), 5);
        } else if (s.filterAttributes().contains("filter2")) {
            QCOMPARE(s.contents().count(), 1);
            QStringList lst;
            lst << "classic.css" << "fancy.html" << "cars.html";
            foreach (QString f, s.files())
                lst.removeAll(f);
            QCOMPARE(lst.count(), 0);
        } else {
            QFAIL("Unexpected filter attribute!");
        }
    }
}

void tst_QHelpProjectData::metaData()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot read qhp file!");

    QCOMPARE(data.metaData().count(), 2);
    QCOMPARE(data.metaData().value("author").toString(),
        QString("Digia Plc and/or its subsidiary(-ies)"));
}

void tst_QHelpProjectData::rootPath()
{
    QHelpProjectData data;
    if (!data.readData(m_inputFile))
        QFAIL("Cannot read qhp file!");

    QFileInfo fi(m_inputFile);
    QCOMPARE(data.rootPath(), fi.absolutePath());
}

QTEST_MAIN(tst_QHelpProjectData)
#include "tst_qhelpprojectdata.moc"
