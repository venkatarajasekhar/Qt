// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['../testing/chromevox_unittest_base.js']);

/**
 * Test fixture.
 * @constructor
 * @extends {ChromeVoxUnitTestBase}
 */
function CvoxKeySequenceUnitTest() {}

CvoxKeySequenceUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.ChromeVox',
    'cvox.KeySequence',
  ],

  /**
   * Create mock event object.
   * @param {number} keyCode The event key code (i.e. 13 for Enter).
   * @param {{altGraphKey: boolean=,
   *         altKey: boolean=,
   *         ctrlKey: boolean=,
   *         metaKey: boolean=,
   *         searchKeyHeld: boolean=,
   *         shiftKey: boolean=,
   *         stickyMode: boolean=,
   *         prefixKey: boolean=}} eventParams The parameters on the event.
   *  altGraphKey: Whether or not the altGraph key was held down.
   *  altKey: Whether or not the alt key was held down.
   *  ctrlKey: Whether or not the ctrl key was held down.
   *  metaKey: Whether or not the meta key was held down.
   *  searchKeyHeld: Whether or not the search key was held down.
   *  shiftKey: Whether or not the shift key was held down.
   *  stickyMode: Whether or not sticky mode is enabled.
   *  prefixKey: Whether or not the prefix key was entered.
   * @return {Object} The mock event.
   */
  createMockEvent: function(keyCode, eventParams) {
    var mockEvent = {};
    mockEvent.keyCode = keyCode;

    if (eventParams == null) {
      return mockEvent;
    }
    if (eventParams.hasOwnProperty('altGraphKey')) {
      mockEvent.altGraphKey = eventParams.altGraphKey;
    }
    if (eventParams.hasOwnProperty('altKey')) {
      mockEvent.altKey = eventParams.altKey;
    }
    if (eventParams.hasOwnProperty('ctrlKey')) {
      mockEvent.ctrlKey = eventParams.ctrlKey;
    }
    if (eventParams.hasOwnProperty('metaKey')) {
      mockEvent.metaKey = eventParams.metaKey;
    }
    if (eventParams.hasOwnProperty('shiftKey')) {
      mockEvent.shiftKey = eventParams.shiftKey;
    }

    if (eventParams.hasOwnProperty('searchKeyHeld')) {
      mockEvent.searchKeyHeld = eventParams.searchKeyHeld;
    }
    if (eventParams.hasOwnProperty('stickyMode')) {
      mockEvent.stickyMode = eventParams.stickyMode;
    }
    if (eventParams.hasOwnProperty('prefixKey')) {
      mockEvent.keyPrefix = eventParams.prefixKey;
    }

    return mockEvent;
  },

  /** @override */
  setUp: function() {
    // Set up mock ChromeVox modifier
    cvox.ChromeVox.modKeyStr = 'Alt';

    // Use these mock events in the tests:

    // Down arrow, no modifiers
    this.downArrowEvent = this.createMockEvent(40);

    // Down arrow key with alt held down. We specified 'Alt' as the
    // mock ChromeVox modifier string, so this means that KeySequence
    // should interpret this as the ChromeVox modifier being active.
    this.altDownArrowEvent = this.createMockEvent(40, {altKey: true});

    // Right arrow, no modifiers
    this.rightArrowEvent = this.createMockEvent(39);

    // Ctrl key, no modifiers
    this.ctrlEvent = this.createMockEvent(17);

    // Ctrl key with sticky mode
    this.ctrlStickyEvent = this.createMockEvent(17, {stickyMode: true});

    // Ctrl key with prefix mode
    this.ctrlPrefixEvent = this.createMockEvent(17, {prefixKey: true});

    // 'a' key, no modifiers
    this.aEvent = this.createMockEvent(65);

    // 'a' key with ctrl held down
    this.ctrlAEvent = this.createMockEvent(65, {ctrlKey: true});

    // 'a' key with meta held down
    this.metaAEvent = this.createMockEvent(65, {metaKey: true});

    // 'a' key with shift held down
    this.shiftAEvent = this.createMockEvent(65, {shiftKey: true});

    // 'a' key with alt (which is the mock ChromeVox modifier) and shift held
    // down.
    this.altShiftAEvent = this.createMockEvent(65, {altKey: true,
                                                    shiftKey: true});

    // 'a' key with shift and prefix held down
    this.shiftAPrefixEvent = this.createMockEvent(65, {shiftKey: true,
                                                       prefixKey: true});

    // 'a' key with shift and sticky mode
    this.shiftAStickyEvent = this.createMockEvent(65, {shiftKey: true,
                                                       stickyMode: true});

    // 'a' key with sticky mode
    this.aEventSticky = this.createMockEvent(65, {stickyMode: true});

    // 'a' key with prefix key
    this.aEventPrefix = this.createMockEvent(65, {prefixKey: true});

    // 'a' key with alt (which is the mock ChromeVox modifier) held down
    this.altAEvent = this.createMockEvent(65, {altKey: true});

    // 'b' key, no modifiers
    this.bEvent = this.createMockEvent(66);

    // 'b' key, with ctrl held down
    this.ctrlBEvent = this.createMockEvent(66, {ctrlKey: true});

    // 'c' key, no modifiers
    this.cEvent = this.createMockEvent(67);

    // Shift key with ctrl held down
    this.ctrlShiftEvent = this.createMockEvent(60, {ctrlKey: true});

    // Ctrl key with shift held down
    this.shiftCtrlEvent = this.createMockEvent(17, {shiftKey: true});
  }
};

TEST_F('CvoxKeySequenceUnitTest', 'SimpleSequenceNoModifier', function() {
  var downKey = new cvox.KeySequence(this.downArrowEvent, false);

  assertEqualsJSON([40], downKey.keys.keyCode);
  assertFalse(downKey.stickyMode);
  assertFalse(downKey.prefixKey);
  assertFalse(downKey.cvoxModifier);

  assertEqualsJSON([false], downKey.keys.altGraphKey);
  assertEqualsJSON([false], downKey.keys.altKey);
  assertEqualsJSON([false], downKey.keys.ctrlKey);
  assertEqualsJSON([false], downKey.keys.metaKey);
  assertEqualsJSON([false], downKey.keys.searchKeyHeld);
  assertEqualsJSON([false], downKey.keys.shiftKey);

  assertEquals(1, downKey.length());
});


/** Test another key sequence, this time with the modifier */
TEST_F('CvoxKeySequenceUnitTest', 'SimpleSequenceWithModifier', function() {
  var downKey = new cvox.KeySequence(this.downArrowEvent, true);

  assertEqualsJSON([40], downKey.keys.keyCode);
  assertFalse(downKey.stickyMode);
  assertFalse(downKey.prefixKey);
  assertTrue(downKey.cvoxModifier);

  assertEqualsJSON([false], downKey.keys.altGraphKey);
  assertEqualsJSON([false], downKey.keys.altKey);
  assertEqualsJSON([false], downKey.keys.ctrlKey);
  assertEqualsJSON([false], downKey.keys.metaKey);
  assertEqualsJSON([false], downKey.keys.searchKeyHeld);
  assertEqualsJSON([false], downKey.keys.shiftKey);

  assertEquals(1, downKey.length());
});


/** Test a key sequence that includes the modifier */
TEST_F('CvoxKeySequenceUnitTest', 'ModifiedSequence', function() {
  var cvoxDownKey = new cvox.KeySequence(this.altDownArrowEvent, true);

  assertEqualsJSON([40], cvoxDownKey.keys.keyCode);
  assertFalse(cvoxDownKey.stickyMode);
  assertFalse(cvoxDownKey.prefixKey);
  assertTrue(cvoxDownKey.cvoxModifier);

  assertEqualsJSON([false], cvoxDownKey.keys.altGraphKey);
  assertEqualsJSON([false], cvoxDownKey.keys.altKey);
  assertEqualsJSON([false], cvoxDownKey.keys.ctrlKey);
  assertEqualsJSON([false], cvoxDownKey.keys.metaKey);
  assertEqualsJSON([false], cvoxDownKey.keys.searchKeyHeld);
  assertEqualsJSON([false], cvoxDownKey.keys.shiftKey);

  assertEquals(1, cvoxDownKey.length());
});


/**
 * Test equality - Ctrl key vs. Ctrl key with sticky mode on
 * These should be equal because Ctrl should still function even with
 * sticky mode on.
*/
TEST_F('CvoxKeySequenceUnitTest', 'StickyEquality', function() {
  var ctrlKey = new cvox.KeySequence(this.ctrlEvent, false);
  var ctrlSticky = new cvox.KeySequence(this.ctrlStickyEvent, false);

  assertTrue(ctrlKey.equals(ctrlSticky));
});


/**
 * Test equality - 'a' key with Shift modifier vs. 'a' key without Shift
 * modifier.
 * These should not be equal because they do not have the same modifiers.
*/
TEST_F('CvoxKeySequenceUnitTest', 'ShiftEquality', function() {
  var aKey = new cvox.KeySequence(this.aEvent, false);
  var shiftA = new cvox.KeySequence(this.shiftAEvent, false);

  assertFalse(aKey.equals(shiftA));
});


/**
 * Test equality - 'a' with ChromeVox modifier specified, 'a' with sticky mode
 * on, 'a' with prefix key, and 'a' with ChromeVox modifier held down. These
 * should all be equal to each other.
 */
TEST_F('CvoxKeySequenceUnitTest', 'FourWayEquality', function() {
  var commandSequence = new cvox.KeySequence(this.aEvent, true);
  var stickySequence = new cvox.KeySequence(this.aEventSticky, false);
  var prefixSequence = new cvox.KeySequence(this.aEventPrefix, false);
  var cvoxModifierSequence = new cvox.KeySequence(this.altAEvent);

  assertTrue(commandSequence.equals(stickySequence));
  assertTrue(commandSequence.equals(prefixSequence));
  assertTrue(commandSequence.equals(cvoxModifierSequence));

  assertTrue(stickySequence.equals(commandSequence));
  assertTrue(stickySequence.equals(prefixSequence));
  assertTrue(stickySequence.equals(cvoxModifierSequence));

  assertTrue(prefixSequence.equals(commandSequence));
  assertTrue(prefixSequence.equals(stickySequence));
  assertTrue(prefixSequence.equals(cvoxModifierSequence));

  assertTrue(cvoxModifierSequence.equals(commandSequence));
  assertTrue(cvoxModifierSequence.equals(stickySequence));
  assertTrue(cvoxModifierSequence.equals(prefixSequence));
});


/**
 * Test equality - 'a' key with Shift modifier and prefix vs. 'a' key with Shift
 * modifier and sticky mode vs. 'a' key with Shift modifier and ChromeVox
 * modifier specified vs. 'a' key with ChromeVox modifier held down.
 * These should all be equal to each other..
*/
TEST_F('CvoxKeySequenceUnitTest', 'ShiftPrefixEquality', function() {
  var shiftAWithModifier = new cvox.KeySequence(this.shiftAEvent, true);
  var shiftAWithPrefix = new cvox.KeySequence(this.shiftAPrefixEvent, false);
  var shiftASticky = new cvox.KeySequence(this.shiftAStickyEvent, false);
  var cvoxShiftA = new cvox.KeySequence(this.altShiftAEvent);

  assertTrue(shiftAWithModifier.equals(shiftAWithPrefix));
  assertTrue(shiftAWithModifier.equals(shiftASticky));
  assertTrue(shiftAWithModifier.equals(cvoxShiftA));

  assertTrue(shiftAWithPrefix.equals(shiftAWithModifier));
  assertTrue(shiftAWithPrefix.equals(shiftASticky));
  assertTrue(shiftAWithPrefix.equals(cvoxShiftA));

  assertTrue(shiftASticky.equals(shiftAWithPrefix));
  assertTrue(shiftASticky.equals(shiftAWithModifier));
  assertTrue(shiftASticky.equals(cvoxShiftA));

  assertTrue(cvoxShiftA.equals(shiftAWithModifier));
  assertTrue(cvoxShiftA.equals(shiftAWithPrefix));
  assertTrue(cvoxShiftA.equals(shiftASticky));
});


/**
 * Test inequality - 'a' with modifier key vs. 'a' without modifier key.
 * These should not be equal.
*/
TEST_F('CvoxKeySequenceUnitTest', 'Inequality', function() {
  var aNoModifier = new cvox.KeySequence(this.aEvent, false);
  var aWithModifier = new cvox.KeySequence(this.aEvent, true);

  assertFalse(aNoModifier.equals(aWithModifier));
  assertFalse(aWithModifier.equals(aNoModifier));
});


/**
 * Test equality - adding an additional key onto a sequence.
 */
TEST_F('CvoxKeySequenceUnitTest', 'CvoxCtrl', function() {
  var cvoxCtrlSequence = new cvox.KeySequence(this.ctrlEvent, true);
  assertTrue(cvoxCtrlSequence.addKeyEvent(this.rightArrowEvent));

  assertEquals(2, cvoxCtrlSequence.length());

  // Can't add more than two key events.
  assertFalse(cvoxCtrlSequence.addKeyEvent(this.rightArrowEvent));

  var cvoxCtrlStickySequence = new cvox.KeySequence(this.ctrlStickyEvent,
                                                    false);
  assertTrue(cvoxCtrlStickySequence.addKeyEvent(this.rightArrowEvent));

  var mockCtrlPrefixSequence = new cvox.KeySequence(this.ctrlPrefixEvent,
                                                    false);
  assertTrue(mockCtrlPrefixSequence.addKeyEvent(this.rightArrowEvent));

  assertTrue(cvoxCtrlSequence.equals(cvoxCtrlStickySequence));
  assertTrue(cvoxCtrlStickySequence.equals(cvoxCtrlSequence));

  assertTrue(cvoxCtrlSequence.equals(mockCtrlPrefixSequence));
  assertTrue(mockCtrlPrefixSequence.equals(cvoxCtrlSequence));

  assertTrue(cvoxCtrlStickySequence.equals(mockCtrlPrefixSequence));
  assertTrue(mockCtrlPrefixSequence.equals(cvoxCtrlStickySequence));
});


/**
 * Test for inequality - key sequences in different orders.
 */
TEST_F('CvoxKeySequenceUnitTest', 'DifferentSequences', function() {
  var cvoxBSequence = new cvox.KeySequence(this.bEvent, true);
  assertTrue(cvoxBSequence.addKeyEvent(this.cEvent));

  var cvoxCSequence = new cvox.KeySequence(this.cEvent, false);
  assertTrue(cvoxCSequence.addKeyEvent(this.bEvent));

  assertFalse(cvoxBSequence.equals(cvoxCSequence));
  assertFalse(cvoxCSequence.equals(cvoxBSequence));
});


/**
 * Tests modifiers (ctrl, alt, etc) - if two sequences have different modifiers
 * held down then they aren't equal.
 */
TEST_F('CvoxKeySequenceUnitTest', 'MoreModifiers', function() {
  var ctrlASequence = new cvox.KeySequence(this.ctrlAEvent, false);
  var ctrlModifierKeyASequence = new cvox.KeySequence(this.ctrlAEvent, true);

  var ctrlBSequence = new cvox.KeySequence(this.ctrlBEvent, false);

  var metaASequence = new cvox.KeySequence(this.metaAEvent, false);

  assertFalse(ctrlASequence.equals(metaASequence));
  assertFalse(ctrlASequence.equals(ctrlModifierKeyASequence));
  assertFalse(ctrlASequence.equals(ctrlBSequence));
});


/**
 * Tests modifier (ctrl, alt, etc) order - if two sequences have the same
 * modifiers but held down in a different order then they aren't equal.
 */
TEST_F('CvoxKeySequenceUnitTest', 'ModifierOrder', function() {
  var ctrlShiftSequence = new cvox.KeySequence(this.ctrlShiftEvent, false);
  var shiftCtrlSequence = new cvox.KeySequence(this.shiftCtrlEvent, true);

  assertFalse(ctrlShiftSequence.equals(shiftCtrlSequence));
});


/**
 * Tests converting from a string to a KeySequence object.
 */
TEST_F('CvoxKeySequenceUnitTest', 'FromStr', function() {
  var ctrlString = cvox.KeySequence.fromStr('Ctrl');
  assertEqualsJSON(ctrlString.keys.ctrlKey, [true]);
  assertEqualsJSON(ctrlString.keys.keyCode, [17]);

  var modifiedLetterString = cvox.KeySequence.fromStr('Ctrl+Z');
  assertEqualsJSON(modifiedLetterString.keys.ctrlKey, [true]);
  assertEqualsJSON(modifiedLetterString.keys.keyCode, [90]);

  var keyCodeString = cvox.KeySequence.fromStr('#9');
  assertEqualsJSON(keyCodeString.keys.keyCode, [9]);

  var modifiedKeyCodeString = cvox.KeySequence.fromStr('Shift+#9');
  assertEqualsJSON(modifiedKeyCodeString.keys.shiftKey, [true]);
  assertEqualsJSON(modifiedKeyCodeString.keys.keyCode, [9]);

  var cvoxLetterString = cvox.KeySequence.fromStr('Cvox+U');
  assertTrue(cvoxLetterString.cvoxModifier);
  assertEqualsJSON(cvoxLetterString.keys.keyCode, [85]);

  var cvoxSequenceString = cvox.KeySequence.fromStr('Cvox+C>T');
  assertTrue(cvoxSequenceString.cvoxModifier);
  assertEqualsJSON(cvoxSequenceString.keys.keyCode, [67, 84]);

  var cvoxSequenceKeyCodeString = cvox.KeySequence.fromStr('Cvox+L>#186');
  assertTrue(cvoxSequenceKeyCodeString.cvoxModifier);
  assertEqualsJSON(cvoxSequenceKeyCodeString.keys.keyCode, [76, 186]);

  var stickyString = cvox.KeySequence.fromStr('Insert>Insert+');
  assertEqualsJSON(stickyString.keys.keyCode, [45, 45]);
});


/**
 * Tests converting from a JSON string to a KeySequence object.
 */
TEST_F('CvoxKeySequenceUnitTest', 'Deserialize', function() {
  var forwardSequence = cvox.KeySequence.deserialize({'cvoxModifier': true,
      'stickyMode': false, 'prefixKey': false, 'keys': {'ctrlKey': [false],
      'searchKeyHeld': [false], 'altKey': [false], 'altGraphKey': [false],
      'shiftKey': [false], 'metaKey': [false], 'keyCode': [40]}});
  assertTrue(forwardSequence.cvoxModifier);
  assertEqualsJSON(forwardSequence.keys.keyCode, [40]);

  var ctrlSequence = cvox.KeySequence.deserialize({'cvoxModifier': false,
      'stickyMode': true, 'prefixKey': false, 'keys': {'ctrlKey': [true],
      'searchKeyHeld': [false], 'altKey': [false], 'altGraphKey': [false],
      'shiftKey': [false], 'metaKey': [false], 'keyCode': [17]}});
  assertEqualsJSON(ctrlSequence.keys.ctrlKey, [true]);
  assertEqualsJSON(ctrlSequence.keys.keyCode, [17]);
});
