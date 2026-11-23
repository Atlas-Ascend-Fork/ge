#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

import json
import os
import tempfile
import unittest

from llm_datadist_v1 import LLMException
from llm_datadist_v1.config import ClusterConfig
from llm_datadist_v1.configs import LlmConfig


class LlmConfigUt(unittest.TestCase):
    def setUp(self) -> None:
        print("Begin ", self._testMethodName)
        self._llm_config = LlmConfig()

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_device_id_list(self):
        self._llm_config.device_id = [0, 1]
        self.assertEqual(self._llm_config.device_id, [0, 1])

    def test_device_id_list_invalid(self):
        with self.assertRaises(LLMException):
            self._llm_config.device_id = [-1]

    def test_sync_kv_timeout(self):
        self._llm_config.sync_kv_timeout = "100"
        with self.assertRaises(LLMException):
            self._llm_config.sync_kv_timeout = "0"
        with self.assertRaises(LLMException):
            self._llm_config.sync_kv_timeout = "abc"

    def test_cache_manager_flags(self):
        self._llm_config.enable_cache_manager = True
        self.assertTrue(self._llm_config.enable_cache_manager)
        self._llm_config.enable_remote_cache_accessible = True
        self.assertTrue(self._llm_config.enable_remote_cache_accessible)

    def test_gen_options_with_cache_manager(self):
        self._llm_config.device_id = [0, 1]
        self._llm_config.sync_kv_timeout = "100"
        self._llm_config.enable_cache_manager = True
        self._llm_config.enable_remote_cache_accessible = True
        options = self._llm_config.generate_options()
        self.assertEqual(options["ge.exec.deviceId"], "0;1")
        self.assertEqual(options["llm.SyncKvCacheWaitTime"], "100")
        self.assertEqual(options["llm.EnableCacheManager"], "1")
        self.assertEqual(options["llm.EnableRemoteCacheAccessible"], "1")


class ClusterConfigUt(unittest.TestCase):
    def setUp(self) -> None:
        print("Begin ", self._testMethodName)
        fd, self._numa_path = tempfile.mkstemp(suffix=".json")
        numa_config = {
            "cluster": [
                {
                    "cluster_nodes": [
                        {
                            "node_id": 0,
                            "is_local": True,
                            "item_list": [{"item_id": 0}, {"item_id": 1}],
                        }
                    ]
                }
            ]
        }
        with os.fdopen(fd, "w") as f:
            json.dump(numa_config, f)

    def tearDown(self) -> None:
        os.remove(self._numa_path)
        print("End ", self._testMethodName)

    def test_from_engine_options_with_rank_and_device(self):
        options = {
            "llm.ClusterInfo": json.dumps(
                {"cluster_id": 0, "logic_device_id": ["0:0:0:0", "0:0:1:0"]}
            ),
            "ge.exec.rankId": "0",
            "ge.exec.deviceId": "0",
            "ge.resourceConfigPath": self._numa_path,
        }
        config = ClusterConfig.from_engine_options(options)
        self.assertEqual(config.rank_id_to_device_id, {0: 0})

    def test_from_engine_options_without_rank_id(self):
        options = {
            "llm.ClusterInfo": json.dumps(
                {"cluster_id": 0, "logic_device_id": ["0:0:0:0", "0:0:1:0"]}
            ),
            "ge.resourceConfigPath": self._numa_path,
        }
        config = ClusterConfig.from_engine_options(options)
        self.assertEqual(config.rank_id_to_device_id, {0: 0, 1: 1})

    def test_from_engine_options_without_cluster_info(self):
        with self.assertRaises(LLMException):
            ClusterConfig.from_engine_options({"ge.exec.rankId": "0"})
