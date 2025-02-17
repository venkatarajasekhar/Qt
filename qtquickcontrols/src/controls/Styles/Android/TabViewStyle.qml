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
import QtQuick.Window 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles.Android 1.0
import "drawables"

Style {
    readonly property TabView control: __control

    readonly property bool tabsMovable: false
    readonly property int tabsAlignment: Qt.AlignLeft
    readonly property int tabOverlap: 0
    readonly property int frameOverlap: 0

    property Component frame: null
    property Component leftCorner: null
    property Component rightCorner: null

    property Component tabBar: Item {
        id: panel

        readonly property var styleDef: AndroidStyle.styleDef.actionBarStyle

        readonly property real minWidth: styleDef.View_minWidth || 0
        readonly property real minHeight: styleDef.View_minHeight || 0

        implicitWidth: Math.max(minWidth, loader.implicitWidth)
        implicitHeight: Math.max(minHeight, loader.implicitHeight)

        DrawableLoader {
            id: loader
            anchors.fill: parent
            focused: control.activeFocus
            window_focused: focused && control.Window.active
            styleDef: panel.styleDef.ActionBar_backgroundStacked
        }
    }

    property Component tab: Item {
        id: tab

        readonly property real minHeight: AndroidStyle.styleDef.actionButtonStyle.View_minHeight || 0
        readonly property real contentHeight: label.implicitHeight + bg.padding.top + bg.padding.bottom

        readonly property real dividerPadding: AndroidStyle.styleDef.actionBarTabBarStyle.LinearLayout_dividerPadding

        implicitWidth: styleData.availableWidth / Math.max(1, control.count)
        implicitHeight: Math.max(minHeight, Math.max(bg.implicitHeight, contentHeight))

        DrawableLoader {
            id: bg
            anchors.fill: parent
            enabled: styleData.enabled
            pressed: styleData.pressed
            selected: styleData.selected
            focused: styleData.activeFocus
            window_focused: focused && control.Window.active
            styleDef: AndroidStyle.styleDef.actionBarTabStyle.View_background
        }

        DrawableLoader {
            id: divider
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            enabled: styleData.enabled
            pressed: styleData.pressed
            selected: styleData.selected
            focused: styleData.activeFocus
            window_focused: focused && control.Window.active
            styleDef: AndroidStyle.styleDef.actionBarTabBarStyle.LinearLayout_divider
            visible: styleData.index < control.count - 1 && control.count > 1
        }

        LabelStyle {
            id: label
            text: styleData.title
            pressed: styleData.pressed
            enabled: styleData.enabled
            focused: styleData.activeFocus
            selected: styleData.selected
            window_focused: control.Window.active
            styleDef: AndroidStyle.styleDef.actionBarTabTextStyle

            anchors.fill: bg
            anchors.topMargin: bg.padding.top
            anchors.leftMargin: bg.padding.left + dividerPadding
            anchors.rightMargin: bg.padding.right + dividerPadding
            anchors.bottomMargin: bg.padding.bottom
        }
    }
}
