/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef METADEF_GRAPH_STAGE_STAGE_SPLITTER_H_
#define METADEF_GRAPH_STAGE_STAGE_SPLITTER_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>
#include "graph/fast_graph/execute_graph.h"
#include "stage/stage_plan.h"

namespace ge {
// 阶段容器节点类型，与 RT2 既有约定一致：Root 下挂 Init/Main/DeInit 三个子图节点。
constexpr const char_t *kStageInitNodeType = "Init";
constexpr const char_t *kStageMainNodeType = "Main";
constexpr const char_t *kStageDeInitNodeType = "DeInit";

// 用户输入来源类别。加载输入、执行输入各自从 0 连续编号；固定槽用负序号，单独登记。
enum class StageInputClass : uint32_t { kLoad = 0, kExecute = 1, kFixed = 2 };

// 输入绑定：用户槽号/固定槽号 -> 阶段子图内的输入节点。
struct StageInputBinding {
  StageInputClass input_class{StageInputClass::kExecute};
  // 用户槽号为非负序号；固定槽号为负序号（沿用 RT2 固定槽约定），两者互不混用。
  int64_t index{0};
  FastNode *node{nullptr};
};

// Main 的有序用户输出槽位：槽号即 NetOutput 输入序号。
struct StageOutputBinding {
  int64_t user_index{0};
  // 实际连到 NetOutput 的节点；跨阶段产出时是该值在 Main 子图内的 InnerData。
  FastNode *node{nullptr};
  int32_t out_index{0};
};

// 把一个已判定阶段的逻辑执行图拆成 Root + Init/Main/DeInit 三层结构。
// 拆分只修改传入的工作图；调用方负责保留原逻辑图。
// 结构约定：
//   Root          三个阶段容器节点，各自挂一个子图；容器之间只承载跨阶段值和生命周期控制边；
//   Init/DeInit   子图内用 InnerNetOutput 导出被后续阶段消费的值；
//   Main          子图内用 NetOutput 表达图输出，另可用 InnerNetOutput 把值交给 DeInit；
//   跨阶段值      生产容器输出端口 = 生产子图 InnerNetOutput 的输入序号，
//                 消费容器输入端口 = 消费子图 InnerData 的 index。
class StageSplitter {
 public:
  explicit StageSplitter(ExecuteGraph &work_graph) : work_graph_(work_graph) {}
  ~StageSplitter() = default;

  // 按 plan 拆分。user_outputs 是用户在图上的有序输出登记，拆分后成为 Main NetOutput 的输入顺序。
  // 全部前置校验在改动图之前完成；校验失败时工作图保持不变。
  graphStatus Split(const StagePlan &plan, const std::vector<std::pair<FastNode *, int32_t>> &user_outputs);

  ExecuteGraph *GetStageGraph(StageKind stage) const;
  const std::vector<StageInputBinding> &GetInputBindings(StageKind stage, StageInputClass input_class) const;
  const std::vector<StageOutputBinding> &GetOutputBindings() const {
    return output_bindings_;
  }
  // 阶段子图的收尾节点：Main 返回 NetOutput，Init/DeInit 返回 InnerNetOutput。
  FastNode *GetStageNetOutput(StageKind stage) const;
  // 跨阶段值在生产阶段容器上的输出端口；未导出时返回 -1。
  int32_t GetExportPort(StageKind stage, const FastNode &producer, int32_t out_index) const;

 private:
  using ValueKey = std::pair<const FastNode *, int32_t>;
  struct CrossEdge {
    const FastNode *src;
    int32_t src_port;
    const FastNode *dst;
    int32_t dst_port;
  };
  // 跨阶段值的消费方；dst 为空表示 Main 的 NetOutput 在 dst_port 槽位。
  struct CrossConsumer {
    StageKind stage;
    const FastNode *dst;
    int32_t dst_port;
  };
  struct Layout {
    // 生产阶段按端口顺序排列的导出值，下标即容器输出端口和 InnerNetOutput 输入序号。
    std::map<StageKind, std::vector<ValueKey>> exports;
    // 消费阶段按端口顺序排列的导入值。
    std::map<StageKind, std::vector<ValueKey>> imports;
    // (消费阶段, 值) -> 容器输入端口，即子图内 InnerData 的 index。
    std::map<std::pair<StageKind, ValueKey>, int32_t> import_ports;
    // 值的全部跨阶段消费方。
    std::map<ValueKey, std::vector<CrossConsumer>> consumers;
  };

  graphStatus ValidatePreconditions(const StagePlan &plan,
                                    const std::vector<std::pair<FastNode *, int32_t>> &user_outputs);
  graphStatus BuildLayout(const StagePlan &plan, const std::vector<std::pair<FastNode *, int32_t>> &user_outputs,
                          Layout &layout) const;
  graphStatus CreateStageContainers();
  graphStatus AddStageControlOrder();
  graphStatus MoveNodesIntoStages(const StagePlan &plan);
  graphStatus RecordInputBindings(const StagePlan &plan);
  graphStatus CreateStageTailNodes(const std::vector<std::pair<FastNode *, int32_t>> &user_outputs);
  graphStatus BuildCrossStageBridges(const StagePlan &plan);
  graphStatus ConnectStageOutputs(const StagePlan &plan,
                                  const std::vector<std::pair<FastNode *, int32_t>> &user_outputs);
  // 拆图改写了拓扑与容器/桥接节点，派生元信息不能沿用：
  // 引用端点必须仍能解析，symbol 数组必须作废等运行期重算。
  graphStatus RefreshDerivedMetadata();
  // 拆图后的结构自检：容器与子图一一对应、根图不留残余节点、各阶段有收尾节点。
  graphStatus ValidateSplitResult() const;
  graphStatus LinkExecutionTargets(StageKind stage);
  FastNode *CreateInnerData(StageKind stage, int32_t port, const ValueKey &value);
  int32_t ExportPortOf(StageKind stage, const ValueKey &value) const;

  ExecuteGraph &work_graph_;
  // 拆分一旦开始改动图就不允许重来，否则上一次残留的阶段结果会与本次混用。
  bool split_started_{false};
  std::map<const FastNode *, FastNode *> node_map_;
  std::map<StageKind, FastNode *> stage_nodes_;
  std::map<StageKind, std::shared_ptr<ExecuteGraph>> stage_graphs_;
  std::map<StageKind, FastNode *> stage_net_outputs_;
  std::map<StageKind, FastNode *> stage_inner_net_outputs_;
  std::map<std::pair<StageKind, ValueKey>, FastNode *> inner_datas_;
  std::vector<CrossEdge> same_stage_data_edges_;
  std::vector<std::pair<const FastNode *, const FastNode *>> same_stage_control_edges_;
  Layout layout_;
  std::map<StageKind, std::map<StageInputClass, std::vector<StageInputBinding>>> input_bindings_;
  std::vector<StageOutputBinding> output_bindings_;
};
}  // namespace ge

#endif  // METADEF_GRAPH_STAGE_STAGE_SPLITTER_H_
