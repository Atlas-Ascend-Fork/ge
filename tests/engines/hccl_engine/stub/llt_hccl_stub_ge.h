/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef __LLT_HCCL_STUB_GE_H__
#define __LLT_HCCL_STUB_GE_H__

#include "graph/compute_graph.h"
#include <map>
#include <unordered_map>
#include "graph_metadef/common/ge_common/util.h"
#include "hccl_stub.h"
#include "llt_hccl_stub_ge_compute_graph.h"

namespace ge {
// llt_hccl_stub_ge.cc 中的 stub 全局状态, 供用例精确构造属性场景并在用例结束时还原
extern std::map<std::string, std::string> dummyStringMAP;
extern std::map<std::string, int64_t> dummy_Int64MAP;
extern bool dummyHasAttr_group;
extern GeTensorDescPtr dummy_inputsDesc;

class AnchorImpl {
 public:
  AnchorImpl(const NodePtr &owner_node, int idx);
  ~AnchorImpl() = default;
  size_t GetPeerAnchorsSize() const;
  Anchor::Vistor<AnchorPtr> GetPeerAnchors(const std::shared_ptr<ConstAnchor> &anchor_ptr) const;
  AnchorPtr GetFirstPeerAnchor() const;
  NodePtr GetOwnerNode() const;
  int GetIdx() const;
  void SetIdx(int index);

  // All peer anchors connected to current anchor
  vector<std::weak_ptr<Anchor>> peer_anchors_;
  // The owner node of anchor
  std::weak_ptr<Node> owner_node_;
  // The index of current anchor
  int idx_;
};

class Node::NodeImpl {
 public:
  NodeImpl() = default;
  NodeImpl(const OpDescPtr &op, const ComputeGraphPtr &owner_graph);
  ~NodeImpl() = default;
  graphStatus Init(const NodePtr &node);
  std::string GetName() const;
  std::string GetType() const;
  bool NodeAttrsAreEqual(const NodeImpl &r_node) const;
  bool NodeMembersAreEqual(const NodeImpl &r_node) const;
  bool NodeAnchorIsEqual(const AnchorPtr &left_anchor, const AnchorPtr &right_anchor, size_t i) const;

  graphStatus AddLinkFrom(const NodePtr &input_node, const NodePtr &owner_node);
  graphStatus AddLinkFrom(const uint32_t &index, const NodePtr &input_node, const NodePtr &owner_node);
  graphStatus AddLinkFromForParse(const NodePtr &input_node, const NodePtr &owner_node);
  graphStatus AddLinkFrom(const string &name, const NodePtr &input_node, const NodePtr &owner_node);

  ComputeGraphPtr GetOwnerComputeGraph() const;
  ComputeGraph *GetOwnerComputeGraphBarePtr() const;
  graphStatus SetOwnerComputeGraph(const ComputeGraphPtr &graph);
  graphStatus ClearOwnerGraph(const ComputeGraphPtr &graph);

  Node::Vistor<InDataAnchorPtr> GetAllInDataAnchors(const ConstNodePtr &owner_node) const;
  Node::Vistor<OutDataAnchorPtr> GetAllOutDataAnchors(const ConstNodePtr &owner_node) const;
  uint32_t GetAllInDataAnchorsSize() const;
  uint32_t GetAllOutDataAnchorsSize() const;
  Node::Vistor<AnchorPtr> GetAllInAnchors(const ConstNodePtr &owner_node) const;
  Node::Vistor<AnchorPtr> GetAllOutAnchors(const ConstNodePtr &owner_node) const;
  InDataAnchorPtr GetInDataAnchor(int idx) const;
  AnchorPtr GetInAnchor(int idx) const;
  AnchorPtr GetOutAnchor(int idx) const;
  OutDataAnchorPtr GetOutDataAnchor(int idx) const;
  InControlAnchorPtr GetInControlAnchor() const;
  OutControlAnchorPtr GetOutControlAnchor() const;

  Node::Vistor<NodePtr> GetInNodes(const ConstNodePtr &owner_node) const;
  bool IsAllInNodesSeen(std::unordered_set<Node *> &nodes_seen) const;
  Node::Vistor<NodePtr> GetInDataNodes(const ConstNodePtr &owner_node) const;
  Node::Vistor<NodePtr> GetInControlNodes(const ConstNodePtr &owner_node) const;
  Node::Vistor<NodePtr> GetOutNodes(const ConstNodePtr &owner_node) const;
  Node::Vistor<NodePtr> GetInAllNodes(const ConstNodePtr &owner_node) const;
  Node::Vistor<NodePtr> GetOutDataNodes(const ConstNodePtr &owner_node) const;
  uint32_t GetOutDataNodesSize() const;
  Node::Vistor<NodePtr> GetOutControlNodes(const ConstNodePtr &owner_node) const;
  Node::Vistor<NodePtr> GetOutAllNodes(const ConstNodePtr &owner_node) const;

  graphStatus InferShapeAndType(const ConstNodePtr &owner_node) const;
  graphStatus InferOriginFormat(const ConstNodePtr &owner_node) const;
  graphStatus Verify(const ConstNodePtr &owner_node) const;
  OpDescPtr GetOpDesc() const;
  graphStatus UpdateOpDesc(const OpDescPtr &op_desc);
  Node::Vistor<std::pair<NodePtr, OutDataAnchorPtr>> GetInDataNodesAndAnchors(const ConstNodePtr &owner_node) const;
  Node::Vistor<std::pair<NodePtr, InDataAnchorPtr>> GetOutDataNodesAndAnchors(const ConstNodePtr &owner_node) const;

  void AddSendEventId(uint32_t event_id) {
    send_event_id_list_.push_back(event_id);
  }
  void AddRecvEventId(uint32_t event_id) {
    recv_event_id_list_.push_back(event_id);
  }

  const std::vector<uint32_t> &GetSendEventIdList() const {
    return send_event_id_list_;
  }
  const std::vector<uint32_t> &GetRecvEventIdList() const {
    return recv_event_id_list_;
  }

  void GetFusionInputFlowList(kFusionDataFlowVec_t &fusion_input_list) {
    fusion_input_list = fusion_input_dataflow_list_;
  }
  void GetFusionOutputFlowList(kFusionDataFlowVec_t &fusion_output_list) {
    fusion_output_list = fusion_output_dataflow_list_;
  }
  void SetFusionInputFlowList(kFusionDataFlowVec_t &fusion_input_list) {
    fusion_input_dataflow_list_ = fusion_input_list;
  }
  void SetFusionOutputFlowList(kFusionDataFlowVec_t &fusion_output_list) {
    fusion_output_dataflow_list_ = fusion_output_list;
  }

  bool GetHostNode() const {
    return host_node_;
  }
  void SetHostNode(bool is_host) {
    host_node_ = is_host;
  }

  void SetOrigNode(const NodePtr &orignode) {
    orig_node_ = orignode;
  }
  NodePtr GetOrigNode() {
    return orig_node_;
  }

  friend class NodeUtils;
  friend class TuningUtils;
  friend class OnnxUtils;
  OpDescPtr op_;
  std::weak_ptr<ComputeGraph> owner_graph_;
  vector<InDataAnchorPtr> in_data_anchors_;
  vector<OutDataAnchorPtr> out_data_anchors_;
  InControlAnchorPtr in_control_anchor_;
  OutControlAnchorPtr out_control_anchor_;
  map<string, GeAttrValue> attrs_;  // lint !e1073
  bool has_init_{false};
  bool host_node_{false};
  bool anchor_status_updated_{false};
  std::vector<uint32_t> send_event_id_list_;
  std::vector<uint32_t> recv_event_id_list_;

  kFusionDataFlowVec_t fusion_input_dataflow_list_;
  kFusionDataFlowVec_t fusion_output_dataflow_list_;
  NodePtr orig_node_{nullptr};
};

class IRMetaData {
 public:
  explicit IRMetaData(const std::string &op_name) : op_name_(op_name) {};
  IRMetaData() = default;
  void SetOpName(const std::string &op_name) {
    op_name_ = op_name;
  }
  void AppendIrInput(std::string name, IrInputType input_type);
  const std::vector<std::pair<std::string, IrInputType>> &GetIrInputs() const;
  IrInputType GetIrInputType(const std::string &name) const;

  void AppendIrOutput(std::string name, IrOutputType output_type);
  const std::vector<std::pair<std::string, IrOutputType>> &GetIrOutputs() const;

  graphStatus AddRegisterInputName(const std::string &name);
  std::vector<std::string> GetRegisterInputName() const;

  graphStatus AddRegisterOptionalInputName(const std::string &name);
  std::set<std::string> GetOptionalInputName() const;
  bool IsOptionalInput(const std::string &name) const;

  graphStatus AddRegisterOutputName(const std::string &name);
  std::vector<std::string> GetRegisterOutputName() const;

  void AppendIrAttrName(std::string name);
  const std::vector<std::string> &GetIrAttrNames() const;

  void RegisterSubgraphIrName(const std::string &name, const SubgraphType type);
  const std::map<std::string, SubgraphType> &GetSubgraphIrNames() const;
  SubgraphType GetSubgraphTypeByIrName(const std::string &name) const;

  graphStatus VerifyIR() const;
  graphStatus VerifyDataTypeSymbol() const;

  bool operator==(const IRMetaData &other) const;

 private:
  bool IsOutputSymbolValid(const std::string &output_symbol) const;
  std::string op_name_;
  std::vector<std::pair<std::string, IrInputType>> ir_inputs_;
  std::vector<std::pair<std::string, IrOutputType>> ir_outputs_;
  std::vector<std::string> register_input_name_;  // todo need to deprecate
  std::set<std::string> optional_input_names_;    // todo need to deprecate
  std::vector<std::string> register_output_name_;
  std::vector<std::string> ir_attr_names_;
  // subgraph ir names to type, for a `if` operator:
  // then_branch: static
  // else_branch: static
  // or for a `case` op:
  // branches: dynamic
  std::map<std::string, SubgraphType> subgraph_ir_names_to_type_;
  std::set<std::string> register_unique_name_;
};

class OpMetadata {
 public:
  OpMetadata() = default;
  ~OpMetadata() = default;
  OpMetadata(std::string name, std::string type) : name_(std::move(name)), type_(std::move(type)), ir_meta_(name) {}
  int64_t GetId() const {
    return id_;
  }
  int64_t GetStreamId() const {
    return stream_id_;
  }
  const std::vector<std::string> &GetInputNames() const {
    return input_names_;
  }
  const std::vector<std::string> &GetSrcNames() const {
    return src_names_;
  }
  const std::vector<int64_t> &GetSrcIndexes() const {
    return src_indexes_;
  }
  const std::vector<std::string> &GetDstNames() const {
    return dst_names_;
  }
  const std::vector<int64_t> &GetDstIndexes() const {
    return dst_indexes_;
  }
  const std::vector<int64_t> &GetInputOffsets() const {
    return input_offsets_;
  }
  const std::vector<int64_t> &GetOutputOffsets() const {
    return output_offsets_;
  }
  const std::vector<bool> &GetIsInputConsts() const {
    return is_input_consts_;
  }
  const std::vector<std::string> &GetSubgraphNames() const {
    return subgraph_names_;
  }
  void AddSubGraphName(const std::string &name) {
    subgraph_names_.push_back(name);
  }
  void ClearSubgraphNames() {
    subgraph_names_.clear();
  }
  void SetOpName(std::string name) {
    name_ = std::move(name);
    ir_meta_.SetOpName(name);
  }

 private:
  friend class OpDescImpl;
  std::string name_;
  std::string type_;
  std::vector<std::string> inputs_;
  bool has_out_attr_{false};
  int64_t id_{0};
  int64_t stream_id_{0};
  std::vector<std::string> input_names_;
  std::vector<std::string> src_names_;
  std::vector<int64_t> src_indexes_;
  std::vector<std::string> dst_names_;
  std::vector<int64_t> dst_indexes_;
  std::vector<int64_t> input_offsets_;
  std::vector<int64_t> output_offsets_;
  std::vector<bool> is_input_consts_;
  std::vector<std::string> subgraph_names_;
  IRMetaData ir_meta_;
};

class OpDescImpl {
 public:
  OpDescImpl();
  OpDescImpl(const std::string &name, const std::string &type);
  OpDescImpl(const ProtoMsgOwner &proto_msg_owner, ge::proto::OpDef *op_def);

  ~OpDescImpl() = default;

  string GetName() const;
  void SetName(const std::string &name);
  string GetType() const;
  void SetType(const string &type);

  graphStatus AddInputDesc(const ge::GeTensorDesc &input_desc);
  graphStatus AddInputDesc(uint32_t index, const ge::GeTensorDesc &input_desc);
  graphStatus AddInputDesc(const string &name, const ge::GeTensorDesc &input_desc);
  graphStatus AddInputDescMiddle(const string &name, const unsigned int num, size_t index);
  graphStatus AddOutputDescMiddle(const string &name, const unsigned int num, size_t index);
  graphStatus AddInputDescForward(const string &name, const unsigned int num);
  graphStatus AddOutputDescForward(const string &name, const unsigned int num);
  graphStatus AddOptionalInputDesc(const string &name, const ge::GeTensorDesc &input_desc);

  graphStatus UpdateInputDesc(uint32_t index, const ge::GeTensorDesc &tensor_Desc);
  graphStatus UpdateInputDesc(const string &name, const ge::GeTensorDesc &tensor_Desc);

  bool OpDescMembersAreEqual(const OpDescImpl &r_op_desc) const;
  bool OpDescAttrsAreEqual(const OpDescImpl &r_op_desc) const;
  bool OpDescGenTensorDescsAreEqual(const OpDescImpl &r_op_desc) const;

  bool operator==(const OpDescImpl &r_op_desc) const;

  bool InputIsSet(const string &name) const;

  const GeTensorDesc &GetInputDesc(uint32_t index) const;
  const GeTensorDesc &GetInputDesc(const string &name) const;
  GeTensorDescPtr MutableInputDesc(uint32_t index) const;
  GeTensorDescPtr MutableInputDesc(const string &name) const;
  OpDesc::Vistor<string> GetAllInputNames(const ConstOpDescPtr &op_desc) const;

  void SetOpKernelLibName(const std::string &name);
  std::string GetOpKernelLibName() const;
  void SetOpEngineName(const std::string &name);
  std::string GetOpEngineName() const;

  OpDesc::Vistor<GeTensorDesc> GetAllInputsDesc(const ConstOpDescPtr &op_desc) const;
  OpDesc::Vistor<GeTensorDescPtr> GetAllInputsDescPtr(const ConstOpDescPtr &op_desc) const;

  size_t GetInputsSize() const;
  size_t GetAllInputsSize() const;

  graphStatus AddOutputDesc(const ge::GeTensorDesc &output_desc);
  graphStatus AddOutputDesc(const string &name, const ge::GeTensorDesc &output_desc);
  graphStatus UpdateOutputDesc(uint32_t index, const ge::GeTensorDesc &tensor_Desc);
  graphStatus UpdateOutputDesc(const string &name, const ge::GeTensorDesc &tensor_Desc);
  const GeTensorDesc &GetOutputDesc(uint32_t index) const;
  const GeTensorDesc &GetOutputDesc(const string &name) const;
  GeTensorDescPtr MutableOutputDesc(uint32_t index) const;
  GeTensorDescPtr MutableOutputDesc(const string &name) const;

  uint32_t GetAllOutputsDescSize() const;
  OpDesc::Vistor<GeTensorDesc> GetAllOutputsDesc(const ConstOpDescPtr &op_desc) const;
  OpDesc::Vistor<GeTensorDescPtr> GetAllOutputsDescPtr(const ConstOpDescPtr &op_desc) const;
  ConstGeTensorDescPtr GetOutputDescPtr(uint32_t index) const;
  size_t GetOutputsSize() const;

  ConstGeTensorDescPtr GetInputDescPtr(uint32_t index) const;
  ConstGeTensorDescPtr GetInputDescPtrDfault(uint32_t index) const;
  ConstGeTensorDescPtr GetInputDescPtr(const string &name) const;

  graphStatus AddRegisterInputName(const std::string &name);
  vector<string> GetRegisterInputName() const;

  graphStatus AddDynamicInputDesc(const string &name, const unsigned int num, bool is_push_back);
  graphStatus AddDynamicInputDescByIndex(const string &name, const unsigned int num, size_t index);

  graphStatus AddRegisterOutputName(const string &name);
  vector<string> GetRegisterOutputName() const;

  graphStatus AddDynamicOutputDesc(const string &name, const unsigned int num, bool is_push_back);
  bool IsOptionalInput(const string &name) const;
  bool IsOptionalInput(uint32_t index) const;
  std::map<string, uint32_t> GetAllInputName() const;
  std::map<string, uint32_t> GetAllOutputName();
  std::map<string, uint32_t> &MutableAllInputName();
  std::map<string, uint32_t> &MutableAllOutputName();
  bool UpdateInputName(std::map<string, uint32_t> input_name_idx);
  bool UpdateOutputName(std::map<string, uint32_t> output_name_idx);

  std::function<graphStatus(Operator &)> GetInferFunc() const;
  std::function<graphStatus(Operator &)> GetVerifyFunc() const;
  void AddInferFunc(const std::function<graphStatus(Operator &)> &func);
  std::function<graphStatus(Operator &)> GetInferFormatFunc() const;
  void AddInferFormatFunc(const std::function<graphStatus(Operator &)> &func);
  void AddVerifierFunc(const std::function<graphStatus(Operator &)> &func);

  graphStatus InferShapeAndType(const OpDescPtr &op_desc);
  graphStatus DefaultInferFormat(const ConstOpDescPtr &op_desc);
  graphStatus OpVerify(const OpDescPtr &op_desc);

  string GetInputNameByIndex(uint32_t index) const;
  int GetInputIndexByName(const string &name) const;
  int GetValidInputIndexByName(const string &name) const;
  string GetValidInputNameByIndex(uint32_t index) const;

  string GetOutputNameByIndex(uint32_t index) const;
  int GetOutputIndexByName(const string &name) const;

  ProtoAttrMap &MutableAttrMap();
  ConstProtoAttrMap &GetAttrMap() const;

  IRMetaData &MutableIRMeta();
  const IRMetaData &GetIRMeta() const;

  void SetId(int64_t id);
  int64_t GetId() const;

  void SetStreamId(int64_t stream_id);
  int64_t GetStreamId() const;

  void SetInputName(const vector<string> &input_name);
  vector<string> GetInputName() const;

  void SetSrcName(const vector<string> &src_name);
  vector<string> GetSrcName() const;

  void SetSrcIndex(const vector<int64_t> &src_index);
  vector<int64_t> GetSrcIndex() const;

  void SetInputOffset(const vector<int64_t> &input);
  vector<int64_t> GetInputOffset() const;

  void SetOutputOffset(const vector<int64_t> &output);
  vector<int64_t> GetOutputOffset() const;

  void SetDstName(const vector<string> &dst_name);
  vector<string> GetDstName() const;

  void SetDstIndex(const vector<int64_t> &dst_index);
  vector<int64_t> GetDstIndex() const;

  void SetWorkspace(const vector<int64_t> &workspace);
  vector<int64_t> GetWorkspace() const;

  void SetWorkspaceBytes(const vector<int64_t> &workspace_bytes);
  vector<int64_t> GetWorkspaceBytes() const;

  void SetOpInferDepends(const vector<string> &depend_names) {}

  void SetIsInputConst(const vector<bool> &is_input_const);
  vector<bool> GetIsInputConst() const;

  graphStatus RestoreInputNameIdx(const string &name, const int &index);
  graphStatus RestoreOutputNameIdx(const string &name, const int &index);

  graphStatus CallInferFunc(Operator &op, const OpDescPtr &op_desc);
  graphStatus CallInferFormatFunc(Operator &op, const ConstOpDescPtr &op_desc);

  std::string GetSubgraphInstanceName(uint32_t index) const;
  const std::vector<std::string> &GetSubgraphInstanceNames() const;
  void RemoveSubgraphInstanceName(const std::string &name);
  graphStatus AddSubgraphName(const std::string &name);
  const std::map<std::string, uint32_t> &GetSubgraphNameIndexes() const;
  graphStatus SetSubgraphInstanceName(uint32_t index, const std::string &name);

  void RegisterSubgraphIrName(const string &name, SubgraphType type);
  const std::map<std::string, SubgraphType> &GetSubgraphIrNames() const;
  SubgraphType GetSubgraphTypeByIrName(const std::string &name) const;
  graphStatus GetSubgraphNameByInstanceName(const std::string &instance_name, std::string &subgraph_name) const;
  graphStatus InferDataSlice(const OpDescPtr &op_desc);

  friend class AttrUtils;
  friend class OpDescUtils;
  friend class ModelSerializeImp;
  friend class OnnxUtils;
  friend class GraphUtils;
  GeIrProtoHelper<ge::proto::OpDef> op_def_;
  std::vector<std::string> subgraph_instance_names_;

  // subgraph names to index, for a `if` operator:
  // then_branch: 0
  // else_branch: 1
  // or for a `case` node:
  // branches0: 0
  // branches1: 1
  // branches2: 2
  std::map<std::string, uint32_t> subgraph_names_to_index_;

  // subgraph ir names to type, for a `if` operator:
  // then_branch: static
  // else_branch: static
  // or for a `case` op:
  // branches: dynamic
  std::map<std::string, SubgraphType> subgraph_ir_names_to_type_;

  vector<GeTensorDescPtr> inputs_desc_{};
  map<string, uint32_t> input_name_idx_{};
  vector<string> register_input_name_{};
  std::set<string> optional_input_names_{};
  vector<GeTensorDescPtr> outputs_desc_{};
  map<string, uint32_t> output_name_idx_{};
  vector<string> register_output_name_{};
  std::function<graphStatus(Operator &)> infer_func_ = nullptr;
  std::function<graphStatus(Operator &)> infer_format_func_ = nullptr;
  std::function<graphStatus(Operator &)> verifier_func_ = nullptr;
  std::function<graphStatus(Operator &)> infer_data_slice_func_ = nullptr;
  string op_kernel_lib_name_;
  string engine_name_;
  OpMetadata meta_data_;
};

class GeTensorDescImpl {
 public:
  GeTensorDescImpl();
  GeTensorDescImpl(GeShape shape, Format format, DataType dt);
  GeTensorDescImpl(const GeTensorDescImpl &desc);
  GeTensorDescImpl(GeTensorDescImpl &&desc);
  GeTensorDescImpl(const ProtoMsgOwner &proto_owner, proto::TensorDescriptor *proto_msg);
  ~GeTensorDescImpl() = default;

  void Init();
  GeShape &ShapeReference() const;

  bool GeTensorDescAttrsAreEqual(const GeTensorDescImpl &r_ge_tensor_desc) const;
  bool operator==(const GeTensorDescImpl &r_ge_tensor_desc) const;

  GeTensorDescImpl &operator=(const GeTensorDescImpl &desc);
  GeTensorDescImpl &operator=(GeTensorDescImpl &&desc);

  ProtoAttrMap &MutableAttrMap();
  ConstProtoAttrMap &GetAttrMap() const;
  void SetShape(const GeShape &shape);
  void SetShape(GeShape &&shape);
  void SetDataType(const DataType dtype);
  graphStatus SetDim(size_t idx, int64_t value);

  DataType GetDataType() const;
  void SetFormat(Format format);
  Format GetFormat() const;
  void SetOriginFormat(Format format);
  void SetDeviceType(DeviceType type);
  void SetName(const std::string &name);
  const std::string GetName() const;
  void RefTo(const GeTensorDescImpl &tensorDesc) {
    tensor_descriptor_ = tensorDesc.tensor_descriptor_;
  }

 private:
  friend class GeTensorImpl;
  friend class TensorUtils;
  friend class GeAttrValueImp;
  friend class ModelSerializeImp;
  friend class OnnxUtils;
  GeIrProtoHelper<proto::TensorDescriptor> tensor_descriptor_;
  // Reference from tensorDescriptor_, do not direct use
  mutable GeShape __shape_;
};
class GEThreadLocalContext {
 public:
  ge::graphStatus GetOption(const std::string &optionExec, std::string &dumpDebugValue);
  ge::graphStatus SetOption(std::string &optionKey, std::string &OptionValue);
  ge::graphStatus ClearOption();
  std::unordered_map<std::string, std::string> options_;
};
ge::GEThreadLocalContext &GetThreadLocalContext();

class GEContext {
 public:
  ge::graphStatus GetOption(const std::string &optionExec, std::string &dumpDebugValue);
  std::unordered_map<std::string, std::string> options_;
};
ge::GEContext &GetContext();

using OpCreator = std::function<Operator(const std::string &)>;
using OpCreatorV2 = std::function<Operator(const AscendString &)>;

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<Anchor>();

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<DataAnchor>();

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<ControlAnchor>();

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<InDataAnchor>();

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<OutDataAnchor>();

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<InControlAnchor>();

template <>
GE_FUNC_DEV_VISIBILITY GE_FUNC_HOST_VISIBILITY TypeId GetTypeId<OutControlAnchor>();
}  // namespace ge
#endif  // __LLT_HCCL_STUB_GE_H__
