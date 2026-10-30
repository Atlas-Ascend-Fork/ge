/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef INC_EXTERNAL_EXE_GRAPH_RUNTIME_TASK_DEF_BUILDER_H_
#define INC_EXTERNAL_EXE_GRAPH_RUNTIME_TASK_DEF_BUILDER_H_
#include "graph/ascend_string.h"
#include <cstdint>
#include <cstddef>

namespace domi {
class TaskDef;
class KernelDef;
class KernelContext;
}  // namespace domi

namespace gert {
/**
 * KernelContext消息的视图构建器，设置任务KernelContext层级的字段，如args_format。
 * 视图对象不可凭空构造，仅可通过KernelDefBuilder::GetContextBuilder获取。
 * @since 9.3.0(2026-10)
 */
class KernelContextBuilder {
 public:
  /**
   * 设置任务的Args地址布局描述，对应KernelContext的args_format字段。
   * @param args_format 序列化后的Args布局描述字符串，由ArgsFormatSerializer::Serialize得到。
   * @return KernelContextBuilder对象引用，支持链式调用，返回值可忽略。
   * @since 9.3.0(2026-10)
   */
  KernelContextBuilder &SetArgsFormat(const ge::AscendString &args_format);

 private:
  friend class KernelDefBuilder;

  explicit KernelContextBuilder(domi::KernelContext *impl);
  domi::KernelContext *impl_;
};

/**
 * KernelDef消息的视图构建器，设置任务KernelDef层级的字段，如block_dim。
 * 视图对象不可凭空构造，仅可通过TaskDefBuilder::GetKernelBuilder获取。
 * @since 9.3.0(2026-10)
 */
class KernelDefBuilder {
 public:
  /**
   * 设置Kernel任务的多核并行度，对应KernelDef的block_dim字段。
   * @param block_dim 核并行度。
   * @return KernelDefBuilder对象引用，支持链式调用，返回值可忽略。
   * @since 9.3.0(2026-10)
   */
  KernelDefBuilder &SetBlockDim(uint32_t block_dim);
  /**
   * 获取KernelContext消息的视图构建器，内部子消息惰性创建，保证视图指向的对象非空。
   * @return KernelContextBuilder视图对象，按值返回，不拥有子消息所有权，生命周期被父对象覆盖。
   * @since 9.3.0(2026-10)
   */
  KernelContextBuilder GetContextBuilder() const;

 private:
  friend class TaskDefBuilder;

  explicit KernelDefBuilder(domi::KernelDef *impl);
  domi::KernelDef *impl_;
};

/**
 * 任务描述构建器，在GenTask回调中构造与domi::TaskDef线格式逐字节兼容的任务描述并序列化为字节流。
 * 根对象独占内部任务描述所有权，不可拷贝、不可赋值；层级视图与proto消息树一一对应。
 * @since 9.3.0(2026-10)
 */
class TaskDefBuilder {
 public:
  /**
   * 构造TaskDefBuilder对象，内部创建空的任务描述。
   * @since 9.3.0(2026-10)
   */
  TaskDefBuilder();
  /**
   * 析构TaskDefBuilder对象，释放内部持有的任务描述。
   * @since 9.3.0(2026-10)
   */
  ~TaskDefBuilder();
  TaskDefBuilder(const TaskDefBuilder &) = delete;
  TaskDefBuilder &operator=(const TaskDefBuilder &) = delete;

  /**
   * 设置任务的流ID，对应TaskDef的stream_id字段。
   * @param stream_id 任务执行的流ID，一般取自ExeResGenerationContext::GetStreamId。
   * @return TaskDefBuilder对象引用，支持链式调用，返回值可忽略。
   * @since 9.3.0(2026-10)
   */
  TaskDefBuilder &SetStreamId(uint32_t stream_id);
  /**
   * 设置任务类型，对应TaskDef的type字段，构造Kernel类型任务时取值0。
   * @param type 任务类型。
   * @return TaskDefBuilder对象引用，支持链式调用，返回值可忽略。
   * @since 9.3.0(2026-10)
   */
  TaskDefBuilder &SetType(uint32_t type);
  /**
   * 获取KernelDef消息的视图构建器，内部子消息惰性创建，保证视图指向的对象非空。
   * @return KernelDefBuilder视图对象，按值返回，不拥有子消息所有权，生命周期被父对象覆盖。
   * @since 9.3.0(2026-10)
   */
  KernelDefBuilder GetKernelBuilder() const;

  /**
   * 获取当前任务描述序列化后的字节长度，与domi::TaskDef::ByteSizeLong结果一致。
   * @return 序列化字节长度；内部对象创建失败时返回0。
   * @since 9.3.0(2026-10)
   */
  size_t ByteSizeLong() const;
  /**
   * 将任务描述序列化为字节流写入用户缓冲区，与domi::TaskDef::SerializeToArray逐字节一致。
   * @param data 序列化输出缓冲区，建议通过ByteSizeLong分配。
   * @param size 缓冲区长度（字节），不应小于ByteSizeLong的返回值。
   * @return true：序列化成功；false：序列化失败（内部对象创建失败、data为空或size为0）。
   * @since 9.3.0(2026-10)
   */
  bool SerializeToArray(uint8_t *data, size_t size) const;

 private:
  domi::TaskDef *impl_;
};
}  // namespace gert

#endif  // INC_EXTERNAL_EXE_GRAPH_RUNTIME_TASK_DEF_BUILDER_H_
