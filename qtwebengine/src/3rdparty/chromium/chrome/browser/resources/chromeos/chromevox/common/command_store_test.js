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
function CvoxCommandStoreUnitTest() {}

CvoxCommandStoreUnitTest.prototype = {
  __proto__: ChromeVoxUnitTestBase.prototype,

  /** @override */
  closureModuleDeps: [
    'cvox.ChromeVoxUserCommands',
    'cvox.CommandStore',
  ]
};

TEST_F('CvoxCommandStoreUnitTest', 'TableData', function() {
  var categories = cvox.CommandStore.categories();
  assertEquals(10, categories.length);
  assertEquals('modifier_keys', categories[0]);
  assertEquals('controlling_speech', categories[1]);
  assertEquals('navigation', categories[2]);
  assertEquals('information', categories[3]);
  assertEquals('help_commands', categories[4]);
  assertEquals('overview', categories[5]);
  assertEquals('jump_commands', categories[6]);
  assertEquals('tables', categories[7]);

  assertEquals('stop_speech_key',
               cvox.CommandStore.messageForCommand('stopSpeech'));
  assertEquals('controlling_speech',
               cvox.CommandStore.categoryForCommand('stopSpeech'));

  var controllingSpeechCmds =
      cvox.CommandStore.commandsForCategory('controlling_speech');
  assertEquals(11, controllingSpeechCmds.length);
  assertEquals('stopSpeech', controllingSpeechCmds[0]);
  assertEquals('toggleChromeVox', controllingSpeechCmds[1]);
  assertEquals('decreaseTtsRate', controllingSpeechCmds[2]);
  assertEquals('increaseTtsRate', controllingSpeechCmds[3]);
  assertEquals('decreaseTtsPitch', controllingSpeechCmds[4]);
  assertEquals('increaseTtsPitch', controllingSpeechCmds[5]);
});


/** Tests that undefined is returned for bad queries. */
TEST_F('CvoxCommandStoreUnitTest', 'InvalidQueries', function() {
  assertThat(cvox.CommandStore.commandsForCategory('foo'), eqJSON([]));
  assertTrue(undefined == cvox.CommandStore.categoryForCommand('foo'));
  assertTrue(undefined == cvox.CommandStore.messageForCommand('foo'));
});


/** Tests the validity of every command. */
TEST_F('CvoxCommandStoreUnitTest', 'CommandValidity', function() {
  var categories = cvox.CommandStore.categories();
  for (var i = 0; i < categories.length; i++) {
    var commands = cvox.CommandStore.commandsForCategory(categories[i]);
    for (j = 0; j < commands.length; j++) {
      var command = commands[j];
      assertEquals(command + ' function',
          command + ' ' + typeof(cvox.ChromeVoxUserCommands.commands[command]));
    }
  }
});
