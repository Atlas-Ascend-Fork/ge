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

import numpy as np
from llm_datadist_v1 import Tensor


class TensorSt(unittest.TestCase):
    def setUp(self) -> None:
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_str_tensor(self):
        arr1 = np.array([["aaaa", "bbbb"], ["ccc", "ddd"]])
        src_type = arr1.dtype
        tensor = Tensor(arr1)
        self.assertEqual(tensor.numpy(True).dtype, src_type)

    def test_str_tensor_numpy_copy_false(self):
        arr1 = np.array([["aaaa", "bbbb"]])
        tensor = Tensor(arr1)
        with self.assertRaises(RuntimeError):
            tensor.numpy(False)
