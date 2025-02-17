/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Private 1.0

Loader {
    id: loader

    property var styleDef

    property bool focused
    property bool pressed
    property bool checked
    property bool selected
    property bool accelerated
    property bool window_focused

    property int index: 0
    property real level: 0
    property string levelId: ""
    property int orientations: Qt.Horizontal
    property int duration: 0

    property var excludes: []
    property var clippables: []

    property Padding padding: Padding {
        top: loader.item ? loader.item.padding.top : 0
        left: loader.item ? loader.item.padding.left : 0
        right: loader.item ? loader.item.padding.right : 0
        bottom: loader.item ? loader.item.padding.bottom : 0
    }

    readonly property string type: styleDef ? styleDef.type : ""

    readonly property bool isExcluded: !!styleDef && excludes.indexOf(styleDef.id) !== -1
    readonly property bool isClippable: !!styleDef && clippables.indexOf(styleDef.id) !== -1

    active: !!styleDef && !isExcluded
    sourceComponent: type === "animation" ? Qt.createComponent("AnimationDrawable.qml") :
      isClippable || type === "clipDrawable" ? Qt.createComponent("ClipDrawable.qml") :
                     type === "color" ? Qt.createComponent("ColorDrawable.qml") :
                     type === "gradient" ? Qt.createComponent("GradientDrawable.qml") :
                     type === "image" ? Qt.createComponent("ImageDrawable.qml") :
                     type === "layer" ? Qt.createComponent("LayerDrawable.qml") :
                     type === "9patch" ? Qt.createComponent("NinePatchDrawable.qml") :
                     type === "rotate" ? Qt.createComponent("RotateDrawable.qml") :
                     type === "stateslist" ? Qt.createComponent("StateDrawable.qml") : null

    Binding { target: loader.item; property: "styleDef"; value: loader.styleDef }
    Binding { target: loader.item; property: "focused"; value: loader.focused }
    Binding { target: loader.item; property: "pressed"; value: loader.pressed }
    Binding { target: loader.item; property: "checked"; value: loader.checked }
    Binding { target: loader.item; property: "selected"; value: loader.selected }
    Binding { target: loader.item; property: "accelerated"; value: loader.accelerated }
    Binding { target: loader.item; property: "window_focused"; value: loader.window_focused }
    Binding { target: loader.item; property: "level"; value: loader.level }
    Binding { target: loader.item; property: "levelId"; value: loader.levelId }
    Binding { target: loader.item; property: "orientations"; value: loader.orientations }
    Binding { target: loader.item; property: "duration"; value: loader.duration }
    Binding { target: loader.item; property: "excludes"; value: loader.excludes }
    Binding { target: loader.item; property: "clippables"; value: loader.clippables }
}
