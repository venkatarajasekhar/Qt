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

#include <QDateTime>
#include <QTimeZone>
#include <QTest>
#include <qdebug.h>

class tst_QDateTime : public QObject
{
    Q_OBJECT

    enum
#if defined(Q_COMPILER_CLASS_ENUM) || (defined(Q_CC_MSVC) && _MSC_VER >= 1700)
        : qint64
#endif
    {
        SECS_PER_DAY = 86400,
        MSECS_PER_DAY = 86400000,
        JULIAN_DAY_1950 = 2433283,
        JULIAN_DAY_1960 = 2436935,
        JULIAN_DAY_2010 = 2455198,
        JULIAN_DAY_2011 = 2455563,
        JULIAN_DAY_2020 = 2458850,
        JULIAN_DAY_2050 = 2469808,
        JULIAN_DAY_2060 = 2473460
    };

private Q_SLOTS:
    void create();
    void isNull();
    void isValid();
    void date();
    void time();
    void timeSpec();
    void offsetFromUtc();
    void timeZoneAbbreviation();
    void toMSecsSinceEpoch();
    void toMSecsSinceEpoch1950();
    void toMSecsSinceEpoch2050();
    void toMSecsSinceEpochTz();
    void toMSecsSinceEpoch1950Tz();
    void toMSecsSinceEpoch2050Tz();
    void setDate();
    void setTime();
    void setTimeSpec();
    void setOffsetFromUtc();
    void setMSecsSinceEpoch();
    void setMSecsSinceEpochTz();
    void toString();
    void toStringTextFormat();
    void toStringIsoFormat();
    void addDays();
    void addDaysTz();
    void addMSecs();
    void addMSecsTz();
    void toTimeSpec();
    void toOffsetFromUtc();
    void daysTo();
    void msecsTo();
    void equivalent();
    void equivalentUtc();
    void lessThan();
    void lessThanUtc();
    void currentDateTime();
    void currentDate();
    void currentTime();
    void currentDateTimeUtc();
    void currentMSecsSinceEpoch();
    void fromString();
    void fromStringText();
    void fromStringIso();
    void fromMSecsSinceEpoch();
    void fromMSecsSinceEpochUtc();
    void fromMSecsSinceEpochTz();
};

void tst_QDateTime::create()
{
    QBENCHMARK {
        for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd) {
            QDateTime test(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0));
            Q_UNUSED(test)
        }
    }
}

void tst_QDateTime::isNull()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.isNull();
    }
}

void tst_QDateTime::isValid()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.isValid();
    }
}

void tst_QDateTime::date()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.date();
    }
}

void tst_QDateTime::time()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.time();
    }
}

void tst_QDateTime::timeSpec()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.timeSpec();
    }
}

void tst_QDateTime::offsetFromUtc()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.offsetFromUtc();
    }
}

void tst_QDateTime::timeZoneAbbreviation()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.timeZoneAbbreviation();
    }
}

void tst_QDateTime::toMSecsSinceEpoch()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toMSecsSinceEpoch();
    }
}

void tst_QDateTime::toMSecsSinceEpoch1950()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_1950; jd < JULIAN_DAY_1960; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toMSecsSinceEpoch();
    }
}

void tst_QDateTime::toMSecsSinceEpoch2050()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2050; jd < JULIAN_DAY_2060; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toMSecsSinceEpoch();
    }
}

void tst_QDateTime::toMSecsSinceEpochTz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0), cet));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            qint64 result = test.toMSecsSinceEpoch();
    }
}

void tst_QDateTime::toMSecsSinceEpoch1950Tz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_1950; jd < JULIAN_DAY_1960; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0), cet));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            qint64 result = test.toMSecsSinceEpoch();
    }
}

void tst_QDateTime::toMSecsSinceEpoch2050Tz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2050; jd < JULIAN_DAY_2060; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0), cet));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            qint64 result = test.toMSecsSinceEpoch();
    }
}

void tst_QDateTime::setDate()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (QDateTime test, list)
            test.setDate(QDate::fromJulianDay(JULIAN_DAY_2010));
    }
}

void tst_QDateTime::setTime()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (QDateTime test, list)
            test.setTime(QTime(12, 0, 0));
    }
}

void tst_QDateTime::setTimeSpec()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (QDateTime test, list)
            test.setTimeSpec(Qt::UTC);
    }
}

void tst_QDateTime::setOffsetFromUtc()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (QDateTime test, list)
            test.setOffsetFromUtc(3600);
    }
}

void tst_QDateTime::setMSecsSinceEpoch()
{
    qint64 msecs = qint64(JULIAN_DAY_2010 + 180) * MSECS_PER_DAY;
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (QDateTime test, list)
            test.setMSecsSinceEpoch(msecs);
    }
}

void tst_QDateTime::setMSecsSinceEpochTz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0), cet));
    QBENCHMARK {
        foreach (QDateTime test, list)
            test.setMSecsSinceEpoch((JULIAN_DAY_2010 + 180) * MSECS_PER_DAY);
    }
}

void tst_QDateTime::toString()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2011; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toString(QStringLiteral("yyy-MM-dd hh:mm:ss.zzz t"));
    }
}

void tst_QDateTime::toStringTextFormat()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2011; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toString(Qt::TextDate);
    }
}

void tst_QDateTime::toStringIsoFormat()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2011; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toString(Qt::ISODate);
    }
}

void tst_QDateTime::addDays()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.addDays(1);
    }
}

void tst_QDateTime::addDaysTz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0), cet));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            QDateTime result = test.addDays(1);
    }
}

void tst_QDateTime::addMSecs()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.addMSecs(1);
    }
}

void tst_QDateTime::addMSecsTz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0), cet));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            QDateTime result = test.addMSecs(1);
    }
}

void tst_QDateTime::toTimeSpec()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toTimeSpec(Qt::UTC);
    }
}

void tst_QDateTime::toOffsetFromUtc()
{
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.toOffsetFromUtc(3600);
    }
}

void tst_QDateTime::daysTo()
{
    QDateTime other = QDateTime::fromMSecsSinceEpoch(qint64(JULIAN_DAY_2010) * MSECS_PER_DAY);
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.daysTo(other);
    }
}

void tst_QDateTime::msecsTo()
{
    QDateTime other = QDateTime::fromMSecsSinceEpoch(qint64(JULIAN_DAY_2010) * MSECS_PER_DAY);
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            test.msecsTo(other);
    }
}

void tst_QDateTime::equivalent()
{
    bool result;
    QDateTime other = QDateTime::fromMSecsSinceEpoch(qint64(JULIAN_DAY_2010) * MSECS_PER_DAY);
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            result = (test == other);
    }
    Q_UNUSED(result)
}

void tst_QDateTime::equivalentUtc()
{
    bool result = false;
    QDateTime other = QDateTime::fromMSecsSinceEpoch(qint64(JULIAN_DAY_2010) * MSECS_PER_DAY, Qt::UTC);
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            result = (test == other);
    }
    Q_UNUSED(result)
}

void tst_QDateTime::lessThan()
{
    bool result = false;
    QDateTime other = QDateTime::fromMSecsSinceEpoch(qint64(JULIAN_DAY_2010) * MSECS_PER_DAY);
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            result = (test < other);
    }
    Q_UNUSED(result)
}

void tst_QDateTime::lessThanUtc()
{
    bool result = false;
    QDateTime other = QDateTime::fromMSecsSinceEpoch(qint64(JULIAN_DAY_2010) * MSECS_PER_DAY, Qt::UTC);
    QList<QDateTime> list;
    for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
        list.append(QDateTime(QDate::fromJulianDay(jd), QTime::fromMSecsSinceStartOfDay(0)));
    QBENCHMARK {
        foreach (const QDateTime &test, list)
            result = (test < other);
    }
    Q_UNUSED(result)
}

void tst_QDateTime::currentDateTime()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::currentDateTime();
    }
}

void tst_QDateTime::currentDate()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDate::currentDate();
    }
}

void tst_QDateTime::currentTime()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QTime::currentTime();
    }
}

void tst_QDateTime::currentDateTimeUtc()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::currentDateTimeUtc();
    }
}

void tst_QDateTime::currentMSecsSinceEpoch()
{
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::currentMSecsSinceEpoch();
    }
}

void tst_QDateTime::fromString()
{
    QString format = "yyy-MM-dd hh:mm:ss.zzz t";
    QString input = "2010-01-01 13:12:11.999 UTC";
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::fromString(input, format);
    }
}

void tst_QDateTime::fromStringText()
{
    QString input = "Wed Jan 2 01:02:03.000 2013 GMT";
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::fromString(input, Qt::TextDate);
    }
}

void tst_QDateTime::fromStringIso()
{
    QString input = "2010-01-01T13:28:34.999Z";
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i)
            QDateTime::fromString(input, Qt::ISODate);
    }
}

void tst_QDateTime::fromMSecsSinceEpoch()
{
    QBENCHMARK {
        for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
            QDateTime::fromMSecsSinceEpoch(jd * MSECS_PER_DAY, Qt::LocalTime);
    }
}

void tst_QDateTime::fromMSecsSinceEpochUtc()
{
    QBENCHMARK {
        for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
            QDateTime::fromMSecsSinceEpoch(jd * MSECS_PER_DAY, Qt::UTC);
    }
}

void tst_QDateTime::fromMSecsSinceEpochTz()
{
    QTimeZone cet = QTimeZone("Europe/Oslo");
    QBENCHMARK {
        for (int jd = JULIAN_DAY_2010; jd < JULIAN_DAY_2020; ++jd)
            QDateTime test = QDateTime::fromMSecsSinceEpoch(jd * MSECS_PER_DAY, cet);
    }
}

QTEST_MAIN(tst_QDateTime)

#include "main.moc"
