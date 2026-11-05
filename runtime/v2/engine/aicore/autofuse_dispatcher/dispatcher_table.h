/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_ENGINE_AICORE_AUTOFUSE_DISPATCHER_DISPATCHER_TABLE_H_
#define AIR_CXX_RUNTIME_V2_ENGINE_AICORE_AUTOFUSE_DISPATCHER_DISPATCHER_TABLE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "ge/ge_api_types.h"
#include "exe_graph/runtime/runtime_tensor.h"

namespace ge {
namespace autofuse_dispatcher {

// 编译期为整张融合图生成的校验函数 ABI，与
// compiler/graph/optimize/symbolic/codegen/guard_codegen.h 的 ge::GuardCheckFunc 一致。
using GuardCheckFuncPtr = bool (*)(gert::Tensor **tensors, size_t num_tensors, char_t *reason, size_t reason_size);

// 整图校验器（autofuse 离线双执行体）：
// 加载编译期产出的 guard_check.so（内容来自 root_graph 属性 _guard_check_so_data），
// 判定输入是否命中优化执行体。与在线 guarded_execution_point 一致：仅日志，不做 profiling 上报（见设计文档 A3/A5）。
class DispatcherTable {
 public:
  DispatcherTable() = default;
  ~DispatcherTable();
  DispatcherTable(const DispatcherTable &) = delete;
  DispatcherTable &operator=(const DispatcherTable &) = delete;

  // guard_so_data：编译期 guard codegen 产出的 guard_check.so 内容（root_graph 属性 _guard_check_so_data）。
  // 为空表示未启用（Match 恒 false）。
  Status LoadGuardSo(const std::string &guard_so_data);

  // true=命中（走优化执行体）；false=未命中（走兜底执行体）。
  bool Match(const std::vector<gert::Tensor *> &inputs, char_t *reason, size_t reason_size) const;

 private:
  // 关闭并重置 guard so 句柄/fd，保证可重复加载且失败路径不残留资源。
  void CloseGuardSo();

  void *guard_so_handle_{nullptr};   // dlopen 返回
  GuardCheckFuncPtr func_{nullptr};  // dlsym("GuardCheckFunc")
  int32_t file_handle_{-1};          // memfd_create
};

}  // namespace autofuse_dispatcher
}  // namespace ge

#endif  // AIR_CXX_RUNTIME_V2_ENGINE_AICORE_AUTOFUSE_DISPATCHER_DISPATCHER_TABLE_H_
