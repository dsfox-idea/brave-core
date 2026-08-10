# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os.path
import override_utils


@override_utils.override_function(globals())
async def _customize_and_sign_chrome(original_function, paths, dist_config,
                                     *args):
    base_config = dist_config.base_config
    # This also serves as a safeguard that .is_in_sign_chrome exists:
    value_before = base_config.is_in_sign_chrome
    base_config.is_in_sign_chrome = True

    # growser: suppress the com.apple.security.get-task-allow entitlement for
    # notarized builds. A non-official Release always runs sign_chrome with
    # --development (build/mac/BUILD.gn: `if (!is_official_build)
    # args += ["--development"]`), and DevelopmentCodeSignConfig sets
    # inject_get_task_allow_entitlement=True (driver.py), which adds the debug
    # entitlement to the main executable and the Renderer/GPU helpers. Apple
    # notarization rejects that for distribution
    # (resolving_common_notarization_issues#3087731). Patch the property on the
    # development config class to return False when the build is being
    # notarized; a non-notarized development build keeps the entitlement for
    # local debugger attaching. This is done here rather than in driver.py
    # because driver.py has no inline_chromium_src_override hook, while this
    # pipeline.py override is already wired in (upstream pipeline.py:829).
    # base_config.invoker.args.notarize is the authoritative notarize level
    # (config._notarize is unreliable on a DistributionCodeSignConfig - see
    # internal_config.py run_spctl_assess).
    notarize = getattr(getattr(base_config.invoker, 'args', None),
                       'notarize', None)
    if notarize is not None and notarize.should_notarize():
        type(base_config).inject_get_task_allow_entitlement = property(
            lambda self: False)

    try:
        return await original_function(paths, dist_config, *args)
    finally:
        base_config.is_in_sign_chrome = value_before


@override_utils.override_function(globals())
def _create_pkgbuild_scripts(original_function, paths, dist_config):
    orig_packaging_dir = paths.packaging_dir

    def new_packaging_dir(*args, **kwargs):
        orig = orig_packaging_dir(*args, **kwargs)
        return os.path.join(orig, 'brave')

    paths.packaging_dir = new_packaging_dir
    try:
        return original_function(paths, dist_config)
    finally:
        paths.packaging_dir = orig_packaging_dir
