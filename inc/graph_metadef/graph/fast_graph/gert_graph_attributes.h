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

/* 输入类别键：tensor 表示已有 RT2 Tensor 对象地址，pointer 表示消费 Kernel 解释的借用指针。 */
#define GERT_ATTR_INPUT_KIND "gert.input.kind"
#define GERT_INPUT_KIND_TENSOR "tensor"
#define GERT_INPUT_KIND_POINTER "pointer"

/* 图输入的序号键，沿用 ES 既有 index 协议。 */
#define GERT_ATTR_INDEX "index"

/* 模型用户 IO 描述：list Tensor（无数据，TensorDesc 携带描述）与同序的 list string 名称。 */
#define GERT_ATTR_MODEL_INPUTS "gert.model.inputs"
#define GERT_ATTR_MODEL_OUTPUTS "gert.model.outputs"
#define GERT_ATTR_MODEL_INPUT_NAMES "gert.model.input_names"
#define GERT_ATTR_MODEL_OUTPUT_NAMES "gert.model.output_names"

/* 统一资源槽数：int64。 */
#define GERT_ATTR_MODEL_STREAM_COUNT "gert.model.stream_count"
#define GERT_ATTR_MODEL_EVENT_COUNT "gert.model.event_count"
#define GERT_ATTR_MODEL_NOTIFY_COUNT "gert.model.notify_count"

/* 加载配置：device_id 必填；stream 可缺省，由内部创建。 */
#define GERT_ATTR_DEVICE_ID "gert.device_id"
#define GERT_ATTR_STREAM "gert.stream"

/* 输出执行值引用协议：节点名（字符串）+ 输出索引（非负 int64）成对设置。 */
#define GERT_ATTR_REF_FROM_NODE "RefFromNode"
#define GERT_ATTR_REF_FROM_INDEX "RefFromIndex"

/* 来源算子描述：ID 列表 + 按 ID 分组的字段。ID 为 [A-Za-z0-9_-]+，仅在同图同类表内唯一。 */
#define GERT_ATTR_SOURCE_IDS "gert.source.ids"
#define GERT_ATTR_SOURCE_ID "gert.source.id"
/* 以下前缀后接 "<id>.<字段>"，字段含义见设计 3.5.4。 */
#define GERT_ATTR_SOURCE_PREFIX "gert.source."

/* SO 资源：用途区分 space_registry / kernel / tiling / custom，未知用途报错。 */
#define GERT_ATTR_SO_IDS "gert.so.ids"
#define GERT_ATTR_SO_PREFIX "gert.so."
#define GERT_ATTR_NODE_SO_IDS "gert.so.ids"

/* 设备二进制（.o）：名称 + 内容，内部核对生成代码的 binary key。 */
#define GERT_ATTR_BINARY_IDS "gert.binary.ids"
#define GERT_ATTR_BINARY_PREFIX "gert.binary."
#define GERT_ATTR_NODE_BINARY_ID "gert.binary.id"

/* 权重资源：内置 / 外置文件 / 外置内存，与 Init/Main 阶段无关。 */
#define GERT_ATTR_WEIGHT_IDS "gert.weight.ids"
#define GERT_ATTR_WEIGHT_PREFIX "gert.weight."
#define GERT_ATTR_NODE_WEIGHT_ID "gert.weight.id"

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

constexpr const char *kInputKind = GERT_ATTR_INPUT_KIND;
constexpr const char *kInputKindTensor = GERT_INPUT_KIND_TENSOR;
constexpr const char *kInputKindPointer = GERT_INPUT_KIND_POINTER;

constexpr const char *kIndex = GERT_ATTR_INDEX;

constexpr const char *kModelInputs = GERT_ATTR_MODEL_INPUTS;
constexpr const char *kModelOutputs = GERT_ATTR_MODEL_OUTPUTS;
constexpr const char *kModelInputNames = GERT_ATTR_MODEL_INPUT_NAMES;
constexpr const char *kModelOutputNames = GERT_ATTR_MODEL_OUTPUT_NAMES;
constexpr const char *kModelStreamCount = GERT_ATTR_MODEL_STREAM_COUNT;
constexpr const char *kModelEventCount = GERT_ATTR_MODEL_EVENT_COUNT;
constexpr const char *kModelNotifyCount = GERT_ATTR_MODEL_NOTIFY_COUNT;

constexpr const char *kDeviceId = GERT_ATTR_DEVICE_ID;
constexpr const char *kStream = GERT_ATTR_STREAM;

constexpr const char *kRefFromNode = GERT_ATTR_REF_FROM_NODE;
constexpr const char *kRefFromIndex = GERT_ATTR_REF_FROM_INDEX;

constexpr const char *kSourceIds = GERT_ATTR_SOURCE_IDS;
constexpr const char *kSourceId = GERT_ATTR_SOURCE_ID;
constexpr const char *kSourcePrefix = GERT_ATTR_SOURCE_PREFIX;

constexpr const char *kSoIds = GERT_ATTR_SO_IDS;
constexpr const char *kSoPrefix = GERT_ATTR_SO_PREFIX;

constexpr const char *kBinaryIds = GERT_ATTR_BINARY_IDS;
constexpr const char *kBinaryPrefix = GERT_ATTR_BINARY_PREFIX;
constexpr const char *kNodeBinaryId = GERT_ATTR_NODE_BINARY_ID;

constexpr const char *kWeightIds = GERT_ATTR_WEIGHT_IDS;
constexpr const char *kWeightPrefix = GERT_ATTR_WEIGHT_PREFIX;
constexpr const char *kNodeWeightId = GERT_ATTR_NODE_WEIGHT_ID;
}  // namespace attr
}  // namespace gert
#endif  // __cplusplus

#endif  // INC_GRAPH_METADEF_GRAPH_GERT_GRAPH_ATTRIBUTES_H_
