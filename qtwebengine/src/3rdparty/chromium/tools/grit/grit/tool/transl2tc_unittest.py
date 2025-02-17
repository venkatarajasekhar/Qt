#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for the 'grit transl2tc' tool.'''


import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import StringIO
import unittest

from grit.tool import transl2tc
from grit import grd_reader
from grit import util


def MakeOptions():
  from grit import grit_runner
  return grit_runner.Options()


class TranslationToTcUnittest(unittest.TestCase):

  def testOutput(self):
    buf = StringIO.StringIO()
    tool = transl2tc.TranslationToTc()
    translations = [
      ['1', 'Hello USERNAME, how are you?'],
      ['12', 'Howdie doodie!'],
      ['123', 'Hello\n\nthere\n\nhow are you?'],
      ['1234', 'Hello is > goodbye but < howdie pardner'],
    ]
    tool.WriteTranslations(buf, translations)
    output = buf.getvalue()
    self.failUnless(output.strip() == '''
1 Hello USERNAME, how are you?
12 Howdie doodie!
123 Hello

there

how are you?
1234 Hello is &gt; goodbye but &lt; howdie pardner
'''.strip())

  def testExtractTranslations(self):
    path = util.PathFromRoot('grit/testdata')
    current_grd = grd_reader.Parse(StringIO.StringIO('''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <messages>
            <message name="IDS_SIMPLE">
              One
            </message>
            <message name="IDS_PLACEHOLDER">
              <ph name="NUMBIRDS">%s<ex>3</ex></ph> birds
            </message>
            <message name="IDS_PLACEHOLDERS">
              <ph name="ITEM">%d<ex>1</ex></ph> of <ph name="COUNT">%d<ex>3</ex></ph>
            </message>
            <message name="IDS_REORDERED_PLACEHOLDERS">
              <ph name="ITEM">$1<ex>1</ex></ph> of <ph name="COUNT">$2<ex>3</ex></ph>
            </message>
            <message name="IDS_CHANGED">
              This is the new version
            </message>
            <message name="IDS_TWIN_1">Hello</message>
            <message name="IDS_TWIN_2">Hello</message>
            <message name="IDS_NOT_TRANSLATEABLE" translateable="false">:</message>
            <message name="IDS_LONGER_TRANSLATED">
              Removed document <ph name="FILENAME">$1<ex>c:\temp</ex></ph>
            </message>
            <message name="IDS_DIFFERENT_TWIN_1">Howdie</message>
            <message name="IDS_DIFFERENT_TWIN_2">Howdie</message>
          </messages>
          <structures>
            <structure type="dialog" name="IDD_ABOUTBOX" encoding="utf-16" file="klonk.rc" />
            <structure type="menu" name="IDC_KLONKMENU" encoding="utf-16" file="klonk.rc" />
          </structures>
        </release>
      </grit>'''), path)
    current_grd.SetOutputLanguage('en')
    current_grd.RunGatherers()

    source_rc_path = util.PathFromRoot('grit/testdata/source.rc')
    source_rc = util.ReadFile(source_rc_path, util.RAW_TEXT)
    transl_rc_path = util.PathFromRoot('grit/testdata/transl.rc')
    transl_rc = util.ReadFile(transl_rc_path, util.RAW_TEXT)

    tool = transl2tc.TranslationToTc()
    output_buf = StringIO.StringIO()
    globopts = MakeOptions()
    globopts.verbose = True
    globopts.output_stream = output_buf
    tool.Setup(globopts, [])
    translations = tool.ExtractTranslations(current_grd,
                                            source_rc, source_rc_path,
                                            transl_rc, transl_rc_path)

    values = translations.values()
    output = output_buf.getvalue()

    self.failUnless('Ein' in values)
    self.failUnless('NUMBIRDS Vogeln' in values)
    self.failUnless('ITEM von COUNT' in values)
    self.failUnless(values.count('Hallo') == 1)
    self.failIf('Dass war die alte Version' in values)
    self.failIf(':' in values)
    self.failIf('Dokument FILENAME ist entfernt worden' in values)
    self.failIf('Nicht verwendet' in values)
    self.failUnless(('Howdie' in values or 'Hallo sagt man' in values) and not
      ('Howdie' in values and 'Hallo sagt man' in values))

    self.failUnless('XX01XX&SkraXX02XX&HaettaXX03XXThetta er "Klonk" sem eg fylaXX04XXgonkurinnXX05XXKlonk && er [good]XX06XX&HjalpXX07XX&Um...XX08XX' in values)

    self.failUnless('I lagi' in values)

    self.failUnless(output.count('Structure of message IDS_REORDERED_PLACEHOLDERS has changed'))
    self.failUnless(output.count('Message IDS_CHANGED has changed'))
    self.failUnless(output.count('Structure of message IDS_LONGER_TRANSLATED has changed'))
    self.failUnless(output.count('Two different translations for "Howdie"'))
    self.failUnless(output.count('IDD_DIFFERENT_LENGTH_IN_TRANSL has wrong # of cliques'))


if __name__ == '__main__':
  unittest.main()
