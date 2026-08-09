# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


# growser (#62): the product name an administrator reads in the policy
# descriptions. This is a SECOND configuration, separate from
# template_writers/writer_configuration.py, and it is the one that decides the
# prose: policy_json.py substitutes $1 with app_name, $3 with frame_name. The
# writer configuration was rebranded first and the templates still said "Brave"
# 580 times, because that layer only names the tree, not the sentences.
#
# os_name stays Google's: $2 appears in ChromeOS policy descriptions, which are
# about Google's operating system rather than about this browser.
@override_utils.override_method(PolicyJson)
def SetDefines(self, _orig_method, _defines):
    self._config = {
        'build': 'growser',
        'app_name': 'Growser',
        'frame_name': 'Growser Frame',
        'os_name': 'Google Chrome OS'
    }
