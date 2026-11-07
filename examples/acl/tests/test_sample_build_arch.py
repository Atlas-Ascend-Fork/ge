#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SCRIPTS = (
    "examples/acl/1_sample_resnet50_imagenet_classification/scripts/build.sh",
    "examples/acl/2_sample_resnet50_imagenet_classification_dynamic_batch/scripts/build.sh",
    "examples/acl/3_sample_qwen_llm/scripts/build.sh",
    "examples/acl/5_sample_yolov13/scripts/build.sh",
    "examples/gesession/sample_dynamic_batch/scripts/build.sh",
)


@unittest.skipUnless(os.name == "posix", "requires a POSIX shell")
class SampleBuildArchitectureTests(unittest.TestCase):
    def run_sample(self, script, lscpu_text, user_input):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            script_path = root / "sample" / "scripts" / "build.sh"
            script_path.parent.mkdir(parents=True)
            script_path.write_text(
                (ROOT / script).read_text(encoding="utf-8"), encoding="utf-8"
            )
            fake_bin = root / "bin"
            fake_bin.mkdir()
            for name in ("dirname", "grep", "head", "awk", "tr", "mkdir", "rm"):
                executable = shutil.which(name)
                self.assertIsNotNone(executable, name)
                (fake_bin / name).symlink_to(executable)
            for name in ("cmake", "make"):
                executable = fake_bin / name
                executable.write_text(
                    '#!/bin/sh\nprintf "%s %s\\n" "' + name + '" "$*" >> "$FAKE_LOG"\n'
                )
                executable.chmod(0o755)
            arch = fake_bin / "arch"
            arch.write_text('#!/bin/sh\nprintf "x86_64\\n"\n')
            arch.chmod(0o755)
            if lscpu_text is not None:
                executable = fake_bin / "lscpu"
                if lscpu_text == "__ERROR__":
                    executable.write_text("#!/bin/sh\nexit 1\n")
                elif lscpu_text == "__ERROR_WITH_FIELD__":
                    executable.write_text(
                        "#!/bin/sh\nprintf 'Architecture: x86_64\\n'\nexit 1\n"
                    )
                else:
                    executable.write_text(
                        "#!/bin/sh\nprintf '%s\\n' " + repr(lscpu_text) + "\n"
                    )
                executable.chmod(0o755)
            log = root / "build.log"
            env = os.environ.copy()
            env.update(PATH=str(fake_bin), FAKE_LOG=str(log))
            result = subprocess.run(
                ["/bin/bash", str(script_path)],
                input=user_input,
                text=True,
                capture_output=True,
                env=env,
                timeout=10,
                check=False,
            )
            calls = log.read_text(encoding="utf-8").splitlines() if log.exists() else []
            return result, calls

    def test_unknown_missing_field_or_missing_lscpu_allows_manual_input(self):
        for script in SCRIPTS:
            for output in (
                "Architecture: mystery",
                "Other: mystery",
                "__ERROR__",
                None,
            ):
                with self.subTest(script=script, lscpu=output):
                    result, calls = self.run_sample(script, output, "x86\n")
                    self.assertEqual(
                        result.returncode, 0, result.stdout + result.stderr
                    )
                    self.assertEqual(len(calls), 2)
                    self.assertIn("-DCMAKE_CXX_COMPILER=g++", calls[0])
                    self.assertEqual(calls[1], "make ")

    def test_invalid_input_is_limited_to_three_attempts(self):
        for script in SCRIPTS:
            with self.subTest(script=script):
                result, calls = self.run_sample(
                    script, "Architecture: mystery", "bad\nbad\nbad\n"
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("three times", result.stdout)
                self.assertEqual(result.stdout.count("[WARNING]"), 3)
                self.assertEqual(calls, [])

    def test_detected_architecture_does_not_require_input(self):
        for script in SCRIPTS:
            for architecture, compiler in (
                ("x86_64", "g++"),
                ("aarch64", "aarch64-linux-gnu-g++"),
            ):
                with self.subTest(script=script, architecture=architecture):
                    result, calls = self.run_sample(
                        script, "Architecture: " + architecture, ""
                    )
                    self.assertEqual(
                        result.returncode, 0, result.stdout + result.stderr
                    )
                    self.assertEqual(len(calls), 2)
                    self.assertIn("-DCMAKE_CXX_COMPILER=" + compiler, calls[0])
                    self.assertEqual(calls[1], "make ")

    def test_third_manual_input_may_be_valid(self):
        for script in SCRIPTS:
            with self.subTest(script=script):
                result, calls = self.run_sample(
                    script, "Architecture: mystery", "bad\nbad\nx86\n"
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(len(calls), 2)

    def test_qwen_failed_lscpu_output_is_not_used(self):
        script = "examples/acl/3_sample_qwen_llm/scripts/build.sh"
        result, calls = self.run_sample(script, "__ERROR_WITH_FIELD__", "arm\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(calls), 2)
        self.assertIn("-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++", calls[0])


if __name__ == "__main__":
    unittest.main()
