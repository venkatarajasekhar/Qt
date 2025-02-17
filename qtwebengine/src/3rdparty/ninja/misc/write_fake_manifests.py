#!/usr/bin/env python

"""Writes large manifest files, for manifest parser performance testing.

The generated manifest files are (eerily) similar in appearance and size to the
ones used in the Chromium project.

Usage:
  python misc/write_fake_manifests.py outdir  # Will run for about 5s.

The program contains a hardcoded random seed, so it will generate the same
output every time it runs.  By changing the seed, it's easy to generate many
different sets of manifest files.
"""

import argparse
import contextlib
import os
import random
import sys

import ninja_syntax


def paretoint(avg, alpha):
    """Returns a random integer that's avg on average, following a power law.
    alpha determines the shape of the power curve. alpha has to be larger
    than 1. The closer alpha is to 1, the higher the variation of the returned
    numbers."""
    return int(random.paretovariate(alpha) * avg / (alpha / (alpha - 1)))


# Based on http://neugierig.org/software/chromium/class-name-generator.html
def moar(avg_options, p_suffix):
    kStart = ['render', 'web', 'browser', 'tab', 'content', 'extension', 'url',
              'file', 'sync', 'content', 'http', 'profile']
    kOption = ['view', 'host', 'holder', 'container', 'impl', 'ref',
               'delegate', 'widget', 'proxy', 'stub', 'context',
               'manager', 'master', 'watcher', 'service', 'file', 'data',
               'resource', 'device', 'info', 'provider', 'internals', 'tracker',
               'api', 'layer']
    kOS = ['win', 'mac', 'aura', 'linux', 'android', 'unittest', 'browsertest']
    num_options = min(paretoint(avg_options, alpha=4), 5)
    # The original allows kOption to repeat as long as no consecutive options
    # repeat.  This version doesn't allow any option repetition.
    name = [random.choice(kStart)] + random.sample(kOption, num_options)
    if random.random() < p_suffix:
        name.append(random.choice(kOS))
    return '_'.join(name)


class GenRandom(object):
    def __init__(self):
        self.seen_names = set([None])
        self.seen_defines = set([None])

    def _unique_string(self, seen, avg_options=1.3, p_suffix=0.1):
        s = None
        while s in seen:
            s = moar(avg_options, p_suffix)
        seen.add(s)
        return s

    def _n_unique_strings(self, n):
        seen = set([None])
        return [self._unique_string(seen, avg_options=3, p_suffix=0.4)
                for _ in xrange(n)]

    def target_name(self):
        return self._unique_string(p_suffix=0, seen=self.seen_names)

    def path(self):
        return os.path.sep.join([
            self._unique_string(self.seen_names, avg_options=1, p_suffix=0)
            for _ in xrange(1 + paretoint(0.6, alpha=4))])

    def src_obj_pairs(self, path, name):
        num_sources = paretoint(55, alpha=2) + 1
        return [(os.path.join('..', '..', path, s + '.cc'),
                 os.path.join('obj', path, '%s.%s.o' % (name, s)))
                for s in self._n_unique_strings(num_sources)]

    def defines(self):
        return [
            '-DENABLE_' + self._unique_string(self.seen_defines).upper()
            for _ in xrange(paretoint(20, alpha=3))]


LIB, EXE = 0, 1
class Target(object):
    def __init__(self, gen, kind):
        self.name = gen.target_name()
        self.dir_path = gen.path()
        self.ninja_file_path = os.path.join(
            'obj', self.dir_path, self.name + '.ninja')
        self.src_obj_pairs = gen.src_obj_pairs(self.dir_path, self.name)
        if kind == LIB:
            self.output = os.path.join('lib' + self.name + '.a')
        elif kind == EXE:
            self.output = os.path.join(self.name)
        self.defines = gen.defines()
        self.deps = []
        self.kind = kind
        self.has_compile_depends = random.random() < 0.4

    @property
    def includes(self):
        return ['-I' + dep.dir_path for dep in self.deps]


def write_target_ninja(ninja, target):
    compile_depends = None
    if target.has_compile_depends:
      compile_depends = os.path.join(
          'obj', target.dir_path, target.name + '.stamp')
      ninja.build(compile_depends, 'stamp', target.src_obj_pairs[0][0])
      ninja.newline()

    ninja.variable('defines', target.defines)
    if target.deps:
        ninja.variable('includes', target.includes)
    ninja.variable('cflags', ['-Wall', '-fno-rtti', '-fno-exceptions'])
    ninja.newline()

    for src, obj in target.src_obj_pairs:
        ninja.build(obj, 'cxx', src, implicit=compile_depends)
    ninja.newline()

    deps = [dep.output for dep in target.deps]
    libs = [dep.output for dep in target.deps if dep.kind == LIB]
    if target.kind == EXE:
        ninja.variable('ldflags', '-Wl,pie')
        ninja.variable('libs', libs)
    link = { LIB: 'alink', EXE: 'link'}[target.kind]
    ninja.build(target.output, link, [obj for _, obj in target.src_obj_pairs],
                implicit=deps)


def write_master_ninja(master_ninja, targets):
    """Writes master build.ninja file, referencing all given subninjas."""
    master_ninja.variable('cxx', 'c++')
    master_ninja.variable('ld', '$cxx')
    master_ninja.newline()

    master_ninja.pool('link_pool', depth=4)
    master_ninja.newline()

    master_ninja.rule('cxx', description='CXX $out',
      command='$cxx -MMD -MF $out.d $defines $includes $cflags -c $in -o $out',
      depfile='$out.d', deps='gcc')
    master_ninja.rule('alink', description='LIBTOOL-STATIC $out',
      command='rm -f $out && libtool -static -o $out $in')
    master_ninja.rule('link', description='LINK $out', pool='link_pool',
      command='$ld $ldflags -o $out $in $libs')
    master_ninja.rule('stamp', description='STAMP $out', command='touch $out')
    master_ninja.newline()

    for target in targets:
        master_ninja.subninja(target.ninja_file_path)
    master_ninja.newline()

    master_ninja.comment('Short names for targets.')
    for target in targets:
        if target.name != target.output:
            master_ninja.build(target.name, 'phony', target.output)
    master_ninja.newline()

    master_ninja.build('all', 'phony', [target.output for target in targets])
    master_ninja.default('all')


@contextlib.contextmanager
def FileWriter(path):
    """Context manager for a ninja_syntax object writing to a file."""
    try:
        os.makedirs(os.path.dirname(path))
    except OSError:
        pass
    f = open(path, 'w')
    yield ninja_syntax.Writer(f)
    f.close()


def random_targets():
    num_targets = 800
    gen = GenRandom()

    # N-1 static libraries, and 1 executable depending on all of them.
    targets = [Target(gen, LIB) for i in xrange(num_targets - 1)]
    for i in range(len(targets)):
        targets[i].deps = [t for t in targets[0:i] if random.random() < 0.05]

    last_target = Target(gen, EXE)
    last_target.deps = targets[:]
    last_target.src_obj_pairs = last_target.src_obj_pairs[0:10]  # Trim.
    targets.append(last_target)
    return targets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('outdir', help='output directory')
    args = parser.parse_args()
    root_dir = args.outdir

    random.seed(12345)

    targets = random_targets()
    for target in targets:
        with FileWriter(os.path.join(root_dir, target.ninja_file_path)) as n:
            write_target_ninja(n, target)

    with FileWriter(os.path.join(root_dir, 'build.ninja')) as master_ninja:
        master_ninja.width = 120
        write_master_ninja(master_ninja, targets)


if __name__ == '__main__':
    sys.exit(main())
