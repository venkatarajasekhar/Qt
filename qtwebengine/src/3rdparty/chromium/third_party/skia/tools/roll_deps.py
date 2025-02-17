#!/usr/bin/python2

# Copyright 2014 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Skia's Chromium DEPS roll script.

This script:
- searches through the last N Skia git commits to find out the hash that is
  associated with the SVN revision number.
- creates a new branch in the Chromium tree, modifies the DEPS file to
  point at the given Skia commit, commits, uploads to Rietveld, and
  deletes the local copy of the branch.
- creates a whitespace-only commit and uploads that to to Rietveld.
- returns the Chromium tree to its previous state.

To specify the location of the git executable, set the GIT_EXECUTABLE
environment variable.

Usage:
  %prog -c CHROMIUM_PATH -r REVISION [OPTIONAL_OPTIONS]
"""


import optparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

import git_utils
import misc_utils


DEFAULT_BOTS_LIST = [
    'android_clang_dbg',
    'android_dbg',
    'android_rel',
    'cros_daisy',
    'linux',
    'linux_asan',
    'linux_chromeos',
    'linux_chromeos_asan',
    'linux_chromium_gn_dbg',
    'linux_gpu',
    'linux_layout',
    'linux_layout_rel',
    'mac',
    'mac_asan',
    'mac_gpu',
    'mac_layout',
    'mac_layout_rel',
    'win',
    'win_gpu',
    'win_layout',
    'win_layout_rel',
]


class DepsRollConfig(object):
    """Contains configuration options for this module.

    Attributes:
        git: (string) The git executable.
        chromium_path: (string) path to a local chromium git repository.
        save_branches: (boolean) iff false, delete temporary branches.
        verbose: (boolean)  iff false, suppress the output from git-cl.
        search_depth: (int) how far back to look for the revision.
        skia_url: (string) Skia's git repository.
        self.skip_cl_upload: (boolean)
        self.cl_bot_list: (list of strings)
    """

    # pylint: disable=I0011,R0903,R0902
    def __init__(self, options=None):
        self.skia_url = 'https://skia.googlesource.com/skia.git'
        self.revision_format = (
            'git-svn-id: http://skia.googlecode.com/svn/trunk@%d ')

        self.git = git_utils.git_executable()

        if not options:
            options = DepsRollConfig.GetOptionParser()
        # pylint: disable=I0011,E1103
        self.verbose = options.verbose
        self.vsp = misc_utils.VerboseSubprocess(self.verbose)
        self.save_branches = not options.delete_branches
        self.search_depth = options.search_depth
        self.chromium_path = options.chromium_path
        self.skip_cl_upload = options.skip_cl_upload
        # Split and remove empty strigns from the bot list.
        self.cl_bot_list = [bot for bot in options.bots.split(',') if bot]
        self.skia_git_checkout_path = options.skia_git_path
        self.default_branch_name = 'autogenerated_deps_roll_branch'
        self.reviewers_list = ','.join([
            # 'rmistry@google.com',
            # 'reed@google.com',
            # 'bsalomon@google.com',
            # 'robertphillips@google.com',
            ])
        self.cc_list = ','.join([
            # 'skia-team@google.com',
            ])

    @staticmethod
    def GetOptionParser():
        # pylint: disable=I0011,C0103
        """Returns an optparse.OptionParser object.

        Returns:
            An optparse.OptionParser object.

        Called by the main() function.
        """
        option_parser = optparse.OptionParser(usage=__doc__)
        # Anyone using this script on a regular basis should set the
        # CHROMIUM_CHECKOUT_PATH environment variable.
        option_parser.add_option(
            '-c', '--chromium_path', help='Path to local Chromium Git'
            ' repository checkout, defaults to CHROMIUM_CHECKOUT_PATH'
            ' if that environment variable is set.',
            default=os.environ.get('CHROMIUM_CHECKOUT_PATH'))
        option_parser.add_option(
            '-r', '--revision', type='int', default=None,
            help='The Skia SVN revision number, defaults to top of tree.')
        option_parser.add_option(
            '-g', '--git_hash', default=None,
            help='A partial Skia Git hash.  Do not set this and revision.')

        # Anyone using this script on a regular basis should set the
        # SKIA_GIT_CHECKOUT_PATH environment variable.
        option_parser.add_option(
            '', '--skia_git_path',
            help='Path of a pure-git Skia repository checkout.  If empty,'
            ' a temporary will be cloned.  Defaults to SKIA_GIT_CHECKOUT'
            '_PATH, if that environment variable is set.',
            default=os.environ.get('SKIA_GIT_CHECKOUT_PATH'))
        option_parser.add_option(
            '', '--search_depth', type='int', default=100,
            help='How far back to look for the revision.')
        option_parser.add_option(
            '', '--delete_branches', help='Delete the temporary branches',
            action='store_true', dest='delete_branches', default=False)
        option_parser.add_option(
            '', '--verbose', help='Do not suppress the output from `git cl`.',
            action='store_true', dest='verbose', default=False)
        option_parser.add_option(
            '', '--skip_cl_upload', help='Skip the cl upload step; useful'
            ' for testing.',
            action='store_true', default=False)

        default_bots_help = (
            'Comma-separated list of bots, defaults to a list of %d bots.'
            '  To skip `git cl try`, set this to an empty string.'
            % len(DEFAULT_BOTS_LIST))
        default_bots = ','.join(DEFAULT_BOTS_LIST)
        option_parser.add_option(
            '', '--bots', help=default_bots_help, default=default_bots)

        return option_parser


class DepsRollError(Exception):
    """Exceptions specific to this module."""
    pass


def get_svn_revision(config, commit):
    """Works in both git and git-svn. returns a string."""
    svn_format = (
        '(git-svn-id: [^@ ]+@|SVN changes up to revision |'
        'LKGR w/ DEPS up to revision )(?P<return>[0-9]+)')
    svn_revision = misc_utils.ReSearch.search_within_output(
        config.verbose, svn_format, None,
        [config.git, 'log', '-n', '1', '--format=format:%B', commit])
    if not svn_revision:
        raise DepsRollError(
            'Revision number missing from Chromium origin/master.')
    return int(svn_revision)


class SkiaGitCheckout(object):
    """Class to create a temporary skia git checkout, if necessary.
    """
    # pylint: disable=I0011,R0903

    def __init__(self, config, depth):
        self._config = config
        self._depth = depth
        self._use_temp = None
        self._original_cwd = None

    def __enter__(self):
        config = self._config
        git = config.git
        skia_dir = None
        self._original_cwd = os.getcwd()
        if config.skia_git_checkout_path:
            if config.skia_git_checkout_path != os.curdir:
                skia_dir = config.skia_git_checkout_path
                ## Update origin/master if needed.
                if self._config.verbose:
                    print '~~$', 'cd', skia_dir
                os.chdir(skia_dir)
            config.vsp.check_call([git, 'fetch', '-q', 'origin'])
            self._use_temp = None
        else:
            skia_dir = tempfile.mkdtemp(prefix='git_skia_tmp_')
            self._use_temp = skia_dir
            try:
                os.chdir(skia_dir)
                config.vsp.check_call(
                    [git, 'clone', '-q', '--depth=%d' % self._depth,
                     '--single-branch', config.skia_url, '.'])
            except (OSError, subprocess.CalledProcessError) as error:
                shutil.rmtree(skia_dir)
                raise error

    def __exit__(self, etype, value, traceback):
        if self._config.skia_git_checkout_path != os.curdir:
            if self._config.verbose:
                print '~~$', 'cd', self._original_cwd
            os.chdir(self._original_cwd)
        if self._use_temp:
            shutil.rmtree(self._use_temp)


def revision_and_hash(config):
    """Finds revision number and git hash of origin/master in the Skia tree.

    Args:
        config: (roll_deps.DepsRollConfig) object containing options.

    Returns:
        A tuple (revision, hash)
            revision: (int) SVN revision number.
            git_hash: (string) full Git commit hash.

    Raises:
        roll_deps.DepsRollError: if the revision can't be found.
        OSError: failed to execute git or git-cl.
        subprocess.CalledProcessError: git returned unexpected status.
    """
    with SkiaGitCheckout(config, 1):
        revision = get_svn_revision(config, 'origin/master')
        git_hash = config.vsp.strip_output(
            [config.git, 'show-ref', 'origin/master', '--hash'])
        if not git_hash:
            raise DepsRollError('Git hash can not be found.')
    return revision, git_hash


def revision_and_hash_from_revision(config, revision):
    """Finds revision number and git hash of a commit in the Skia tree.

    Args:
        config: (roll_deps.DepsRollConfig) object containing options.
        revision: (int) SVN revision number.

    Returns:
        A tuple (revision, hash)
            revision: (int) SVN revision number.
            git_hash: (string) full Git commit hash.

    Raises:
        roll_deps.DepsRollError: if the revision can't be found.
        OSError: failed to execute git or git-cl.
        subprocess.CalledProcessError: git returned unexpected status.
    """
    with SkiaGitCheckout(config, config.search_depth):
        revision_regex = config.revision_format % revision
        git_hash = config.vsp.strip_output(
            [config.git, 'log', '--grep', revision_regex,
             '--format=format:%H', 'origin/master'])
        if not git_hash:
            raise DepsRollError('Git hash can not be found.')
    return revision, git_hash


def revision_and_hash_from_partial(config, partial_hash):
    """Returns the SVN revision number and full git hash.

    Args:
        config: (roll_deps.DepsRollConfig) object containing options.
        partial_hash: (string) Partial git commit hash.

    Returns:
        A tuple (revision, hash)
            revision: (int) SVN revision number.
            git_hash: (string) full Git commit hash.

    Raises:
        roll_deps.DepsRollError: if the revision can't be found.
        OSError: failed to execute git or git-cl.
        subprocess.CalledProcessError: git returned unexpected status.
    """
    with SkiaGitCheckout(config, config.search_depth):
        git_hash = config.vsp.strip_output(
            ['git', 'log', '-n', '1', '--format=format:%H', partial_hash])
        if not git_hash:
            raise DepsRollError('Partial Git hash can not be found.')
        revision = get_svn_revision(config, git_hash)
    return revision, git_hash


def change_skia_deps(revision, git_hash, depspath):
    """Update the DEPS file.

    Modify the skia_revision and skia_hash entries in the given DEPS file.

    Args:
        revision: (int) Skia SVN revision.
        git_hash: (string) Skia Git hash.
        depspath: (string) path to DEPS file.
    """
    temp_file = tempfile.NamedTemporaryFile(delete=False,
                                            prefix='skia_DEPS_ROLL_tmp_')
    try:
        deps_regex_rev = re.compile('"skia_revision": "[0-9]*",')
        deps_regex_hash = re.compile('"skia_hash": "[0-9a-f]*",')

        deps_regex_rev_repl = '"skia_revision": "%d",' % revision
        deps_regex_hash_repl = '"skia_hash": "%s",' % git_hash

        with open(depspath, 'r') as input_stream:
            for line in input_stream:
                line = deps_regex_rev.sub(deps_regex_rev_repl, line)
                line = deps_regex_hash.sub(deps_regex_hash_repl, line)
                temp_file.write(line)
    finally:
        temp_file.close()
    shutil.move(temp_file.name, depspath)


def git_cl_uploader(config, message, file_list):
    """Create a commit in the current git branch; upload via git-cl.

    Assumes that you are already on the branch you want to be on.

    Args:
        config: (roll_deps.DepsRollConfig) object containing options.
        message: (string) the commit message, can be multiline.
        file_list: (list of strings) list of filenames to pass to `git add`.

    Returns:
        The output of `git cl issue`, if not config.skip_cl_upload, else ''.
    """

    git, vsp = config.git, config.vsp
    svn_info = str(get_svn_revision(config, 'HEAD'))

    for filename in file_list:
        assert os.path.exists(filename)
        vsp.check_call([git, 'add', filename])

    vsp.check_call([git, 'commit', '-q', '-m', message])

    git_cl = [git, 'cl', 'upload', '-f',
              '--bypass-hooks', '--bypass-watchlists']
    if config.cc_list:
        git_cl.append('--cc=%s' % config.cc_list)
    if config.reviewers_list:
        git_cl.append('--reviewers=%s' % config.reviewers_list)

    git_try = [
        git, 'cl', 'try', '-m', 'tryserver.chromium', '--revision', svn_info]
    git_try.extend([arg for bot in config.cl_bot_list for arg in ('-b', bot)])

    branch_name = git_utils.git_branch_name(vsp.verbose)

    if config.skip_cl_upload:
        space = '   '
        print 'You should call:'
        print '%scd %s' % (space, os.getcwd())
        misc_utils.print_subprocess_args(space, [git, 'checkout', branch_name])
        misc_utils.print_subprocess_args(space, git_cl)
        if config.cl_bot_list:
            misc_utils.print_subprocess_args(space, git_try)
        print
        return ''
    else:
        vsp.check_call(git_cl)
        issue = vsp.strip_output([git, 'cl', 'issue'])
        if config.cl_bot_list:
            vsp.check_call(git_try)
        return issue


def roll_deps(config, revision, git_hash):
    """Upload changed DEPS and a whitespace change.

    Given the correct git_hash, create two Reitveld issues.

    Args:
        config: (roll_deps.DepsRollConfig) object containing options.
        revision: (int) Skia SVN revision.
        git_hash: (string) Skia Git hash.

    Returns:
        a tuple containing textual description of the two issues.

    Raises:
        OSError: failed to execute git or git-cl.
        subprocess.CalledProcessError: git returned unexpected status.
    """

    git = config.git
    with misc_utils.ChangeDir(config.chromium_path, config.verbose):
        config.vsp.check_call([git, 'fetch', '-q', 'origin'])

        old_revision = misc_utils.ReSearch.search_within_output(
            config.verbose, '"skia_revision": "(?P<return>[0-9]+)",', None,
            [git, 'show', 'origin/master:DEPS'])
        assert old_revision
        if revision == int(old_revision):
            print 'DEPS is up to date!'
            return (None, None)

        master_hash = config.vsp.strip_output(
            [git, 'show-ref', 'origin/master', '--hash'])
        master_revision = get_svn_revision(config, 'origin/master')

        # master_hash[8] gives each whitespace CL a unique name.
        if config.save_branches:
            branch = 'control_%s' % master_hash[:8]
        else:
            branch = None
        message = ('whitespace change %s\n\n'
                   'Chromium base revision: %d / %s\n\n'
                   'This CL was created by Skia\'s roll_deps.py script.\n'
                  ) % (master_hash[:8], master_revision, master_hash[:8])
        with git_utils.ChangeGitBranch(branch, 'origin/master',
                                       config.verbose):
            branch = git_utils.git_branch_name(config.vsp.verbose)

            with open('build/whitespace_file.txt', 'a') as output_stream:
                output_stream.write('\nCONTROL\n')

            whitespace_cl = git_cl_uploader(
                config, message, ['build/whitespace_file.txt'])

            control_url = misc_utils.ReSearch.search_within_string(
                whitespace_cl, '(?P<return>https?://[^) ]+)', '?')
            if config.save_branches:
                whitespace_cl = '%s\n    branch: %s' % (whitespace_cl, branch)

        if config.save_branches:
            branch = 'roll_%d_%s' % (revision, master_hash[:8])
        else:
            branch = None
        message = (
            'roll skia DEPS to %d\n\n'
            'Chromium base revision: %d / %s\n'
            'Old Skia revision: %s\n'
            'New Skia revision: %d\n'
            'Control CL: %s\n\n'
            'This CL was created by Skia\'s roll_deps.py script.\n\n'
            'Bypassing commit queue trybots:\n'
            'NOTRY=true\n'
            % (revision, master_revision, master_hash[:8],
               old_revision, revision, control_url))
        with git_utils.ChangeGitBranch(branch, 'origin/master',
                                       config.verbose):
            branch = git_utils.git_branch_name(config.vsp.verbose)

            change_skia_deps(revision, git_hash, 'DEPS')
            deps_cl = git_cl_uploader(config, message, ['DEPS'])
            if config.save_branches:
                deps_cl = '%s\n    branch: %s' % (deps_cl, branch)

        return deps_cl, whitespace_cl


def find_hash_and_roll_deps(config, revision=None, partial_hash=None):
    """Call find_hash_from_revision() and roll_deps().

    The calls to git will be verbose on standard output.  After a
    successful upload of both issues, print links to the new
    codereview issues.

    Args:
        config: (roll_deps.DepsRollConfig) object containing options.
        revision: (int or None) the Skia SVN revision number or None
            to use the tip of the tree.
        partial_hash: (string or None) a partial pure-git Skia commit
            hash.  Don't pass both partial_hash and revision.

    Raises:
        roll_deps.DepsRollError: if the revision can't be found.
        OSError: failed to execute git or git-cl.
        subprocess.CalledProcessError: git returned unexpected status.
    """

    if revision and partial_hash:
        raise DepsRollError('Pass revision or partial_hash, not both.')

    if partial_hash:
        revision, git_hash = revision_and_hash_from_partial(
            config, partial_hash)
    elif revision:
        revision, git_hash = revision_and_hash_from_revision(config, revision)
    else:
        revision, git_hash = revision_and_hash(config)

    print 'revision=%r\nhash=%r\n' % (revision, git_hash)

    deps_issue, whitespace_issue = roll_deps(config, revision, git_hash)

    if deps_issue and whitespace_issue:
        print 'DEPS roll:\n    %s\n' % deps_issue
        print 'Whitespace change:\n    %s\n' % whitespace_issue
    else:
        print >> sys.stderr, 'No issues created.'


def main(args):
    """main function; see module-level docstring and GetOptionParser help.

    Args:
        args: sys.argv[1:]-type argument list.
    """
    option_parser = DepsRollConfig.GetOptionParser()
    options = option_parser.parse_args(args)[0]

    if not options.chromium_path:
        option_parser.error('Must specify chromium_path.')
    if not os.path.isdir(options.chromium_path):
        option_parser.error('chromium_path must be a directory.')

    if not git_utils.git_executable():
        option_parser.error('Invalid git executable.')

    config = DepsRollConfig(options)
    find_hash_and_roll_deps(config, options.revision, options.git_hash)


if __name__ == '__main__':
    main(sys.argv[1:])

