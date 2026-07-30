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
    """Минимальный app-Info.plist с двумя иконками-ключами."""
    with open(path, 'wb') as f:
        plistlib.dump({
            'CFBundleIdentifier': 'com.growser.Browser',
            'CFBundleShortVersionString': '1.0.0',
            # app.icns = наша «G»
            'CFBundleIconFile': 'app.icns',
            # AppIcon из Assets.car = лев Brave; должен быть срезан.
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
                             'CFBundleIconName должен быть срезан — иначе '
                             'Assets.car (лев Brave) перекрывает app.icns')

    def test_cf_bundle_icon_file_preserved(self):
        # app.icns (наша «G») должен остаться авторитетной иконкой.
        with tempfile.TemporaryDirectory() as tmp:
            src = os.path.join(tmp, 'Info.plist')
            out = os.path.join(tmp, 'Out.plist')
            _make_plist(src)
            self._run(src, out)
            with open(out, 'rb') as f:
                plist = plistlib.load(f)
            self.assertEqual(plist.get('CFBundleIconFile'), 'app.icns')

    def test_idempotent_when_key_absent(self):
        # Если ключа уже нет (напр. upstream убрал) — скрипт не должен падать.
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


if __name__ == '__main__':
    unittest.main()