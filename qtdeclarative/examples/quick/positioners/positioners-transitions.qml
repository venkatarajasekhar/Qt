/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Item {
    id: page

    property real effectiveOpacity: 1.0
    property real ratio: width / 320 < height / 440 ? width / 320 : height / 440
    property int smallSize: 45 * ratio
    property int bigSize: 2 * smallSize
    property int elementSpacing: 0.14 * smallSize

    anchors.fill: parent

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: effectiveOpacity = (effectiveOpacity == 1.0 ? 0.0 : 1.0);
    }

    Column {
        anchors {
            left: parent.left
            leftMargin: page.width / 32
            top: parent.top
            topMargin: page.height / 48
        }
        spacing: elementSpacing

        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 100; easing.type: Easing.OutBounce }
        }
        add: Transition {
            NumberAnimation { properties: "y"; easing.type: Easing.OutQuad }
        }
        move: Transition {
            NumberAnimation { properties: "y"; easing.type: Easing.OutBounce }
        }

        Rectangle { color: "#80c342"; width: bigSize; height: smallSize }

        Rectangle {
            id: greenV1
            visible: opacity != 0
            width: bigSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#14aaff"; width: bigSize; height: smallSize }

        Rectangle {
            id: greenV2
            visible: opacity != 0
            width: bigSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#6400aa"; width: bigSize; height: smallSize }
        Rectangle { color: "#80c342"; width: bigSize; height: smallSize }
    }

    Row {
        anchors {
            left: page.left
            leftMargin: page.width / 32
            bottom: page.bottom
            bottomMargin: page.height / 48
        }
        spacing: elementSpacing

        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 100; easing.type: Easing.OutBounce }
        }
        add: Transition {
            NumberAnimation { properties: "x"; easing.type: Easing.OutQuad }
        }
        move: Transition {
            NumberAnimation { properties: "x"; easing.type: Easing.OutBounce }
        }

        Rectangle { color: "#80c342"; width: smallSize; height: bigSize }

        Rectangle {
            id: blueH1
            visible: opacity != 0
            width: smallSize; height: bigSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#14aaff"; width: smallSize; height: bigSize }

        Rectangle {
            id: greenH2
            visible: opacity != 0
            width: smallSize; height: bigSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#6400aa"; width: smallSize; height: bigSize }
        Rectangle { color: "#80c342"; width: smallSize; height: bigSize }
    }

    Grid {
        anchors.top: parent.top
        anchors.topMargin: page.height / 48
        anchors.left: flow.left
        columns: 3
        spacing: elementSpacing

        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 100; easing.type: Easing.OutBounce }
        }
        add: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }
        move: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }

        Rectangle { color: "#80c342"; width: smallSize; height: smallSize }

        Rectangle {
            id: greenG1
            visible: opacity != 0
            width: smallSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#14aaff"; width: smallSize; height: smallSize }

        Rectangle {
            id: greenG2
            visible: opacity != 0
            width: smallSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#6400aa"; width: smallSize; height: smallSize }

        Rectangle {
            id: greenG3
            visible: opacity != 0
            width: smallSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#80c342"; width: smallSize; height: smallSize }
        Rectangle { color: "#14aaff"; width: smallSize; height: smallSize }
        Rectangle { color: "#6400aa"; width: smallSize; height: smallSize }
    }

    Flow {
        id: flow

        anchors.right: page.right
        anchors.rightMargin: page.width / 32
        y: 2 * bigSize
        width: 1.8 * bigSize
        spacing: elementSpacing

        //! [move]
        move: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }
        //! [move]

        //! [add]
        add: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }
        //! [add]

        //! [populate]
        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 100; easing.type: Easing.OutBounce }
        }
        //! [populate]

        Rectangle { color: "#80c342"; width: smallSize; height: smallSize }

        Rectangle {
            id: greenF1
            visible: opacity != 0
            width: 0.6 * bigSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#14aaff"; width: 0.3 * bigSize; height: smallSize }

        Rectangle {
            id: greenF2
            visible: opacity != 0
            width: 0.6 * bigSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#6400aa"; width: smallSize; height: smallSize }

        Rectangle {
            id: greenF3
            visible: opacity != 0
            width: 0.4 * bigSize; height: smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "#80c342"; width: 0.8 * bigSize; height: smallSize }
    }
}
