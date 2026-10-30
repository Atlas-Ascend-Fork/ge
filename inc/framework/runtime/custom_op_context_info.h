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

#include "exe_graph/runtime/eager_op_execution_context.h"

namespace gert {
class AttachedStreamProvider;

/**
 * 自定义算子的执行上下文伴生信息，承载GE与自定义算子之间的编译/运行期交互参数。
 *
 * 由框架在构造自定义算子上下文时创建，并作为附加输出传入；自定义算子通过上下文的GetContextValue读取，不直接感知本类。
 *
 * 与ArgsHandler分离，避免上下文能力堆叠到args内存管理接口上。
 */
class CustomOpContextInfo {
 public:
  CustomOpContextInfo() = default;
  ~CustomOpContextInfo() = default;

  /// 取值来源（内部细节，勿写入对外头）：V1下沉路径CustomTaskInfo::UpdateContextInfo恒置true；
  /// V2动态路径不设置（保持false）
  void SetStaticModel(const bool is_static_model) {
    is_static_model_ = is_static_model;
  }

  /// 取值来源（内部）：V1由DavinciModel的地址刷新使能（地址未固定 && feature base可刷新 && I/O含可刷新内存）判定
  void SetAddressRefreshEnabled(const bool enabled) {
    address_refresh_enabled_ = enabled;
  }

  /// 取值来源（内部）：V1取DavinciModel的根图id；V2由lowering注入kernel输入后写入。
  /// 恒有效（graph id的0是合法值，故用has_graph_id_ 区分"未设置"）
  void SetGraphId(const uint64_t graph_id) {
    graph_id_ = graph_id;
    has_graph_id_ = true;
  }

  /// 取值来源（内部）：算子节点id（OpDesc::GetId）；未设置时查询失败（0亦为合法值）
  void SetNodeId(const uint64_t node_id) {
    node_id_ = node_id;
    has_node_id_ = true;
  }

  /// 取值来源（内部）：算子 _op_vectorcore_num属性；属性缺失或非法时不设置（查询失败）
  void SetVectorCoreNum(const uint64_t vector_core_num) {
    vector_core_num_ = vector_core_num;
    has_vector_core_num_ = true;
  }

  /// 辅流提供者：由GE运行时（模型加载路径）装入，算子经EagerOpExecutionContext::RequestAttachedStream查询。
  /// 注意：裸指针，生命周期由框架保证（容器晚于伴生对象释放），本类不拥有也不销毁。
  void SetAttachedStreamProvider(AttachedStreamProvider *provider) {
    attached_stream_provider_ = provider;
  }
  AttachedStreamProvider *GetAttachedStreamProvider() const {
    return attached_stream_provider_;
  }

  bool GetContextValue(const ContextParam key, uint64_t &value) const {
    switch (key) {
      case ContextParam::kStaticModel:
        value = is_static_model_ ? 1UL : 0UL;
        return true;
      case ContextParam::kAddressRefreshEnabled:
        value = address_refresh_enabled_ ? 1UL : 0UL;
        return true;
      case ContextParam::kGraphId:
        if (!has_graph_id_) {
          return false;  // graph id的0是合法值，必须显式区分“未设置”
        }
        value = graph_id_;
        return true;
      case ContextParam::kNodeId:
        if (!has_node_id_) {
          return false;  // node id的0是合法值，必须显式区分“未设置”
        }
        value = node_id_;
        return true;
      case ContextParam::kVectorCoreNum:
        if (!has_vector_core_num_) {
          return false;  // 属性缺失时不写出，调用方据此判定不可用
        }
        value = vector_core_num_;
        return true;
      default:
        return false;
    }
  }

 private:
  bool is_static_model_{false};
  bool address_refresh_enabled_{false};
  uint64_t graph_id_{0UL};
  bool has_graph_id_{false};
  uint64_t node_id_{0UL};
  bool has_node_id_{false};
  uint64_t vector_core_num_{0UL};
  bool has_vector_core_num_{false};
  AttachedStreamProvider *attached_stream_provider_{nullptr};
};
}  // namespace gert

#endif  // GE_INC_FRAMEWORK_RUNTIME_EAGER_EXECUTION_CONTEXT_INFO_H_
