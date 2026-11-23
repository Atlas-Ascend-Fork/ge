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

from llm_datadist_v1.utils.utils import (
    check_dict,
    check_int32,
    check_int64,
    check_positive_or_set_default,
    check_uint8,
    check_uint16,
)


class LlmUtilsCheckSt(unittest.TestCase):
    def setUp(self) -> None:
        print("Begin ", self._testMethodName)

    def tearDown(self) -> None:
        print("End ", self._testMethodName)

    def test_check_dict_key_type_error(self):
        with self.assertRaises(TypeError):
            check_dict("cfg", {1: "value"}, str, str)

    def test_check_int64_out_of_range(self):
        with self.assertRaises(ValueError):
            check_int64("size", 2**63)

    def test_check_int32_out_of_range(self):
        with self.assertRaises(ValueError):
            check_int32("size", 2**31)

    def test_check_uint16_out_of_range(self):
        with self.assertRaises(ValueError):
            check_uint16("size", 2**16)

    def test_check_uint8_out_of_range(self):
        with self.assertRaises(ValueError):
            check_uint8("size", 2**8)

    def test_check_positive_or_set_default_negative(self):
        with self.assertRaises(ValueError):
            check_positive_or_set_default("offset", -1)
