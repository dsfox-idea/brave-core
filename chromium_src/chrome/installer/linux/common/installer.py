# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


# This override:
# - Maps dev/unstable/alpha -> dev
# - Adds nightly as a supported channel
# - Returns the appropriate releaste notes link for all channels
@override_utils.override_function(globals())
def normalize_channel(original_function, channel):
    if channel == "stable":
        return "stable", "https://brave.com/latest/"
    if channel in ["beta", "testing"]:
        return "beta", "https://brave.com/latest/"
    if channel in ["dev", "unstable", "alpha"]:
        return "dev", "https://brave.com/latest/"
    if channel == "nightly":
        return "nightly", "https://brave.com/latest/"
    return original_function(channel)


# This override adds or modifies the following data values:
# - package_and_channel: required by our templates (drops channel name for stable)
# - version/versionfull: drops MAJOR from the version string
@override_utils.override_function(InstallerConfig)
def _load_branding_and_version(original_method, output_dir, branding,
                               channel) -> dict[str, typing.Any]:
    data = original_method(output_dir, branding, channel)

    data["package_and_channel"] = data["info_vars"]["PACKAGE"]

    version = (f"{data['version_vars']['MINOR']}."
               f"{data['version_vars']['BUILD']}."
               f"{data['version_vars']['PATCH']}")
    data["version"] = version
    data["versionfull"] = version
    return data

# Growser-207: the repository the package points a machine at.
#
# Chromium hardcodes Google here, and brave-core overrides it nowhere, so a
# package built from this tree tells apt and dnf to fetch OUR browser from
# GOOGLE - and, through key.include, to trust Google to sign it. An apt source
# is not a download, it is standing permission, so this is the half of the
# packaging that matters most.
#
# The deb side reads a REPOCONFIG environment variable before falling back to
# its default, but that is not enough on its own: the REGEX that finds an
# existing entry to update or remove is built from the hardcoded Google base
# either way, so a machine configured by an earlier install would never be
# recognised. Both halves have to move together.
GROWSER_APT = "growser.org/apt/ stable main"
GROWSER_RPM = "https://growser.org/rpm/stable"


# override_function, not override_method: both of these are staticmethods
# called on the class, so nothing binds a self - override_method would
# hand the first argument in as one and the two would swap places.
@override_utils.override_function(InstallerConfig)
def _compute_deb_repoconfig(original_function, arch: str,
                            package: str) -> tuple[str, str]:
    repoconfig = os.environ.get("REPOCONFIG",
                                f"deb [arch={arch}] https://{GROWSER_APT}")
    # The backslashes are doubled on purpose and it matters: in an f-string
    # "\b" is a BACKSPACE byte, not a word boundary, and the first version of
    # this line had exactly that - invisible in a diff and in every count.
    regex = (f"deb (\\[arch=[^]]*\\b{arch}\\b[^]]*\\]"
             f"[[:space:]]*) https?://{GROWSER_APT}")
    return repoconfig, regex


@override_utils.override_function(InstallerConfig)
def _compute_rpm_repoconfig(original_function, package: str) -> tuple[str, str]:
    return GROWSER_RPM, ""


# This override stages Brave resources
@override_utils.override_method(InstallerConfig)
def get_resource_artifacts(self, original_method) -> list[Artifact]:
    artifacts = original_method(self)
    artifacts.append(
        Artifact(
            "installer/common/LICENSE",
            "LICENSE",
            ArtifactType.RESOURCE,
            StandardPermissions.REGULAR,
        ))
    artifacts.append(
        Artifact(
            "brave_resources.pak",
            "brave_resources.pak",
            ArtifactType.RESOURCE,
            StandardPermissions.REGULAR,
        ))
    artifacts.append(
        Artifact(
            "brave_100_percent.pak",
            "brave_100_percent.pak",
            ArtifactType.RESOURCE,
            StandardPermissions.REGULAR,
        ))
    artifacts.append(
        Artifact(
            "brave_200_percent.pak",
            "brave_200_percent.pak",
            ArtifactType.RESOURCE,
            StandardPermissions.REGULAR,
        ))
    # localization files for Brave extension
    locales_dir = self.output_dir / pathlib.Path(
        "resources/brave_extension/_locales")
    for locale in locales_dir.iterdir():
        locale_file = locale / "messages.json"
        artifacts.append(
            Artifact(
                locale_file,
                locale_file.relative_to(self.output_dir),
                ArtifactType.RESOURCE,
                StandardPermissions.REGULAR,
            ))
    return artifacts
