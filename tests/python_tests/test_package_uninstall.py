# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------------------------------------

"""Regression tests for wheel uninstall isolation from the default environment."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[2]
SCRIPT = REPO / "scripts/package/ge-compiler/scripts/ge-compiler_custom_uninstall.sh"


class PackageUninstallTest(unittest.TestCase):
    def run_uninstall(self, local_kind=None, missing=False, fallback=False, fail=False):
        with tempfile.TemporaryDirectory(prefix="wheel-uninstall-") as directory:
            work = Path(directory)
            local_root = work / "cann root" / "python" / "site-packages"
            if local_kind:
                name = (
                    "first_pkg"
                    if local_kind == "package"
                    else "first_pkg-1.0.dist-info"
                )
                (local_root / name).mkdir(parents=True)
            source = SCRIPT.read_text()
            function = source.split("whl_uninstall_package()", 1)[1].split(
                "\nremove_rl_soft_link()", 1
            )[0]
            shell = r"""
log() { echo "$*"; }
record_pip() {
    printf '%s|%s\n' "$*" "${PYTHONPATH:-}" >> "$CALL_LOG"
    case "$1" in
        show) return "$SHOW_STATUS" ;;
        uninstall) return "$UNINSTALL_STATUS" ;;
    esac
}
python3() {
    if [ "$*" = "-m pip --version" ]; then return "$PYTHON_PIP_STATUS"; fi
    shift 2
    record_pip "$@"
}
pip3() { record_pip "$@"; }
"""
            shell += "\nwhl_uninstall_package()" + function
            shell += r"""
whl_uninstall_package first-pkg "$WHL_INSTALL_DIR_PATH"
whl_uninstall_package second "$WHL_INSTALL_DIR_PATH"
printf 'after|%s\n' "$PYTHONPATH" >> "$CALL_LOG"
"""
            # A namespace distribution need not have a same-name import directory.
            if local_kind == "package":
                (local_root / "first_pkg").rename(local_root / "first-pkg")
            env = dict(
                os.environ,
                CALL_LOG=str(work / "calls"),
                WHL_INSTALL_DIR_PATH=str(local_root),
                PYTHONPATH="original-path",
                SHOW_STATUS="1" if missing else "0",
                PYTHON_PIP_STATUS="1" if fallback else "0",
                UNINSTALL_STATUS="7" if fail else "0",
            )
            result = subprocess.run(
                ["bash", "-c", shell],
                env=env,
                capture_output=True,
                text=True,
                timeout=10,
            )
            lines = (
                (work / "calls").read_text().splitlines()
                if (work / "calls").exists()
                else []
            )
            return result, lines, str(local_root)

    def test_default_environment_package_is_preserved(self):
        result, lines, _ = self.run_uninstall()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            lines,
            ["after|original-path"],
        )

    def test_local_package_and_metadata_only(self):
        for kind in ("package", "metadata"):
            with self.subTest(kind=kind):
                result, lines, root = self.run_uninstall(local_kind=kind)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(
                    lines,
                    [
                        "uninstall -y first-pkg|" + root,
                        "after|original-path",
                    ],
                )

    def test_missing_package_is_skipped(self):
        result, lines, _ = self.run_uninstall(missing=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(lines, ["after|original-path"])

    def test_pip3_fallback(self):
        result, lines, root = self.run_uninstall(local_kind="metadata", fallback=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            lines, ["uninstall -y first-pkg|" + root, "after|original-path"]
        )

    def test_uninstall_failure_is_not_swallowed(self):
        for kind in ("package", "metadata"):
            with self.subTest(kind=kind):
                result, lines, _ = self.run_uninstall(local_kind=kind, fail=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(any("second" in line for line in lines))


if __name__ == "__main__":
    unittest.main()
