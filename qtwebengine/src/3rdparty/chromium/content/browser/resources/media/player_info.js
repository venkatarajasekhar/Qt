// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A class for keeping track of the details of a player.
 */

var PlayerInfo = (function() {
  'use strict';

  /**
   * A class that keeps track of properties on a media player.
   * @param id A unique id that can be used to identify this player.
   */
  function PlayerInfo(id) {
    this.id = id;
    // The current value of the properties for this player.
    this.properties = {};
    // All of the past (and present) values of the properties.
    this.pastValues = {};

    // Every single event in the order in which they were received.
    this.allEvents = [];
    this.lastRendered = 0;

    this.firstTimestamp_ = -1;
  }

  PlayerInfo.prototype = {
    /**
     * Adds or set a property on this player.
     * This is the default logging method as it keeps track of old values.
     * @param timestamp  The time in milliseconds since the Epoch.
     * @param key A String key that describes the property.
     * @param value The value of the property.
     */
    addProperty: function(timestamp, key, value) {
      // The first timestamp that we get will be recorded.
      // Then, all future timestamps are deltas of that.
      if (this.firstTimestamp_ === -1) {
        this.firstTimestamp_ = timestamp;
      }

      if (typeof key !== 'string') {
        throw new Error(typeof key + ' is not a valid key type');
      }

      this.properties[key] = value;

      if (!this.pastValues[key]) {
        this.pastValues[key] = [];
      }

      var recordValue = {
        time: timestamp - this.firstTimestamp_,
        key: key,
        value: value
      };

      this.pastValues[key].push(recordValue);
      this.allEvents.push(recordValue);
    },

    /**
     * Adds or set a property on this player.
     * Does not keep track of old values.  This is better for
     * values that get spammed repeatedly.
     * @param timestamp The time in milliseconds since the Epoch.
     * @param key A String key that describes the property.
     * @param value The value of the property.
     */
    addPropertyNoRecord: function(timestamp, key, value) {
      this.addProperty(timestamp, key, value);
      this.allEvents.pop();
    }
  };

  return PlayerInfo;
}());
