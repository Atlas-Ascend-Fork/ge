/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef INC_GRAPH_METADEF_GRAPH_GERT_GRAPH_ATTRIBUTES_H_
#define INC_GRAPH_METADEF_GRAPH_GERT_GRAPH_ATTRIBUTES_H_

/**
 * @file gert_graph_attributes.h
 * @brief 执行图处理使用的内部属性键名与取值常量。
 *
 * 这些键为仓内图处理约定，不作为对外接口发布；字段校验与编码转换由编译入口负责。
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 阶段键：字符串，取值为下面的三个阶段之一；缺省按 Main 处理。 */
#define GERT_ATTR_STAGE "gert.stage"
#define GERT_STAGE_INIT "init"
#define GERT_STAGE_MAIN "main"
#define GERT_STAGE_DEINIT "deinit"

/* 图输入的序号键，沿用 ES 既有 index 协议。 */
#define GERT_ATTR_INDEX "index"

/* 模型用户 IO 描述：list Tensor，按模型 IO 顺序排列，每个 TensorDesc 携带名称和描述。 */
#define GERT_ATTR_MODEL_INPUTS "gert.model.inputs"
#define GERT_ATTR_MODEL_OUTPUTS "gert.model.outputs"

/* 统一资源槽数：int64。 */
#define GERT_ATTR_MODEL_STREAM_COUNT "gert.model.stream_count"
#define GERT_ATTR_MODEL_EVENT_COUNT "gert.model.event_count"
#define GERT_ATTR_MODEL_NOTIFY_COUNT "gert.model.notify_count"
#define GERT_ATTR_MODEL_ATTACHED_STREAM_COUNT "gert.model.attached_stream_count"

/* 输出执行值引用协议：节点名（字符串）+ 输出索引（非负 int64）成对设置。 */
#define GERT_ATTR_REF_FROM_NODE "RefFromNode"
#define GERT_ATTR_REF_FROM_INDEX "RefFromIndex"

/* 来源算子描述：执行节点上的版本化 JSON 字符串。 */
#define GERT_ATTR_SOURCE_OP "gert.source_op"

/* 设备二进制（.o）：gert.binary.ids 枚举 ID，每个 ID 对应 gert.binary.<id>.name 和
 * gert.binary.<id>.content。后续可优化按 ID 组织资源属性的方式。
 */
#define GERT_ATTR_BINARY_IDS "gert.binary.ids"
#define GERT_ATTR_BINARY_PREFIX "gert.binary."

/* Custom Kernel registration metadata. */
#define GERT_ATTR_CUSTOM_KERNEL_NAME "_custom_kernel_name"
#define GERT_ATTR_CUSTOM_KERNEL_SO_PATH "_custom_kernel_so_path"
#define GERT_ATTR_CUSTOM_RUN_FUNC_SYMBOL "_custom_run_func_symbol"
#define GERT_ATTR_CUSTOM_OUTPUTS_CREATOR_SYMBOL "_custom_outputs_creator_symbol"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 常量放在全局 gert 命名空间：与运行库一致，且不会在 namespace ge 内遮蔽 ::gert，
   否则 ge 内部出现的 gert::Tensor 等非限定名会被解析到这个别名命名空间上。 */
namespace gert {
namespace attr {
constexpr const char *kStage = GERT_ATTR_STAGE;
constexpr const char *kStageInit = GERT_STAGE_INIT;
constexpr const char *kStageMain = GERT_STAGE_MAIN;
constexpr const char *kStageDeInit = GERT_STAGE_DEINIT;

constexpr const char *kIndex = GERT_ATTR_INDEX;

constexpr const char *kModelInputs = GERT_ATTR_MODEL_INPUTS;
constexpr const char *kModelOutputs = GERT_ATTR_MODEL_OUTPUTS;
constexpr const char *kModelStreamCount = GERT_ATTR_MODEL_STREAM_COUNT;
constexpr const char *kModelEventCount = GERT_ATTR_MODEL_EVENT_COUNT;
constexpr const char *kModelNotifyCount = GERT_ATTR_MODEL_NOTIFY_COUNT;
constexpr const char *kModelAttachedStreamCount = GERT_ATTR_MODEL_ATTACHED_STREAM_COUNT;

constexpr const char *kRefFromNode = GERT_ATTR_REF_FROM_NODE;
constexpr const char *kRefFromIndex = GERT_ATTR_REF_FROM_INDEX;

constexpr const char *kSourceOp = GERT_ATTR_SOURCE_OP;

constexpr const char *kBinaryIds = GERT_ATTR_BINARY_IDS;
constexpr const char *kBinaryPrefix = GERT_ATTR_BINARY_PREFIX;

constexpr const char *kCustomKernelName = GERT_ATTR_CUSTOM_KERNEL_NAME;
constexpr const char *kCustomKernelSoPath = GERT_ATTR_CUSTOM_KERNEL_SO_PATH;
constexpr const char *kCustomRunFuncSymbol = GERT_ATTR_CUSTOM_RUN_FUNC_SYMBOL;
constexpr const char *kCustomOutputsCreatorSymbol = GERT_ATTR_CUSTOM_OUTPUTS_CREATOR_SYMBOL;

// 外部执行图属性是否无条件必填。资源前缀表示按 ID 展开的属性族。
enum class Presence { kRequired, kOptional };
struct AttrSpec {
  const char *key;
  Presence presence;
};

constexpr AttrSpec kGraphAttrs[] = {
    {kModelInputs, Presence::kRequired},      {kModelOutputs, Presence::kRequired},
    {kModelStreamCount, Presence::kOptional}, {kModelEventCount, Presence::kOptional},
    {kModelNotifyCount, Presence::kOptional}, {kModelAttachedStreamCount, Presence::kOptional},
    {kBinaryIds, Presence::kOptional},        {kBinaryPrefix, Presence::kOptional},
};

constexpr AttrSpec kNodeAttrs[] = {
    {kStage, Presence::kOptional},
    {kIndex, Presence::kOptional},
    {kSourceOp, Presence::kOptional},
    {kRefFromNode, Presence::kOptional},  // 输出 TensorDesc 上的引用属性。
    {kRefFromIndex, Presence::kOptional},
    {kCustomKernelName, Presence::kOptional},
    {kCustomKernelSoPath, Presence::kOptional},
    {kCustomRunFuncSymbol, Presence::kOptional},
    {kCustomOutputsCreatorSymbol, Presence::kOptional},
};
}  // namespace attr
}  // namespace gert
#endif  // __cplusplus

#endif  // INC_GRAPH_METADEF_GRAPH_GERT_GRAPH_ATTRIBUTES_H_
