/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtXmlPatterns module of the Qt Toolkit.
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

#include "qabstractdatetime_p.h"
#include "qcontextfns_p.h"
#include "qdate_p.h"
#include "qschemadatetime_p.h"
#include "qdaytimeduration_p.h"
#include "qpatternistlocale_p.h"
#include "qschematime_p.h"

#include "qtimezonefns_p.h"

QT_BEGIN_NAMESPACE

using namespace QPatternist;

Item AdjustTimezone::evaluateSingleton(const DynamicContext::Ptr &context) const
{
    enum
    {
        /**
         * The maximum zone offset, @c PT14H, in milli seconds.
         */
        MSecLimit = 14 * 60/*M*/ * 60/*S*/ * 1000/*ms*/
    };


    const Item arg(m_operands.first()->evaluateSingleton(context));
    if(!arg)
        return Item();

    QDateTime dt(arg.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(false);
    Q_ASSERT(dt.isValid());
    DayTimeDuration::Ptr tz;

    if(m_operands.count() == 2)
        tz = DayTimeDuration::Ptr(m_operands.at(1)->evaluateSingleton(context).as<DayTimeDuration>());
    else
        tz = context->implicitTimezone();

    if(tz)
    {
        const MSecondCountProperty tzMSecs = tz->value();

        if(tzMSecs % (1000 * 60) != 0)
        {
            context->error(QtXmlPatterns::tr("A zone offset must be in the "
                                             "range %1..%2 inclusive. %3 is "
                                             "out of range.")
                           .arg(formatData("-PT14H"))
                           .arg(formatData("PT14H"))
                           .arg(formatData(tz->stringValue())),
                           ReportContext::FODT0003, this);
            return Item();
        }
        else if(tzMSecs > MSecLimit ||
                tzMSecs < -MSecLimit)
        {
            context->error(QtXmlPatterns::tr("%1 is not a whole number of minutes.")
                                            .arg(formatData(tz->stringValue())),
                                       ReportContext::FODT0003, this);
            return Item();
        }

        const SecondCountProperty tzSecs = tzMSecs / 1000;

        if(dt.timeSpec() == Qt::LocalTime) /* $arg has no time zone. */
        {
            /* "If $arg does not have a timezone component and $timezone is not
             * the empty sequence, then the result is $arg with $timezone as
             * the timezone component." */
            //dt.setTimeSpec(QDateTime::Spec(QDateTime::OffsetFromUTC, tzSecs));
            dt.setOffsetFromUtc(tzSecs);
            Q_ASSERT(dt.isValid());
            return createValue(dt);
        }
        else
        {
            /* "If $arg has a timezone component and $timezone is not the empty sequence,
             * then the result is an xs:dateTime value with a timezone component of
             * $timezone that is equal to $arg." */
            dt = dt.toUTC();
            dt = dt.addSecs(tzSecs);
            //dt.setTimeSpec(QDateTime::Spec(QDateTime::OffsetFromUTC, tzSecs));
            dt.setOffsetFromUtc(tzSecs);
            Q_ASSERT(dt.isValid());
            return createValue(dt);
        }
    }
    else
    { /* $timezone is the empty sequence. */
        if(dt.timeSpec() == Qt::LocalTime) /* $arg has no time zone. */
        {
            /* "If $arg does not have a timezone component and $timezone is
             * the empty sequence, then the result is $arg." */
            return arg;
        }
        else
        {
            /* "If $arg has a timezone component and $timezone is the empty sequence,
             * then the result is the localized value of $arg without its timezone component." */
            dt.setTimeSpec(Qt::LocalTime);
            return createValue(dt);
        }
    }
}

Item AdjustDateTimeToTimezoneFN::createValue(const QDateTime &dt) const
{
    Q_ASSERT(dt.isValid());
    return DateTime::fromDateTime(dt);
}

Item AdjustDateToTimezoneFN::createValue(const QDateTime &dt) const
{
    Q_ASSERT(dt.isValid());
    return Date::fromDateTime(dt);
}

Item AdjustTimeToTimezoneFN::createValue(const QDateTime &dt) const
{
    Q_ASSERT(dt.isValid());
    return SchemaTime::fromDateTime(dt);
}

QT_END_NAMESPACE
