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
function CvoxContentEditableExtractorUnitTest() {}

CvoxContentEditableExtractorUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.ContentEditableExtractor',
  ]
};

TEST_F('CvoxContentEditableExtractorUnitTest', 'EmptyElement', function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox" contentEditable="true"></div>
    </div>
  */});

  var textbox = $('textbox');
  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('', extractor.getText());
  assertEquals(0, extractor.getStartIndex());
  assertEquals(0, extractor.getEndIndex(0));
  assertEquals(0, extractor.getLineIndex(0));
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(0, extractor.getLineEnd(0));
});

/**
 * Test getting text and selections from a single contenteditable node.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'SingleTextNode', function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox" contentEditable="true">Hello</div>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('Hello', extractor.getText());
  assertEquals(0, extractor.getLineIndex(0));
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(5, extractor.getLineEnd(0));
  assertEquals(5, extractor.getStartIndex());
  assertEquals(5, extractor.getEndIndex());

  // Test all possible cursor positions.
  for (var i = 0; i <= 5; i++) {
    setSelection(textbox.firstChild, i, textbox.firstChild, i);
    extractor.update(textbox);
    assertEquals(i, extractor.getStartIndex());
    assertEquals(i, extractor.getEndIndex());
  }

  // Test all possible ways to select one character.
  for (i = 0; i < 5; i++) {
    setSelection(textbox.firstChild, i, textbox.firstChild, i + 1);
    extractor.update(textbox);
    assertEquals(i, extractor.getStartIndex());
    assertEquals(i + 1, extractor.getEndIndex());
  }

  // Test selecting everything.
  setSelection(textbox.firstChild, 0, textbox.firstChild, 5);
  extractor.update(textbox);
  assertEquals(0, extractor.getStartIndex());
  assertEquals(5, extractor.getEndIndex());
});

/**
 * Test getting text and selections from a contenteditable node with
 * nonprinted whitespace.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'TextWithWhitespace',
    function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox" contentEditable="true"> Hello  World </div>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('Hello World', extractor.getText());
  assertEquals(0, extractor.getLineIndex(0));
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(11, extractor.getLineEnd(0));
  assertEquals(11, extractor.getStartIndex());
  assertEquals(11, extractor.getEndIndex());

  // Test all *reasonable* indexes of a selection into this text node
  // and the logical index into the text that these should result in.
  var expectedIndexMap = {
    0: 0,
    1: 0,
    2: 1,
    3: 2,
    4: 3,
    5: 4,
    6: 5,
    // Note: index=7 should never happen
    8: 6,
    9: 7,
    10: 8,
    11: 9,
    12: 10,
    13: 11,
    14: 11
  };
  for (var srcIndex in expectedIndexMap) {
    var dstIndex = expectedIndexMap[srcIndex];
    setSelection(textbox.firstChild, srcIndex, textbox.firstChild, srcIndex);
    extractor.update(textbox);
    assertEquals(dstIndex, extractor.getStartIndex());
    assertEquals(dstIndex, extractor.getEndIndex());
  }
});

/**
 * Test getting text and selections from a contenteditable node with
 * preformatted text.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'Preformatted', function() {
  this.loadDoc(function() {/*!
    <div>
      <pre id="textbox" contentEditable="true">aaaaaaaaaa
bbbbbbbbbb
cccccccccc</pre>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('aaaaaaaaaa\nbbbbbbbbbb\ncccccccccc', extractor.getText());
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(11, extractor.getLineEnd(0));
  assertEquals(11, extractor.getLineStart(1));
  assertEquals(22, extractor.getLineEnd(1));
  assertEquals(22, extractor.getLineStart(2));
  assertEquals(32, extractor.getLineEnd(2));

  // Test all possible cursor positions.
  for (var i = 0; i <= 32; i++) {
    setSelection(textbox.firstChild, i, textbox.firstChild, i);
    extractor.update(textbox);
    assertEquals(i, extractor.getStartIndex());
    assertEquals(i, extractor.getEndIndex());
  }
});

/**
 * Test getting text and selections from a contenteditable node with
 * wrapping.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'WordWrap', function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox"
           style="width: 1em; word-wrap: normal"
           contentEditable="true">One two three</div>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('One\ntwo\nthree', extractor.getText());
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(4, extractor.getLineEnd(0));
  assertEquals(4, extractor.getLineStart(1));
  assertEquals(8, extractor.getLineEnd(1));
  assertEquals(8, extractor.getLineStart(2));
  assertEquals(13, extractor.getLineEnd(2));

  // Test all possible cursor positions.
  for (var i = 0; i <= 13; i++) {
    setSelection(textbox.firstChild, i, textbox.firstChild, i);
    extractor.update(textbox);
    assertEquals(i, extractor.getStartIndex());
    assertEquals(i, extractor.getEndIndex());
  }
});

/**
 * Test getting text and lines from a contenteditable region
 * containing two paragraphs and an explicit line break.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'TwoParas', function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox" contentEditable="true">
        <p>One</p>
        <p>Two<br>Three</p>
      </div>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('One\nTwo\nThree',
               extractor.getText());
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(4, extractor.getLineEnd(0));
  assertEquals(4, extractor.getLineStart(1));
  assertEquals(8, extractor.getLineEnd(1));
  assertEquals(8, extractor.getLineStart(2));
  assertEquals(13, extractor.getLineEnd(2));
});

/**
 * Test getting text and lines from a contenteditable region
 * containing two paragraphs, this time with added whitespace.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'TwoParasWithWhitespace',
    function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox" contentEditable="true">
        <p> One </p>
        <p> Two <br> Three </p>
      </div>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('One\nTwo Three',
               extractor.getText());
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(4, extractor.getLineEnd(0));
  assertEquals(4, extractor.getLineStart(1));
  assertEquals(8, extractor.getLineEnd(1));
  assertEquals(8, extractor.getLineStart(2));
  assertEquals(13, extractor.getLineEnd(2));
});

/**
 * Test getting text and lines from a contenteditable region
 * containing some raw text and then some text in a block-level element.
 */
TEST_F('CvoxContentEditableExtractorUnitTest', 'NodePlusElement', function() {
  this.loadDoc(function() {/*!
    <div>
      <div id="textbox"
           contentEditable="true">One<div>Two<br>Three</div></div>
    </div>
  */});
  var textbox = $('textbox');

  var extractor = new cvox.ContentEditableExtractor();
  extractor.update(textbox);
  assertEquals('One\nTwo\nThree',
               extractor.getText());
  assertEquals(0, extractor.getLineStart(0));
  assertEquals(4, extractor.getLineEnd(0));
  assertEquals(4, extractor.getLineStart(1));
  assertEquals(8, extractor.getLineEnd(1));
  assertEquals(8, extractor.getLineStart(2));
  assertEquals(13, extractor.getLineEnd(2));

  var oneTextNode = textbox.firstChild;
  assertEquals('One', oneTextNode.data);
  var twoTextNode = textbox.firstElementChild.firstChild;
  assertEquals('Two', twoTextNode.data);
  var threeTextNode = twoTextNode.nextSibling.nextSibling;
  assertEquals('Three', threeTextNode.data);

  // End of first line.
  setSelection(oneTextNode, 3, oneTextNode, 3);
  extractor.update(textbox);
  assertEquals(3, extractor.getStartIndex());
  assertEquals(3, extractor.getEndIndex());

  // Beginning of second line.
  setSelection(twoTextNode, 0, twoTextNode, 0);
  extractor.update(textbox);
  assertEquals(4, extractor.getStartIndex());
  assertEquals(4, extractor.getEndIndex());

  // End of second line.
  setSelection(twoTextNode, 3, twoTextNode, 3);
  extractor.update(textbox);
  assertEquals(7, extractor.getStartIndex());
  assertEquals(7, extractor.getEndIndex());

  // Beginning of third line.
  setSelection(threeTextNode, 0, threeTextNode, 0);
  extractor.update(textbox);
  assertEquals(8, extractor.getStartIndex());
  assertEquals(8, extractor.getEndIndex());
});
