# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

assert ('CHROMIUM_POLICY_KEY' in globals())

# This override controls the constant written out to:
# `//out/<build_type_here>/gen/components/policy/policy_constants.cc`
# which is then used for the `policy_templates.zip`
#
# growser (#58): our own key. Note there are TWO places that decide this and
# they are easy to mistake for one - writer_configuration.py feeds the ADMX and
# documentation templates, while this one feeds kRegistryChromePolicyKey, which
# is what the browser actually reads. Changing only the first leaves the binary
# on the old key.
CHROMIUM_POLICY_KEY = 'SOFTWARE\\\\Policies\\\\Growser'
