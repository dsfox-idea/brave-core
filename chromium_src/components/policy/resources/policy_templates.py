# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import glob
import os

import brave_chromium_utils
import override_utils

# Brave's policy templates directory mirrors the upstream layout, so the
# upstream loader can be pointed at it as-is:
# `//brave/components/policy/resources/templates/policy_definitions/BraveSoftware` # pylint: disable=line-too-long
BRAVE_TEMPLATES_PATH = os.path.relpath(
    brave_chromium_utils.wspath(
        '//brave/components/policy/resources/templates'))

BRAVE_GROUP_NAME = 'BraveSoftware'


@override_utils.override_function(globals())
def _GetPoliciesAndGroups(orig_func):
    # Load Brave's policy definitions in addition to the upstream ones by
    # running the upstream loader a second time with `TEMPLATES_PATH` pointing
    # at Brave's templates directory. Brave policies must never be copied into
    # the Chromium tree: files created by a build action are unknown to the
    # build graph, which makes the generated depfile reference dependencies the
    # build system doesn't know exist.
    # The override runs in the scope of the overridden script, so its globals
    # are shared with it.
    # pylint: disable=global-statement,global-variable-undefined
    # pylint: disable=used-before-assignment
    global TEMPLATES_PATH

    result = orig_func()
    # Ignore leftovers of the old copy-into-Chromium-tree approach.
    result.pop(BRAVE_GROUP_NAME, None)

    chromium_templates_path = TEMPLATES_PATH
    TEMPLATES_PATH = BRAVE_TEMPLATES_PATH
    try:
        brave_result = orig_func()
    finally:
        TEMPLATES_PATH = chromium_templates_path

    assert BRAVE_GROUP_NAME in brave_result, (
        f"'{BRAVE_GROUP_NAME}' policies not found in {BRAVE_TEMPLATES_PATH}")
    result.update(brave_result)

    return result


@override_utils.override_function(globals())
def _LoadPolicies(orig_func):
    policies = orig_func()

    # `policies` has two notable keys:

    # 1) "policy_definitions"
    # there will be one "group" for every folder found under
    # `//components/policy/resources/templates/policy_definitions`
    # Chromium considers the folder name the group name for the policy.
    # growser (#62) uses the group name "Growser"; upstream Brave used
    # "BraveSoftware". The child element for the group is the policy itself
    # (those are the yaml files in the folder). Note that renaming the group
    # orphans the old directory in Chromium's tree - the pruning below only
    # covers groups the list still names.
    #
    # Our entries are copied into place by sync_policy_files() below.
    # We copy the files from:
    # `//brave/components/policy/resources/templates/policy_definitions/Growser` # pylint: disable=line-too-long
    # to:
    # `//components/policy/resources/templates/policy_definitions`
    policy_definition_yaml = policies['policy_definitions']
    assert policy_definition_yaml, "'policy_definitions' is None (did upstream change?)"  # pylint: disable=line-too-long

    brave_policies = []
    brave_policy_section = policy_definition_yaml['Growser']
    assert brave_policy_section, "'policy_definitions > Growser' entries not found (failed to copy?)"  # pylint: disable=line-too-long

    brave_policy_items = brave_policy_section['policies']
    for key, _ in brave_policy_items.items():
        brave_policies.append(key)

    # 2) "policies"
    # This has the contents of:
    # `//components/policy/resources/templates/policies.yaml`
    # This is where we need to inject the Brave specific names. The policies
    # themselves are already defined (under `policy_definitions`), we just need
    # to add a mapping for ID (integer; unique) and name (matches name under
    # `policy_definitions`).
    policy_yaml = policies['policies']
    assert policy_yaml, "'policies' is None (did upstream change?)"

    policy_section = policy_yaml['policies']
    assert policy_section, "'policies > policies' is None (did upstream change?)"  # pylint: disable=line-too-long

    offset = max(map(int, policy_section), default=0)
    for i, entry in enumerate(brave_policies):
        policy_section[offset + i + 1] = entry

    return policies


def sync_policy_files():
    # Chromium stores all group policy definitions under
    # `//components/policy/resources/templates/policy_definitions/`
    #
    # The name of the file (minus the extension; ex: TorDisable.yaml would be
    # TorDisable) corresponds to an auto-generated entry in:
    # `//out/<build_type_here>/gen/chrome/app/policy/policy_templates.json
    #
    # That auto-generated value (ex: `policy::key::kTorDisabled`) is referenced
    # when we map to a preference in our policy map:
    # `//brave/browser/policy/brave_simple_policy_map.h`
    #
    # When the code below is ran this will copy the group policy files from
    # Brave's policy definitions to Chromium's policy definitions.
    with open("gen/brave_policies_sync_config.json", "r") as f:
        brave_policies = json.load(f)

    copy_from = brave_policies["copy_from"]
    copy_to = brave_policies["copy_to"]
    for policy in brave_policies["policies"]:
        copy_only_if_modified(f'{copy_from}/{policy}', f'{copy_to}/{policy}')

    # growser (#62): drop copies whose source is gone.
    #
    # copy_only_if_modified only ever writes. A policy removed from the list
    # stayed behind in Chromium's tree and kept being generated into
    # policy_templates.json and policy_constants.h - so the removal built
    # cleanly, changed nothing, and gave no hint why. Found by checking the
    # generated header rather than trusting a green build.
    #
    # Pruning is limited to the group directories the list itself names, so
    # Chromium's own policy definitions are never touched.
    wanted = set(brave_policies["policies"])
    groups = {os.path.dirname(p) for p in wanted if os.path.dirname(p)}
    for group in groups:
        dest_dir = os.path.join(copy_to, group)
        if not os.path.isdir(dest_dir):
            continue
        for name in os.listdir(dest_dir):
            if f'{group}/{name}' not in wanted:
                os.remove(os.path.join(dest_dir, name))

    # growser(#62): drop whole Brave group directories the list no longer names.
    # The per-file loop above only walks the groups the current list names, so
    # renaming our group (BraveSoftware -> Growser) orphaned the old directory:
    # its stale Brave*.yaml kept being loaded as a policy group whose names are
    # absent from the id map (only the Growser group is injected into
    # policies.yaml by _LoadPolicies), and _BuildPolicyTemplate raised
    # KeyError: 'BraveAIChatEnabled'. Only names Brave has ever owned are
    # ever removed, so Chromium's own groups are still never touched.
    brave_group_names = {"Growser", "BraveSoftware"}
    for brave_group in brave_group_names - groups:
        orphan_dir = os.path.join(copy_to, brave_group)
        if os.path.isdir(orphan_dir):
            shutil.rmtree(orphan_dir)


def copy_only_if_modified(src, dst):
    """Copy file if it doesn't exist or if its hash is different."""

    def file_hash(file_path):
        with open(file_path, "rb") as f:
            return hashlib.file_digest(f, "sha256").digest()

    if not os.path.exists(dst) or file_hash(src) != file_hash(dst):
        dest_dir = os.path.dirname(dst)
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)
        shutil.copy2(src, dst)


@override_utils.override_function(globals())
def _WriteDepFile(orig_func, dep_file, target, source_files):
    # Upstream only globs its own templates directory, so list Brave's policy
    # files as dependencies too. Leftover copies in the Chromium tree (see
    # `_GetPoliciesAndGroups`) are filtered out, they are not used anymore.
    stale_path = f'/{POLICY_DEFINITIONS_KEY}/{BRAVE_GROUP_NAME}/'
    source_files = [f for f in source_files if stale_path not in f]
    brave_files = sorted(
        f.replace('\\', '/') for f in glob.glob(
            f'{BRAVE_TEMPLATES_PATH}/**/*.yaml', recursive=True))

    orig_func(dep_file, target, source_files + brave_files)
