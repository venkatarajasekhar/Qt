#!/bin/bash -e

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to install everything needed to build chromium on android that
# requires sudo privileges.
# See http://code.google.com/p/chromium/wiki/AndroidBuildInstructions

# This script installs the sun-java6 packages (bin, jre and jdk). Sun requires
# a license agreement, so upon installation it will prompt the user. To get
# past the curses-based dialog press TAB <ret> TAB <ret> to agree.

if ! uname -m | egrep -q "i686|x86_64"; then
  echo "Only x86 architectures are currently supported" >&2
  exit
fi

# Install first the default Linux build deps.
"$(dirname "${BASH_SOURCE[0]}")/install-build-deps.sh" \
    --no-syms --no-arm --no-chromeos-fonts --no-nacl --no-prompt

# The temporary directory used to store output of update-java-alternatives
TEMPDIR=$(mktemp -d)
cleanup() {
  local status=${?}
  trap - EXIT
  rm -rf "${TEMPDIR}"
  exit ${status}
}
trap cleanup EXIT

sudo apt-get update

# Fix deps
sudo apt-get -f install

# Install deps
# This step differs depending on what Ubuntu release we are running
# on since the package names are different, and Sun's Java must
# be installed manually on late-model versions.

# common
sudo apt-get -y install checkstyle lighttpd python-pexpect xvfb x11-utils

# Few binaries in the Android SDK require 32-bit libraries on the host.
sudo apt-get -y install lib32z1 g++-multilib

sudo apt-get -y install ant

# Install openjdk and openjre 7 stuff
sudo apt-get -y install openjdk-7-jre openjdk-7-jdk

# Switch version of Java to openjdk 7.
# Some Java plugins (e.g. for firefox, mozilla) are not required to build, and
# thus are treated only as warnings. Any errors in updating java alternatives
# which are not '*-javaplugin.so' will cause errors and stop the script from
# completing successfully.
if ! sudo update-java-alternatives -s java-1.7.0-openjdk-amd64 \
           >& "${TEMPDIR}"/update-java-alternatives.out
then
  # Check that there are the expected javaplugin.so errors for the update
  if grep 'javaplugin.so' "${TEMPDIR}"/update-java-alternatives.out >& \
      /dev/null
  then
    # Print as warnings all the javaplugin.so errors
    echo 'WARNING: java-6-sun has no alternatives for the following plugins:'
    grep 'javaplugin.so' "${TEMPDIR}"/update-java-alternatives.out
  fi
  # Check if there are any errors that are not javaplugin.so
  if grep -v 'javaplugin.so' "${TEMPDIR}"/update-java-alternatives.out \
      >& /dev/null
  then
    # If there are non-javaplugin.so errors, treat as errors and exit
    echo 'ERRORS: Failed to update alternatives for java-6-sun:'
    grep -v 'javaplugin.so' "${TEMPDIR}"/update-java-alternatives.out
    exit 1
  fi
fi

echo "install-build-deps-android.sh complete."
