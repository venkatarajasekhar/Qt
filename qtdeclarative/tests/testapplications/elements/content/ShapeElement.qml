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

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: shapeelementtest
    anchors.fill: parent
    property string testtext: ""

    ParticleSystem {
        id: particlesystem
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        height: 300
        width: 300

        ImageParticle {
            id: imageparticle
            source: "pics/star.png"
            color: "red"
        }

        Emitter {
            id: emitter
            property real magn: 0.1
            anchors.fill: parent
            emitRate: 500
            lifeSpan: 2000
            velocity: TargetDirection {
                targetX: particlesystem.width/2
                targetY: particlesystem.height/2
                proportionalMagnitude: true
                magnitude: emitter.magn
                magnitudeVariation: emitter.magn
            }
            shape: rectangleshapeelement
        }
        Emitter {
            id: emitter2
            enabled: false
            anchors.fill: parent
            emitRate: 200
            lifeSpan: 1000
            velocity: TargetDirection {
                targetX: particlesystem.width/2
                targetY: particlesystem.height/2
                proportionalMagnitude: true
                magnitude: 0
                magnitudeVariation: 0
            }
            shape: lineshapeelement2
        }
        // Shapes
        EllipseShape {
            id: ellipseshapeelement
            fill: false
        }
        RectangleShape {
            id: rectangleshapeelement
            fill: false
        }
        LineShape {
            id: lineshapeelement
        }
        LineShape {
            id: lineshapeelement2
            mirrored: true
        }
        MaskShape {
            id: maskshapeelement
            source: "pics/logo-hollowed.png"
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: shapeelementtest
                testtext: "This is an Shape element, used by Emitter. There should be a rectangle "+
                "shape emitting into its center.\n"+
                "Next, let's change the shape to an ellipse." }
        },
        State { name: "ellipse"; when: statenum == 2
            PropertyChanges { target: emitter; shape: ellipseshapeelement }
            PropertyChanges { target: shapeelementtest
                testtext: "The particles should now be emitted in a circular shape.\n"+
                "Next, let's change the shape to a line." }
        },
        State { name: "line"; when: statenum == 3
            PropertyChanges { target: emitter; shape: lineshapeelement; lifeSpan: 1000; emitRate: 200; }
            PropertyChanges { target: emitter2; enabled: true }
            PropertyChanges { target: shapeelementtest
            testtext: "The particles should now be emitted from two lines, creating an X shape.\n"+
                "Next, let's change the shape to an image." }
        },
        State { name: "enduring"; when: statenum == 4
            PropertyChanges { target: emitter; shape: maskshapeelement; lifeSpan: 1000; emitRate: 1000; magn: 0 }
            PropertyChanges { target: shapeelementtest
                testtext: "The particles should now be sparkling, stationary within a 'Qt' text image.\n"+
                "Advance to restart the test." }
        }
    ]
}