// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['../testing/chromevox_unittest_base.js']);

/**
 * Test fixture for aria_util.js.
 * @constructor
 * @extends {ChromeVoxUnitTestBase}
 */
function CvoxAriaUtilUnitTest() {}

CvoxAriaUtilUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.AriaUtil',
    'cvox.ChromeVox',
    'cvox.DomUtil',]
};

TEST_F('CvoxAriaUtilUnitTest', 'GetStateGridWithActiveCell', function() {
  this.loadDoc(function() {/*!
    <div id="grid" role="grid" aria-activedescendant="cell">
      <div role="row">
        <div id="cell" role="gridcell">
      </div>
    </div>
  */});
  assertThat(
      cvox.AriaUtil.getStateMsgs($('grid'), true),
      eqJSON([['aria_role_gridcell_pos', 1, 1]]));
});

TEST_F('CvoxAriaUtilUnitTest', 'GetActiveDescendant', function() {
  this.loadDoc(function() {/*!
    <div id="top" aria-activedescendant="child">
      <div id="child" />
    </div>
    <div id="top_2" aria-activedescendant="child_2">
      <div id="child_2" aria-activedescendant="grandchild_2">
        <div id="grandchild_2" />
      </div>
    </div>

    <h1>The buggy cases.</h1>
    <div id="loop" aria-activedescendant="loop" />
    <div id="circleA" aria-activedescendant="circleB">
      <div id="circleB" aria-activedescendant="circleA" />
    </div>
  */});

  // The typical case.
  var topElt = $('top');
  var childElt = $('child');
  assertEquals(childElt, cvox.AriaUtil.getActiveDescendant(topElt));

  // childElt has not aria-activedescendant, so return null.
  assertEquals(null, cvox.AriaUtil.getActiveDescendant(childElt));

  // The chained case.
  var top2Elt = $('top_2');
  var grandchild2Elt = $('grandchild_2');
  assertEquals(grandchild2Elt, cvox.AriaUtil.getActiveDescendant(top2Elt));

  // The buggy cases.  These are invalid, so return null as if the
  // aria-activedescendant tags did not exist.
  var loopElt = $('loop');
  assertEquals(null, cvox.AriaUtil.getActiveDescendant(loopElt));

  var circleAElt = $('circleA');
  assertEquals(null, cvox.AriaUtil.getActiveDescendant(circleAElt));
});

TEST_F('CvoxAriaUtilUnitTest', 'ListIndexAndState', function() {
  this.loadDoc(function() {/*!
    <div id="l" role="listbox" tabindex="0" aria-activedescendant="l2">
      <div id="l1" role="option">A</div>
      <div id="l2" role="option">B</div>
      <div id="l3" role="option">C</div>
    </div>
    <div id="a" role="listbox" tabindex="0" aria-activedescendant="a2">
      <div id="a1" role="option" aria-setsize="10" aria-posinset="5">A</div>
      <div id="a2" role="option" aria-setsize="20" aria-posinset="15">B</div>
      <div id="a3" role="option" aria-setsize="30" aria-posinset="25">C</div>
    </div>
    <div id="b" role="listbox" tabindex="0" aria-activedescendant="b2">
      <div id="b1" role="option" aria-posinset="3">A</div>
      <div id="b2" role="option" aria-posinset="2">B</div>
      <div id="b3" role="option" aria-posinset="1">C</div>
    </div>
  */});

  var optionElt = $('l2');
  assertThat(
      cvox.AriaUtil.getStateMsgs(optionElt),
      eqJSON([['list_position', 2, 3]]));

  var ariaOptionElt = $('a2');
  assertThat(
      cvox.AriaUtil.getStateMsgs(ariaOptionElt),
      eqJSON([['list_position', 15, 20]]));

  ariaOptionElt = $('b3');
  assertThat(
      cvox.AriaUtil.getStateMsgs(ariaOptionElt),
      eqJSON([['list_position', 1, 3]]));
});

TEST_F('CvoxAriaUtilUnitTest', 'GetLiveRegions', function() {
  this.loadDoc(function() {/*!
   <div id="outer">
    <div id="progress" role="progressbar" aria-live="polite" aria-valuenow="1">
      <div id="ptext">
        1% complete.
      </div>
    </div>
    <div id="progress2" role="progressbar" aria-live="polite" aria-valuenow="1">
      <div id="ptext2">
        1% complete.
      </div>
    </div>
   </div>
  */});

  var progressLiveRegions = cvox.AriaUtil.getLiveRegions(progress);
  assertEquals(1, progressLiveRegions.length);
  assertNotEquals(-1, progressLiveRegions.indexOf(progress));

  var outerLiveRegions = cvox.AriaUtil.getLiveRegions(outer);
  assertEquals(2, outerLiveRegions.length);
  assertNotEquals(-1, outerLiveRegions.indexOf(progress));
  assertNotEquals(-1, outerLiveRegions.indexOf(progress2));

  // getLiveRegions works walking up the tree as well.
  var ptextLiveRegions = cvox.AriaUtil.getLiveRegions(ptext);
  assertEquals(1, ptextLiveRegions.length);
  assertNotEquals(-1, ptextLiveRegions.indexOf(progress));
});
