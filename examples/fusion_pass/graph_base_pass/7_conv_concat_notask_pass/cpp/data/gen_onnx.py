#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# ----------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------
"""
生成包含 Conv2D + Concat 结构的 ONNX 模型。
模型结构:
  input(NCHW) -> Conv1 -> conv1_out \
                Conv2 -> conv2_out /  -> Concat(axis=1) -> output
conv 输出为 NCHW 格式，shape 均为 [1, 8, 64, 64]，C 维度大小（8*4=32B）为 32B 的整数倍，满足 pass 的 C 维度 32B 对齐要求。
"""

import os

import onnx
from onnx import TensorProto, helper


def make_conv_weight(name):
    values = [(i % 7) * 0.1 for i in range(8 * 3 * 3 * 3)]
    return helper.make_tensor(name, TensorProto.FLOAT, [8, 3, 3, 3], values)


def convert():
    input_info = helper.make_tensor_value_info(
        "input", TensorProto.FLOAT, [1, 3, 64, 64]
    )
    output_info = helper.make_tensor_value_info(
        "output", TensorProto.FLOAT, [1, 16, 64, 64]
    )

    conv1 = helper.make_node(
        "Conv",
        ["input", "conv1_weight"],
        ["conv1_out"],
        kernel_shape=[3, 3],
        pads=[1, 1, 1, 1],
        name="conv1",
    )
    conv2 = helper.make_node(
        "Conv",
        ["input", "conv2_weight"],
        ["conv2_out"],
        kernel_shape=[3, 3],
        pads=[1, 1, 1, 1],
        name="conv2",
    )
    concat = helper.make_node(
        "Concat", ["conv1_out", "conv2_out"], ["output"], axis=1, name="concat"
    )

    graph = helper.make_graph(
        [conv1, conv2, concat],
        "conv_concat_test_graph",
        [input_info],
        [output_info],
        [make_conv_weight("conv1_weight"), make_conv_weight("conv2_weight")],
    )
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 18)])
    onnx.checker.check_model(model)

    output_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "model.onnx")
    onnx.save(model, output_path)


if __name__ == "__main__":
    convert()
