/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_NORMAL_GRAPH_GNODE_IMPL_H_
#define METADEF_GRAPH_NORMAL_GRAPH_GNODE_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "graph/gnode.h"

namespace ge {
// NodeImpl：GNode 公共门面的多态状态层。类别即动态类型——计算/执行节点后端各自继承本基类，
// GNode 经 impl_ 虚调用分发，不持有 kind 字段与路由分支；节点引用晚绑定由 NodeAdapter 完成
// （默认构造即计算节点、空引用）。门面层保留公共对象检查（impl_ 判空），正文全部在后端实现。
class NodeImpl {
 public:
  NodeImpl(const NodeImpl &) = delete;
  NodeImpl &operator=(const NodeImpl &) = delete;
  virtual ~NodeImpl() = default;

  virtual graphStatus GetType(AscendString &type) const = 0;
  virtual graphStatus GetName(AscendString &name) const = 0;
  virtual std::pair<GNodePtr, int32_t> GetInDataNodesAndPortIndexs(const int32_t index) const = 0;
  // 控制节点合并入口：门面以 input 参数组合出 GetInControlNodes/GetOutControlNodes
  virtual std::vector<GNodePtr> GetControlNodes(const bool input) const = 0;
  virtual std::vector<std::pair<GNodePtr, int32_t>> GetOutDataNodesAndPortIndexs(const int32_t index) const = 0;
  virtual graphStatus GetInputConstData(const int32_t index, Tensor &data) const = 0;
  virtual graphStatus GetInputIndexByName(const AscendString &name, int32_t &index) const = 0;
  virtual graphStatus GetOutputIndexByName(const AscendString &name, int32_t &index) const = 0;
  virtual graphStatus GetDynamicInputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const = 0;
  virtual graphStatus GetDynamicOutputIndexesByName(const AscendString &name, std::vector<int32_t> &indexes) const = 0;
  virtual size_t GetInputsSize() const = 0;
  virtual size_t GetOutputsSize() const = 0;
  // 端口描述合并入口：门面以 input 参数组合出 Get/UpdateInputDesc 与 Get/UpdateOutputDesc
  virtual graphStatus GetDesc(const int32_t index, TensorDesc &tensor_desc, const bool input) const = 0;
  virtual graphStatus UpdateDesc(const int32_t index, const TensorDesc &tensor_desc, const bool input) const = 0;

  // 属性接口：模板不能为虚，19+19 逐类型虚重载显式声明（含 AscendString/AttrValue 系）
  virtual graphStatus GetAttr(const AscendString &name, int64_t &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, int32_t &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, uint32_t &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, float32_t &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, AscendString &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, bool &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, Tensor &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<AscendString> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<bool> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, OpBytes &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, ge::DataType &attr_value) const = 0;
  virtual graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, int64_t &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, int32_t &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, uint32_t &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, float32_t &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, AscendString &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, bool &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, Tensor &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<int64_t> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<int32_t> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<uint32_t> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<float32_t> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<AscendString> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<bool> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<Tensor> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, OpBytes &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<std::vector<int64_t>> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, std::vector<ge::DataType> &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, ge::DataType &attr_value) const = 0;
  virtual graphStatus SetAttr(const AscendString &name, AttrValue &attr_value) const = 0;

  virtual graphStatus GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const = 0;
  virtual graphStatus SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const = 0;
  virtual bool HasAttr(const AscendString &name) const = 0;

  virtual graphStatus GetSubgraph(const uint32_t index, GraphPtr &graph) const = 0;
  virtual graphStatus GetAllSubgraphs(std::vector<GraphPtr> &graph_list) const = 0;
  virtual graphStatus SetSubgraph(const AscendString &subgraph_ir_name, const Graph &subgraph) const = 0;
  virtual graphStatus SetSubgraphs(const AscendString &subgraph_ir_name, const std::vector<Graph> &subgraphs) const = 0;
  // 端口属性合并入口：input 选择输入/输出端口
  virtual graphStatus PortAttr(const AscendString &name, const uint32_t index, AttrValue &attr_value,
                               const bool input) const = 0;
  virtual graphStatus SetPortAttr(const AscendString &name, const uint32_t index, const AttrValue &attr_value,
                                  const bool input) const = 0;

 protected:
  NodeImpl() = default;
};
}  // namespace ge
#endif  // METADEF_GRAPH_NORMAL_GRAPH_GNODE_IMPL_H_
