#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for ChromeScaledImage.'''


import re
import struct
import unittest
import zlib

from grit import exception
from grit import util
from grit.format import data_pack
from grit.tool import build


_OUTFILETYPES = [
  ('.h', 'rc_header'),
  ('_map.cc', 'resource_map_source'),
  ('_map.h', 'resource_map_header'),
  ('.pak', 'data_package'),
  ('.rc', 'rc_all'),
]


_PNG_HEADER = (
    '\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52'
    '\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90\x77\x53'
    '\xde')
_PNG_FOOTER = (
    '\x00\x00\x00\x0c\x49\x44\x41\x54\x18\x57\x63\xf8\xff\xff\x3f\x00'
    '\x05\xfe\x02\xfe\xa7\x35\x81\x84\x00\x00\x00\x00\x49\x45\x4e\x44'
    '\xae\x42\x60\x82')


def _MakePNG(chunks):
  pack_int32 = struct.Struct('>i').pack
  chunks = [pack_int32(len(payload)) + type + payload + pack_int32(zlib.crc32(type + payload))
            for type, payload in chunks]
  return _PNG_HEADER + ''.join(chunks) + _PNG_FOOTER


def _GetFilesInPak(pakname):
  '''Get a set of the files that were actually included in the .pak output.
  '''
  return set(data_pack.DataPack.ReadDataPack(pakname).resources.values())


def _GetFilesInRc(rcname, tmp_dir, contents):
  '''Get a set of the files that were actually included in the .rc output.
  '''
  data = util.ReadFile(rcname, util.BINARY).decode('utf-16')
  contents = dict((tmp_dir.GetPath(k), v) for k, v in contents.items())
  return set(contents[m.group(1)]
             for m in re.finditer(ur'(?m)^\w+\s+BINDATA\s+"([^"]+)"$', data))


def _MakeFallbackAttr(fallback):
  if fallback is None:
    return ''
  else:
    return ' fallback_to_low_resolution="%s"' % ('false', 'true')[fallback]


def _Structures(fallback, *body):
  return '<structures%s>\n%s\n</structures>' % (
      _MakeFallbackAttr(fallback), '\n'.join(body))


def _Structure(name, file, fallback=None):
  return '<structure name="%s" file="%s" type="chrome_scaled_image"%s />' % (
      name, file, _MakeFallbackAttr(fallback))


def _If(expr, *body):
  return '<if expr="%s">\n%s\n</if>' % (expr, '\n'.join(body))


def _RunBuildTest(self, structures, inputs, expected_outputs, skip_rc=False):
  outputs = '\n'.join('<output filename="out/%s%s" type="%s" context="%s" />'
                              % (context, ext, type, context)
                      for ext, type in _OUTFILETYPES
                      for context in expected_outputs)
  infiles = {
    'in/in.grd': '''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="0" current_release="1">
        <outputs>
          %s
        </outputs>
        <release seq="1">
          %s
        </release>
      </grit>
      ''' % (outputs, structures),
  }
  for pngpath, pngdata in inputs.items():
    infiles['in/' + pngpath] = pngdata
  class Options(object):
    pass
  with util.TempDir(infiles) as tmp_dir:
    with tmp_dir.AsCurrentDir():
      options = Options()
      options.input = tmp_dir.GetPath('in/in.grd')
      options.verbose = False
      options.extra_verbose = False
      build.RcBuilder().Run(options, [])
    for context, expected_data in expected_outputs.items():
      self.assertEquals(expected_data,
                        _GetFilesInPak(tmp_dir.GetPath('out/%s.pak' % context)))
      if not skip_rc:
        self.assertEquals(expected_data,
                          _GetFilesInRc(tmp_dir.GetPath('out/%s.rc' % context),
                                        tmp_dir, infiles))


class ChromeScaledImageUnittest(unittest.TestCase):
  def testNormalFallback(self):
    d123a = _MakePNG([('AbCd', '')])
    t123a = _MakePNG([('EfGh', '')])
    d123b = _MakePNG([('IjKl', '')])
    _RunBuildTest(self,
        _Structures(None,
            _Structure('IDR_A', 'a.png'),
            _Structure('IDR_B', 'b.png'),
        ),
        {'default_123_percent/a.png': d123a,
         'tactile_123_percent/a.png': t123a,
         'default_123_percent/b.png': d123b,
        },
        {'default_123_percent': set([d123a, d123b]),
         'tactile_123_percent': set([t123a, d123b]),
        })

  def testNormalFallbackFailure(self):
    self.assertRaises(exception.FileNotFound,
        _RunBuildTest, self,
            _Structures(None,
                _Structure('IDR_A', 'a.png'),
            ),
            {'default_100_percent/a.png': _MakePNG([('AbCd', '')]),
             'tactile_100_percent/a.png': _MakePNG([('EfGh', '')]),
            },
            {'tactile_123_percent': 'should fail before using this'})

  def testLowresFallback(self):
    png = _MakePNG([('Abcd', '')])
    png_with_csCl = _MakePNG([('csCl', ''),('Abcd', '')])
    for outer in (None, False, True):
      for inner in (None, False, True):
        args = (
            self,
            _Structures(outer,
                _Structure('IDR_A', 'a.png', inner),
            ),
            {'default_100_percent/a.png': png},
            {'tactile_200_percent': set([png_with_csCl])})
        if inner or (inner is None and outer):
          # should fall back to 100%
          _RunBuildTest(*args, skip_rc=True)
        else:
          # shouldn't fall back
          self.assertRaises(exception.FileNotFound, _RunBuildTest, *args)

    # Test fallback failure with fallback_to_low_resolution=True
    self.assertRaises(exception.FileNotFound,
        _RunBuildTest, self,
            _Structures(True,
                _Structure('IDR_A', 'a.png'),
            ),
            {},  # no files
            {'tactile_123_percent': 'should fail before using this'})
