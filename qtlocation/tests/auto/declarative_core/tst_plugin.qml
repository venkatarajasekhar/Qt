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

Item {

    Plugin { id: unattachedPlugin }
    Plugin { id: nokiaPlugin; name: "nokia"}
    Plugin { id: invalidPlugin; name: "invalid"; allowExperimental: true }
    Plugin { id: testPlugin;
            name: "qmlgeo.test.plugin"
            allowExperimental: true
            parameters: [
                // Parms to guide the test plugin
                PluginParameter { name: "supported"; value: true},
                PluginParameter { name: "finishRequestImmediately"; value: true},
                PluginParameter { name: "validateWellKnownValues"; value: true}
            ]
        }
    SignalSpy {id: invalidAttachedSpy; target: invalidPlugin; signalName: "attached"}

    Plugin {
        id: requiredPlugin
        allowExperimental: true
        required {
            mapping: Plugin.OfflineMappingFeature;
            geocoding: Plugin.OfflineGeocodingFeature;
            places: Plugin.AnyPlacesFeatures;
        }
    }

    TestCase {
        name: "Plugin properties"
        function test_plugin() {
            verify (invalidPlugin.availableServiceProviders.length > 0)
            verify (invalidPlugin.availableServiceProviders.indexOf('qmlgeo.test.plugin') > -1) // at least test plugin must be present

            // invalid plugins should have no features
            verify(invalidPlugin.isAttached)
            verify(!(invalidPlugin.supportsMapping()))
            verify(!(invalidPlugin.supportsGeocoding()))
            verify(!(invalidPlugin.supportsRouting()))
            verify(!(invalidPlugin.supportsPlaces()))

            if (invalidPlugin.availableServiceProviders.indexOf('qmlgeo.test.plugin') > -1) {
                verify(testPlugin.isAttached)
                verify(testPlugin.supportsMapping())
                verify(testPlugin.supportsGeocoding())
                verify(testPlugin.supportsPlaces())
                verify(testPlugin.supportsRouting())
            }

            if (invalidPlugin.availableServiceProviders.indexOf('nokia')) {
                verify(nokiaPlugin.isAttached)
                verify(nokiaPlugin.supportsMapping(Plugin.OnlineMappingFeature))
                verify(nokiaPlugin.supportsGeocoding(Plugin.OnlineGeocodingFeature))
                verify(nokiaPlugin.supportsRouting(Plugin.OnlineRoutingFeature))
            }

            verify(!unattachedPlugin.isAttached)

            // test changing name of plugin
            invalidAttachedSpy.clear()
            compare(invalidAttachedSpy.count, 0)
            invalidPlugin.name = 'qmlgeo.test.plugin'
            tryCompare(invalidAttachedSpy, 'count', 1)
            verify(invalidPlugin.isAttached)

            verify(invalidPlugin.supportsMapping())
            verify(invalidPlugin.supportsGeocoding())
            verify(invalidPlugin.supportsRouting())
            verify(invalidPlugin.supportsPlaces())

            invalidPlugin.name = ''
            compare(invalidAttachedSpy.count, 2)

            verify(!invalidPlugin.supportsMapping())
            verify(!invalidPlugin.supportsGeocoding())
            verify(!invalidPlugin.supportsRouting())
            verify(!invalidPlugin.supportsPlaces())
        }

        function test_required() {
            // the required plugin should either get nokia or qmlgeo.test.plugin
            // either way the name will be non-empty and it'll meet the spec
            verify(requiredPlugin.name !== "")
            verify(requiredPlugin.supportsMapping(requiredPlugin.required.mapping))
            verify(requiredPlugin.supportsGeocoding(requiredPlugin.required.geocoding))
            verify(requiredPlugin.supportsPlaces(requiredPlugin.required.places))
        }

        function test_placesFeatures() {
            verify(testPlugin.supportsPlaces(Plugin.SavePlaceFeature))
            verify(testPlugin.supportsPlaces(Plugin.SaveCategoryFeature))
            verify(testPlugin.supportsPlaces(Plugin.SearchSuggestionsFeature))
            verify(!testPlugin.supportsPlaces(Plugin.RemovePlaceFeature))
        }

        function test_locale() {
            compare(nokiaPlugin.locales, [Qt.locale().name]);

            //try assignment of a single locale
            nokiaPlugin.locales = "fr_FR";
            compare(nokiaPlugin.locales, ["fr_FR"]);

            //try assignment of multiple locales
            nokiaPlugin.locales = ["fr_FR","en_US"];
            compare(nokiaPlugin.locales, ["fr_FR","en_US"]);

            //check that assignment of empty locale list defaults to system locale
            nokiaPlugin.locales = [];
            compare(nokiaPlugin.locales, [Qt.locale().name]);
        }
    }
}
