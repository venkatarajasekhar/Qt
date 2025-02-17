#!/usr/bin/env python
# Copyright 2012 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest
import httpclient


class RealHttpFetchTest(unittest.TestCase):

  # Initialize test data
  CONTENT_TYPE = 'content-type: image/x-icon'
  COOKIE_1 = ('Set-Cookie: GMAIL_IMP=EXPIRED; '
              'Expires=Thu, 12-Jul-2012 22:41:22 GMT; '
              'Path=/mail; Secure')
  COOKIE_2 = ('Set-Cookie: GMAIL_STAT_205a=EXPIRED; '
              'Expires=Thu, 12-Jul-2012 22:42:24 GMT; '
              'Path=/mail; Secure')
  FIRST_LINE = 'fake-header: first line'
  SECOND_LINE = ' second line'
  THIRD_LINE = '\tthird line'
  BAD_HEADER = 'this is a bad header'

  def test__GetHeaderNameValueBasic(self):
    """Test _GetHeaderNameValue with normal header."""

    real_http_fetch = httpclient.RealHttpFetch
    name_value = real_http_fetch._GetHeaderNameValue(self.CONTENT_TYPE)
    self.assertEqual(name_value, ('content-type', 'image/x-icon'))

  def test__GetHeaderNameValueLowercasesName(self):
    """_GetHeaderNameValue lowercases header name."""

    real_http_fetch = httpclient.RealHttpFetch
    header = 'X-Google-Gfe-Backend-Request-Info: eid=1KMAUMeiK4eMiAL52YyMBg'
    expected = ('x-google-gfe-backend-request-info',
                'eid=1KMAUMeiK4eMiAL52YyMBg')
    name_value = real_http_fetch._GetHeaderNameValue(header)
    self.assertEqual(name_value, expected)

  def test__GetHeaderNameValueBadLineGivesNone(self):
    """_GetHeaderNameValue returns None for a header in wrong format."""

    real_http_fetch = httpclient.RealHttpFetch
    name_value = real_http_fetch._GetHeaderNameValue(self.BAD_HEADER)
    self.assertIsNone(name_value)

  def test__ToTuplesBasic(self):
    """Test _ToTuples with normal input."""

    real_http_fetch = httpclient.RealHttpFetch
    headers = [self.CONTENT_TYPE, self.COOKIE_1, self.FIRST_LINE]
    result = real_http_fetch._ToTuples(headers)
    expected = [('content-type', 'image/x-icon'),
                ('set-cookie', self.COOKIE_1[12:]),
                ('fake-header', 'first line')]
    self.assertEqual(result, expected)

  def test__ToTuplesMultipleHeadersWithSameName(self):
    """Test mulitple headers with the same name."""

    real_http_fetch = httpclient.RealHttpFetch
    headers = [self.CONTENT_TYPE, self.COOKIE_1, self.COOKIE_2, self.FIRST_LINE]
    result = real_http_fetch._ToTuples(headers)
    expected = [('content-type', 'image/x-icon'),
                ('set-cookie', self.COOKIE_1[12:]),
                ('set-cookie', self.COOKIE_2[12:]),
                ('fake-header', 'first line')]
    self.assertEqual(result, expected)

  def test__ToTuplesAppendsContinuationLine(self):
    """Test continuation line is handled."""

    real_http_fetch = httpclient.RealHttpFetch
    headers = [self.CONTENT_TYPE, self.COOKIE_1, self.FIRST_LINE,
               self.SECOND_LINE, self.THIRD_LINE]
    result = real_http_fetch._ToTuples(headers)
    expected = [('content-type', 'image/x-icon'),
                ('set-cookie', self.COOKIE_1[12:]),
                ('fake-header', 'first line\n second line\n third line')]
    self.assertEqual(result, expected)

  def test__ToTuplesIgnoresBadHeader(self):
    """Test bad header is ignored."""

    real_http_fetch = httpclient.RealHttpFetch
    bad_headers = [self.CONTENT_TYPE, self.BAD_HEADER, self.COOKIE_1]
    expected = [('content-type', 'image/x-icon'),
                ('set-cookie', self.COOKIE_1[12:])]
    result = real_http_fetch._ToTuples(bad_headers)
    self.assertEqual(result, expected)

  def test__ToTuplesIgnoresMisplacedContinuationLine(self):
    """Test misplaced continuation line is ignored."""

    real_http_fetch = httpclient.RealHttpFetch
    misplaced_headers = [self.THIRD_LINE, self.CONTENT_TYPE,
                         self.COOKIE_1, self.FIRST_LINE, self.SECOND_LINE]
    result = real_http_fetch._ToTuples(misplaced_headers)
    expected = [('content-type', 'image/x-icon'),
                ('set-cookie', self.COOKIE_1[12:]),
                ('fake-header', 'first line\n second line')]
    self.assertEqual(result, expected)


if __name__ == '__main__':
  unittest.main()
