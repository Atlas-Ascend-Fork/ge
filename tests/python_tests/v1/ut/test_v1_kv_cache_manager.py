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

import unittest

from llm_datadist_v1 import (
    BlocksCacheKey,
    CacheDesc,
    DataType,
    KvCacheManager,
    LLMException,
    LLMRole,
)


class KvCacheManagerUt(unittest.TestCase):
    def setUp(self) -> None:
        print("Begin ", self._testMethodName)
        self._manager = KvCacheManager(None, LLMRole.PROMPT)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_pull_blocks_role_not_supported(self):
        with self.assertRaises(LLMException):
            self._manager.pull_blocks(None, None, None, None)

    def test_allocate_blocks_cache_invalid_num_tensors(self):
        cache_desc = CacheDesc(0, [2, 8], DataType.DT_FLOAT16)
        with self.assertRaises(LLMException):
            self._manager.allocate_blocks_cache(cache_desc)

    def test_allocate_blocks_cache_key_not_supported_by_decoder(self):
        manager = KvCacheManager(None, LLMRole.DECODER)
        cache_desc = CacheDesc(10, [2, 8], DataType.DT_FLOAT16)
        with self.assertRaises(LLMException):
            manager.allocate_blocks_cache(cache_desc, BlocksCacheKey(0, 0))

    def test_allocate_cache_invalid_num_tensors(self):
        cache_desc = CacheDesc(0, [2, 8], DataType.DT_FLOAT16)
        with self.assertRaises(LLMException):
            self._manager.allocate_cache(cache_desc)
