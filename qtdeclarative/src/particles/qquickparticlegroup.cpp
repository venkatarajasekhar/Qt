/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickparticlegroup_p.h"

/*!
    \qmltype ParticleGroup
    \instantiates QQuickParticleGroup
    \inqmlmodule QtQuick.Particles
    \brief For setting attributes on a logical particle group
    \ingroup qtquick-particles

    This element allows you to set timed transitions on particle groups.

    You can also use this element to group particle system elements related to the logical
    particle group. Emitters, Affectors and Painters set as direct children of a ParticleGroup
    will automatically apply to that logical particle group. TrailEmitters will automatically follow
    the group.

    If a ParticleGroup element is not defined for a group, the group will function normally as if
    none of the transition properties were set.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleGroup::system
    This is the system which will contain the group.

    If the ParticleGroup is a direct child of a ParticleSystem, it will automatically be associated with it.
*/
/*!
    \qmlproperty string QtQuick.Particles::ParticleGroup::name
    This is the name of the particle group, and how it is generally referred to by other elements.

    If elements refer to a name which does not have an explicit ParticleGroup created, it will
    work normally (with no transitions specified for the group). If you do not need to assign
    duration based transitions to a group, you do not need to create a ParticleGroup with that name (although you may).
*/
/*!
    \qmlproperty int QtQuick.Particles::ParticleGroup::duration
    The time in milliseconds before the group will attempt to transition.

*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleGroup::durationVariation
    The maximum number of milliseconds that the duration of the transition cycle varies per particle in the group.

    Default value is zero.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleGroup::to
    The weighted list of transitions valid for this group.

    If the chosen transition stays in this group, another duration (+/- up to durationVariation)
    milliseconds will occur before another transition is attempted.
*/

QQuickParticleGroup::QQuickParticleGroup(QObject* parent)
    : QQuickStochasticState(parent)
    , m_system(0)
{

}

void delayedRedirect(QQmlListProperty<QObject> *prop, QObject *value)
{
    QQuickParticleGroup* pg = qobject_cast<QQuickParticleGroup*>(prop->object);
    if (pg)
        pg->delayRedirect(value);
}

QQmlListProperty<QObject> QQuickParticleGroup::particleChildren()
{
    QQuickParticleSystem* system = qobject_cast<QQuickParticleSystem*>(parent());
    if (system)
        return QQmlListProperty<QObject>(this, 0, &QQuickParticleSystem::statePropertyRedirect, 0, 0, 0);
    else
        return QQmlListProperty<QObject>(this, 0, &delayedRedirect, 0, 0, 0);
}

void QQuickParticleGroup::setSystem(QQuickParticleSystem* arg)
{
    if (m_system != arg) {
        m_system = arg;
        m_system->registerParticleGroup(this);
        performDelayedRedirects();
        emit systemChanged(arg);
    }
}

void QQuickParticleGroup::delayRedirect(QObject *obj)
{
    m_delayedRedirects << obj;
}

void QQuickParticleGroup::performDelayedRedirects()
{
    if (!m_system)
        return;
    foreach (QObject* obj, m_delayedRedirects)
        m_system->stateRedirect(this, m_system, obj);

    m_delayedRedirects.clear();
}

void QQuickParticleGroup::componentComplete(){
    if (!m_system && qobject_cast<QQuickParticleSystem*>(parent()))
        setSystem(qobject_cast<QQuickParticleSystem*>(parent()));
}
