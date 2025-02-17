/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
import QtTest 1.0

Item {
    id: container
    width: 300
    height: 300

TestCase {
    id: testCase
    name: "Tests_TextArea"
    when: windowShown
    width: 400
    height: 400

    function test_append() {
        var textarea = Qt.createQmlObject('import QtQuick.Controls 1.2; TextArea {}', testCase, '')

        compare(textarea.text, "")

        textarea.append("my")
        compare(textarea.text, "my")

        textarea.append("name");
        compare(textarea.text, "my\nname")
        textarea.destroy()
    }

    function test_activeFocusOnPress(){
        var control = Qt.createQmlObject('import QtQuick.Controls 1.2; TextArea {x: 20; y: 20; width: 100; height: 50}', container, '')
        control.activeFocusOnPress = false
        verify(!control.activeFocus)
        mouseClick(control, 30, 30)
        verify(!control.activeFocus)
        control.activeFocusOnPress = true
        verify(!control.activeFocus)
        mouseClick(control, 30, 30)
        verify(control.activeFocus)
        control.destroy()
    }

    function test_activeFocusOnTab() {
        // Set TextArea readonly so the tab/backtab can be tested toward the navigation
        var test_control = 'import QtQuick 2.2; \
        import QtQuick.Controls 1.2;            \
        Item {                                  \
            width: 200;                         \
            height: 200;                        \
            property alias control1: _control1; \
            property alias control2: _control2; \
            property alias control3: _control3; \
            TextArea  {                         \
                y: 20;                          \
                id: _control1;                  \
                activeFocusOnTab: true;         \
                text: "control1";               \
                readOnly: true                  \
            }                                   \
            TextArea  {                         \
                y: 70;                          \
                id: _control2;                  \
                activeFocusOnTab: false;        \
                text: "control2";               \
                readOnly: true                  \
            }                                   \
            TextArea  {                         \
                y: 120;                         \
                id: _control3;                  \
                activeFocusOnTab: true;         \
                text: "control3";               \
                readOnly: true                  \
            }                                   \
        }                                       '

        var control = Qt.createQmlObject(test_control, container, '')

        control.control1.forceActiveFocus()
        verify(control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(!control.control3.activeFocus)
        keyPress(Qt.Key_Tab)
        verify(!control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(control.control3.activeFocus)
        keyPress(Qt.Key_Tab)
        verify(control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(!control.control3.activeFocus)
        keyPress(Qt.Key_Tab, Qt.ShiftModifier)
        verify(!control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(control.control3.activeFocus)
        keyPress(Qt.Key_Tab, Qt.ShiftModifier)
        verify(control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(!control.control3.activeFocus)

        control.control2.activeFocusOnTab = true
        control.control3.activeFocusOnTab = false
        keyPress(Qt.Key_Tab)
        verify(!control.control1.activeFocus)
        verify(control.control2.activeFocus)
        verify(!control.control3.activeFocus)
        keyPress(Qt.Key_Tab)
        verify(control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(!control.control3.activeFocus)
        keyPress(Qt.Key_Tab, Qt.ShiftModifier)
        verify(!control.control1.activeFocus)
        verify(control.control2.activeFocus)
        verify(!control.control3.activeFocus)
        keyPress(Qt.Key_Tab, Qt.ShiftModifier)
        verify(control.control1.activeFocus)
        verify(!control.control2.activeFocus)
        verify(!control.control3.activeFocus)
        control.destroy()
    }

    function test_keys() {
        var component = Qt.createComponent("textarea/ta_keys.qml")
        compare(component.status, Component.Ready)
        var test =  component.createObject(container);
        verify(test !== null, "test control created is null")
        var control1 = test.control1
        verify(control1 !== null)

        control1.forceActiveFocus()
        verify(control1.activeFocus)

        verify(control1.gotit === false)
        verify(control1.text === "")

        keyPress(Qt.Key_A)
        verify(control1.activeFocus)
        verify(control1.gotit === false)
        verify(control1.text === "a")

        keyPress(Qt.Key_B)
        verify(control1.activeFocus)
        verify(control1.gotit === true)
        verify(control1.text === "a")

        keyPress(Qt.Key_B)
        verify(control1.activeFocus)
        verify(control1.gotit === true)
        verify(control1.text === "ab")

        test.destroy()
    }
}
}
