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
    id: flickableelementtest
    anchors.fill: parent
    property string testtext: ""
    property int sidelength: 1500

    Rectangle {
        id: flickableelementbox
        color: "lightgray"; border.color: "gray"; radius: 5; clip: true; opacity: .1
        height: 250; width: parent.width *.8
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15

        Flickable {
            id: flickableelement
            anchors.fill: parent
            contentHeight: sidelength; contentWidth: sidelength; contentX: (sidelength/2)-125; contentY: (sidelength/2)-125
            onAtYBeginningChanged: {
                if (flicking && atYBeginning && statenum == 1) { advance(); }
                else if (flicking && atYBeginning && atXBeginning && statenum == 4) { advance(); }
            }
            onAtXEndChanged: {
                if (atYEnd && atXEnd && statenum == 2) {
                    if (flicking){ advance();
                    } else {
                        testtext = "The view must be flicked into the bottom left corner. Move the grid away slightly and try again"
                    }
                }
            }
            onAtXBeginningChanged: {
                if (atYEnd && atXBeginning && statenum == 3) {
                    if (!flicking) { advance(); } else { testtext = "Drag - do not flick. Move the grid away slightly and try again" }
                }
            }

            Row {
                width: sidelength; height: sidelength
                Repeater {
                    model: 30
                    Column {
                        Repeater {
                            id: griprep
                            height: flickableelement.contentHeight; width: 50; model: 30
                            Rectangle { height: 50; width: 50; color: "gray"; border.color: "black" }
                        }
                    }
                }
            }
            Text { anchors.centerIn: parent; text: "Center" }
        }
    }
    Text { anchors.left: flickableelementbox.left; anchors.top: flickableelementbox.bottom; anchors.topMargin: 5;
        text: "Dragging"; color: !flickableelement.flicking && flickableelement.moving ? "Green" : "Red"
    }
    Text { anchors.right: flickableelementbox.right; anchors.top: flickableelementbox.bottom; anchors.topMargin: 5;
        text: "Flicking"; color: flickableelement.flicking ? "Green" : "Red"
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            StateChangeScript { script: { flickableelement.contentX = (sidelength/2)-125; flickableelement.contentY = (sidelength/2)-125 } }
            PropertyChanges { target: helpbubble; showadvance: false }
            PropertyChanges { target: flickableelementtest
                testtext: "This is a Flickable element. At present it should be displaying a grid,\n"+
                "with the center marked and in the center of the view.\n"+
                "Next, please flick the view to the top" }
        },
        State { name: "top"; when: statenum == 2
            PropertyChanges { target: helpbubble; showadvance: false }
            PropertyChanges { target: flickableelementtest; testtext: "Good. Now flick to the bottom right hand corner." }
        },
        State { name: "bottomright"; when: statenum == 3
            PropertyChanges { target: helpbubble; showadvance: false }
            PropertyChanges { target: flickableelementtest; testtext: "Great. Now drag, not flick, the view to the bottom left." }
        },
        State { name: "bottomleft"; when: statenum == 4
            PropertyChanges { target: helpbubble; showadvance: false }
            PropertyChanges { target: flickableelementtest; testtext: "Almost there. Flick to the top left" }
        },
        State { name: "sections"; when: statenum == 5
            PropertyChanges { target: helpbubble; showadvance: true }
            PropertyChanges { target: flickableelementtest
                testtext: "Excellent.\n"+
                "Advance to restart the test." }
        }
    ]

}
