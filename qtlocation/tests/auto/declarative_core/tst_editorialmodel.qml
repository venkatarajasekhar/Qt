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
import QtTest 1.0
import QtLocation 5.3
import "utils.js" as Utils

TestCase {
    id: testCase

    name: "EditorialModel"

    Plugin {
        id: testPlugin
        name: "qmlgeo.test.plugin"
        allowExperimental: true
        parameters: [
            PluginParameter {
                name: "initializePlaceData"
                value: true
            }
        ]
    }

    EditorialModel {
        id: testModel
    }

    Place {
        id: testPlace
        name: "Test Place"
    }

    Place {
        id: parkViewHotel
        placeId: "4dcc74ce-fdeb-443e-827c-367438017cf1"
        plugin: testPlugin
    }

    Place {
        id: seaViewHotel
        placeId: "8f72057a-54b2-4e95-a7bb-97b4d2b5721e"
        plugin: testPlugin
    }

    function test_setAndGet_data() {
        return [
            { tag: "place", property: "place", signal: "placeChanged", value: testPlace },
            { tag: "batchSize", property: "batchSize", signal: "batchSizeChanged", value: 10, reset: 1 },
        ];
    }

    function test_setAndGet(data) {
        Utils.testObjectProperties(testCase, testModel, data);
    }

    function test_consecutive_fetch_data() {
        return [
            { tag: "batchSize 1", batchSize: 1 },
            { tag: "batchSize 2", batchSize: 2 },
            { tag: "batchSize 5", batchSize: 5 },
            { tag: "batchSize 10", batchSize: 10 },
        ];
    }

    function test_consecutive_fetch(data) {
        var expectedEditorials = [
                    {
                        "title": "Editorial 1",
                        "text": "Editorial 1 Text",
                        "language": "en"
                    },
                    {
                        "title": "Editorial 2",
                        "text": "Editorial 2 Text",
                        "language": "en"
                    },
                    {
                        "title": "Editorial 3",
                        "text": "Editorial 3 Text",
                        "language": "en"
                    },
                    {
                        "title": "",
                        "text": "",
                        "language": "",
                    },
                    {
                        "title": "Editorial 5",
                        "text": "Editorial 5 Text",
                        "language": "en"
                    }
                ]

        var model = createModel();
        Utils.testConsecutiveFetch(testCase, model, parkViewHotel, expectedEditorials, data);
        model.destroy();
    }

    function test_reset() {
        var model = createModel();
        Utils.testReset(testCase, model, parkViewHotel);
        model.destroy();
    }

    function test_fetch_data() {
        return [
                    {
                        tag: "fetch all editorials in a single batch",
                        model: createModel(),
                        batchSize: 10,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 5
                    },
                    {
                        tag: "fetch from a place with no editorials",
                        model: createModel(),
                        batchSize: 1,
                        place: seaViewHotel,
                        expectedTotalCount: 0,
                        expectedCount: 0
                    },
                    {
                        tag: "fetch with batch size one less than the total",
                        model: createModel(),
                        batchSize: 4,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 4
                    },
                    {
                        tag: "fetch with batch size equal to the total",
                        model: createModel(),
                        batchSize: 5,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 5
                    },
                    {
                        tag: "fetch with batch size larger than the total",
                        model: createModel(),
                        batchSize: 6,
                        place: parkViewHotel,
                        expectedTotalCount: 5,
                        expectedCount: 5
                    }
                ]
    }

    function test_fetch(data) {
        Utils.testFetch(testCase, data);
        data.model.destroy();
    }

    function createModel() {
        return Qt.createQmlObject('import QtLocation 5.3; EditorialModel {}',
                                  testCase, "editorialModel");
    }
}
