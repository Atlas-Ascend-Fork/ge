/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef GE_INC_FRAMEWORK_RUNTIME_CUSTOM_OP_CONTEXT_INFO_H_
#define GE_INC_FRAMEWORK_RUNTIME_CUSTOM_OP_CONTEXT_INFO_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "exe_graph/runtime/eager_op_execution_context.h"

namespace gert {
class AttachedStreamProvider;

/**
 * 自定义算子的执行上下文伴生信息，承载GE与自定义算子之间的编译/运行期交互参数。
 *
 * 由框架在构造自定义算子上下文时创建，并作为附加输出传入；自定义算子通过上下文的GetContextValue读取，不直接感知本类。
 *
 * 取值以ContextParam枚举值为下标存放在表中，未设置项查询返回false；
 * 与ArgsHandler分离，避免上下文能力堆叠到args内存管理接口上。
 */
class CustomOpContextInfo {
 public:
  CustomOpContextInfo() = default;
  ~CustomOpContextInfo() = default;

  /// 取值来源（内部细节，勿写入对外头）：V1下沉路径CustomTaskInfo::UpdateContextInfo恒置true；
  /// V2动态路径不设置（保持false）
  void SetStaticModel(const bool is_static_model) {
    Set(ContextParam::kStaticModel, is_static_model ? 1UL : 0UL);
  }

  /// 取值来源（内部）：V1由DavinciModel的地址刷新使能（地址未固定 && feature base可刷新 && I/O含可刷新内存）判定
  void SetAddressRefreshEnabled(const bool enabled) {
    Set(ContextParam::kAddressRefreshEnabled, enabled ? 1UL : 0UL);
  }

  /// 取值来源（内部）：V1取DavinciModel的根图id；V2由lowering注入kernel输入后写入
  void SetGraphId(const uint64_t graph_id) {
    Set(ContextParam::kGraphId, graph_id);
  }

  /// 取值来源（内部）：算子节点id（OpDesc::GetId）
  void SetNodeId(const uint64_t node_id) {
    Set(ContextParam::kNodeId, node_id);
  }

  /// 取值来源（内部）：算子 _op_vectorcore_num属性；属性缺失或非法时不设置（查询失败）
  void SetVectorCoreNum(const uint64_t vector_core_num) {
    Set(ContextParam::kVectorCoreNum, vector_core_num);
  }

  /// 取值来源（内部）：算子最终生效的AI core数量；未知时不设置（查询失败）
  void SetAiCoreNum(const uint64_t ai_core_num) {
    Set(ContextParam::kAiCoreNum, ai_core_num);
  }

  /// 取值来源（内部）：设备AICPU核数量；获取失败时不设置（查询失败）
  void SetAiCpuNum(const uint64_t ai_cpu_num) {
    Set(ContextParam::kAiCpuNum, ai_cpu_num);
  }

  /// 辅流提供者：由GE运行时（模型加载路径）装入，算子经EagerOpExecutionContext::RequestAttachedStream查询。
  /// 注意：裸指针，生命周期由框架保证（容器晚于伴生对象释放），本类不拥有也不销毁。
  void SetAttachedStreamProvider(AttachedStreamProvider *provider) {
    attached_stream_provider_ = provider;
  }
  AttachedStreamProvider *GetAttachedStreamProvider() const {
    return attached_stream_provider_;
  }

  bool GetContextValue(ContextParam key, uint64_t &value) const;

 private:
  struct ValueEntry {
    uint64_t value{0UL};
    bool has_value{false};
  };

  void Set(ContextParam key, uint64_t value) {
    const auto index = static_cast<size_t>(key);
    if (index < values_.size()) {
      values_[index] = ValueEntry{value, true};
    }
  }

  std::array<ValueEntry, static_cast<size_t>(ContextParam::kNum)> values_{};
  AttachedStreamProvider *attached_stream_provider_{nullptr};
};
}  // namespace gert

#endif  // GE_INC_FRAMEWORK_RUNTIME_CUSTOM_OP_CONTEXT_INFO_H_
