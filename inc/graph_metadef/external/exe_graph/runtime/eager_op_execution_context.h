/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_CXX_INC_EXE_GRAPH_RUNTIME_EAGER_OP_EXECUTION_CONTEXT_H_
#define METADEF_CXX_INC_EXE_GRAPH_RUNTIME_EAGER_OP_EXECUTION_CONTEXT_H_

#include <type_traits>
#include "graph/tensor.h"
#include "exe_graph/runtime/extended_kernel_context.h"
#include "exe_graph/runtime/kernel_args.h"
#include "graph/ascend_string.h"

namespace gert {
using rtStream = void *;

/**
 * GE与自定义算子之间的运行期交互参数。
 * 统一按数值取值语义暴露（布尔参数取值为0/1）；数值类参数按需扩展，
 * 新增参数只需增加枚举值，不改变接口签名。
 */
enum class ContextParam : uint32_t {
  kStaticModel = 0,            // 是否运行于静态图（含动态shape的静态子图），取值0/1
  kAddressRefreshEnabled = 1,  // 地址是否需要刷新，取值0/1
  kGraphId = 2,                // 根图graph id（恒为根图，不受子图影响）
  kNodeId = 3,                 // 算子节点在图内的id（图内唯一）
  kVectorCoreNum = 4,          // 算子节点可用的vector core数量
  kNum,                        // 预留
};

class EagerOpExecutionContext : public ExtendedKernelContext {
 public:
  /**
   * 获取所属的执行流
   * @return
   */
  rtStream GetStream() const;

  /**
   * 根据输入index，获取输入tensor指针
   * @param index 输入index
   * @return Tensor指针，异常时返回空指针
   */
  const Tensor *GetInputTensor(size_t index) const {
    if (static_cast<int64_t>(index) >= GetAdditionalInputStartIndex()) {
      return nullptr;
    }
    return GetInputPointer<Tensor>(index);
  }

  /**
   * 基于算子IR原型定义，获取`REQUIRED_INPUT`类型的输入Tensor指针
   * @param ir_index IR原型定义中的index
   * @return Tensor指针，异常时返回空指针
   */
  const Tensor *GetRequiredInputTensor(size_t ir_index) const {
    return GetDynamicInputPointer<Tensor>(ir_index, 0);
  }

  /**
   * 基于算子IR原型定义，获取`OPTIONAL_INPUT`类型的输入tensor指针
   * @param ir_index IR原型定义中的index
   * @return Tensor指针，异常时返回空指针
   */
  const Tensor *GetOptionalInputTensor(size_t ir_index) const {
    return GetDynamicInputPointer<Tensor>(ir_index, 0);
  }

  /**
   * 基于算子IR原型定义，获取`DYNAMIC_INPUT`类型的输入Tensor指针
   * @param ir_index IR原型定义中的index
   * @param relative_index
   * 该输入实例化后的相对index，例如某个DYNAMIC_INPUT实例化了3个输入，那么relative_index的有效范围是[0,2]
   * @return Tensor指针，异常时返回空指针
   */
  const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const {
    return GetDynamicInputPointer<Tensor>(ir_index, relative_index);
  }

  /**
   * 为某个输出tensor申请device内存，同时初始化输出tensor的基本信息
   * @param index 输出索引
   * @param shape 输出tensor的shape
   * @param format 输出tensor的format
   * @param dtype 输出tensor的data type
   * @return Tensor指针，异常时返回空指针
   * 生命周期: 该输出tensor的内存由context构造方管理。接口调用者不需要主动释放
   */
  Tensor *MallocOutputTensor(size_t index, const StorageShape &shape, const StorageFormat &format, ge::DataType dtype);

  /**
   * 指定某输出的内存地址引用自某个输入
   * @param output_index 输出索引
   * @param input_index 输入索引
   * @return output_index对应的输出Tensor指针
   */
  Tensor *MakeOutputRefInput(size_t output_index, size_t input_index) const;

  /**
   * 分配workspace内存，placement为device
   * @param size 内存大小，单位为字节
   * @return 地址指针，异常时返回空指针
   * 生命周期：内存由context构造方管理，接口调用者不需要主动释放
   */
  void *MallocWorkSpace(size_t size);

  /**
   * 获取index指定的输出Tensor指针
   * @param index 输出索引
   * @return 输出Tensor指针，异常时返回空指针
   */
  const Tensor *GetOutputTensor(size_t index) const {
    return GetOutputPointer<Tensor>(index);
  }

  /**
   * 为 host 侧 args buffer 分配设备侧内存，并返回只读 KernelArgs 指针。
   *
   * KernelArgs 的 args_data 字段保存设备侧 args 地址，用于自定义算子
   * EagerExecute 中 kernel launch。
   *
   * 本接口在模型执行前将 host_args 内容同步到设备侧
   * 接口返回后调用方可立即释放host_args，无需手动 H2D 拷贝。
   *
   * 返回的 args_data 地址为只读，调用方不得通过该指针修改 args_data指向的内容。
   *
   * @param host_args Host 侧 args buffer 指针，接口返回后调用方可立即释放。
   * @param args_size Args buffer 大小（字节）。
   * @return 只读 KernelArgs 指针，args_data 字段为设备侧 args 地址，
   *         失败返回 nullptr。
   */
  const KernelArgs *MallocReadOnlyDevArgs(void *host_args, size_t args_size) const;

  /**
   * 按 key 获取 eager 自定义算子使用的辅流。
   * @param key 辅流复用 key
   * @param flags RTS流创建标志原值（如RT_STREAM_PERSISTENT | RT_STREAM_FORCE_COPY，可组合），
   *              由算子自行传入，GE直接透传给辅流创建；仅新建时生效，复用命中时忽略。
   *              不需要特殊标志时填0，按框架默认创建参数建流
   * @return 托管的辅流句柄；不支持或参数无效时返回 nullptr
   */
  rtStream RequestAttachedStream(const ge::AscendString &key, uint32_t flags);

  /**
   * 查询GE与自定义算子之间的运行期交互参数取值。
   * 各参数的语义见ContextParam；布尔参数的取值为0/1，数值参数为对应数值。
   * @param key 交互参数
   * @param value 输出取值
   * @return 成功返回true；key未定义或当前上下文不可用时返回false（不写出value）
   */
  bool GetContextValue(ContextParam key, uint64_t &value) const;

  /**
   * 获取运行期选项（全局/会话/图三级合并语义）。
   * @param option_key 选项key
   * @param option_value 输出选项值
   * @return 成功返回true，失败或未配置返回false
   */
  bool GetOption(const ge::AscendString &option_key, ge::AscendString &option_value) const;

  enum class AdditionalInputIndex : uint32_t { kDeviceAllocator = 0, kStream, kNum };

  enum class AdditionalOutputIndex : uint32_t { kWorkSpace = 0, kArgsHandler, kContextInfo, kNum };

 protected:
  int64_t GetAdditionalInputStartIndex() const {
    const auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return -1;
    }
    return compute_node_info->GetInputsNum();
  }

  int64_t GetAdditionalOutputStartIndex() const {
    const auto compute_node_info = GetComputeNodeInfo();
    if (compute_node_info == nullptr) {
      return -1;
    }
    return compute_node_info->GetOutputsNum();
  }
};

static_assert(std::is_standard_layout<EagerOpExecutionContext>::value,
              "The class EagerOpExecutionContext must be a POD");
}  // namespace gert
#endif  // METADEF_CXX_INC_EXE_GRAPH_RUNTIME_EAGER_OP_EXECUTION_CONTEXT_H_
