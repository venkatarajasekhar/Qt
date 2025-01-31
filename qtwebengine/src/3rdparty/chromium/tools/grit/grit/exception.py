#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Exception types for GRIT.
'''

class Base(Exception):
  '''A base exception that uses the class's docstring in addition to any
  user-provided message as the body of the Base.
  '''
  def __init__(self, msg=''):
    if len(msg):
      if self.__doc__:
        msg = self.__doc__ + ': ' + msg
    else:
      msg = self.__doc__
    super(Base, self).__init__(msg)


class Parsing(Base):
  '''An error occurred parsing a GRD or XTB file.'''
  pass


class UnknownElement(Parsing):
  '''An unknown node type was encountered.'''
  pass


class MissingElement(Parsing):
  '''An expected element was missing.'''
  pass


class UnexpectedChild(Parsing):
  '''An unexpected child element was encountered (on a leaf node).'''
  pass


class UnexpectedAttribute(Parsing):
  '''The attribute was not expected'''
  pass


class UnexpectedContent(Parsing):
  '''This element should not have content'''
  pass


class MissingMandatoryAttribute(Parsing):
  '''This element is missing a mandatory attribute'''
  pass


class MutuallyExclusiveMandatoryAttribute(Parsing):
  '''This element has 2 mutually exclusive mandatory attributes'''
  pass


class DuplicateKey(Parsing):
  '''A duplicate key attribute was found.'''
  pass


class TooManyExamples(Parsing):
  '''Only one <ex> element is allowed for each <ph> element.'''
  pass


class GotPathExpectedFilenameOnly(Parsing):
  '''The 'filename' attribute of <output> and the 'file' attribute of <part>
  must be bare filenames, not paths.
  '''
  pass


class FileNotFound(Parsing):
  '''The resource file was not found.
  '''
  pass


class InvalidMessage(Base):
  '''The specified message failed validation.'''
  pass


class InvalidTranslation(Base):
  '''Attempt to add an invalid translation to a clique.'''
  pass


class NoSuchTranslation(Base):
  '''Requested translation not available'''
  pass


class NotReady(Base):
  '''Attempt to use an object before it is ready, or attempt to translate
  an empty document.'''
  pass


class TooManyPlaceholders(Base):
  '''Too many placeholders for elements of the same type.'''
  pass


class MismatchingPlaceholders(Base):
  '''Placeholders do not match.'''
  pass


class InvalidPlaceholderName(Base):
  '''Placeholder name can only contain A-Z, a-z, 0-9 and underscore.'''
  pass


class BlockTagInTranslateableChunk(Base):
  '''A block tag was encountered where it wasn't expected.'''
  pass


class SectionNotFound(Base):
  '''The section you requested was not found in the RC file. Make
sure the section ID is correct (matches the section's ID in the RC file).
Also note that you may need to specify the RC file's encoding (using the
encoding="" attribute) if it is not in the default Windows-1252 encoding.
  '''
  pass


class IdRangeOverlap(Base):
  '''ID range overlap.'''
  pass

