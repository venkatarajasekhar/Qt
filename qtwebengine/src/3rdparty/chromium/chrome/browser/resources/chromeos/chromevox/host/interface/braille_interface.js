// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Defines a Braille interface.
 *
 * All Braille engines in ChromeVox conform to this interface.
 *
 */

goog.provide('cvox.BrailleInterface');

goog.require('cvox.BrailleKeyCommand');
goog.require('cvox.BrailleKeyEvent');
goog.require('cvox.NavBraille');

/**
 * @interface
 */
cvox.BrailleInterface = function() { };

/**
 * Sends the given params to the Braille display for output.
 * @param {!cvox.NavBraille} params Parameters to send to the
 * platform braille service.
 */
cvox.BrailleInterface.prototype.write =
    function(params) { };

/**
 * Sets a callback for handling braille keyboard commands.
 *
 * @param {function(!cvox.BrailleKeyEvent, cvox.NavBraille)} func The function
 * to be called when the user invokes a keyboard command on the braille
 * display. The first parameter is the key event.  The second parameter is
 * the content that was present on the display when the key command
 * was invoked, if available.
 */
cvox.BrailleInterface.prototype.setCommandListener =
    function(func) { };
