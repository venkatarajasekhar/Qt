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
function CvoxSelectionUtilUnitTest() {}

CvoxSelectionUtilUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.SelectionUtil'
  ]
};

TEST_F('CvoxSelectionUtilUnitTest', 'SimpleFindPos', function() {
  this.loadDoc(function() {/*!
    <div id="foo" style="position:absolute;top:50px">
    </div>
  */});
  element = $('foo');
  assertEquals(cvox.SelectionUtil.findPos_(element)[1], 50);
});
