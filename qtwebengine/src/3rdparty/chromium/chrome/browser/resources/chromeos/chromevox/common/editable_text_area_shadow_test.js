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
function CvoxShadowUnitTest() {}

CvoxShadowUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.EditableTextAreaShadow'
  ]
};

TEST_F('CvoxShadowUnitTest', 'MultilineLines', function() {
  this.loadDoc(function() {/*!
    <div><textarea id="area">
one

two

three
</textarea></div> */});

  var area = $('area');

  var shadow = new cvox.EditableTextAreaShadow();
  shadow.update(area);
  assertEquals(0, shadow.getLineIndex(0));
  assertEquals(0, shadow.getLineIndex(3));
  assertEquals(1, shadow.getLineIndex(4));
  assertEquals(2, shadow.getLineIndex(5));
  assertEquals(2, shadow.getLineIndex(8));
  assertEquals(3, shadow.getLineIndex(9));
  assertEquals(4, shadow.getLineIndex(10));
  assertEquals(4, shadow.getLineIndex(14));
});

/**
 * Test the get line of a multiline textarea with wrapping instead of
 * explicit newlines.
 */
TEST_F('CvoxShadowUnitTest', 'MultilineWrap', function() {
  this.loadDoc(function() {/*!
    <div><textarea id="area"
          cols=4 rows=20>One two thr fou fiv six sev eig</textarea>
    </div> */});

  var area = $('area');

  var shadow = new cvox.EditableTextAreaShadow();
  shadow.update(area);
  for (var i = 0; i < 32; i++) {
    assertEquals(Math.floor(i / 4), shadow.getLineIndex(i));
  }
});
