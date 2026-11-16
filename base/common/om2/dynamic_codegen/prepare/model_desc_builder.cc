/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "model_desc_builder.h"

#include <memory>
#include <utility>

#include "common/checker.h"
#include "common/plugin/ge_make_unique_util.h"
#include "exe_graph/runtime/shape.h"
#include "framework/runtime/model_desc.h"

namespace gert {
namespace {
constexpr int64_t kUnknownDimNum = -2;

// 只做形状搬运，不经过 NodeConverterUtils：后者会连带引入整个 lowering 上下文，
// 而公开属性路径本来就只拿到 TensorDesc，不需要 lowering。
Shape GetShapeFromGeShape(const ge::GeShape &ge_shape) {
  Shape shape;
  if (ge_shape.IsUnknownDimNum()) {
    shape.AppendDim(kUnknownDimNum);
    return shape;
  }
  shape.SetDimNum(ge_shape.GetDimNum());
  for (size_t i = 0U; i < ge_shape.GetDimNum(); ++i) {
    shape.SetDim(i, ge_shape.GetDim(i));
  }
  return shape;
}

ShapeRange ConstructShapeRange(const std::vector<std::pair<int64_t, int64_t>> &range) {
  ShapeRange shape_range;
  shape_range.MutableMin().SetDimNum(range.size());
  shape_range.MutableMax().SetDimNum(range.size());
  for (size_t i = 0U; i < range.size(); ++i) {
    shape_range.MutableMin().SetDim(i, range[i].first);
    shape_range.MutableMax().SetDim(i, range[i].second);
  }
  return shape_range;
}

void FillIoDesc(const ModelIoEntry &entry, bg::BufferPool &pool, ModelIoDesc &io_desc) {
  const auto name_id = pool.AddStr(entry.name.c_str());
  io_desc.SetName(reinterpret_cast<const char *>(name_id));
  io_desc.SetDataType(entry.desc.GetDataType());
  io_desc.SetStorageFormat(entry.desc.GetFormat());
  io_desc.SetOriginFormat(entry.desc.GetOriginFormat());
  io_desc.MutableStorageShape() = GetShapeFromGeShape(entry.desc.GetShape());
  io_desc.MutableOriginShape() = GetShapeFromGeShape(entry.desc.GetOriginShape());

  std::vector<std::pair<int64_t, int64_t>> range;
  entry.desc.GetShapeRange(range);
  io_desc.MutableStorageShapeRange() = ConstructShapeRange(range);
  range.clear();
  entry.desc.GetOriginShapeRange(range);
  io_desc.MutableOriginShapeRange() = ConstructShapeRange(range);

  if (!entry.has_aipp_dims) {
    return;
  }
  auto &aipp_shape = io_desc.MutableAippShape();
  aipp_shape.SetDimNum(entry.aipp_dims.size());
  for (size_t i = 0U; i < entry.aipp_dims.size(); ++i) {
    aipp_shape.SetDim(i, entry.aipp_dims[i]);
  }
}
}  // namespace

ge::graphStatus BuildModelDescBuffer(const std::vector<ModelIoEntry> &inputs, const std::vector<ModelIoEntry> &outputs,
                                     const ModelDescResources &resources, bg::BufferPool &pool,
                                     bg::BufferPool::BufId &model_desc_id) {
  GE_ASSERT_TRUE(resources.reusable_stream_num > 0, "Model must reserve at least the caller main stream");
  const size_t input_num = inputs.size();
  const size_t output_num = outputs.size();
  const size_t total_size = ModelDesc::CalcSize(input_num, output_num);
  auto holder = ge::MakeUnique<uint8_t[]>(total_size);
  GE_ASSERT_NOTNULL(holder);

  auto *model_desc = reinterpret_cast<ModelDesc *>(holder.get());
  model_desc->SetInputNum(input_num);
  model_desc->SetOutputNum(output_num);
  model_desc->SetReusableStreamNum(static_cast<size_t>(resources.reusable_stream_num));
  model_desc->SetReusableEventNum(static_cast<size_t>(resources.reusable_event_num));
  model_desc->SetReusableNotifyNum(static_cast<size_t>(resources.reusable_notify_num));
  model_desc->SetAttachedStreamNum(static_cast<size_t>(resources.attached_stream_num));

  size_t mutable_input_num = 0U;
  size_t mutable_output_num = 0U;
  ModelIoDesc *io_descs = model_desc->AllMutableIoDesc(mutable_input_num, mutable_output_num);
  for (size_t i = 0U; i < input_num; ++i) {
    FillIoDesc(inputs[i], pool, io_descs[i]);
  }
  for (size_t i = 0U; i < output_num; ++i) {
    FillIoDesc(outputs[i], pool, io_descs[input_num + i]);
  }
  model_desc_id = pool.AddBuf(holder.get(), total_size);
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
