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

Item {
    id: flipableelementtest
    anchors.fill: parent
    property string testtext: ""

    Flipable {
        id: flipableelement
        height: 250; width: 250
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        front: Rectangle { color: "green"; anchors.fill: parent; border.color: "gray"; border.width: 3; opacity: .9; radius: 20; clip: true }
        back: Rectangle { color: "red"; anchors.fill: parent; border.color: "gray"; border.width: 3; opacity: .9; radius: 20; clip: true }
        transform: Rotation {
            id: rotation
            origin.x: flipableelement.width/2
            origin.y: flipableelement.height/2
            axis.x: 0; axis.y: 1; axis.z: 0
            angle: 0
        }

    }
    transitions: Transition {
         NumberAnimation { target: rotation; property: "angle"; duration: 2000 }
     }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: flipableelementtest
                testtext: "This is a Flipable element. At present it should be showing a green rectangle.\n"+
                "Next, let's flip the Flipable horizontally to show the back" }
        },
        State { name: "back"; when: statenum == 2
            PropertyChanges { target: rotation; angle: 180 }
            //FIXED PropertyChanges { target: bugpanel; bugnumber: "19901" }
            PropertyChanges { target: flipableelementtest
                testtext: "Flipable should now be showing a red rectangle.\n"+
                "Next, let's flip the Flipable to again show the front." }
        },
        State { name: "front"; when: statenum == 3
            PropertyChanges { target: flipableelementtest
                testtext: "Flipable should have flipped and now be showing a green rectangle.\n"+
                "Next, let's flip vertically." }
        },
        State { name: "backvertical"; when: statenum == 4
            PropertyChanges { target: rotation; axis.y: 0; axis.x: 1; angle: 180 }
            //FIXED PropertyChanges { target: bugpanel; bugnumber: "19901" }
            PropertyChanges { target: flipableelementtest
                testtext: "Flipable should have flipped vertically and now be showing a red rectangle.\n"+
                "Next, let's flip back." }
        },
        State { name: "frontvertical"; when: statenum == 5
            PropertyChanges { target: rotation; axis.y: 0; axis.x: 1; angle: 0 }
            PropertyChanges { target: flipableelementtest
                testtext: "Flipable should have flipped vertically and now be showing a green rectangle.\n"+
                "Next, let's restart the test." }
        }
    ]
}
