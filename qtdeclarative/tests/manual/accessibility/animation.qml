/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

Rectangle {
    id: scene
    width: 800; height: 600

    Rectangle {
        id: behavior
        x : 50
        y : 100
        width: 100; height: 100
        color: "red"

        Text {
            text : "Behavior"
        }

        Behavior on x {
            NumberAnimation { duration: 1000 }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: behavior.x += 50
        }
    }

    Rectangle {
        id: transition
        x : 400
        y : 100
        width: 100; height: 100
        color: "red"

        MouseArea {
            id: mouseArea
            anchors.fill: parent
        }

        Text {
            text : "Transition"
        }

        states: State {
            name: "moved"; when: mouseArea.pressed
            PropertyChanges { target: transition; x: 500; y: 200 }
        }

        transitions: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 1000 }
        }
    }

    Rectangle {
        id : animatee
        width: 100; height: 100
        x : 50
        y : 300
        color: "blue"
        opacity: 0.5
        Text {
            anchors.centerIn: parent
            text : "NumberAnimation"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                animatePosition.start()
            }
        }

        NumberAnimation {
            id: animatePosition
            target: animatee
            properties: "x"
            from: animatee.x
            to: animatee.x + 50
            loops: 1
            easing {type: Easing.Linear;}
        }
    }

    ListView {
        id : content
        x : 400
        y : 300
        width: 300
        height: 200

        model : 200
        delegate : Text { text : "Flickable" + index; height : 50 }
    }
}
