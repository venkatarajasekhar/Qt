/****************************************************************************
**
** Copyright (C) 2014 Gunnar Sletta <gunnar@sletta.org>
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

import QtQuick 2.2
import QtTest 1.1

Item {
    id: root;
    width: 200
    height: 200

    TestCase {
        id: testcase
        name: "animators-targetdestroyed"
        when: false
        function test_endresult() {
            verify(true, "Got here :)");
        }
    }

    Rectangle {
        id: box
        width: 10
        height: 10
        color: "steelblue"
    }

    YAnimator {
        id: anim
        target: box
        from: 0;
        to: 100
        duration: 100
        loops: Animation.Infinite
        running: true
    }

    SequentialAnimation {
        running: true
        PauseAnimation { duration: 150 }
        ScriptAction { script: box.destroy(); }
        PauseAnimation { duration: 50 }
        ScriptAction { script: anim.destroy(); }
        PauseAnimation { duration: 50 }
        ScriptAction { script: testcase.when = true }
    }
}
