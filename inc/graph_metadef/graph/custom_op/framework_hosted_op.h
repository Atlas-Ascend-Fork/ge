/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_INC_GRAPH_CUSTOM_OP_FRAMEWORK_HOSTED_OP_H_
#define METADEF_CXX_INC_GRAPH_CUSTOM_OP_FRAMEWORK_HOSTED_OP_H_

namespace ge {
// 标记接口：算子实现由框架进程内的适配器承载（如 Python 自定义算子经 bridge 注册的适配器），
// 不存在可独立交付的算子实现 so，因此不参与 SO in OM 收集。
// 这类算子的实例类编译在框架库内，按实例虚表 dladdr 只会解析到框架库自身；框架库不导出算子 so 的
// pull creator ABI 符号（GetRegisteredCustomOpCreator*），一旦被当作算子 so 打包进 OM，
// 加载期解析符号失败会导致整个模型加载失败。
class FrameworkHostedOp {
 public:
  virtual ~FrameworkHostedOp() = default;
};
}  // namespace ge

#endif  // METADEF_CXX_INC_GRAPH_CUSTOM_OP_FRAMEWORK_HOSTED_OP_H_
