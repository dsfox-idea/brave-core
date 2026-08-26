# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils

@override_utils.override_function(globals())
def GetConfigurationForBuild(original_function, defines):
    base = original_function(defines)
    merged = _merge_dicts(_BRAVE_VALUES, base)

    # Remove Google.Policies namespace. Microsoft.Policies.Windows remains, as it is hardcoded in admx_writer.py.
    merged.pop('admx_using_namespaces', None)

    return merged


# growser (#62): the registry keys became ours in #58, but everything an
# administrator actually reads was still Brave - the policy tree they import
# said "Brave" while writing to Growser's keys. These are the display side of
# the same configuration.
#
# The category path follows unbranded Chromium's single level rather than
# Brave's two ('Cat_Brave' -> 'brave'): the outer level is a company grouping,
# and we have no company component anywhere else either (kCompanyPathName is
# empty, see chromium_install_modes.h).
_BRAVE_VALUES = {
    'build': 'growser',
    'app_name': 'Growser',
    'doc_url': 'https://github.com/dsfox-idea/growser',
    'frame_name': 'Growser Frame',
    'webview_name': 'Growser WebView',
    'win_config': {
        'win': {
            # growser (#58): our own policy key. This feeds the GENERATED
            # components/policy/core/common/policy_constants.cc, which is why
            # grepping the source tree for the old value finds nothing. Leaving
            # it as Brave's meant an administrator's Brave policies applied to
            # Growser on the same machine.
            'reg_mandatory_key_name': 'Software\\Policies\\Growser',
            'reg_recommended_key_name': 'Software\\Policies\\Growser\\Recommended',
            'mandatory_category_path': ['growser'],
            'recommended_category_path': ['growser_recommended'],
            'category_path_strings': {
                'growser': 'Growser',
                'growser_recommended': 'Growser - {doc_recommended}'
            },
            'namespace': 'Growser.Policies.Growser',
        },
    },
    'admx_prefix': 'growser',
    'linux_policy_path': '/etc/growser/policies/',
    'bundle_id': 'com.growser.ios.core',
}

def _merge_dicts(src, dst):
    result = dict(dst)
    for k, v in src.items():
        result[k] = _merge_dicts(v, dst.get(k, {})) if isinstance(v,
                                                                  dict) else v
    return result
