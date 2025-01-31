/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

Rectangle {
    id: root
    width: 440
    height: 160
    color: "transparent"

    property bool drawOpenPrice: openButton.buttonEnabled
    property bool drawClosePrice: closeButton.buttonEnabled
    property bool drawHighPrice: highButton.buttonEnabled
    property bool drawLowPrice: lowButton.buttonEnabled

    property string openColor: "#face20"
    property string closeColor: "#14aaff"
    property string highColor: "#80c342"
    property string lowColor: "#f30000"
    property string volumeColor: "#14aaff"

    Text {
        id: openText
        anchors.left: root.left
        anchors.top: root.top
        color: "#000000"
        font.family: "Open Sans"
        font.pointSize: 19
        text: "Open"
    }

    Text {
        id: closeText
        anchors.left: root.left
        anchors.top: openText.bottom
        anchors.topMargin: 10
        color: "#000000"
        font.family: "Open Sans"
        font.pointSize: 19
        text: "Close"
    }

    Text {
        id: highText
        anchors.left: root.left
        anchors.top: closeText.bottom
        anchors.topMargin: 10
        color: "#000000"
        font.family: "Open Sans"
        font.pointSize: 19
        text: "High"
    }

    Text {
        id: lowText
        anchors.left: root.left
        anchors.top: highText.bottom
        anchors.topMargin: 10
        color: "#000000"
        font.family: "Open Sans"
        font.pointSize: 19
        text: "Low"
    }

    Rectangle {
        height: 4
        anchors.left: root.left
        anchors.leftMargin: 114
        anchors.right: openButton.left
        anchors.rightMargin: 65
        anchors.verticalCenter: openText.verticalCenter
        color: openColor
    }

    Rectangle {
        height: 4
        anchors.left: root.left
        anchors.leftMargin: 114
        anchors.right: closeButton.left
        anchors.rightMargin: 65
        anchors.verticalCenter: closeText.verticalCenter
        color: closeColor
    }

    Rectangle {
        height: 4
        anchors.left: root.left
        anchors.leftMargin: 114
        anchors.right: highButton.left
        anchors.rightMargin: 65
        anchors.verticalCenter: highText.verticalCenter
        color: highColor
    }

    Rectangle {
        height: 4
        anchors.left: root.left
        anchors.leftMargin: 114
        anchors.right: lowButton.left
        anchors.rightMargin: 65
        anchors.verticalCenter: lowText.verticalCenter
        color: lowColor
    }

    CheckBox {
        id: openButton
        buttonEnabled: false
        anchors.verticalCenter: openText.verticalCenter
        anchors.right: root.right
        anchors.rightMargin: 40
    }

    CheckBox {
        id: closeButton
        buttonEnabled: false
        anchors.verticalCenter: closeText.verticalCenter
        anchors.right: root.right
        anchors.rightMargin: 40
    }

    CheckBox {
        id: highButton
        buttonEnabled: true
        anchors.verticalCenter: highText.verticalCenter
        anchors.right: root.right
        anchors.rightMargin: 40
    }

    CheckBox {
        id: lowButton
        buttonEnabled: true
        anchors.verticalCenter: lowText.verticalCenter
        anchors.right: root.right
        anchors.rightMargin: 40
    }
}
