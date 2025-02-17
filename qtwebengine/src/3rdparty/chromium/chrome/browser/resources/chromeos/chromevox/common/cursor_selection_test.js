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
function CvoxCursorSelectionUnitTest() {}

CvoxCursorSelectionUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.CursorSelection'
  ]
};

TEST_F('CvoxCursorSelectionUnitTest', 'Reverse', function() {
  this.loadDoc(function() {/*!
     <div>
       <p id="a">a</p>
       <p id="b">b</p>
     </div>
  */});
  var a = new cvox.Cursor($('a'), 0, '');
  var b = new cvox.Cursor($('b'), 0, '');

  var aa = new cvox.CursorSelection(a, a);
  assertEquals(false, aa.isReversed());
  aa.setReversed(true);
  assertEquals(true, aa.isReversed());

  var ab = new cvox.CursorSelection(a, b);
  assertEquals(false, ab.isReversed());
  ab.setReversed(true);
  assertEquals(true, ab.isReversed());
  assertEquals(true, ab.start.equals(b));
  assertEquals(true, ab.end.equals(a));
  ab.setReversed(false);
  assertEquals(false, ab.isReversed());
  assertEquals(true, ab.start.equals(a));
  assertEquals(true, ab.end.equals(b));

  ab = new cvox.CursorSelection(b, a);
  assertEquals(false, ab.isReversed());
  assertEquals(true, ab.start.equals(a));
  assertEquals(true, ab.end.equals(b));

  var ba = new cvox.CursorSelection(b, a, true);
  assertEquals(true, ba.isReversed());
  assertEquals(true, ba.start.equals(b));
  assertEquals(true, ba.end.equals(a));

  ba = new cvox.CursorSelection(a, b, true);
  assertEquals(true, ba.isReversed());
  assertEquals(true, ba.start.equals(b));
  assertEquals(true, ba.end.equals(a));
});


/** Tests correctness of collapsing selections.  */
TEST_F('CvoxCursorSelectionUnitTest', 'Collapse', function() {
  this.loadDoc(function() {/*!
    <p id='1'>This is a test.</p>
  */});
  var text = $('1').firstChild;
  var a = new cvox.Cursor(text, 0, 'This is a test.');
  var b = new cvox.Cursor(text, 13, 'This is a test.');
  var c = new cvox.Cursor(text, 5, 'This is a test.');
  var d = new cvox.Cursor(text, 8, 'This is a test.');

  var aa = new cvox.CursorSelection(a, a).collapse();
  assertEquals(0, aa.start.index);
  assertEquals(0, aa.end.index);

  var ab = new cvox.CursorSelection(a, b).collapse();
  assertEquals(0, ab.start.index);
  assertEquals(1, ab.end.index);

  var ba = new cvox.CursorSelection(b, a, true).collapse();
  assertEquals(12, ba.absStart().index);
  assertEquals(13, ba.absEnd().index);

  var cd = new cvox.CursorSelection(c, d).collapse();
  assertEquals(5, cd.start.index);
  assertEquals(6, cd.end.index);

  var dc = new cvox.CursorSelection(d, c, true).collapse();
  assertEquals(7, dc.absStart().index);
  assertEquals(8, dc.absEnd().index);
});
