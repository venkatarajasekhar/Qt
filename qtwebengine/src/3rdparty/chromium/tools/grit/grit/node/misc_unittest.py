#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for misc.GritNode'''


import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import unittest
import StringIO

from grit import grd_reader
import grit.exception
from grit import util
from grit.format import rc
from grit.node import misc


class GritNodeUnittest(unittest.TestCase):
  def testUniqueNameAttribute(self):
    try:
      restree = grd_reader.Parse(
        util.PathFromRoot('grit/testdata/duplicate-name-input.xml'))
      self.fail('Expected parsing exception because of duplicate names.')
    except grit.exception.Parsing:
      pass  # Expected case

  def testReadFirstIdsFromFile(self):
    test_resource_ids = os.path.join(os.path.dirname(__file__), '..',
                                     'testdata', 'resource_ids')
    base_dir = os.path.dirname(test_resource_ids)

    src_dir, id_dict = misc._ReadFirstIdsFromFile(
        test_resource_ids,
        {
          'FOO': os.path.join(base_dir, 'bar'),
          'SHARED_INTERMEDIATE_DIR': os.path.join(base_dir,
                                                  'out/Release/obj/gen'),
        })
    self.assertEqual({}, id_dict.get('bar/file.grd', None))
    self.assertEqual({},
        id_dict.get('out/Release/obj/gen/devtools/devtools.grd', None))

    src_dir, id_dict = misc._ReadFirstIdsFromFile(
        test_resource_ids,
        {
          'SHARED_INTERMEDIATE_DIR': '/outside/src_dir',
        })
    self.assertEqual({}, id_dict.get('devtools.grd', None))

class IfNodeUnittest(unittest.TestCase):
  def testIffyness(self):
    grd = grd_reader.Parse(StringIO.StringIO('''
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <messages>
            <if expr="'bingo' in defs">
              <message name="IDS_BINGO">
                Bingo!
              </message>
            </if>
            <if expr="'hello' in defs">
              <message name="IDS_HELLO">
                Hello!
              </message>
            </if>
            <if expr="lang == 'fr' or 'FORCE_FRENCH' in defs">
              <message name="IDS_HELLO" internal_comment="French version">
                Good morning
              </message>
            </if>
            <if expr="is_win">
              <message name="IDS_ISWIN">is_win</message>
            </if>
          </messages>
        </release>
      </grit>'''), dir='.')

    messages_node = grd.children[0].children[0]
    bingo_message = messages_node.children[0].children[0]
    hello_message = messages_node.children[1].children[0]
    french_message = messages_node.children[2].children[0]
    is_win_message = messages_node.children[3].children[0]

    self.assertTrue(bingo_message.name == 'message')
    self.assertTrue(hello_message.name == 'message')
    self.assertTrue(french_message.name == 'message')

    grd.SetOutputLanguage('fr')
    grd.SetDefines({'hello': '1'})
    active = set(grd.ActiveDescendants())
    self.failUnless(bingo_message not in active)
    self.failUnless(hello_message in active)
    self.failUnless(french_message in active)

    grd.SetOutputLanguage('en')
    grd.SetDefines({'bingo': 1})
    active = set(grd.ActiveDescendants())
    self.failUnless(bingo_message in active)
    self.failUnless(hello_message not in active)
    self.failUnless(french_message not in active)

    grd.SetOutputLanguage('en')
    grd.SetDefines({'FORCE_FRENCH': '1', 'bingo': '1'})
    active = set(grd.ActiveDescendants())
    self.failUnless(bingo_message in active)
    self.failUnless(hello_message not in active)
    self.failUnless(french_message in active)

    grd.SetOutputLanguage('en')
    grd.SetDefines({})
    self.failUnless(grd.target_platform == sys.platform)
    grd.SetTargetPlatform('darwin')
    active = set(grd.ActiveDescendants())
    self.failUnless(is_win_message not in active)
    grd.SetTargetPlatform('win32')
    active = set(grd.ActiveDescendants())
    self.failUnless(is_win_message in active)

  def testElsiness(self):
    grd = util.ParseGrdForUnittest('''
        <messages>
          <if expr="True">
            <then> <message name="IDS_YES1"></message> </then>
            <else> <message name="IDS_NO1"></message> </else>
          </if>
          <if expr="True">
            <then> <message name="IDS_YES2"></message> </then>
            <else> </else>
          </if>
          <if expr="True">
            <then> </then>
            <else> <message name="IDS_NO2"></message> </else>
          </if>
          <if expr="True">
            <then> </then>
            <else> </else>
          </if>
          <if expr="False">
            <then> <message name="IDS_NO3"></message> </then>
            <else> <message name="IDS_YES3"></message> </else>
          </if>
          <if expr="False">
            <then> <message name="IDS_NO4"></message> </then>
            <else> </else>
          </if>
          <if expr="False">
            <then> </then>
            <else> <message name="IDS_YES4"></message> </else>
          </if>
          <if expr="False">
            <then> </then>
            <else> </else>
          </if>
        </messages>''')
    included = [msg.attrs['name'] for msg in grd.ActiveDescendants()
                                  if msg.name == 'message']
    self.assertEqual(['IDS_YES1', 'IDS_YES2', 'IDS_YES3', 'IDS_YES4'], included)

  def testIffynessWithOutputNodes(self):
    grd = grd_reader.Parse(StringIO.StringIO('''
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <outputs>
          <output filename="uncond1.rc" type="rc_data" />
          <if expr="lang == 'fr' or 'hello' in defs">
            <output filename="only_fr.adm" type="adm" />
            <output filename="only_fr.plist" type="plist" />
          </if>
          <if expr="lang == 'ru'">
            <output filename="doc.html" type="document" />
          </if>
          <output filename="uncond2.adm" type="adm" />
          <output filename="iftest.h" type="rc_header">
            <emit emit_type='prepend'></emit>
          </output>
        </outputs>
      </grit>'''), dir='.')

    outputs_node = grd.children[0]
    uncond1_output = outputs_node.children[0]
    only_fr_adm_output = outputs_node.children[1].children[0]
    only_fr_plist_output = outputs_node.children[1].children[1]
    doc_output = outputs_node.children[2].children[0]
    uncond2_output = outputs_node.children[0]
    self.assertTrue(uncond1_output.name == 'output')
    self.assertTrue(only_fr_adm_output.name == 'output')
    self.assertTrue(only_fr_plist_output.name == 'output')
    self.assertTrue(doc_output.name == 'output')
    self.assertTrue(uncond2_output.name == 'output')

    grd.SetOutputLanguage('ru')
    grd.SetDefines({'hello': '1'})
    outputs = [output.GetFilename() for output in grd.GetOutputFiles()]
    self.assertEquals(
        outputs,
        ['uncond1.rc', 'only_fr.adm', 'only_fr.plist', 'doc.html',
         'uncond2.adm', 'iftest.h'])

    grd.SetOutputLanguage('ru')
    grd.SetDefines({'bingo': '2'})
    outputs = [output.GetFilename() for output in grd.GetOutputFiles()]
    self.assertEquals(
        outputs,
        ['uncond1.rc', 'doc.html', 'uncond2.adm', 'iftest.h'])

    grd.SetOutputLanguage('fr')
    grd.SetDefines({'hello': '1'})
    outputs = [output.GetFilename() for output in grd.GetOutputFiles()]
    self.assertEquals(
        outputs,
        ['uncond1.rc', 'only_fr.adm', 'only_fr.plist', 'uncond2.adm',
         'iftest.h'])

    grd.SetOutputLanguage('en')
    grd.SetDefines({'bingo': '1'})
    outputs = [output.GetFilename() for output in grd.GetOutputFiles()]
    self.assertEquals(outputs, ['uncond1.rc', 'uncond2.adm', 'iftest.h'])

    grd.SetOutputLanguage('fr')
    grd.SetDefines({'bingo': '1'})
    outputs = [output.GetFilename() for output in grd.GetOutputFiles()]
    self.assertNotEquals(outputs, ['uncond1.rc', 'uncond2.adm', 'iftest.h'])

  def testChildrenAccepted(self):
    grd = grd_reader.Parse(StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <includes>
            <if expr="'bingo' in defs">
              <include type="gif" name="ID_LOGO2" file="images/logo2.gif" />
            </if>
            <if expr="'bingo' in defs">
              <if expr="'hello' in defs">
                <include type="gif" name="ID_LOGO2" file="images/logo2.gif" />
              </if>
            </if>
          </includes>
          <structures>
            <if expr="'bingo' in defs">
              <structure type="dialog" name="IDD_ABOUTBOX" file="grit\\test\data\klonk.rc" encoding="utf-16" />
            </if>
            <if expr="'bingo' in defs">
              <if expr="'hello' in defs">
                <structure type="dialog" name="IDD_ABOUTBOX" file="grit\\test\data\klonk.rc" encoding="utf-16" />
              </if>
            </if>
          </structures>
          <messages>
            <if expr="'bingo' in defs">
              <message name="IDS_BINGO">Bingo!</message>
            </if>
            <if expr="'bingo' in defs">
              <if expr="'hello' in defs">
                <message name="IDS_BINGO">Bingo!</message>
              </if>
            </if>
          </messages>
        </release>
        <translations>
          <if expr="'bingo' in defs">
            <file lang="nl" path="nl_translations.xtb" />
          </if>
          <if expr="'bingo' in defs">
            <if expr="'hello' in defs">
              <file lang="nl" path="nl_translations.xtb" />
            </if>
          </if>
        </translations>
      </grit>'''), dir='.')

  def testIfBadChildrenNesting(self):
    # includes
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <includes>
            <if expr="'bingo' in defs">
              <structure type="dialog" name="IDD_ABOUTBOX" file="grit\\test\data\klonk.rc" encoding="utf-16" />
            </if>
          </includes>
        </release>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    # messages
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <messages>
            <if expr="'bingo' in defs">
              <structure type="dialog" name="IDD_ABOUTBOX" file="grit\\test\data\klonk.rc" encoding="utf-16" />
            </if>
          </messages>
        </release>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    # structures
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <structures>
            <if expr="'bingo' in defs">
              <message name="IDS_BINGO">Bingo!</message>
            </if>
          </structures>
        </release>
      </grit>''')
    # translations
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <translations>
          <if expr="'bingo' in defs">
            <message name="IDS_BINGO">Bingo!</message>
          </if>
        </translations>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    # same with nesting
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <includes>
            <if expr="'bingo' in defs">
              <if expr="'hello' in defs">
                <structure type="dialog" name="IDD_ABOUTBOX" file="grit\\test\data\klonk.rc" encoding="utf-16" />
              </if>
            </if>
          </includes>
        </release>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <messages>
            <if expr="'bingo' in defs">
              <if expr="'hello' in defs">
                <structure type="dialog" name="IDD_ABOUTBOX" file="grit\\test\data\klonk.rc" encoding="utf-16" />
              </if>
            </if>
          </messages>
        </release>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <structures>
            <if expr="'bingo' in defs">
              <if expr="'hello' in defs">
                <message name="IDS_BINGO">Bingo!</message>
              </if>
            </if>
          </structures>
        </release>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)
    xml = StringIO.StringIO('''<?xml version="1.0"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <translations>
          <if expr="'bingo' in defs">
            <if expr="'hello' in defs">
              <message name="IDS_BINGO">Bingo!</message>
            </if>
          </if>
        </translations>
      </grit>''')
    self.assertRaises(grit.exception.UnexpectedChild, grd_reader.Parse, xml)


class ReleaseNodeUnittest(unittest.TestCase):
  def testPseudoControl(self):
    grd = grd_reader.Parse(StringIO.StringIO('''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="1" source_lang_id="en-US" current_release="2" base_dir=".">
        <release seq="1" allow_pseudo="false">
          <messages>
            <message name="IDS_HELLO">
              Hello
            </message>
          </messages>
          <structures>
            <structure type="dialog" name="IDD_ABOUTBOX" encoding="utf-16" file="klonk.rc" />
          </structures>
        </release>
        <release seq="2">
          <messages>
            <message name="IDS_BINGO">
              Bingo
            </message>
          </messages>
          <structures>
            <structure type="menu" name="IDC_KLONKMENU" encoding="utf-16" file="klonk.rc" />
          </structures>
        </release>
      </grit>'''), util.PathFromRoot('grit/testdata'))
    grd.SetOutputLanguage('en')
    grd.RunGatherers()

    hello = grd.GetNodeById('IDS_HELLO')
    aboutbox = grd.GetNodeById('IDD_ABOUTBOX')
    bingo = grd.GetNodeById('IDS_BINGO')
    menu = grd.GetNodeById('IDC_KLONKMENU')

    for node in [hello, aboutbox]:
      self.failUnless(not node.PseudoIsAllowed())

    for node in [bingo, menu]:
      self.failUnless(node.PseudoIsAllowed())

    # TODO(benrg): There was a test here that formatting hello and aboutbox with
    # a pseudo language should fail, but they do not fail and the test was
    # broken and failed to catch it. Fix this.

    # Should not raise an exception since pseudo is allowed
    rc.FormatMessage(bingo, 'xyz-pseudo')
    rc.FormatStructure(menu, 'xyz-pseudo', '.')


if __name__ == '__main__':
  unittest.main()
