#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.

"""Native wheel ABI and installed bridge dependency regression tests."""

import argparse
import importlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[6]
BUILD_SCRIPTS = ROOT / "api/python/ge/python_pass_native_build"
sys.path.insert(0, str(BUILD_SCRIPTS))
try:
    wheel_builder = importlib.import_module("build_python_native_wheel")
    matrix_builder = importlib.import_module("build_python_native_matrix")
    resource_builder = importlib.import_module("gen_fallback_resources")
finally:
    sys.path.pop(0)


class NativeWheelAbiTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = {
            "component": "pass",
            "abi": {"key": "bridge_abi", "value": 2},
            "native": {"artifact": "_ge_pass_native.so"},
            "bridge": {"artifact": "libge_python_pass_bridge.so"},
            "wheel": {
                "package_name": "ge-py-pass-bridge",
                "dist_name": "ge_py_pass_bridge",
                "package": "ge.passes",
                "artifact_root": "python_pass_artifacts",
            },
        }
        self.manifest = {
            "python_tag": "cp37",
            "platform": "linux-x86_64",
            "bridge_abi": 2,
            "artifacts": {
                "native": "_ge_pass_native.so",
                "bridge": "libge_python_pass_bridge.so",
            },
        }
        self.args = argparse.Namespace(
            component_config=self.config,
            artifact_dir=str(self.root / "artifact"),
            output_dir=str(self.root / "dist"),
            python_tag="cp37",
            abi_tag="cp37m",
            platform_tag="linux-x86_64",
            wheel_platform="linux_x86_64",
            version="0.0.1",
            current_tag="cp37",
            current_wheel=self.root / "current.whl",
            dist_dir=self.root / "matrix",
        )
        artifact = Path(self.args.artifact_dir)
        artifact.mkdir()
        for name in self.manifest["artifacts"].values():
            (artifact / name).write_bytes(b"test artifact")

    def write_manifest(self):
        (Path(self.args.artifact_dir) / "manifest.json").write_text(
            json.dumps(self.manifest)
        )

    def write_wheel(self):
        prefix = "ge/passes/python_pass_artifacts/cp37-linux-x86_64/"
        with zipfile.ZipFile(self.args.current_wheel, "w") as archive:
            archive.writestr(prefix + "manifest.json", json.dumps(self.manifest))
            for name in self.manifest["artifacts"].values():
                archive.writestr(prefix + name, b"test artifact")

    def test_packaging_rejects_old_abi_before_build(self):
        self.manifest["bridge_abi"] = 1
        self.write_manifest()
        with mock.patch.object(wheel_builder, "_run_bdist_wheel") as build:
            with self.assertRaisesRegex(RuntimeError, "bridge_abi mismatch"):
                wheel_builder.build_wheel(self.args)
            build.assert_not_called()
        self.assertFalse(Path(self.args.output_dir).exists())

    def test_packaging_accepts_cp37m_with_bridge_abi_two(self):
        self.write_manifest()

        def build(project, output, tag, abi, platform):
            self.assertEqual((tag, abi), ("cp37", "cp37m"))
            (output / "ge_py_pass_bridge-0.0.1-cp37-cp37m-linux_x86_64.whl").touch()

        with mock.patch.object(wheel_builder, "_run_bdist_wheel", side_effect=build):
            self.assertTrue(wheel_builder.build_wheel(self.args).is_file())

    def test_reuse_rejects_old_abi_before_copy(self):
        self.manifest["bridge_abi"] = 1
        self.write_wheel()
        with self.assertRaisesRegex(RuntimeError, "bridge_abi mismatch"):
            matrix_builder._copy_current_wheel(self.args, "cp37")
        self.assertFalse(self.args.dist_dir.exists())

    def test_reuse_accepts_current_abi(self):
        self.write_wheel()
        copied = matrix_builder._copy_current_wheel(self.args, "cp37")
        self.assertEqual(copied.read_bytes(), self.args.current_wheel.read_bytes())

    def test_packaging_rejects_missing_bridge(self):
        self.write_manifest()
        (Path(self.args.artifact_dir) / "libge_python_pass_bridge.so").unlink()
        with self.assertRaisesRegex(RuntimeError, "Missing native artifact"):
            wheel_builder.build_wheel(self.args)

    def test_runtime_component_keeps_its_own_abi(self):
        config = {
            "abi": {"key": "native_abi", "value": 1},
            "native": {"artifact": "_ge_runtime_native.so"},
        }
        manifest = {
            "python_tag": "cp37",
            "platform": "linux-x86_64",
            "native_abi": 1,
            "artifacts": {"native": "_ge_runtime_native.so"},
        }
        wheel_builder.validate_artifact_manifest(
            manifest, config, "cp37", "linux-x86_64"
        )

    def test_reuse_rejects_incomplete_wheel(self):
        prefix = "ge/passes/python_pass_artifacts/cp37-linux-x86_64/"
        with zipfile.ZipFile(self.args.current_wheel, "w") as archive:
            archive.writestr(prefix + "manifest.json", json.dumps(self.manifest))
            archive.writestr(prefix + "_ge_pass_native.so", b"test artifact")
        with self.assertRaises(KeyError):
            matrix_builder._copy_current_wheel(self.args, "cp37")
        self.assertFalse(self.args.dist_dir.exists())

    def test_manifest_checks_python_and_platform(self):
        for key, value in (("python_tag", "cp311"), ("platform", "linux-aarch64")):
            with self.subTest(key=key):
                manifest = dict(self.manifest, **{key: value})
                with self.assertRaisesRegex(RuntimeError, key + " mismatch"):
                    wheel_builder.validate_artifact_manifest(
                        manifest, self.config, "cp37", "linux-x86_64"
                    )


@unittest.skipUnless(shutil.which("c++"), "needs C++")
class PassFallbackHeadersTest(unittest.TestCase):
    def test_runtime_header_compiles_without_source_tree(self):
        # Use the production resource lists and generator command, without a toolkit.
        cmake = (ROOT / "api/python/ge/CMakeLists.txt").read_text()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            native_stub = root / "native_stub.cc"
            native_stub.write_text(
                "// Native module is outside this header regression.\n"
            )
            variables = {
                "AIR_CODE_DIR": str(ROOT),
                "CMAKE_CURRENT_SOURCE_DIR": str(ROOT / "api/python/ge"),
                "HI_PYTHON": sys.executable,
                "GE_PYTHON_PASS_FALLBACK_CODEGEN_DIR": str(root / "codegen"),
                "GE_PYTHON_PASS_BUILD_METADATA_DIR": str(root / "metadata"),
                "ASCEND_INSTALL_PATH": str(root / "cann"),
                "GE_PYTHON_PASS_BRIDGE_ABI_VERSION": "2",
                "GE_PYTHON_PASS_NATIVE_SOURCES": str(native_stub),
                "GE_PYTHON_PASS_NATIVE_HEADERS": str(native_stub),
            }

            def expand(match):
                return variables[match.group(1)]

            for match in re.finditer(
                r"set\((GE_PYTHON_PASS_\w+_(?:SOURCES|HEADERS))\s+(.*?)\)",
                cmake,
                re.S,
            ):
                variables[match.group(1)] = re.sub(
                    r"\$\{(\w+)\}", expand, match.group(2)
                )
            command = re.search(
                r"COMMAND (\$\{HI_PYTHON\} [^\n]*gen_fallback_resources\.py.*?)\s+DEPENDS",
                cmake,
                re.S,
            ).group(1)
            argv = shlex.split(re.sub(r"\$\{(\w+)\}", expand, command))
            # Metadata generation is independent of the embedded header inventory.
            with mock.patch.object(sys, "argv", argv[1:]):
                with mock.patch.object(
                    resource_builder, "_build_config", return_value=({}, [])
                ):
                    resource_builder.main()
            resources = {}
            exec((root / "codegen/_sources.py").read_text(), resources)
            materialized = root / "fallback_sources"
            resources["materialize"](materialized)

            # The public API is supplied by the installed toolkit in production.
            # Only its Status typedef is needed to compile this private header.
            public = root / "cann/include/ge"
            public.mkdir(parents=True)
            (public / "ge_api_error_codes.h").write_text(
                "namespace ge { using Status = unsigned int; }\n"
            )
            probe = root / "probe.cc"
            probe.write_text(
                '#include "common/python_runtime/ge_python_runtime_manager.h"\n'
                "ge::Status Probe() {\n"
                "  return ge::GePythonRuntimeManager::Instance().EnsureReady();\n"
                "}\n"
            )
            env = dict(os.environ)
            for name in ("CPATH", "CPLUS_INCLUDE_PATH", "C_INCLUDE_PATH"):
                env.pop(name, None)
            result = subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-c",
                    str(probe),
                    "-I",
                    str(materialized / "include/bridge"),
                    "-I",
                    str(public.parent),
                    "-o",
                    str(root / "probe.o"),
                ],
                cwd=root,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertTrue((root / "probe.o").is_file())
            self.assertEqual(
                (
                    materialized / "include/bridge/common/python_runtime/"
                    "ge_python_runtime_manager.h"
                ).read_bytes(),
                (
                    ROOT / "base/common/python_runtime/ge_python_runtime_manager.h"
                ).read_bytes(),
            )


class FallbackResourceCompressionTest(unittest.TestCase):
    # The resource generator must run on every HI_PYTHON of the cp37-cp314
    # matrix even though some hosts only provide Python 3.7/3.8.
    def test_parse_args_accepts_link_python_flag_pair(self):
        # argparse.BooleanOptionalAction is 3.9+; the flag pair must keep 3.7 working.
        base = [
            "gen",
            "--output-dir",
            "o",
            "--build-dir",
            "b",
            "--cann-root",
            "c",
            "--abi",
            "2",
            "--native-source",
            "s.cc",
            "--native-header",
            "s.h",
        ]
        with mock.patch.object(sys, "argv", base + ["--no-link-python"]):
            args = resource_builder.parse_args()
        self.assertFalse(args.link_python)
        with mock.patch.object(sys, "argv", base + ["--link-python"]):
            args = resource_builder.parse_args()
        self.assertTrue(args.link_python)

    def test_deterministic_gzip_matches_gzip_compress(self):
        import gzip as gzip_module

        payload = bytes(range(256)) * 64
        compressed = resource_builder._gzip_compress_deterministic(payload)
        self.assertEqual(gzip_module.decompress(compressed), payload)
        # mtime field (bytes 4-7) must be zeroed so output is reproducible.
        self.assertEqual(compressed[4:8], b"\x00\x00\x00\x00")
        # gzip.compress(mtime=...) exists from 3.8. The OS byte (offset 9)
        # differs across interpreter vintages (0xff on 3.8, 0x03 on 3.9+),
        # so compare with that byte masked out.
        try:
            reference = gzip_module.compress(payload, mtime=0)
        except TypeError:
            self.skipTest("gzip.compress without mtime (Python 3.7)")
        self.assertEqual(compressed[:9], reference[:9])
        self.assertEqual(compressed[10:], reference[10:])


@unittest.skipUnless(
    shutil.which("cmake") and shutil.which("c++"), "needs CMake and C++"
)
class CommonLinkInterfaceTest(unittest.TestCase):
    def test_bridge_links_without_internal_static_archives(self):
        # Exercise the actual ge_common link declaration with small library implementations.
        declaration = re.search(
            r"target_link_libraries\(ge_common\n.*?^\)",
            (ROOT / "base/CMakeLists.txt").read_text(),
            re.M | re.S,
        ).group(0)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sources = {
                "zip.cc": 'extern "C" int zip_value() { return 7; }',
                "serializer.cc": 'extern "C" int zip_value(); extern "C" int serialize() { return zip_value(); }',
                "common.cc": 'extern "C" int serialize(); extern "C" int common() { return serialize(); }',
                "bridge.cc": (
                    'extern "C" int common(); extern "C" int serialize(); '
                    "int main() { return common() != serialize(); }"
                ),
            }
            for name, source in sources.items():
                (root / name).write_text(source)
            (root / "CMakeLists.txt").write_text(
                """cmake_minimum_required(VERSION 3.16)
project(ge_common_interface LANGUAGES CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
foreach(dep ge_intf_pub msprof_headers runtime_headers static_mmpa graph ge_common_base
            ascend_protobuf c_sec error_manager unified_dlog platform json air_headers ge_metadef_headers)
    add_library(${dep} INTERFACE)
endforeach()
add_library(minizip_static STATIC zip.cc)
add_library(gert_model_data_serializer STATIC serializer.cc)
target_link_libraries(gert_model_data_serializer PUBLIC minizip_static)
add_library(ge_common SHARED common.cc)
"""
                + declaration
                + """
add_executable(bridge bridge.cc)
target_link_libraries(bridge PRIVATE ge_common)
"""
            )
            build = root / "cmake-build-gcov"
            for command in (
                ["cmake", "-G", "Unix Makefiles", "-S", str(root), "-B", str(build)],
                ["cmake", "--build", str(build), "-j", "8"],
            ):
                result = subprocess.run(
                    command,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout)
            link = (build / "CMakeFiles/bridge.dir/link.txt").read_text()
            self.assertNotIn("libgert_model_data_serializer.a", link)
            self.assertNotIn("libminizip_static.a", link)
            env = dict(os.environ)
            env.pop("LD_LIBRARY_PATH", None)
            env.pop("ASCEND_OPP_PATH", None)
            subprocess.run([str(build / "bridge")], check=True, env=env)


if __name__ == "__main__":
    unittest.main()
