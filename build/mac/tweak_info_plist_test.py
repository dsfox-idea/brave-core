#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. A copy of the MPL was not distributed with this file,
# You can obtain one at https://www.mozilla.org/MPL/2.0/.

import os
import plistlib
import sys
import tempfile
import unittest

import tweak_info_plist


def _make_plist(path):
    """A minimal app-Info.plist carrying both icon keys."""
    with open(path, 'wb') as f:
        plistlib.dump({
            'CFBundleIdentifier': 'com.growser.Browser',
            'CFBundleShortVersionString': '1.0.0',
            # app.icns = our "G"
            'CFBundleIconFile': 'app.icns',
            # AppIcon from Assets.car = the Brave lion; must be stripped.
            'CFBundleIconName': 'AppIcon',
        }, f)


class TestRemoveBundleIconName(unittest.TestCase):

    def _run(self, plist_path, output_path):
        orig = sys.argv
        sys.argv = [
            'tweak_info_plist.py',
            '--plist', plist_path,
            '--output', output_path,
            '--brave_version', '0.0.0',
            '--format', 'xml1',
        ]
        try:
            self.assertEqual(tweak_info_plist.Main(), 0)
        finally:
            sys.argv = orig

    def test_cf_bundle_icon_name_removed(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 'Info.plist')
            out = os.path.join(tmp, 'Out.plist')
            _make_plist(src)
            self._run(src, out)
            with open(out, 'rb') as f:
                plist = plistlib.load(f)
            self.assertNotIn('CFBundleIconName', plist,
                             'CFBundleIconName must be stripped - otherwise '
                             'Assets.car (the Brave lion) wins over app.icns')

    def test_cf_bundle_icon_file_preserved(self):
        # app.icns (our "G") must stay the authoritative icon.
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 'Info.plist')
            out = os.path.join(tmp, 'Out.plist')
            _make_plist(src)
            self._run(src, out)
            with open(out, 'rb') as f:
                plist = plistlib.load(f)
            self.assertEqual(plist.get('CFBundleIconFile'), 'app.icns')

    def test_idempotent_when_key_absent(self):
        # If the key is already gone (upstream dropped it) this must not fail.
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 'Info.plist')
            out = os.path.join(tmp, 'Out.plist')
            with open(src, 'wb') as f:
                plistlib.dump({
                    'CFBundleShortVersionString': '1.0.0',
                    'CFBundleIconFile': 'app.icns',
                }, f)
            self._run(src, out)
            with open(out, 'rb') as f:
                plist = plistlib.load(f)
            self.assertNotIn('CFBundleIconName', plist)


def _newer(candidate, published):
    """Component-wise integer comparison, the way Sparkle compares pure-integer
    dotted version strings (and the way Omaha compares them on Windows). '10'
    sorts after '9', which a string comparison gets wrong."""
    return [int(x) for x in candidate.split('.')] > \
           [int(x) for x in published.split('.')]


class TestOverrideVersionKey(unittest.TestCase):
    """#85: CFBundleVersion must be the full product version (151.26.811.0), not
    the old "minor + 100 * major" formula, which broke at the year boundary."""

    def _run(self, plist_path, output_path, brave_version):
        orig = sys.argv
        sys.argv = [
            'tweak_info_plist.py',
            '--plist', plist_path,
            '--output', output_path,
            '--brave_version', brave_version,
            '--format', 'xml1',
        ]
        try:
            self.assertEqual(tweak_info_plist.Main(), 0)
        finally:
            sys.argv = orig

    def _bundle_version(self, brave_version):
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 'Info.plist')
            out = os.path.join(tmp, 'Out.plist')
            with open(src, 'wb') as f:
                plistlib.dump({
                    'CFBundleIdentifier': 'com.growser.Browser',
                    # Mirrors the real pipeline: Chromium's apple tweak sets this
                    # from src/chrome/VERSION as @MAJOR@.@MINOR@.@BUILD@.@PATCH@.
                    'CFBundleShortVersionString': brave_version,
                }, f)
            self._run(src, out, brave_version)
            with open(out, 'rb') as f:
                return plistlib.load(f)

    def test_full_version_set_as_bundle_version(self):
        plist = self._bundle_version('151.26.811.0')
        self.assertEqual(plist.get('CFBundleVersion'), '151.26.811.0')
        # CFBundleShortVersionString is untouched by _OverrideVersionKey.
        self.assertEqual(plist.get('CFBundleShortVersionString'), '151.26.811.0')

    def test_no_formula_mangling(self):
        # The old formula turned 26.1231 -> 3831 and 27.101 -> 2801. The full
        # version must survive unchanged.
        for v in ('151.26.1231.0', '151.27.101.0', '151.26.811.10'):
            self.assertEqual(self._bundle_version(v).get('CFBundleVersion'), v)

    def test_year_rollover_orders_correctly(self):
        # The bug: December 2026 (151.26.1231.0) must sort BELOW January 2027
        # (151.27.101.0). The old formula reversed this (3831.0 > 2801.0).
        self.assertTrue(_newer('151.27.101.0', '151.26.1231.0'))
        self.assertFalse(_newer('151.26.1231.0', '151.27.101.0'))

    def test_same_day_serial_orders_correctly(self):
        # A second build the same day bumps the serial; '10' > '9' as integers.
        self.assertTrue(_newer('151.26.811.1', '151.26.811.0'))
        self.assertTrue(_newer('151.26.811.10', '151.26.811.9'))

    def test_none_brave_version_leaves_key_alone(self):
        # Helpers without --brave_version must not crash and must not write a
        # None CFBundleVersion. (The real pipeline always passes a version, but
        # the guard keeps the script robust.)
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 'Info.plist')
            out = os.path.join(tmp, 'Out.plist')
            with open(src, 'wb') as f:
                plistlib.dump({'CFBundleShortVersionString': '1.0.0'}, f)
            orig = sys.argv
            sys.argv = ['tweak_info_plist.py', '--plist', src,
                        '--output', out, '--format', 'xml1']
            try:
                self.assertEqual(tweak_info_plist.Main(), 0)
            finally:
                sys.argv = orig
            with open(out, 'rb') as f:
                plist = plistlib.load(f)
            self.assertNotIn('CFBundleVersion', plist)


if __name__ == '__main__':
    unittest.main()