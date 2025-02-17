/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
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

#include "qdeclarativeradiodata_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype RadioData
    \instantiates QDeclarativeRadioData
    \inqmlmodule QtMultimedia
    \brief Access RDS data from a QML application.
    \ingroup multimedia_qml
    \ingroup multimedia_radio_qml
    \inherits Item

    This type is part of the \b{QtMultimedia 5.0} module.

    \c RadioData is your gateway to all the data available through RDS. RDS is the Radio Data System
    which allows radio stations to broadcast information like the \l stationId, \l programType, \l programTypeName,
    \l stationName, and \l radioText. This information can be read from the \c RadioData. It also allows
    you to set whether the radio should tune to alternative frequencies if the current signal strength falls too much.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Rectangle {
        width: 480
        height: 320

        Radio {
            id: radio
            band: Radio.FM
        }

        Column {
            Text {
                text: radio.radioData.stationName
            }

            Text {
                text: radio.radioData.programTypeName
            }

            Text {
                text: radio.radioData.radioText
            }
        }
    }

    \endqml

    You use \c RadioData together with a \l Radio, either by
    accessing the \c radioData property of the Radio, or
    creating a separate RadioData. The properties of the
    RadioData type will reflect the information broadcast by the
    radio station the Radio is currently tuned to.

    \sa {Radio Overview}
*/
QDeclarativeRadioData::QDeclarativeRadioData(QObject *parent) :
    QObject(parent)
{
    m_radioTuner = new QRadioTuner(this);
    m_radioData = m_radioTuner->radioData();

    connectSignals();
}

QDeclarativeRadioData::QDeclarativeRadioData(QRadioTuner *tuner, QObject *parent) :
    QObject(parent)
{
    m_radioTuner = tuner;
    m_radioData = m_radioTuner->radioData();

    connectSignals();
}

QDeclarativeRadioData::~QDeclarativeRadioData()
{
}

/*!
    \qmlproperty enumeration QtMultimedia::RadioData::availability

    Returns the availability state of the radio data interface.

    This is one of:

    \table
    \header \li Value \li Description
    \row \li Available
        \li The radio data interface is available to use
    \row \li Busy
        \li The radio data interface is usually available to use, but is currently busy.
    \row \li Unavailable
        \li The radio data interface is not available to use (there may be no radio
           hardware)
    \row \li ResourceMissing
        \li There is one or more resources missing, so the radio cannot
           be used.  It may be possible to try again at a later time.
    \endtable
 */
QDeclarativeRadioData::Availability QDeclarativeRadioData::availability() const
{
    return Availability(m_radioData->availability());
}


/*!
    \qmlproperty string QtMultimedia::RadioData::stationId

    This property allows you to read the station Id of the currently tuned radio
    station.
  */
QString QDeclarativeRadioData::stationId() const
{
    return m_radioData->stationId();
}

/*!
    \qmlproperty enumeration QtMultimedia::RadioData::programType

    This property holds the type of the currently playing program as transmitted
    by the radio station. The value can be any one of the values defined in the
    table below.

    \table
    \header \li Value
        \row \li Undefined
        \row \li News
        \row \li CurrentAffairs
        \row \li Information
        \row \li Sport
        \row \li Education
        \row \li Drama
        \row \li Culture
        \row \li Science
        \row \li Varied
        \row \li PopMusic
        \row \li RockMusic
        \row \li EasyListening
        \row \li LightClassical
        \row \li SeriousClassical
        \row \li OtherMusic
        \row \li Weather
        \row \li Finance
        \row \li ChildrensProgrammes
        \row \li SocialAffairs
        \row \li Religion
        \row \li PhoneIn
        \row \li Travel
        \row \li Leisure
        \row \li JazzMusic
        \row \li CountryMusic
        \row \li NationalMusic
        \row \li OldiesMusic
        \row \li FolkMusic
        \row \li Documentary
        \row \li AlarmTest
        \row \li Alarm
        \row \li Talk
        \row \li ClassicRock
        \row \li AdultHits
        \row \li SoftRock
        \row \li Top40
        \row \li Soft
        \row \li Nostalgia
        \row \li Classical
        \row \li RhythmAndBlues
        \row \li SoftRhythmAndBlues
        \row \li Language
        \row \li ReligiousMusic
        \row \li ReligiousTalk
        \row \li Personality
        \row \li Public
        \row \li College

    \endtable
  */
QDeclarativeRadioData::ProgramType QDeclarativeRadioData::programType() const
{
    return static_cast<QDeclarativeRadioData::ProgramType>(m_radioData->programType());
}

/*!
    \qmlproperty string QtMultimedia::RadioData::programTypeName

    This property holds a string representation of the \l programType.
  */
QString QDeclarativeRadioData::programTypeName() const
{
    return m_radioData->programTypeName();
}

/*!
    \qmlproperty string QtMultimedia::RadioData::stationName

    This property has the name of the currently tuned radio station.
  */
QString QDeclarativeRadioData::stationName() const
{
    return m_radioData->stationName();
}

/*!
    \qmlproperty string QtMultimedia::RadioData::radioText

    This property holds free-text transmitted by the radio station. This is typically used to
    show supporting information for the currently playing content, for instance song title or
    artist name.
  */
QString QDeclarativeRadioData::radioText() const
{
    return m_radioData->radioText();
}

/*!
    \qmlproperty bool QtMultimedia::RadioData::alternativeFrequenciesEnabled

    This property allows you to specify whether the radio should try and tune to alternative
    frequencies if the signal strength of the current station becomes too weak. The alternative
    frequencies are emitted over RDS by the radio station, and the tuning happens automatically.
  */
bool QDeclarativeRadioData::alternativeFrequenciesEnabled() const
{
    return m_radioData->isAlternativeFrequenciesEnabled();
}

void QDeclarativeRadioData::setAlternativeFrequenciesEnabled(bool enabled)
{
    m_radioData->setAlternativeFrequenciesEnabled(enabled);
}

void QDeclarativeRadioData::_q_programTypeChanged(QRadioData::ProgramType programType)
{
    emit programTypeChanged(static_cast<QDeclarativeRadioData::ProgramType>(programType));
}

void QDeclarativeRadioData::_q_error(QRadioData::Error errorCode)
{
    emit error(static_cast<QDeclarativeRadioData::Error>(errorCode));
    emit errorChanged();
}

void QDeclarativeRadioData::_q_availabilityChanged(QMultimedia::AvailabilityStatus availability)
{
    emit availabilityChanged(Availability(availability));
}

void QDeclarativeRadioData::connectSignals()
{
    if (!m_radioData)
        return;

    connect(m_radioData, SIGNAL(programTypeChanged(QRadioData::ProgramType)), this,
                                 SLOT(_q_programTypeChanged(QRadioData::ProgramType)));

    connect(m_radioData, SIGNAL(stationIdChanged(QString)), this, SIGNAL(stationIdChanged(QString)));
    connect(m_radioData, SIGNAL(programTypeNameChanged(QString)), this, SIGNAL(programTypeNameChanged(QString)));
    connect(m_radioData, SIGNAL(stationNameChanged(QString)), this, SIGNAL(stationNameChanged(QString)));
    connect(m_radioData, SIGNAL(radioTextChanged(QString)), this, SIGNAL(radioTextChanged(QString)));
    connect(m_radioData, SIGNAL(alternativeFrequenciesEnabledChanged(bool)), this,
                         SIGNAL(alternativeFrequenciesEnabledChanged(bool)));
    // Since the radio data type depends on the service for the tuner, the availability is also dictated from the tuner
    connect(m_radioTuner, SIGNAL(availabilityChanged(QMultimedia::AvailabilityStatus)), this, SLOT(_q_availabilityChanged(QMultimedia::AvailabilityStatus)));

    connect(m_radioData, SIGNAL(error(QRadioData::Error)), this, SLOT(_q_error(QRadioData::Error)));
}

QT_END_NAMESPACE
