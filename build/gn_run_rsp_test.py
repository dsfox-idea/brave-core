#!/usr/bin/env python3
# Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""growser#368: the SDKROOT gn_run_rsp.py hands a tool is a real path.

The build points SDKROOT at out/<dir>/sdk/xcode_links/<sdk>, a symlink. xcrun
cannot resolve a tool against it, so a build script's bare `git` opened the
Command Line Tools installer on macOS.

    python3 brave/build/gn_run_rsp_test.py
"""

import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      'gn_run_rsp.py')
PRINT_SDKROOT = 'import os; print(os.environ["SDKROOT"])'


def run_rsp(tmp, sdkroot, *command):
    rsp = os.path.join(tmp, 'run.rsp')
    # gn_run_rsp.py reads the file with shlex.split on every platform.
    with open(rsp, 'w', encoding='utf-8') as f:
        f.write(' '.join(shlex.quote(a)
                         for a in (f'SDKROOT={sdkroot}', *command)))
    # The build runs it from the output directory, which abs@ is relative to.
    return subprocess.run([sys.executable, SCRIPT, rsp], cwd=tmp,
                          capture_output=True, text=True, check=False)


class SdkRootTest(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.real = os.path.join(self.tmp, 'Xcode', 'MacOSX.sdk')
        os.makedirs(self.real)
        self.link = os.path.join(self.tmp, 'xcode_links')
        try:
            os.symlink(self.real, self.link, target_is_directory=True)
        except OSError as error:
            self.skipTest(f'cannot create a symlink here: {error}')

    def test_symlinked_sdkroot_reaches_the_tool_resolved(self):
        result = run_rsp(self.tmp, self.link, sys.executable, '-c',
                         PRINT_SDKROOT)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(os.path.normcase(result.stdout.strip()),
                         os.path.normcase(os.path.realpath(self.real)))

    def test_abs_marker_is_resolved_too(self):
        result = run_rsp(self.tmp, 'abs@xcode_links', sys.executable, '-c',
                         PRINT_SDKROOT)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(os.path.normcase(result.stdout.strip()),
                         os.path.normcase(os.path.realpath(self.real)))

    @unittest.skipUnless(sys.platform == 'darwin', 'xcrun is macOS only')
    def test_xcrun_finds_git_under_the_build_sdkroot(self):
        # The build links Xcode's SDK; a bare --show-sdk-path may name the
        # Command Line Tools one, under which xcrun finds git either way.
        sdk = subprocess.run(['xcrun', '--sdk', 'macosx', '--show-sdk-path'],
                             capture_output=True, text=True,
                             check=True).stdout.strip()
        os.rmdir(self.real)
        os.remove(self.link)
        os.symlink(sdk, self.link)
        result = run_rsp(self.tmp, self.link, '/usr/bin/xcrun', '--find', 'git')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
