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
    id: flowelementtest
    anchors.fill: parent
    property string testtext: ""

    Rectangle { anchors.fill: flowelement; color: "lightsteelblue"; radius: 5 }

    Flow {
        id: flowelement
        height: 110; width: 110
        spacing: 5; flow: Flow.LeftToRight
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        Rectangle { id: gr; color: "green"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true
            Text { anchors.centerIn: parent; text: "1" }
        }
        Rectangle { id: re; color: "red"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true
            Text { anchors.centerIn: parent; text: "2" }
        }
        Rectangle { id: bl; color: "blue"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true; visible: false
            Text { anchors.centerIn: parent; text: "3" }
        }
        Rectangle { id: bk; color: "black"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true
            Text { anchors.centerIn: parent; text: "4"; color: "lightgray" }
        }

        move: Transition { NumberAnimation { properties: "x"; duration: 500; easing.type: Easing.OutBounce } }
        add: Transition { NumberAnimation { properties: "x"; duration: 500; easing.type: Easing.OutBounce } }

    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: flowelementtest
                testtext: "This is a Flow element. At present it should be showing three rectangles - green, red and black.\n"+
                "Next, let's add a rectangle to the Flow - it should slide in from the left and the black rectangle should move to give it space" }
        },
        State { name: "added"; when: statenum == 2
            PropertyChanges { target: bl; visible: true }
            PropertyChanges { target: flowelementtest
                testtext: "Flow should now be showing four rectangles - green, red, blue and black.\n"+
                "Next let's change the direction of the flow to vertical." }
        },
        State { name: "vertical"; when: statenum == 3
            PropertyChanges { target: bl; visible: true }
            PropertyChanges { target: flowelement; flow: Flow.TopToBottom }
            PropertyChanges { target: flowelementtest
                testtext: "Flow should now be showing four rectangles - green, blue, red and black.\n"+
                "Next, let's flip the layout direction to RightToLeft." }
        },
        State { name: "rtlvertical"; when: statenum == 4
            PropertyChanges { target: bl; visible: true }
            PropertyChanges { target: flowelement; flow: Flow.TopToBottom; layoutDirection: Qt.RightToLeft }
            PropertyChanges { target: flowelementtest
                testtext: "Flow should now be showing the four rectangles aligned to the right and in a column order flowing to the left"+
                "Advance to restart the test." }
        }
    ]
}
