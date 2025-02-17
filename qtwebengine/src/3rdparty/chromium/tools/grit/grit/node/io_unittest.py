#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for io.FileNode'''

import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import os
import StringIO
import unittest

from grit.node import misc
from grit.node import io
from grit.node import empty
from grit import grd_reader
from grit import util


class FileNodeUnittest(unittest.TestCase):
  def testGetPath(self):
    root = misc.GritNode()
    root.StartParsing(u'grit', None)
    root.HandleAttribute(u'latest_public_release', u'0')
    root.HandleAttribute(u'current_release', u'1')
    root.HandleAttribute(u'base_dir', ur'..\resource')
    translations = empty.TranslationsNode()
    translations.StartParsing(u'translations', root)
    root.AddChild(translations)
    file_node = io.FileNode()
    file_node.StartParsing(u'file', translations)
    file_node.HandleAttribute(u'path', ur'flugel\kugel.pdf')
    translations.AddChild(file_node)
    root.EndParsing()

    self.failUnless(root.ToRealPath(file_node.GetInputPath()) ==
                    util.normpath(
                      os.path.join(ur'../resource', ur'flugel/kugel.pdf')))

  def VerifyCliquesContainEnglishAndFrenchAndNothingElse(self, cliques):
    for clique in cliques:
      self.failUnlessEquals(len(clique[0].clique), 2)
      self.failUnless('en' in cliques[i][0].clique)
      self.failUnless('fr' in cliques[i][0].clique)

  def testLoadTranslations(self):
    xml = '''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <translations>
          <file path="generated_resources_fr.xtb" lang="fr" />
        </translations>
        <release seq="3">
          <messages>
            <message name="ID_HELLO">Hello!</message>
            <message name="ID_HELLO_USER">Hello <ph name="USERNAME">%s<ex>Joi</ex></ph></message>
          </messages>
        </release>
      </grit>'''
    grd = grd_reader.Parse(StringIO.StringIO(xml),
                           util.PathFromRoot('grit/testdata'))
    grd.SetOutputLanguage('en')
    grd.RunGatherers()
    self.VerifyCliquesContainEnglishAndFrenchAndNothingElse(grd.GetCliques())

  def testIffyness(self):
    grd = grd_reader.Parse(StringIO.StringIO('''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <translations>
          <if expr="lang == 'fr'">
            <file path="generated_resources_fr.xtb" lang="fr" />
          </if>
        </translations>
        <release seq="3">
          <messages>
            <message name="ID_HELLO">Hello!</message>
            <message name="ID_HELLO_USER">Hello <ph name="USERNAME">%s<ex>Joi</ex></ph></message>
          </messages>
        </release>
      </grit>'''), util.PathFromRoot('grit/testdata'))
    grd.SetOutputLanguage('en')
    grd.RunGatherers()

    grd.SetOutputLanguage('fr')
    grd.RunGatherers()

  def testConditionalLoadTranslations(self):
    xml = '''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3"
            base_dir=".">
        <translations>
          <if expr="True">
            <file path="generated_resources_fr.xtb" lang="fr" />
          </if>
          <if expr="False">
            <file path="no_such_file.xtb" lang="de" />
          </if>
        </translations>
        <release seq="3">
          <messages>
            <message name="ID_HELLO">Hello!</message>
            <message name="ID_HELLO_USER">Hello <ph name="USERNAME">%s<ex>
              Joi</ex></ph></message>
          </messages>
        </release>
      </grit>'''
    grd = grd_reader.Parse(StringIO.StringIO(xml),
                           util.PathFromRoot('grit/testdata'))
    grd.SetOutputLanguage('en')
    grd.RunGatherers()
    self.VerifyCliquesContainEnglishAndFrenchAndNothingElse(grd.GetCliques())

  def testConditionalOutput(self):
    xml = '''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3"
            base_dir=".">
        <outputs>
          <output filename="resource.h" type="rc_header" />
          <output filename="en/generated_resources.rc" type="rc_all"
                  lang="en" />
          <if expr="pp_if('NOT_TRUE')">
            <output filename="de/generated_resources.rc" type="rc_all"
                    lang="de" />
          </if>
        </outputs>
        <release seq="3">
          <messages>
            <message name="ID_HELLO">Hello!</message>
          </messages>
        </release>
      </grit>'''
    grd = grd_reader.Parse(StringIO.StringIO(xml),
                           util.PathFromRoot('grit/test/data'),
                           defines={})
    grd.SetOutputLanguage('en')
    grd.RunGatherers()
    outputs = grd.GetChildrenOfType(io.OutputNode)
    active = set(grd.ActiveDescendants())
    self.failUnless(outputs[0] in active)
    self.failUnless(outputs[0].GetType() == 'rc_header')
    self.failUnless(outputs[1] in active)
    self.failUnless(outputs[1].GetType() == 'rc_all')
    self.failUnless(outputs[2] not in active)
    self.failUnless(outputs[2].GetType() == 'rc_all')

  # Verify that 'iw' and 'no' language codes in xtb files are mapped to 'he' and
  # 'nb'.
  def testLangCodeMapping(self):
    grd = grd_reader.Parse(StringIO.StringIO('''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <translations>
          <file path="generated_resources_no.xtb" lang="nb" />
          <file path="generated_resources_iw.xtb" lang="he" />
        </translations>
        <release seq="3">
          <messages></messages>
        </release>
      </grit>'''), util.PathFromRoot('grit/testdata'))
    grd.SetOutputLanguage('en')
    grd.RunGatherers()


if __name__ == '__main__':
  unittest.main()
