/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "ge_graph_dsl/op_desc/op_desc_cfg_box.h"
#include "easy_graph/infra/status.h"
#include "ge_graph_dsl/op_desc/op_desc_cfg_repo.h"
#include "framework/common/framework_types_internal.h"
#include "graph/gnode.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"

using ::EG_NS::Status;
using ::GE_NS::OpDescCfg;

GE_NS_BEGIN

OpDescCfgBox::OpDescCfgBox(const OpType &opType) : OpDescCfg(opType) {
  auto opCfg = OpDescCfgRepo::GetInstance().FindBy(opType);
  if (opCfg != nullptr) {
    ::OpDescCfg &base = *this;
    base = (*opCfg);
  }
}

OpDescCfgBox &OpDescCfgBox::InCnt(int in_cnt) {
  this->in_cnt_ = in_cnt;
  return *this;
}

OpDescCfgBox &OpDescCfgBox::OutCnt(int out_cnt) {
  this->out_cnt_ = out_cnt;
  return *this;
}

OpDescCfgBox &OpDescCfgBox::ParentNodeIndex(int node_index) {
  this->Attr(ATTR_NAME_PARENT_NODE_INDEX, node_index);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::StreamId(int stream_id) {
  this->stream_id_ = stream_id;
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, int32_t value) {
  this->Attr(name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, uint32_t value) {
  this->Attr(name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<int32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return Attr(name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<uint32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return Attr(name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<int64_t> &value) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(value);
  attrs_.emplace(std::make_pair(name, attrvalue));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const string &name, const vector<std::string> &value) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(value);
  attrs_.emplace(std::make_pair(name, attrvalue));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const char *value) {
  this->Attr(name, std::string(value));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const NamedAttrs &named_attrs) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(named_attrs);
  attrs_.emplace(std::make_pair(name, attrvalue));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<NamedAttrs> &named_attrs_vec) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(named_attrs_vec);
  attrs_.emplace(std::make_pair(name, attrvalue));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::InputAttr(const size_t index, const std::string &name, int32_t value) {
  this->InputAttr(index, name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::InputAttr(const size_t index, const std::string &name, uint32_t value) {
  this->InputAttr(index, name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::InputAttr(const size_t index, const std::string &name, const std::vector<int32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return InputAttr(index, name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::InputAttr(const size_t index, const std::string &name, const std::vector<uint32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return InputAttr(index, name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::InputAttr(const size_t index, const std::string &name, const std::vector<int64_t> &value) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(value);
  auto kv = std::make_pair(name, attrvalue);
  auto kvs = input_attrs_.find(index);
  if (kvs == input_attrs_.end()) {
    input_attrs_[index] = {kv};
  } else {
    kvs->second.emplace(kv);
  }
  return *this;
}

OpDescCfgBox &OpDescCfgBox::InputAttr(const size_t index, const std::string &name, const char *value) {
  this->Attr(name, std::string(value));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::OutputAttr(const size_t index, const std::string &name, int32_t value) {
  this->OutputAttr(index, name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::OutputAttr(const size_t index, const std::string &name, uint32_t value) {
  this->OutputAttr(index, name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::OutputAttr(const size_t index, const std::string &name, const std::vector<int32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return OutputAttr(index, name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::OutputAttr(const size_t index, const std::string &name,
                                       const std::vector<uint32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return OutputAttr(index, name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::OutputAttr(const size_t index, const std::string &name, const std::vector<int64_t> &value) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(value);
  auto kv = std::make_pair(name, attrvalue);
  auto kvs = input_attrs_.find(index);
  if (kvs == input_attrs_.end()) {
    input_attrs_[index] = {kv};
  } else {
    kvs->second.emplace(kv);
  }
  return *this;
}

OpDescCfgBox &OpDescCfgBox::OutputAttr(const size_t index, const std::string &name, const char *value) {
  this->Attr(name, std::string(value));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Weight(GeTensorPtr &tensor_ptr) {
  this->Attr<GeTensor>(ATTR_NAME_WEIGHTS, *tensor_ptr);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::TensorDesc(Format format, DataType data_type, std::vector<int64_t> shape) {
  default_tensor_.format_ = format;
  default_tensor_.data_type_ = data_type;
  default_tensor_.shape_ = shape;
  return *this;
}

void OpDescCfgBox::UpdateAttrs(OpDescPtr &op_desc) const {
  std::for_each(attrs_.begin(), attrs_.end(),
                [&op_desc](const auto &attr) { op_desc->SetAttr(attr.first, attr.second); });
  std::for_each(input_attrs_.begin(), input_attrs_.end(), [&op_desc](const auto &attr) {
    const size_t index = attr.first;
    const auto &kvs = attr.second;
    auto tensor = op_desc->MutableInputDesc(index);
    for (auto &kv : kvs) {
      tensor->SetAttr(kv.first, kv.second);
    }
  });
  std::for_each(output_attrs_.begin(), output_attrs_.end(), [&op_desc](const auto &attr) {
    const size_t index = attr.first;
    const auto &kvs = attr.second;
    auto tensor = op_desc->MutableOutputDesc(index);
    for (auto &kv : kvs) {
      tensor->SetAttr(kv.first, kv.second);
    }
  });
}

void OpDescCfgBox::AddDefaultConstWeights(OpDescPtr &op_desc) const {
  if ((GetType() != CONSTANT) || op_desc->HasAttr(ATTR_NAME_WEIGHTS)) {
    return;
  }
  // Graph prepare validates the weights (value) attr of Const nodes: data size must match shape/dtype.
  // Bare Const cfgs without explicit weights get content-agnostic zero weights matching the default desc.
  GeTensorDesc weight_desc(ge::GeShape(default_tensor_.shape_), default_tensor_.format_, default_tensor_.data_type_);
  int64_t tensor_size = 0;
  if (TensorUtils::GetTensorSizeInBytes(weight_desc, tensor_size) != GRAPH_SUCCESS) {
    return;
  }
  const auto data = std::vector<uint8_t>(static_cast<size_t>(tensor_size), 0U);
  const GeTensorPtr weight = std::make_shared<GeTensor>(weight_desc, data.data(), data.size());
  (void)AttrUtils::SetTensor(op_desc, ATTR_NAME_WEIGHTS, weight);
}

OpDescPtr OpDescCfgBox::Build(const ::EG_NS::NodeId &id) const {
  auto opPtr = std::make_shared<OpDesc>(id, GetType());
  GeTensorDesc tensor_desc(ge::GeShape(default_tensor_.shape_), default_tensor_.format_, default_tensor_.data_type_);
  tensor_desc.SetOriginShape(ge::GeShape(default_tensor_.shape_));
  if (in_names_.empty()) {
    for (int i = 0; i < in_cnt_; i++) {
      opPtr->AddInputDesc(tensor_desc);
    }
  } else {
    for (const auto &name : in_names_) {
      opPtr->AddInputDesc(name, tensor_desc);
    }
  }

  if (out_names_.empty()) {
    for (int i = 0; i < out_cnt_; i++) {
      opPtr->AddOutputDesc(tensor_desc);
    }
  } else {
    for (const auto &name : out_names_) {
      opPtr->AddOutputDesc(name, tensor_desc);
    }
  }

  if (stream_id_ != -1) {
    opPtr->SetStreamId(stream_id_);
  }

  UpdateAttrs(opPtr);
  AddDefaultConstWeights(opPtr);
  return opPtr;
}
OpDescCfgBox &OpDescCfgBox::InNames(std::initializer_list<std::string> names) {
  in_names_.insert(in_names_.end(), names.begin(), names.end());
  in_cnt_ = static_cast<int32_t>(in_names_.size());
  return *this;
}
OpDescCfgBox &OpDescCfgBox::OutNames(std::initializer_list<std::string> names) {
  out_names_.insert(out_names_.end(), names.begin(), names.end());
  out_cnt_ = static_cast<int32_t>(out_names_.size());
  return *this;
}

GE_NS_END
