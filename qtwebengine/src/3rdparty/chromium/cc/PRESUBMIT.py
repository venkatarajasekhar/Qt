# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for cc.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import re
import string

CC_SOURCE_FILES=(r'^cc/.*\.(cc|h)$',)

def CheckChangeLintsClean(input_api, output_api):
  input_api.cpplint._cpplint_state.ResetErrorCounts()  # reset global state
  source_filter = lambda x: input_api.FilterSourceFile(
    x, white_list=CC_SOURCE_FILES, black_list=None)
  files = [f.AbsoluteLocalPath() for f in
           input_api.AffectedSourceFiles(source_filter)]
  level = 1  # strict, but just warn

  for file_name in files:
    input_api.cpplint.ProcessFile(file_name, level)

  if not input_api.cpplint._cpplint_state.error_count:
    return []

  return [output_api.PresubmitPromptWarning(
    'Changelist failed cpplint.py check.')]

def CheckAsserts(input_api, output_api, white_list=CC_SOURCE_FILES, black_list=None):
  black_list = tuple(black_list or input_api.DEFAULT_BLACK_LIST)
  source_file_filter = lambda x: input_api.FilterSourceFile(x, white_list, black_list)

  assert_files = []
  notreached_files = []

  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    # WebKit ASSERT() is not allowed.
    if re.search(r"\bASSERT\(", contents):
      assert_files.append(f.LocalPath())
    # WebKit ASSERT_NOT_REACHED() is not allowed.
    if re.search(r"ASSERT_NOT_REACHED\(", contents):
      notreached_files.append(f.LocalPath())

  if assert_files:
    return [output_api.PresubmitError(
      'These files use ASSERT instead of using DCHECK:',
      items=assert_files)]
  if notreached_files:
    return [output_api.PresubmitError(
      'These files use ASSERT_NOT_REACHED instead of using NOTREACHED:',
      items=notreached_files)]
  return []

def CheckStdAbs(input_api, output_api,
                white_list=CC_SOURCE_FILES, black_list=None):
  black_list = tuple(black_list or input_api.DEFAULT_BLACK_LIST)
  source_file_filter = lambda x: input_api.FilterSourceFile(x,
                                                            white_list,
                                                            black_list)

  using_std_abs_files = []
  found_fabs_files = []
  missing_std_prefix_files = []

  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    if re.search(r"using std::f?abs;", contents):
      using_std_abs_files.append(f.LocalPath())
    if re.search(r"\bfabsf?\(", contents):
      found_fabs_files.append(f.LocalPath());

    no_std_prefix = r"(?<!std::)"
    # Matches occurrences of abs/absf/fabs/fabsf without a "std::" prefix.
    abs_without_prefix = r"%s(\babsf?\()" % no_std_prefix
    fabs_without_prefix = r"%s(\bfabsf?\()" % no_std_prefix
    # Skips matching any lines that have "// NOLINT".
    no_nolint = r"(?![^\n]*//\s+NOLINT)"

    expression = re.compile("(%s|%s)%s" %
        (abs_without_prefix, fabs_without_prefix, no_nolint))
    if expression.search(contents):
      missing_std_prefix_files.append(f.LocalPath())

  result = []
  if using_std_abs_files:
    result.append(output_api.PresubmitError(
        'These files have "using std::abs" which is not permitted.',
        items=using_std_abs_files))
  if found_fabs_files:
    result.append(output_api.PresubmitError(
        'std::abs() should be used instead of std::fabs() for consistency.',
        items=found_fabs_files))
  if missing_std_prefix_files:
    result.append(output_api.PresubmitError(
        'These files use abs(), absf(), fabs(), or fabsf() without qualifying '
        'the std namespace. Please use std::abs() in all places.',
        items=missing_std_prefix_files))
  return result

def CheckPassByValue(input_api,
                     output_api,
                     white_list=CC_SOURCE_FILES,
                     black_list=None):
  black_list = tuple(black_list or input_api.DEFAULT_BLACK_LIST)
  source_file_filter = lambda x: input_api.FilterSourceFile(x,
                                                            white_list,
                                                            black_list)

  local_errors = []

  # Well-defined simple classes containing only <= 4 ints, or <= 2 floats.
  pass_by_value_types = ['base::Time',
                         'base::TimeTicks',
                         ]

  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    match = re.search(
      r'\bconst +' + '(?P<type>(%s))&' %
        string.join(pass_by_value_types, '|'),
      contents)
    if match:
      local_errors.append(output_api.PresubmitError(
        '%s passes %s by const ref instead of by value.' %
        (f.LocalPath(), match.group('type'))))
  return local_errors

def CheckTodos(input_api, output_api):
  errors = []

  source_file_filter = lambda x: x
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    if ('FIX'+'ME') in contents:
      errors.append(f.LocalPath())

  if errors:
    return [output_api.PresubmitError(
      'All TODO comments should be of the form TODO(name).',
      items=errors)]
  return []

def FindUnquotedQuote(contents, pos):
  match = re.search(r"(?<!\\)(?P<quote>\")", contents[pos:])
  return -1 if not match else match.start("quote") + pos

def FindUselessIfdefs(input_api, output_api):
  errors = []
  source_file_filter = lambda x: x
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    if re.search(r'#if\s*0\s', contents):
      errors.append(f.LocalPath())
  if errors:
    return [output_api.PresubmitError(
      'Don\'t use #if '+'0; just delete the code.',
      items=errors)]
  return []

def FindNamespaceInBlock(pos, namespace, contents, whitelist=[]):
  open_brace = -1
  close_brace = -1
  quote = -1
  name = -1
  brace_count = 1
  quote_count = 0
  while pos < len(contents) and brace_count > 0:
    if open_brace < pos: open_brace = contents.find("{", pos)
    if close_brace < pos: close_brace = contents.find("}", pos)
    if quote < pos: quote = FindUnquotedQuote(contents, pos)
    if name < pos: name = contents.find(("%s::" % namespace), pos)

    if name < 0:
      return False # The namespace is not used at all.
    if open_brace < 0:
      open_brace = len(contents)
    if close_brace < 0:
      close_brace = len(contents)
    if quote < 0:
      quote = len(contents)

    next = min(open_brace, min(close_brace, min(quote, name)))

    if next == open_brace:
      brace_count += 1
    elif next == close_brace:
      brace_count -= 1
    elif next == quote:
      quote_count = 0 if quote_count else 1
    elif next == name and not quote_count:
      in_whitelist = False
      for w in whitelist:
        if re.match(w, contents[next:]):
          in_whitelist = True
          break
      if not in_whitelist:
        return True
    pos = next + 1
  return False

# Checks for the use of cc:: within the cc namespace, which is usually
# redundant.
def CheckNamespace(input_api, output_api):
  errors = []

  source_file_filter = lambda x: x
  for f in input_api.AffectedSourceFiles(source_file_filter):
    contents = input_api.ReadFile(f, 'rb')
    match = re.search(r'namespace\s*cc\s*{', contents)
    if match:
      whitelist = [
        r"cc::remove_if\b",
        ]
      if FindNamespaceInBlock(match.end(), 'cc', contents, whitelist=whitelist):
        errors.append(f.LocalPath())

  if errors:
    return [output_api.PresubmitError(
      'Do not use cc:: inside of the cc namespace.',
      items=errors)]
  return []

def CheckForUseOfWrongClock(input_api,
                            output_api,
                            white_list=CC_SOURCE_FILES,
                            black_list=None):
  """Make sure new lines of code don't use a clock susceptible to skew."""
  black_list = tuple(black_list or input_api.DEFAULT_BLACK_LIST)
  source_file_filter = lambda x: input_api.FilterSourceFile(x,
                                                            white_list,
                                                            black_list)
  # Regular expression that should detect any explicit references to the
  # base::Time type (or base::Clock/DefaultClock), whether in using decls,
  # typedefs, or to call static methods.
  base_time_type_pattern = r'(^|\W)base::(Time|Clock|DefaultClock)(\W|$)'

  # Regular expression that should detect references to the base::Time class
  # members, such as a call to base::Time::Now.
  base_time_member_pattern = r'(^|\W)(Time|Clock|DefaultClock)::'

  # Regular expression to detect "using base::Time" declarations.  We want to
  # prevent these from triggerring a warning.  For example, it's perfectly
  # reasonable for code to be written like this:
  #
  #   using base::Time;
  #   ...
  #   int64 foo_us = foo_s * Time::kMicrosecondsPerSecond;
  using_base_time_decl_pattern = r'^\s*using\s+(::)?base::Time\s*;'

  # Regular expression to detect references to the kXXX constants in the
  # base::Time class.  We want to prevent these from triggerring a warning.
  base_time_konstant_pattern = r'(^|\W)Time::k\w+'

  problem_re = input_api.re.compile(
      r'(' + base_time_type_pattern + r')|(' + base_time_member_pattern + r')')
  exception_re = input_api.re.compile(
      r'(' + using_base_time_decl_pattern + r')|(' +
      base_time_konstant_pattern + r')')
  problems = []
  for f in input_api.AffectedSourceFiles(source_file_filter):
    for line_number, line in f.ChangedContents():
      if problem_re.search(line):
        if not exception_re.search(line):
          problems.append(
              '  %s:%d\n    %s' % (f.LocalPath(), line_number, line.strip()))

  if problems:
    return [output_api.PresubmitPromptOrNotify(
        'You added one or more references to the base::Time class and/or one\n'
        'of its member functions (or base::Clock/DefaultClock). In cc code,\n'
        'it is most certainly incorrect! Instead use base::TimeTicks.\n\n'
        '\n'.join(problems))]
  else:
    return []

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results += CheckAsserts(input_api, output_api)
  results += CheckStdAbs(input_api, output_api)
  results += CheckPassByValue(input_api, output_api)
  results += CheckChangeLintsClean(input_api, output_api)
  results += CheckTodos(input_api, output_api)
  results += CheckNamespace(input_api, output_api)
  results += CheckForUseOfWrongClock(input_api, output_api)
  results += FindUselessIfdefs(input_api, output_api)
  results += input_api.canned_checks.CheckPatchFormatted(input_api, output_api)
  return results

def GetPreferredTryMasters(project, change):
  return {
    'tryserver.chromium': {
      'linux_blink_rel': set(['defaulttests']),
    },
    'tryserver.chromium.gpu': {
      'linux_gpu': set(['defaulttests']),
      'mac_gpu': set(['defaulttests']),
      'win_gpu': set(['defaulttests']),
    },
  }
