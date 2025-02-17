// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['../testing/chromevox_unittest_base.js']);

/**
 * A TTS class implementing speak and stop methods intended only for testing.
 * @constructor
 * @implements cvox.TtsInterface
 */
function TestTts() {
  this.strings = [];
}

/**
 * The strings that were spoken since the last call to get().
 * @type {Array.<string>}
 */
TestTts.prototype.strings;

/**
 * Returns the list of strings spoken since the last time this method was
 * called, and then clears the list.
 * @return {Array.<string>} The list of strings.
 */
TestTts.prototype.get = function() {
  var result = this.strings;
  this.strings = [];
  return result;
};

/** @override */
TestTts.prototype.speak = function(text, queueMode, properties) {
  this.strings.push(text);
};

/** @override */
TestTts.prototype.isSpeaking = function() {
  return false;
};

/** @override */
TestTts.prototype.stop = function() {
  // Do nothing.
};

/** @override */
TestTts.prototype.increaseOrDecreaseProperty =
    function(propertyName, increase) {
  // Do nothing.
};

/**
 * Stores the last braille content.
 * @constructor
 * @implements cvox.BrailleInterface
 */
function TestBraille() {
  this.content = null;
}

/** @override */
TestBraille.prototype.write = function(params) {
  this.content = params;
};

/**
 * Asserts the current braille content.
 *
 * @param {string} text Braille text.
 * @param {number=} opt_start Selection start.
 * @param {number=} opt_end Selection end.
 */
TestBraille.assertContent = function(text, opt_start, opt_end) {
  var c = cvox.ChromeVox.braille.content;
  assertTrue(c != null);
  opt_start = opt_start !== undefined ? opt_start : -1;
  opt_end = opt_end !== undefined ? opt_end : opt_start;
  assertEquals(text, c.text.toString());
  assertEquals(opt_start, c.startIndex);
  assertEquals(opt_end, c.endIndex);
};

/**
 * Test fixture.
 * @constructor
 * @extends {ChromeVoxUnitTestBase}
 */
function CvoxEditableTextUnitTest() {}

CvoxEditableTextUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.ChromeVoxEditableHTMLInput',
    'cvox.ChromeVoxEditableTextBase',
    'cvox.ChromeVoxEventWatcher',
    'cvox.TextChangeEvent',
    'cvox.TtsInterface',
    'cvox.TypingEcho',
  ],

  /** @override */
  setUp: function() {
    // TODO: These tests are all assuming we used the IBeam cursor.
    // We need to add coverage for block cursor.
    cvox.ChromeVoxEditableTextBase.useIBeamCursor = true;
    cvox.ChromeVox.typingEcho = cvox.TypingEcho.CHARACTER_AND_WORD;
    cvox.ChromeVoxEditableTextBase.eventTypingEcho = false;
    cvox.ChromeVox.braille = new TestBraille();

    /** Simple mock. */
    cvox.ChromeVox.msgs = {};

    /**
     * Simply return the message id.
     * @param {string} msg Message id.
     * @return {string} Message id.
     */
    cvox.ChromeVox.msgs.getMsg = function(msg) {
      return msg;
    };
  },

  /**
   * Sets up for a cursor movement test.
   * @param {string} tagName Desired tag name, "input" or "textarea".
   * @return {Object} object containing the editable element, and functions
   *     to prepare, run the test, and tear down.
   * @private
   */
  setUpForCursorTest_: function(tagName) {
    var element, editable;
    switch (tagName) {
      case 'input':
        element = document.createElement('input');
        editable = new cvox.ChromeVoxEditableHTMLInput(element, new TestTts());
        break;
      case 'textarea':
        element = document.createElement('textarea');
        editable = new cvox.ChromeVoxEditableTextArea(element, new TestTts());
        break;
      default:
        throw 'invalid tagName in setUpForCursorTest_';
    }
    document.body.appendChild(element);
    element.focus();

    var expect = function(str) {
      assertEquals(element.selectionStart, element.selectionEnd);
      assertEquals(str, element.value.substring(0, element.selectionStart) +
          '|' + element.value.substring(element.selectionEnd));
    };
    return {
      editable: editable,
      expect: expect,
      prepare: function(str) {
        var position = str.indexOf('|');
        var value = str.substring(0, position) + str.substring(position + 1);
        element.value = value;
        element.selectionStart = element.selectionEnd = position;
        editable.update(true /* triggeredByUser */);
        expect(str);
      },
      tearDown: function() {
        document.body.removeChild(element);
      }
    };
  }
};

TEST_F('CvoxEditableTextUnitTest', 'CursorNavigation', function() {
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('Hello', 0, 0, false, tts);

  obj.changed(new cvox.TextChangeEvent('Hello', 1, 1));
  obj.changed(new cvox.TextChangeEvent('Hello', 2, 2));
  obj.changed(new cvox.TextChangeEvent('Hello', 3, 3));
  obj.changed(new cvox.TextChangeEvent('Hello', 4, 4));
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  obj.changed(new cvox.TextChangeEvent('Hello', 4, 4));
  obj.changed(new cvox.TextChangeEvent('Hello', 3, 3));
  assertEqualStringArrays(['H', 'e', 'l', 'l', 'o',
                            'o', 'l'], tts.get());
  obj.changed(new cvox.TextChangeEvent('Hello', 0, 0));
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  assertEqualStringArrays(['Hel', 'Hello'], tts.get());
});

/** Test typing words. */
TEST_F('CvoxEditableTextUnitTest', 'TypingWords', function() {
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('', 0, 0, false, tts);
  obj.changed(new cvox.TextChangeEvent('H', 1, 1));
  obj.changed(new cvox.TextChangeEvent('He', 2, 2));
  obj.changed(new cvox.TextChangeEvent('Hel', 3, 3));
  obj.changed(new cvox.TextChangeEvent('Hell', 4, 4));
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  obj.changed(new cvox.TextChangeEvent('Hello,', 6, 6));
  obj.changed(new cvox.TextChangeEvent('Hello, ', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, W', 8, 8));
  obj.changed(new cvox.TextChangeEvent('Hello, Wo', 9, 9));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 10, 10));
  obj.changed(new cvox.TextChangeEvent('Hello, Worl', 11, 11));
  obj.changed(new cvox.TextChangeEvent('Hello, World', 12, 12));
  obj.changed(new cvox.TextChangeEvent('Hello, World.', 13, 13));
  assertEqualStringArrays(['H', 'e', 'l', 'l', 'o', 'Hello,',
                            ' ',
                            'W', 'o', 'r', 'l', 'd', 'World.'],
                           tts.get());

  // Backspace
  obj.changed(new cvox.TextChangeEvent('Hello, World', 12, 12));
  obj.changed(new cvox.TextChangeEvent('Hello, Worl', 11, 11));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 10, 10));
  assertEqualStringArrays(['.', 'd', 'l'], tts.get());

  // Forward-delete
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 9, 9));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 8, 8));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, or', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, r', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, ', 7, 7));
  assertEqualStringArrays(['r', 'o', 'W', 'W', 'o', 'r'], tts.get());

  // Clear all
  obj.changed(new cvox.TextChangeEvent('', 0, 0));
  assertEqualStringArrays(['Hello, , deleted'], tts.get());

  // Paste / insert a whole word
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  assertEqualStringArrays(['Hello'], tts.get());
  obj.changed(new cvox.TextChangeEvent('Hello, World', 12, 12));
  assertEqualStringArrays([', World'], tts.get());
});

/** Test selection. */
TEST_F('CvoxEditableTextUnitTest', 'Selection', function() {
  var tts = new TestTts();
  var obj =
      new cvox.ChromeVoxEditableTextBase('Hello, world.', 0, 0, false, tts);
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 1));
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 2));
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 3));
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 4));
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 5));
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 6));
  assertEqualStringArrays(['H', 'selected',
                            'e', 'added_to_selection',
                            'l', 'added_to_selection',
                            'l', 'added_to_selection',
                            'o', 'added_to_selection',
                            ',', 'added_to_selection'],
                           tts.get());
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 0, 12));
  assertEqualStringArrays([' world', 'added_to_selection'],
                           tts.get());
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 1, 12));
  assertEqualStringArrays(['H', 'removed_from_selection'],
                           tts.get());
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 2, 5));
  assertEqualStringArrays(['llo', 'selected'],
                           tts.get());
  obj.changed(new cvox.TextChangeEvent('Hello, world.', 2, 2));
  assertEqualStringArrays(['Unselected'],
                           tts.get());
});


/** Test multi-line text. */
TEST_F('CvoxEditableTextUnitTest', 'MultiLineText', function() {
  var str = 'This string\nspans\nfive lines.\n  \n';
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase(str, 0, 0, false, tts);
  obj.multiline = true;
  obj.getLineIndex = function(index) {
    if (index >= 33) {
      return 4;
    } else if (index >= 30) {
      return 3;
    } else if (index >= 18) {
      return 2;
    } else if (index >= 12) {
      return 1;
    } else {
      return 0;
    }
  };
  obj.getLineStart = function(index) {
    return [0, 12, 18, 30, 33][index];
  };
  obj.getLineEnd = function(index) {
    return [11, 17, 29, 32, 33][index];
  };
  assertEquals('This string', obj.getLine(0));
  obj.changed(new cvox.TextChangeEvent(str, 12, 12));
  assertEqualStringArrays(['spans'], tts.get());
  TestBraille.assertContent('spans', 0);
  obj.changed(new cvox.TextChangeEvent(str, 18, 18));
  assertEqualStringArrays(['five lines.'], tts.get());
  TestBraille.assertContent('five lines.', 0);
  obj.changed(new cvox.TextChangeEvent(str, 30, 30));
  assertEqualStringArrays(['text_box_whitespace'], tts.get());
  TestBraille.assertContent('  ', 0);
  obj.changed(new cvox.TextChangeEvent(str, 33, 33));
  assertEqualStringArrays(['text_box_blank'], tts.get());
  TestBraille.assertContent('', 0);
  obj.changed(new cvox.TextChangeEvent(str, 0, 1));
  assertEqualStringArrays(['T', 'selected'], tts.get());
  TestBraille.assertContent('This string', 0, 1);
  obj.changed(new cvox.TextChangeEvent(str, 0, 12));
  assertEqualStringArrays(['his string\n', 'added_to_selection'],
      tts.get());
  // Newline stripped, thus 11, not 12.
  TestBraille.assertContent('This string', 0, 11);
  obj.changed(new cvox.TextChangeEvent(str, 0, str.length));
  assertEqualStringArrays([str.substr(12), 'added_to_selection'],
      tts.get());
  TestBraille.assertContent('This string', 0, 11);
  obj.changed(new cvox.TextChangeEvent(str, 12, 19));
  assertEqualStringArrays(['spans\nf', 'selected'], tts.get());
  TestBraille.assertContent('spans', 0, 5);
});


/**
 * Test autocomplete; suppose a user is typing "google.com/firefox" into an
 * address bar, and it's being autocompleted. Sometimes it's autocompleted
 * as they type, sometimes there's a short delay.
 */
TEST_F('CvoxEditableTextUnitTest', 'Autocomplete', function() {
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('', 0, 0, false, tts);

  // User types 'g'
  obj.changed(new cvox.TextChangeEvent('g', 1, 1));
  assertEqualStringArrays(['g'], tts.get());

  // The rest of 'google.com' is autocompleted and automatically selected.
  obj.changed(new cvox.TextChangeEvent('google.com', 1, 10));
  assertEqualStringArrays(['oogle.com, oogle.com'], tts.get());

  // The user doesn't realize it and types a few more characters of 'google.com'
  // and this changes the selection (unselecting) as the user types them.
  obj.changed(new cvox.TextChangeEvent('google.com', 2, 10));
  assertEqualStringArrays(['o', 'ogle.com'], tts.get());
  obj.changed(new cvox.TextChangeEvent('google.com', 3, 10));
  assertEqualStringArrays(['o', 'gle.com'], tts.get());
  obj.changed(new cvox.TextChangeEvent('google.com', 4, 10));
  assertEqualStringArrays(['g', 'le.com'], tts.get());

  // The user presses right-arrow, which fully unselects the remaining text.
  obj.changed(new cvox.TextChangeEvent('google.com', 10, 10));
  assertEqualStringArrays(['Unselected'], tts.get());

  // The user types '/'
  obj.changed(new cvox.TextChangeEvent('google.com/', 11, 11));
  assertEqualStringArrays(['com/'], tts.get());

  // The user types 'f', and 'finance' is autocompleted
  obj.changed(new cvox.TextChangeEvent('google.com/finance', 12, 18));
  assertEqualStringArrays(['finance, inance'], tts.get());

  // The user types 'i'
  obj.changed(new cvox.TextChangeEvent('google.com/finance', 13, 18));
  assertEqualStringArrays(['i', 'nance'], tts.get());

  // The user types 'r', now 'firefox' is autocompleted
  obj.changed(new cvox.TextChangeEvent('google.com/firefox', 14, 18));
  assertEqualStringArrays(['refox, efox'], tts.get());

  // The user presses right-arrow to accept the completion.
  obj.changed(new cvox.TextChangeEvent('google.com/firefox', 18, 18));
  assertEqualStringArrays(['Unselected'], tts.get());
});


/**
 * Test a few common scenarios where text is replaced.
 */
TEST_F('CvoxEditableTextUnitTest', 'ReplacingText', function() {
  // Initial value is Alabama.
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('Alabama', 0, 0, false, tts);

  // Entire text replaced with Alaska.
  obj.changed(new cvox.TextChangeEvent('Alaska', 0, 0));
  assertEqualStringArrays(['Alaska'], tts.get());

  // Entire text selected.
  obj.changed(new cvox.TextChangeEvent('Alaska', 0, 6));
  assertEqualStringArrays(['Alaska', 'selected'], tts.get());

  // Entire text replaced with Arizona.
  obj.changed(new cvox.TextChangeEvent('Arizona', 7, 7));
  assertEqualStringArrays(['Arizona'], tts.get());

  // Entire text selected.
  obj.changed(new cvox.TextChangeEvent('Arizona', 0, 7));
  assertEqualStringArrays(['Arizona', 'selected'], tts.get());

  // Click between 'r' and 'i'.
  obj.changed(new cvox.TextChangeEvent('Arizona', 2, 2));
  assertEqualStringArrays(['Unselected'], tts.get());

  // Next character removed from selection.
  obj.changed(new cvox.TextChangeEvent('Arizona', 2, 7));
  assertEqualStringArrays(['izona', 'selected'], tts.get());

  // Selection replaced with "kansas" to make Arkansas.  This time it
  // says "kansas" because the deleted text was selected.
  obj.changed(new cvox.TextChangeEvent('Arkansas', 8, 8));
  assertEqualStringArrays(['kansas'], tts.get());
});


/**
 * Test feedback when text changes in a long sentence.
 */
TEST_F('CvoxEditableTextUnitTest', 'ReplacingLongText', function() {
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase(
    'I love deadlines. I like the whooshing sound they make as they fly by.',
    0, 0, false, tts);

  // Change the whole sentence without moving the cursor. It should speak
  // only the part that changed, but it should speak whole words.
  obj.changed(new cvox.TextChangeEvent(
    'I love deadlines. I love the whooshing sounds they make as they fly by.',
    0, 0));
  assertEqualStringArrays(['love the whooshing sounds'], tts.get());
});

/** Tests character echo. */
TEST_F('CvoxEditableTextUnitTest', 'CharacterEcho', function() {
  cvox.ChromeVox.typingEcho = cvox.TypingEcho.CHARACTER;
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('', 0, 0, false, tts);
  obj.changed(new cvox.TextChangeEvent('H', 1, 1));
  obj.changed(new cvox.TextChangeEvent('He', 2, 2));
  obj.changed(new cvox.TextChangeEvent('Hel', 3, 3));
  obj.changed(new cvox.TextChangeEvent('Hell', 4, 4));
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  obj.changed(new cvox.TextChangeEvent('Hello,', 6, 6));
  obj.changed(new cvox.TextChangeEvent('Hello, ', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, W', 8, 8));
  obj.changed(new cvox.TextChangeEvent('Hello, Wo', 9, 9));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 10, 10));
  obj.changed(new cvox.TextChangeEvent('Hello, Worl', 11, 11));
  obj.changed(new cvox.TextChangeEvent('Hello, World', 12, 12));
  obj.changed(new cvox.TextChangeEvent('Hello, World.', 13, 13));
  assertEqualStringArrays(
      ['H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '.'],
       tts.get());
});


/** Tests word echo. */
TEST_F('CvoxEditableTextUnitTest', 'WordEcho', function() {
  cvox.ChromeVox.typingEcho = cvox.TypingEcho.WORD;
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('', 0, 0, false, tts);
  obj.changed(new cvox.TextChangeEvent('H', 1, 1));
  obj.changed(new cvox.TextChangeEvent('He', 2, 2));
  obj.changed(new cvox.TextChangeEvent('Hel', 3, 3));
  obj.changed(new cvox.TextChangeEvent('Hell', 4, 4));
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  obj.changed(new cvox.TextChangeEvent('Hello,', 6, 6));
  obj.changed(new cvox.TextChangeEvent('Hello, ', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, W', 8, 8));
  obj.changed(new cvox.TextChangeEvent('Hello, Wo', 9, 9));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 10, 10));
  obj.changed(new cvox.TextChangeEvent('Hello, Worl', 11, 11));
  obj.changed(new cvox.TextChangeEvent('Hello, World', 12, 12));
  obj.changed(new cvox.TextChangeEvent('Hello, World.', 13, 13));
  assertEqualStringArrays(
      ['Hello,', 'World.'],
       tts.get());
});


/** Tests no echo. */
TEST_F('CvoxEditableTextUnitTest', 'NoEcho', function() {
  cvox.ChromeVox.typingEcho = cvox.TypingEcho.NONE;
  var tts = new TestTts();
  var obj = new cvox.ChromeVoxEditableTextBase('', 0, 0, false, tts);
  obj.changed(new cvox.TextChangeEvent('H', 1, 1));
  obj.changed(new cvox.TextChangeEvent('He', 2, 2));
  obj.changed(new cvox.TextChangeEvent('Hel', 3, 3));
  obj.changed(new cvox.TextChangeEvent('Hell', 4, 4));
  obj.changed(new cvox.TextChangeEvent('Hello', 5, 5));
  obj.changed(new cvox.TextChangeEvent('Hello,', 6, 6));
  obj.changed(new cvox.TextChangeEvent('Hello, ', 7, 7));
  obj.changed(new cvox.TextChangeEvent('Hello, W', 8, 8));
  obj.changed(new cvox.TextChangeEvent('Hello, Wo', 9, 9));
  obj.changed(new cvox.TextChangeEvent('Hello, Wor', 10, 10));
  obj.changed(new cvox.TextChangeEvent('Hello, Worl', 11, 11));
  obj.changed(new cvox.TextChangeEvent('Hello, World', 12, 12));
  obj.changed(new cvox.TextChangeEvent('Hello, World.', 13, 13));
  assertEqualStringArrays(
      [],
       tts.get());
});

/** Tests cursor movement in an input field by character. */
TEST_F('CvoxEditableTextUnitTest', 'CursorMovementByCharacter', function() {
  var test = this.setUpForCursorTest_('input');
  var editable = test.editable, prepare = test.prepare, expect = test.expect;
  try {
    // Moving near the beginning of the text.
    prepare('|"Hello," says Sally.');
    editable.moveCursorToPreviousCharacter();
    expect('|"Hello," says Sally.');
    editable.moveCursorToNextCharacter();
    expect('"|Hello," says Sally.');
    editable.moveCursorToNextCharacter();
    expect('"H|ello," says Sally.');

    // Moving near the end of the text.
    prepare('"Hello," says Sally|.');
    editable.moveCursorToPreviousCharacter();
    expect('"Hello," says Sall|y.');
    editable.moveCursorToNextCharacter();
    expect('"Hello," says Sally|.');
    editable.moveCursorToNextCharacter();
    expect('"Hello," says Sally.|');
    editable.moveCursorToNextCharacter();
    expect('"Hello," says Sally.|');
  } finally {
    test.tearDown();
  }
});

/** Tests cursor movement in an input field by word. */
TEST_F('CvoxEditableTextUnitTest', 'CursorMovementByWord', function() {
  var test = this.setUpForCursorTest_('input');
  var editable = test.editable, prepare = test.prepare, expect = test.expect;
  try {
    // Moving forward.
    prepare('"He|llo," says Sally.');
    editable.moveCursorToNextWord();
    expect('"Hello|," says Sally.');
    editable.moveCursorToNextWord();
    expect('"Hello," says| Sally.');
    editable.moveCursorToNextWord();
    expect('"Hello," says Sally|.');
    editable.moveCursorToNextWord();
    expect('"Hello," says Sally.|');
    editable.moveCursorToNextWord();
    expect('"Hello," says Sally.|');

    // Moving backward.
    prepare('"Hello," says S|ally.');
    editable.moveCursorToPreviousWord();
    expect('"Hello," says |Sally.');
    editable.moveCursorToPreviousWord();
    expect('"Hello," |says Sally.');
    editable.moveCursorToPreviousWord();
    expect('"|Hello," says Sally.');
    editable.moveCursorToPreviousWord();
    expect('|"Hello," says Sally.');
    editable.moveCursorToPreviousWord();
    expect('|"Hello," says Sally.');
  } finally {
    test.tearDown();
  }
});

/** Tests that character and word movement still work in <textarea>. */
TEST_F('CvoxEditableTextUnitTest', 'CursorMovementTextArea', function() {
  var test = this.setUpForCursorTest_('textarea');
  var editable = test.editable, prepare = test.prepare, expect = test.expect;
  try {
    prepare('|Hello, Larry.\nHello, Sergey.');
    editable.moveCursorToNextCharacter();
    expect('H|ello, Larry.\nHello, Sergey.');
    editable.moveCursorToNextWord();
    expect('Hello|, Larry.\nHello, Sergey.');
    editable.moveCursorToNextWord();
    expect('Hello, Larry|.\nHello, Sergey.');
    editable.moveCursorToNextWord();
    expect('Hello, Larry.\nHello|, Sergey.');
    editable.moveCursorToNextCharacter();
    expect('Hello, Larry.\nHello,| Sergey.');
    editable.moveCursorToPreviousWord();
    expect('Hello, Larry.\n|Hello, Sergey.');
    editable.moveCursorToPreviousCharacter();
    expect('Hello, Larry.|\nHello, Sergey.');
  } finally {
    test.tearDown();
  }
});

/** Tests that line navigation works. */
TEST_F('CvoxEditableTextUnitTest', 'CursorMovementByLine', function() {
  var test = this.setUpForCursorTest_('textarea');
  var editable = test.editable, prepare = test.prepare, expect = test.expect;
  try {
    prepare('123\n1234\n1234|5\n\nHi');
    editable.moveCursorToPreviousLine();
    expect('123\n1234|\n12345\n\nHi');
    editable.moveCursorToPreviousLine();
    expect('123|\n1234\n12345\n\nHi');
    editable.moveCursorToNextLine();
    expect('123\n123|4\n12345\n\nHi');
    editable.moveCursorToNextLine();
    expect('123\n1234\n123|45\n\nHi');
    editable.moveCursorToNextLine();
    expect('123\n1234\n12345\n|\nHi');
    editable.moveCursorToNextLine();
    expect('123\n1234\n12345\n\n|Hi');
    editable.moveCursorToNextLine();
    expect('123\n1234\n12345\n\nHi|');

    prepare('foo|bar');
    editable.moveCursorToPreviousLine();
    expect('|foobar');
    editable.moveCursorToPreviousLine();
    expect('|foobar');
    editable.moveCursorToNextLine();
    expect('foobar|');
    editable.moveCursorToNextLine();
    expect('foobar|');
  } finally {
    test.tearDown();
  }
});

/** Tests that paragraph navigation works. */
TEST_F('CvoxEditableTextUnitTest', 'CursorMovementByParagraph', function() {
  var test = this.setUpForCursorTest_('textarea');
  var editable = test.editable, prepare = test.prepare, expect = test.expect;
  try {
    prepare('Para|graph 1\nParagraph 2\nParagraph 3');
    editable.moveCursorToNextParagraph();
    expect('Paragraph 1\n|Paragraph 2\nParagraph 3');
    editable.moveCursorToNextParagraph();
    expect('Paragraph 1\nParagraph 2\n|Paragraph 3');
    editable.moveCursorToNextParagraph();
    expect('Paragraph 1\nParagraph 2\nParagraph 3|');
    editable.moveCursorToPreviousParagraph();
    expect('Paragraph 1\nParagraph 2\n|Paragraph 3');
    editable.moveCursorToPreviousParagraph();
    expect('Paragraph 1\n|Paragraph 2\nParagraph 3');
    editable.moveCursorToPreviousParagraph();
    expect('|Paragraph 1\nParagraph 2\nParagraph 3');
  } finally {
    test.tearDown();
  }
});

/** Tests normalization of TextChangeEvent's */
TEST_F('CvoxEditableTextUnitTest', 'TextChangeEvent', function() {
  var event1 = new cvox.TextChangeEvent('foo', 0, 1, true);
  var event2 = new cvox.TextChangeEvent('foo', 1, 0, true);
  var event3 = new cvox.TextChangeEvent('foo', 1, 1, true);

  assertEquals(0, event1.start);
  assertEquals(1, event1.end);

  assertEquals(0, event2.start);
  assertEquals(1, event2.end);

  assertEquals(1, event3.start);
  assertEquals(1, event3.end);
});
