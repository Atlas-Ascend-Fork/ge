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
from typing import Optional

from llm_datadist_v1 import (
    CacheDesc,
    DataType,
    LayerSynchronizer,
    LLMException,
    LLMStatusCode,
    TransferConfig,
    llm_wrapper,
)
from llm_datadist_v1.llm_types import KvCache
from llm_datadist_v1.llm_utils import (
    CacheDescParser,
    TransferCacheJob,
    TransferCacheParameters,
    transfer_cache_async,
)


class LayerSynchronizerImpl(LayerSynchronizer):
    def __init__(self, ret=True):
        self._ret = ret

    def synchronize_layer(
        self, layer_index: int, timeout_in_millis: Optional[int]
    ) -> bool:
        return self._ret


def _transfer_failed(_task_id, _transfer_config, _block_config):
    return llm_wrapper.kFailed


def _transfer_success(_task_id, _transfer_config, _block_config):
    return llm_wrapper.kSuccess


class LlmUtilsUt(unittest.TestCase):
    def setUp(self) -> None:
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    @staticmethod
    def _make_cache():
        cache_desc = CacheDesc(10, [2, 8], DataType.DT_FLOAT16)
        return KvCache(1, cache_desc, [], None)

    def test_parse_by_options_infer_seq_len_dim(self):
        options = {
            "llm.RefInputShapes": "2,-1,128",
            "llm.RefInputDtypes": str(DataType.DT_FLOAT16.value),
        }
        cache_desc = CacheDescParser.parse_by_options(options)
        self.assertEqual(cache_desc.seq_len_dim_index, 1)

    def test_parse_by_options_multi_dynamic_dim(self):
        options = {
            "llm.RefInputShapes": "2,-1,-1",
            "llm.RefInputDtypes": str(DataType.DT_FLOAT16.value),
        }
        with self.assertRaises(LLMException):
            CacheDescParser.parse_by_options(options)

    def test_transfer_layers_failed(self):
        config = TransferConfig(1, [10000000] * 6, range(0, 3))
        params = TransferCacheParameters(self._make_cache(), [config])
        job = TransferCacheJob(params, LayerSynchronizerImpl(True), _transfer_failed)
        job.init()
        job.transfer_layers()
        self.assertEqual(job.get_results()[0], LLMStatusCode.LLM_FAILED)

    def test_transfer_layers_success(self):
        config = TransferConfig(2, [10000000] * 10)
        params = TransferCacheParameters(self._make_cache(), [config])
        job = TransferCacheJob(params, LayerSynchronizerImpl(True), _transfer_success)
        job.init()
        job.transfer_layers()
        self.assertEqual(job.get_results()[0], LLMStatusCode.LLM_SUCCESS)

    def test_transfer_cache_async_blocks_to_cache_unsupported(self):
        params = TransferCacheParameters(self._make_cache(), [], src_block_indices=[0])
        with self.assertRaisesRegex(
            LLMException, "transfer from blocks to cache is not supported"
        ):
            transfer_cache_async(params, LayerSynchronizerImpl(True), None)

    def test_transfer_cache_async_synchronizer_none(self):
        params = TransferCacheParameters(self._make_cache(), [])
        with self.assertRaises(TypeError):
            transfer_cache_async(params, None, None)
