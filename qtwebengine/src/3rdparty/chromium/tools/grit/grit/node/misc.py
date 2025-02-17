#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Miscellaneous node types.
"""

import os.path
import re
import sys

from grit import constants
from grit import exception
from grit import util
import grit.format.rc_header
from grit.node import base
from grit.node import io
from grit.node import message


# RTL languages
# TODO(jennyz): remove this fixed set of RTL language array
# now that generic expand_variable code exists.
_RTL_LANGS = (
    'ar',  # Arabic
    'fa',  # Farsi
    'iw',  # Hebrew
    'ks',  # Kashmiri
    'ku',  # Kurdish
    'ps',  # Pashto
    'ur',  # Urdu
    'yi',  # Yiddish
)


def _ReadFirstIdsFromFile(filename, defines):
  """Read the starting resource id values from |filename|.  We also
  expand variables of the form <(FOO) based on defines passed in on
  the command line.

  Returns a tuple, the absolute path of SRCDIR followed by the
  first_ids dictionary.
  """
  first_ids_dict = eval(util.ReadFile(filename, util.RAW_TEXT))
  src_root_dir = os.path.abspath(os.path.join(os.path.dirname(filename),
                                              first_ids_dict['SRCDIR']))

  def ReplaceVariable(matchobj):
    for key, value in defines.iteritems():
      if matchobj.group(1) == key:
        value = os.path.abspath(value)
        return value
    return ''

  renames = []
  for grd_filename in first_ids_dict:
    new_grd_filename = re.sub(r'<\(([A-Za-z_]+)\)', ReplaceVariable,
                              grd_filename)
    if new_grd_filename != grd_filename:
      abs_grd_filename = os.path.abspath(new_grd_filename)
      if abs_grd_filename[:len(src_root_dir)] != src_root_dir:
        new_grd_filename = os.path.basename(abs_grd_filename)
      else:
        new_grd_filename = abs_grd_filename[len(src_root_dir) + 1:]
        new_grd_filename = new_grd_filename.replace('\\', '/')
      renames.append((grd_filename, new_grd_filename))

  for grd_filename, new_grd_filename in renames:
    first_ids_dict[new_grd_filename] = first_ids_dict[grd_filename]
    del(first_ids_dict[grd_filename])

  return (src_root_dir, first_ids_dict)


class SplicingNode(base.Node):
  """A node whose children should be considered to be at the same level as
  its siblings for most purposes. This includes <if> and <part> nodes.
  """

  def _IsValidChild(self, child):
    assert self.parent, '<%s> node should never be root.' % self.name
    if isinstance(child, SplicingNode):
      return True  # avoid O(n^2) behavior
    return self.parent._IsValidChild(child)


class IfNode(SplicingNode):
  """A node for conditional inclusion of resources.
  """

  def MandatoryAttributes(self):
    return ['expr']

  def _IsValidChild(self, child):
    return (isinstance(child, (ThenNode, ElseNode)) or
            super(IfNode, self)._IsValidChild(child))

  def EndParsing(self):
    children = self.children
    self.if_then_else = False
    if any(isinstance(node, (ThenNode, ElseNode)) for node in children):
      if (len(children) != 2 or not isinstance(children[0], ThenNode) or
                                not isinstance(children[1], ElseNode)):
        raise exception.UnexpectedChild(
            '<if> element must be <if><then>...</then><else>...</else></if>')
      self.if_then_else = True

  def ActiveChildren(self):
    cond = self.EvaluateCondition(self.attrs['expr'])
    if self.if_then_else:
      return self.children[0 if cond else 1].ActiveChildren()
    else:
      # Equivalent to having all children inside <then> with an empty <else>
      return super(IfNode, self).ActiveChildren() if cond else []


class ThenNode(SplicingNode):
  """A <then> node. Can only appear directly inside an <if> node."""
  pass


class ElseNode(SplicingNode):
  """An <else> node. Can only appear directly inside an <if> node."""
  pass


class PartNode(SplicingNode):
  """A node for inclusion of sub-grd (*.grp) files.
  """

  def __init__(self):
    super(PartNode, self).__init__()
    self.started_inclusion = False

  def MandatoryAttributes(self):
    return ['file']

  def _IsValidChild(self, child):
    return self.started_inclusion and super(PartNode, self)._IsValidChild(child)


class ReleaseNode(base.Node):
  """The <release> element."""

  def _IsValidChild(self, child):
    from grit.node import empty
    return isinstance(child, (empty.IncludesNode, empty.MessagesNode,
                              empty.StructuresNode, empty.IdentifiersNode))

  def _IsValidAttribute(self, name, value):
    return (
      (name == 'seq' and int(value) <= self.GetRoot().GetCurrentRelease()) or
      name == 'allow_pseudo'
    )

  def MandatoryAttributes(self):
    return ['seq']

  def DefaultAttributes(self):
    return { 'allow_pseudo' : 'true' }

  def GetReleaseNumber():
    """Returns the sequence number of this release."""
    return self.attribs['seq']

class GritNode(base.Node):
  """The <grit> root element."""

  def __init__(self):
    super(GritNode, self).__init__()
    self.output_language = ''
    self.defines = {}
    self.substituter = None
    self.target_platform = sys.platform

  def _IsValidChild(self, child):
    from grit.node import empty
    return isinstance(child, (ReleaseNode, empty.TranslationsNode,
                              empty.OutputsNode))

  def _IsValidAttribute(self, name, value):
    if name not in ['base_dir', 'first_ids_file', 'source_lang_id',
                    'latest_public_release', 'current_release',
                    'enc_check', 'tc_project', 'grit_version',
                    'output_all_resource_defines', 'rc_header_format']:
      return False
    if name in ['latest_public_release', 'current_release'] and value.strip(
      '0123456789') != '':
      return False
    return True

  def MandatoryAttributes(self):
    return ['latest_public_release', 'current_release']

  def DefaultAttributes(self):
    return {
      'base_dir' : '.',
      'first_ids_file': '',
      'grit_version': 1,
      'source_lang_id' : 'en',
      'enc_check' : constants.ENCODING_CHECK,
      'tc_project' : 'NEED_TO_SET_tc_project_ATTRIBUTE',
      'output_all_resource_defines': 'true',
      'rc_header_format': None
    }

  def EndParsing(self):
    super(GritNode, self).EndParsing()
    if (int(self.attrs['latest_public_release'])
        > int(self.attrs['current_release'])):
      raise exception.Parsing('latest_public_release cannot have a greater '
                              'value than current_release')

    self.ValidateUniqueIds()

    # Add the encoding check if it's not present (should ensure that it's always
    # present in all .grd files generated by GRIT). If it's present, assert if
    # it's not correct.
    if 'enc_check' not in self.attrs or self.attrs['enc_check'] == '':
      self.attrs['enc_check'] = constants.ENCODING_CHECK
    else:
      assert self.attrs['enc_check'] == constants.ENCODING_CHECK, (
        'Are you sure your .grd file is in the correct encoding (UTF-8)?')

  def ValidateUniqueIds(self):
    """Validate that 'name' attribute is unique in all nodes in this tree
    except for nodes that are children of <if> nodes.
    """
    unique_names = {}
    duplicate_names = []
    # To avoid false positives from mutually exclusive <if> clauses, check
    # against whatever the output condition happens to be right now.
    # TODO(benrg): do something better.
    for node in self.ActiveDescendants():
      if node.attrs.get('generateid', 'true') == 'false':
        continue  # Duplication not relevant in that case

      for node_id in node.GetTextualIds():
        if util.SYSTEM_IDENTIFIERS.match(node_id):
          continue  # predefined IDs are sometimes used more than once

        if node_id in unique_names and node_id not in duplicate_names:
          duplicate_names.append(node_id)
        unique_names[node_id] = 1

    if len(duplicate_names):
      raise exception.DuplicateKey(', '.join(duplicate_names))


  def GetCurrentRelease(self):
    """Returns the current release number."""
    return int(self.attrs['current_release'])

  def GetLatestPublicRelease(self):
    """Returns the latest public release number."""
    return int(self.attrs['latest_public_release'])

  def GetSourceLanguage(self):
    """Returns the language code of the source language."""
    return self.attrs['source_lang_id']

  def GetTcProject(self):
    """Returns the name of this project in the TranslationConsole, or
    'NEED_TO_SET_tc_project_ATTRIBUTE' if it is not defined."""
    return self.attrs['tc_project']

  def SetOwnDir(self, dir):
    """Informs the 'grit' element of the directory the file it is in resides.
    This allows it to calculate relative paths from the input file, which is
    what we desire (rather than from the current path).

    Args:
      dir: r'c:\bla'

    Return:
      None
    """
    assert dir
    self.base_dir = os.path.normpath(os.path.join(dir, self.attrs['base_dir']))

  def GetBaseDir(self):
    """Returns the base directory, relative to the working directory.  To get
    the base directory as set in the .grd file, use GetOriginalBaseDir()
    """
    if hasattr(self, 'base_dir'):
      return self.base_dir
    else:
      return self.GetOriginalBaseDir()

  def GetOriginalBaseDir(self):
    """Returns the base directory, as set in the .grd file.
    """
    return self.attrs['base_dir']

  def ShouldOutputAllResourceDefines(self):
    """Returns true if all resource defines should be output, false if
    defines for resources not emitted to resource files should be
    skipped.
    """
    return self.attrs['output_all_resource_defines'] == 'true'

  def GetRcHeaderFormat(self):
    return self.attrs['rc_header_format']

  def AssignRcHeaderFormat(self, rc_header_format):
    self.attrs['rc_header_format'] = rc_header_format

  def GetInputFiles(self):
    """Returns the list of files that are read to produce the output."""

    # Importing this here avoids a circular dependency in the imports.
    # pylint: disable-msg=C6204
    from grit.node import include
    from grit.node import misc
    from grit.node import structure
    from grit.node import variant

    # Check if the input is required for any output configuration.
    input_files = set()
    old_output_language = self.output_language
    for lang, ctx in self.GetConfigurations():
      self.SetOutputLanguage(lang or self.GetSourceLanguage())
      self.SetOutputContext(ctx)
      for node in self.ActiveDescendants():
        if isinstance(node, (io.FileNode, include.IncludeNode, misc.PartNode,
                             structure.StructureNode, variant.SkeletonNode)):
          input_files.add(node.GetInputPath())
    self.SetOutputLanguage(old_output_language)
    return sorted(map(self.ToRealPath, input_files))

  def GetFirstIdsFile(self):
    """Returns a usable path to the first_ids file, if set, otherwise
    returns None.

    The first_ids_file attribute is by default relative to the
    base_dir of the .grd file, but may be prefixed by GRIT_DIR/,
    which makes it relative to the directory of grit.py
    (e.g. GRIT_DIR/../gritsettings/resource_ids).
    """
    if not self.attrs['first_ids_file']:
      return None

    path = self.attrs['first_ids_file']
    GRIT_DIR_PREFIX = 'GRIT_DIR'
    if (path.startswith(GRIT_DIR_PREFIX)
        and path[len(GRIT_DIR_PREFIX)] in ['/', '\\']):
      return util.PathFromRoot(path[len(GRIT_DIR_PREFIX) + 1:])
    else:
      return self.ToRealPath(path)

  def GetOutputFiles(self):
    """Returns the list of <output> nodes that are descendants of this node's
    <outputs> child and are not enclosed by unsatisfied <if> conditionals.
    """
    for child in self.children:
      if child.name == 'outputs':
        return [node for node in child.ActiveDescendants()
                     if node.name == 'output']
    raise exception.MissingElement()

  def GetConfigurations(self):
    """Returns the distinct (language, context) pairs from the output nodes.
    """
    return set((n.GetLanguage(), n.GetContext()) for n in self.GetOutputFiles())

  def GetSubstitutionMessages(self):
    """Returns the list of <message sub_variable="true"> nodes."""
    return [n for n in self.ActiveDescendants()
            if isinstance(n, message.MessageNode)
                and n.attrs['sub_variable'] == 'true']

  def SetOutputLanguage(self, output_language):
    """Set the output language. Prepares substitutions.

    The substitutions are reset every time the language is changed.
    They include messages designated as variables, and language codes for html
    and rc files.

    Args:
      output_language: a two-letter language code (eg: 'en', 'ar'...) or ''
    """
    if not output_language:
      # We do not specify the output language for .grh files,
      # so we get an empty string as the default.
      # The value should match grit.clique.MessageClique.source_language.
      output_language = self.GetSourceLanguage()
    if output_language != self.output_language:
      self.output_language = output_language
      self.substituter = None  # force recalculate

  def SetOutputContext(self, output_context):
    self.output_context = output_context
    self.substituter = None  # force recalculate

  def SetDefines(self, defines):
    self.defines = defines
    self.substituter = None  # force recalculate

  def SetTargetPlatform(self, target_platform):
    self.target_platform = target_platform

  def GetSubstituter(self):
    if self.substituter is None:
      self.substituter = util.Substituter()
      self.substituter.AddMessages(self.GetSubstitutionMessages(),
                                   self.output_language)
      if self.output_language in _RTL_LANGS:
        direction = 'dir="RTL"'
      else:
        direction = 'dir="LTR"'
      self.substituter.AddSubstitutions({
          'GRITLANGCODE': self.output_language,
          'GRITDIR': direction,
      })
      from grit.format import rc  # avoid circular dep
      rc.RcSubstitutions(self.substituter, self.output_language)
    return self.substituter

  def AssignFirstIds(self, filename_or_stream, defines):
    """Assign first ids to each grouping node based on values from the
    first_ids file (if specified on the <grit> node).
    """
    # If the input is a stream, then we're probably in a unit test and
    # should skip this step.
    if type(filename_or_stream) not in (str, unicode):
      return

    # Nothing to do if the first_ids_filename attribute isn't set.
    first_ids_filename = self.GetFirstIdsFile()
    if not first_ids_filename:
      return

    src_root_dir, first_ids = _ReadFirstIdsFromFile(first_ids_filename,
                                                    defines)
    from grit.node import empty
    for node in self.Preorder():
      if isinstance(node, empty.GroupingNode):
        abs_filename = os.path.abspath(filename_or_stream)
        if abs_filename[:len(src_root_dir)] != src_root_dir:
          filename = os.path.basename(filename_or_stream)
          # If the file is not within src_root_dir and the basename is not
          # found as a key, fall back to using the absolute file name.
          if not filename in first_ids:
            filename = abs_filename
            filename = filename.replace('\\', '/')
        else:
          filename = abs_filename[len(src_root_dir) + 1:]
          filename = filename.replace('\\', '/')

        if node.attrs['first_id'] != '':
          raise Exception(
              "Don't set the first_id attribute when using the first_ids_file "
              "attribute on the <grit> node, update %s instead." %
              first_ids_filename)

        try:
          id_list = first_ids[filename][node.name]
        except KeyError, e:
          print '-' * 78
          print 'Resource id not set for %s (%s)!' % (filename, node.name)
          print ('Please update %s to include an entry for %s.  See the '
                 'comments in resource_ids for information on why you need to '
                 'update that file.' % (first_ids_filename, filename))
          print '-' * 78
          raise e

        try:
          node.attrs['first_id'] = str(id_list.pop(0))
        except IndexError, e:
          raise Exception('Please update %s and add a first id for %s (%s).'
                          % (first_ids_filename, filename, node.name))

  def RunGatherers(self, debug=False):
    '''Call RunPreSubstitutionGatherer() on every node of the tree, then apply
    substitutions, then call RunPostSubstitutionGatherer() on every node.

    The substitutions step requires that the output language has been set.
    Locally, get the Substitution messages and add them to the substituter.
    Also add substitutions for language codes in the Rc.

    Args:
      debug: will print information while running gatherers.
    '''
    for node in self.ActiveDescendants():
      if hasattr(node, 'RunPreSubstitutionGatherer'):
        with node:
          node.RunPreSubstitutionGatherer(debug=debug)

    assert self.output_language
    self.SubstituteMessages(self.GetSubstituter())

    for node in self.ActiveDescendants():
      if hasattr(node, 'RunPostSubstitutionGatherer'):
        with node:
          node.RunPostSubstitutionGatherer(debug=debug)


class IdentifierNode(base.Node):
  """A node for specifying identifiers that should appear in the resource
  header file, and be unique amongst all other resource identifiers, but don't
  have any other attributes or reference any resources.
  """

  def MandatoryAttributes(self):
    return ['name']

  def DefaultAttributes(self):
    return { 'comment' : '', 'id' : '', 'systemid': 'false' }

  def GetId(self):
    """Returns the id of this identifier if it has one, None otherwise
    """
    if 'id' in self.attrs:
      return self.attrs['id']
    return None

  def EndParsing(self):
    """Handles system identifiers."""
    super(IdentifierNode, self).EndParsing()
    if self.attrs['systemid'] == 'true':
      util.SetupSystemIdentifiers((self.attrs['name'],))

  @staticmethod
  def Construct(parent, name, id, comment, systemid='false'):
    """Creates a new node which is a child of 'parent', with attributes set
    by parameters of the same name.
    """
    node = IdentifierNode()
    node.StartParsing('identifier', parent)
    node.HandleAttribute('name', name)
    node.HandleAttribute('id', id)
    node.HandleAttribute('comment', comment)
    node.HandleAttribute('systemid', systemid)
    node.EndParsing()
    return node
