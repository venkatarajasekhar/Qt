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
function CvoxFindUtilUnitTest() {}

CvoxFindUtilUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.FindUtil',
  ]
};

TEST_F('CvoxFindUtilUnitTest', 'Links', function() {
  this.loadDoc(function() {/*!
    <div>
      <p id="before">Before</p>
      <h2>Chapter 1</h2>
      <h2>Chapter 2</h2>
      <a href="#c3" id="c3" name="chapter_3"></a>
      <h2 id="c3_2">Chapter 3</h2>
      <h2 id="c4">Chapter 4</h2>
      <h2>Chapter 5</h2>
      <h2>Chapter 6</h2>
      <a href='#c7' id="c7" name="chapter_7"><h2 id="c7_2">Chapter 7</h2></a>
      <h2 id="c8">Chapter 8</h2>
      <p id="after">After</p>
     </div>
  */});

  var sel = cvox.CursorSelection.fromNode($('before'));

  var ret = cvox.FindUtil.findNext(sel, cvox.DomPredicates.linkPredicate);
  assertEquals('c3', ret.start.node.id);
  ret = cvox.FindUtil.findNext(ret, cvox.DomPredicates.linkPredicate);
  assertEquals('c7', ret.start.node.id);

  ret.setReversed(true);
  ret = cvox.FindUtil.findNext(ret, cvox.DomPredicates.linkPredicate);
  assertEquals('c3', ret.start.node.id);
});
