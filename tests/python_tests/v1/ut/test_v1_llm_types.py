#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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
from unittest import mock

from llm_datadist_v1 import (
    BlocksCacheKey,
    CacheDesc,
    CacheKey,
    CacheKeyByIdAndIndex,
    CacheTask,
    DataType,
    KvCache,
    LLMException,
    LLMStatusCode,
    Placement,
    TransferWithCacheKeyConfig,
)
from llm_datadist_v1 import llm_wrapper


class CacheDescUt(unittest.TestCase):
    def setUp(self):
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def _make_desc(self):
        return CacheDesc(2, [4, 8], DataType.DT_INT32, Placement.HOST)

    def test_size_ok(self):
        desc = self._make_desc()
        with mock.patch.object(llm_wrapper, "calc_tensor_size", return_value=128):
            self.assertEqual(desc.size, 128)

    def test_size_calc_failed(self):
        desc = self._make_desc()
        with mock.patch.object(llm_wrapper, "calc_tensor_size", return_value=-1):
            with self.assertRaises(LLMException):
                _ = desc.size

    def test_update_dim_resets_size(self):
        desc = self._make_desc()
        with mock.patch.object(llm_wrapper, "calc_tensor_size", return_value=128):
            self.assertEqual(desc.size, 128)
        desc.update_dim(0, 16)
        with mock.patch.object(llm_wrapper, "calc_tensor_size", return_value=256):
            self.assertEqual(desc.size, 256)


class BlocksCacheKeyUt(unittest.TestCase):
    def setUp(self):
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_construct_with_cluster_id(self):
        key = BlocksCacheKey(cluster_id=1, model_id=2)
        self.assertEqual(key.cluster_id, 1)
        self.assertEqual(key.model_id, 2)
        self.assertIn("cluster_id=1", repr(key))

    def test_construct_with_prompt_cluster_id(self):
        key = BlocksCacheKey(prompt_cluster_id=3)
        self.assertEqual(key.prompt_cluster_id, 3)
        self.assertEqual(key.model_id, 0)
        self.assertIn("prompt_cluster_id=3", repr(key))

    def test_construct_positional(self):
        key = BlocksCacheKey(5)
        self.assertEqual(key.cluster_id, 5)

    def test_construct_missing_param(self):
        with self.assertRaises(LLMException):
            BlocksCacheKey(model_id=1)

    def test_construct_unsupported_param(self):
        with self.assertRaises(LLMException):
            BlocksCacheKey(unknown_key=1)

    def test_construct_type_error(self):
        with self.assertRaises(TypeError):
            BlocksCacheKey(cluster_id="not_int")


class CacheKeyUt(unittest.TestCase):
    def setUp(self):
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_construct_positional(self):
        key = CacheKey(1, 2, 3, 4)
        self.assertEqual(key.cluster_id, 1)
        self.assertEqual(key.req_id, 2)
        self.assertEqual(key.model_id, 3)
        self.assertEqual(key.prefix_id, 4)

    def test_construct_with_cluster_id_kwargs(self):
        key = CacheKey(cluster_id=1, req_id=2)
        self.assertEqual(key.cluster_id, 1)
        self.assertEqual(key.req_id, 2)
        self.assertEqual(key.model_id, 0)

    def test_construct_with_prompt_cluster_id_kwargs(self):
        key = CacheKey(prompt_cluster_id=7, req_id=8)
        self.assertEqual(key.prompt_cluster_id, 7)
        self.assertEqual(key.req_id, 8)

    def test_construct_missing_req_id(self):
        with self.assertRaises(LLMException):
            CacheKey(cluster_id=1)

    def test_construct_missing_cluster_param(self):
        with self.assertRaises(LLMException):
            CacheKey(req_id=1)

    def test_construct_unsupported_param(self):
        with self.assertRaises(LLMException):
            CacheKey(cluster_id=1, req_id=2, unknown_key=3)

    def test_construct_param_num_over_limit(self):
        with self.assertRaises(LLMException):
            CacheKey(1, 2, 3, 4, 5)


class KvCacheUt(unittest.TestCase):
    def setUp(self):
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    @staticmethod
    def _make_desc():
        return CacheDesc(1, [4], DataType.DT_INT32, Placement.HOST)

    def test_del_with_initialized_manager(self):
        manager = mock.MagicMock()
        manager.is_initialized.return_value = True
        cache = KvCache(1, self._make_desc(), [[1, 2]], manager)
        del cache
        manager.deallocate_cache.assert_called_once()

    def test_del_with_uninitialized_manager(self):
        manager = mock.MagicMock()
        manager.is_initialized.return_value = False
        cache = KvCache(1, self._make_desc(), [[1, 2]], manager)
        self.assertEqual(cache.cache_id, 1)
        del cache
        manager.deallocate_cache.assert_not_called()

    def test_del_without_manager(self):
        cache = KvCache(1, self._make_desc(), [[1, 2]], None)
        self.assertIn("KvCache", str(cache))
        self.assertIn("KvCache", repr(cache))
        del cache

    def test_create_cpu_cache_with_int_addrs(self):
        desc = CacheDesc(2, [4], DataType.DT_INT32, Placement.HOST)
        cache = KvCache.create_cpu_cache(desc, [1, 2])
        self.assertEqual(cache.per_device_tensor_addrs, [[1, 2]])
        self.assertEqual(cache.cache_id, -1)

    def test_create_cpu_cache_with_list_addrs(self):
        desc = CacheDesc(2, [4], DataType.DT_INT32, Placement.HOST)
        cache = KvCache.create_cpu_cache(desc, [[1, 2], [3, 4]])
        self.assertEqual(cache.per_device_tensor_addrs, [[1, 2], [3, 4]])

    def test_create_cpu_cache_wrong_placement(self):
        desc = CacheDesc(1, [4], DataType.DT_INT32, Placement.DEVICE)
        with self.assertRaises(LLMException):
            KvCache.create_cpu_cache(desc, [1])

    def test_create_cpu_cache_inconsistent_type(self):
        desc = self._make_desc()
        with self.assertRaises(LLMException):
            KvCache.create_cpu_cache(desc, [1, [2]])

    def test_create_cpu_cache_num_tensors_mismatch(self):
        desc = self._make_desc()
        with self.assertRaises(LLMException):
            KvCache.create_cpu_cache(desc, [1, 2])
        desc2 = CacheDesc(3, [4], DataType.DT_INT32, Placement.HOST)
        with self.assertRaises(LLMException):
            KvCache.create_cpu_cache(desc2, [[1, 2]])


class TransferWithCacheKeyConfigUt(unittest.TestCase):
    def setUp(self):
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_construct_with_blocks_cache_key(self):
        config = TransferWithCacheKeyConfig(BlocksCacheKey(1), range(1, 5), range(1, 5))
        self.assertEqual(config.src_batch_index, 0)
        self.assertEqual(config.dst_cluster_id, 1)
        self.assertIn("TransferWithCacheKeyConfig", repr(config))

    def test_construct_with_cache_key_by_index(self):
        config = TransferWithCacheKeyConfig(
            CacheKeyByIdAndIndex(1, 2), range(1, 5), range(1, 5), src_batch_index=1
        )
        self.assertEqual(config.src_batch_index, 1)

    def test_construct_range_size_mismatch(self):
        with self.assertRaises(LLMException):
            TransferWithCacheKeyConfig(
                CacheKeyByIdAndIndex(1, 2), range(1, 5), range(1, 6)
            )

    def test_construct_invalid_cache_key_type(self):
        with self.assertRaises(TypeError):
            TransferWithCacheKeyConfig("not_a_key", range(1, 5), range(1, 5))


class CacheTaskUt(unittest.TestCase):
    def setUp(self):
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_get_results_with_timeout(self):
        task = mock.MagicMock()
        task.get_results.return_value = [LLMStatusCode.LLM_SUCCESS]
        cache_task = CacheTask(task)
        results = cache_task.get_results(1000)
        self.assertEqual(results, [LLMStatusCode.LLM_SUCCESS])
        task.get_results.assert_called_once_with(1.0)

    def test_get_results_without_timeout(self):
        task = mock.MagicMock()
        task.get_results.return_value = [LLMStatusCode.LLM_SUCCESS]
        cache_task = CacheTask(task)
        results = cache_task.get_results()
        self.assertEqual(results, [LLMStatusCode.LLM_SUCCESS])
        task.get_results.assert_called_once_with(None)

    def test_synchronize_with_timeout(self):
        task = mock.MagicMock()
        task.get.return_value = LLMStatusCode.LLM_SUCCESS
        cache_task = CacheTask(task)
        status = cache_task.synchronize(2000)
        self.assertEqual(status, LLMStatusCode.LLM_SUCCESS)
        task.get.assert_called_once_with(2.0)
