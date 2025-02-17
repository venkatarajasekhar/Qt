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

#include <QDate>
#include <QtDebug>

#include "UserTestCase.h"
#include "ui_ui_BaseLinePage.h"

#include "TestCaseView.h"

using namespace QPatternistSDK;

/**
 * Removes all tabs in @p widget, and deletes its page widgets.
 */
static void clearTabWidget(QTabWidget *const widget)
{
    Q_ASSERT(widget);

    /* Very annoying, we can't cache count(). */
    /* Idea: QTabWidget::clear(bool deletePageWidgets = false); */
    while(widget->count() != 0)
    {
        delete widget->widget(0);
        widget->removeTab(0);
    }

    Q_ASSERT(widget->count() == 0);
}

TestCaseView::TestCaseView(QWidget *const p) : QDockWidget(QLatin1String("Test Case View"), p)
{
    setObjectName(QLatin1String("TestCaseView"));
    setWidget(new QWidget());
    setupUi(widget());
    displayNone();
    clearTabWidget(baselinesTabs);
}

void TestCaseView::displayNone()
{
    stackedWidget->setCurrentIndex(1);
}

void TestCaseView::displayTestCase(TestCase *const tc)
{
    if(!tc)
    {
        displayNone();
        return;
    }

    name->setText(tc->title());
    description->setText(tc->description());
    isXPath->setText(tc->isXPath() ? QLatin1String("yes") : QLatin1String("no"));
    author->setText(tc->creator());
    type->setText(TestCase::displayName(tc->scenario()));
    if(tc->lastModified().isNull())
        lastModified->setText(QLatin1String("Not specified."));
    else
        lastModified->setText(tc->lastModified().toString());

    if(tc->contextItemSource().isValid())
        focusDocument->setText(tc->contextItemSource().toLocalFile());

    /* Not used. */
    bool ok = false;

    const QString sourceCode(tc->sourceCode(ok));

    if(sourceCode.isEmpty())
        sourceEdit->setPlainText(QLatin1String("No source code available."));
    else
        sourceEdit->setPlainText(sourceCode);

    stackedWidget->setCurrentIndex(0);

    displayBaseLines(tc);
}

void TestCaseView::displayBaseLines(const TestCase *const tc)
{
    clearTabWidget(baselinesTabs);
    Q_ASSERT(tc);
    const TestBaseLine::List bs(tc->baseLines());
    const TestBaseLine::List::const_iterator end(bs.constEnd());
    TestBaseLine::List::const_iterator it(bs.constBegin());

    for(; it != end; ++it)
    {
        const TestBaseLine *const bl = *it;
        Q_ASSERT(bl);
        const TestBaseLine::Type t = bl->type();

        QString title(TestBaseLine::displayName(t));
        const QString details(bl->details());

        QWidget *const currPage = new QWidget();
        Ui::BaseLinePage setupPage;
        setupPage.setupUi(currPage);

        /* Make this title a bit better: "ExpectedError: XPTY0004", for example. */
        switch(t)
        {
            case TestBaseLine::ExpectedError:
            {
                title += (QLatin1String(": ") + details);
                /* Fallthrough. */
            }
            case TestBaseLine::Ignore:
            {
                setupPage.contentEdit->setEnabled(false);
                break;
            }
            default:
            {
                setupPage.contentEdit->setPlainText(details);
                break;
            }
        }

        baselinesTabs->addTab(currPage, title);
    }

    int tabIndex = baselinesTabs->count(); /* The tab we're about to add. */
    baselinesTabs->addTab(new QWidget(), QLatin1String("AST Baseline"));
    baselinesTabs->setTabToolTip(tabIndex,
                                 QLatin1String("Expected AST baselines are not yet implemented."));
    baselinesTabs->setTabEnabled(tabIndex, false);

    ++tabIndex; /* Again, the tab we're about to add. */
    baselinesTabs->addTab(new QWidget(), QLatin1String("Message Baseline"));
    baselinesTabs->setTabToolTip(tabIndex,
                                 QLatin1String("Expected Message baselines are not yet implemented."));
    baselinesTabs->setTabEnabled(tabIndex, false);

    baselinesTabs->setCurrentIndex(0);
}

// vim: et:ts=4:sw=4:sts=4
