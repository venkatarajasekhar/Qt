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
import QtPositioning 5.3

Item {
    width:100
    height:100
    // General-purpose elements for the test:
    Plugin { id: testPlugin; name: "qmlgeo.test.plugin"; allowExperimental: true }


    property var coordinateList: []
    property int coordinateCount: 0
    property int animationDuration: 100

    Map {id: map
         plugin: testPlugin
         width: 100
         height: 100

         Behavior on center {
             id: centerBehavior

             enabled: false
             CoordinateAnimation { duration: animationDuration }
         }

         onCenterChanged: {
             if (!coordinateList) {
                 coordinateList = []
             }

             coordinateList[coordinateCount] = {'latitude': center.latitude, 'longitude': center.longitude}
             coordinateCount++
         }
    }

    function toMercator(coord) {
        var pi = Math.PI
        var lon = coord.longitude / 360.0 + 0.5;

        var lat = coord.latitude;
        lat = 0.5 - (Math.log(Math.tan((pi / 4.0) + (pi / 2.0) * lat / 180.0)) / pi) / 2.0;
        lat = Math.max(0.0, lat);
        lat = Math.min(1.0, lat);

        return {'latitude': lat, 'longitude': lon};
    }

    TestCase {
        when: windowShown
        name: "Coordinate animation"

        function test_coordinate_animation() {

            coordinateList = []
            coordinateCount = 0

            var from = {'latitude': 58.0, 'longitude': 12.0}
            var to = {'latitude': 62.0, 'longitude': 24.0}


            var fromMerc = toMercator(from)
            var toMerc = toMercator(to)

            var delta = (toMerc.latitude - fromMerc.latitude) / (toMerc.longitude - fromMerc.longitude)

            // Set from coordinate with animation disabled.
            map.center = QtPositioning.coordinate(from.latitude, from.longitude)

            // Expect only one update
            compare(coordinateList.length, 1)

            // Set to coordinate with animation enabled
            centerBehavior.enabled = true
            map.center = QtPositioning.coordinate(to.latitude, to.longitude)
            wait(animationDuration * 2)

            //check correct start position
            compare(coordinateList[0].latitude, from.latitude)
            compare(coordinateList[0].longitude, from.longitude)

            //check correct end position
            compare(coordinateList[coordinateList.length - 1].latitude, to.latitude)
            compare(coordinateList[coordinateList.length - 1].longitude, to.longitude)

            var i
            var lastLatitude
            for (i in coordinateList) {
                var coordinate = coordinateList[i]
                var mercCoordinate = toMercator(coordinate)

                //check that coordinates from the animation is along a straight line between from and to
                var estimatedLatitude = fromMerc.latitude + (mercCoordinate.longitude - fromMerc.longitude) * delta
                verify(mercCoordinate.latitude - estimatedLatitude < 0.00000000001);

                //check that each step has moved in the right direction
                if (lastLatitude) {
                    verify(coordinate.latitude > lastLatitude)
                }
                lastLatitude = coordinate.latitude
            }
        }
    }
}
