# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilies and constants specific to Chromium C++ code.
"""

from code import Code
from datetime import datetime
from model import PropertyType
import os
import re

CHROMIUM_LICENSE = (
"""// Copyright (c) %d The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.""" % datetime.now().year
)
GENERATED_FILE_MESSAGE = """// GENERATED FROM THE API DEFINITION IN
//   %s
// DO NOT EDIT.
"""
GENERATED_BUNDLE_FILE_MESSAGE = """// GENERATED FROM THE API DEFINITIONS IN
//   %s
// DO NOT EDIT.
"""
GENERATED_FEATURE_MESSAGE = """// GENERATED FROM THE FEATURE DEFINITIONS IN
//   %s
// DO NOT EDIT.
"""

def Classname(s):
  """Translates a namespace name or function name into something more
  suited to C++.

  eg experimental.downloads -> Experimental_Downloads
  updateAll -> UpdateAll.
  """
  return '_'.join([x[0].upper() + x[1:] for x in re.split('\W', s)])


def GetAsFundamentalValue(type_, src, dst):
  """Returns the C++ code for retrieving a fundamental type from a
  Value into a variable.

  src: Value*
  dst: Property*
  """
  return {
      PropertyType.BOOLEAN: '%s->GetAsBoolean(%s)',
      PropertyType.DOUBLE: '%s->GetAsDouble(%s)',
      PropertyType.INTEGER: '%s->GetAsInteger(%s)',
      PropertyType.STRING: '%s->GetAsString(%s)',
  }[type_.property_type] % (src, dst)


def GetValueType(type_):
  """Returns the Value::Type corresponding to the model.Type.
  """
  return {
      PropertyType.ARRAY: 'base::Value::TYPE_LIST',
      PropertyType.BINARY: 'base::Value::TYPE_BINARY',
      PropertyType.BOOLEAN: 'base::Value::TYPE_BOOLEAN',
      # PropertyType.CHOICES can be any combination of types.
      PropertyType.DOUBLE: 'base::Value::TYPE_DOUBLE',
      PropertyType.ENUM: 'base::Value::TYPE_STRING',
      PropertyType.FUNCTION: 'base::Value::TYPE_DICTIONARY',
      PropertyType.INTEGER: 'base::Value::TYPE_INTEGER',
      PropertyType.OBJECT: 'base::Value::TYPE_DICTIONARY',
      PropertyType.STRING: 'base::Value::TYPE_STRING',
  }[type_.property_type]


def GetParameterDeclaration(param, type_):
  """Gets a parameter declaration of a given model.Property and its C++
  type.
  """
  if param.type_.property_type in (PropertyType.ANY,
                                   PropertyType.ARRAY,
                                   PropertyType.CHOICES,
                                   PropertyType.OBJECT,
                                   PropertyType.REF,
                                   PropertyType.STRING):
    arg = 'const %(type)s& %(name)s'
  else:
    arg = '%(type)s %(name)s'
  return arg % {
    'type': type_,
    'name': param.unix_name,
  }


def GenerateIfndefName(path, filename):
  """Formats a path and filename as a #define name.

  e.g chrome/extensions/gen, file.h becomes CHROME_EXTENSIONS_GEN_FILE_H__.
  """
  return (('%s_%s_H__' % (path, filename))
          .upper().replace(os.sep, '_').replace('/', '_'))


def PadForGenerics(var):
  """Appends a space to |var| if it ends with a >, so that it can be compiled
  within generic types.
  """
  return ('%s ' % var) if var.endswith('>') else var


def OpenNamespace(namespace):
  """Get opening root namespace declarations.
  """
  c = Code()
  # In lieu of GYP supporting None for the namespace variable the '' namespace
  # implies there is no root namespace.
  if namespace == '':
    return c
  for component in namespace.split('::'):
    c.Append('namespace %s {' % component)
  return c


def CloseNamespace(namespace):
  """Get closing root namespace declarations.
  """
  c = Code()
  # In lieu of GYP supporting None for the namespace variable the '' namespace
  # implies there is no root namespace.
  if namespace == '':
    return c
  for component in reversed(namespace.split('::')):
    c.Append('}  // namespace %s' % component)
  return c


def ConstantName(feature_name):
  """Returns a kName for a feature's name.
  """
  return ('k' + ''.join(word[0].upper() + word[1:]
      for word in feature_name.replace('.', ' ').split()))


def CamelCase(unix_name):
  return ''.join(word.capitalize() for word in unix_name.split('_'))


def ClassName(filepath):
  return CamelCase(os.path.split(filepath)[1])
