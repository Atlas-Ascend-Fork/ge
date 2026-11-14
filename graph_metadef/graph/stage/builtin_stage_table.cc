/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "stage/builtin_stage_table.h"
#include <unordered_set>

namespace ge {
namespace {
// 来源：openspec/changes/gert-model-generation-api/executegraph_kernel_phase_analysis.xlsx
// 的“当前阶段分布”列（Init 71 项 / DeInit 5 项）。只收录单阶段结论；
// Init/Main、Main/DeInit 及“无生产构图证据”的项不进入固定表。
const std::unordered_set<std::string> &InitTypes() {
  static const std::unordered_set<std::string> kTypes = {"AllocFixedFeatureMemory",
                                                         "AllocLaunchArg",
                                                         "AppendCoreTypeToPlatform",
                                                         "AssignWeightMemory",
                                                         "BuildCCArgs",
                                                         "BuildCCArgsBinHandle",
                                                         "BuildDualStageAclnnOpFwkData",
                                                         "BuildExtInfoHandle",
                                                         "BuildHostCCArgs",
                                                         "BuildSingleStageAclnnOpFwkData",
                                                         "BuildTfArgs",
                                                         "BuildTfArgsBinHandle",
                                                         "CompatibleTilingParse",
                                                         "CopyAtomicTilingdata",
                                                         "CopyTilingdata",
                                                         "CreateEvent",
                                                         "CreateGertEvents",
                                                         "CreateInitL2Allocator",
                                                         "CreateL1Allocator",
                                                         "CreateL2Allocators",
                                                         "CreateMemAssigner",
                                                         "CreateOpFromBuffer",
                                                         "CreateSession",
                                                         "CreateStepId",
                                                         "DavinciModelCreate",
                                                         "DavinciModelCreateV2",
                                                         "EmptyTensorData",
                                                         "EnsureCreateTfSession",
                                                         "FftsTaskInfoPreProc",
                                                         "FileConstantKernel",
                                                         "FileConstantUserMemKernel",
                                                         "FindCompatibleInferShapeFunc",
                                                         "FindCompatibleInferShapeRangeFunc",
                                                         "FindCompatibleTilingFunc",
                                                         "FindCustomOp",
                                                         "FindInferShapeFunc",
                                                         "FindInferShapeRangeFunc",
                                                         "FindKernelFunc",
                                                         "FindOpExe2PhaseFunc",
                                                         "FindOpExeFunc",
                                                         "FindTilingFunc",
                                                         "GetAutofuseFuncs",
                                                         "GetExternalL1Allocator",
                                                         "GetExtInfoAddrFromArgs",
                                                         "GetFileConstantWeightDir",
                                                         "GetOrCreateWeightMem",
                                                         "GetPlatformInfo",
                                                         "GetSessionId",
                                                         "GetSpaceRegistries",
                                                         "GetSpaceRegistry",
                                                         "GetUserAllocatorOrFixedBaseAllocator",
                                                         "InitCmoArgs",
                                                         "LaunchAicpuCustKernel",
                                                         "LoadShapeRuleFromBinary",
                                                         "LoadShapeRuleFromJson",
                                                         "NodeMemParaAssign",
                                                         "PrepareCacheableTilingFwkData",
                                                         "PrepareTilingFwkData",
                                                         "RedirectLaunchArgs",
                                                         "ReusableStreamAllocator",
                                                         "SinkFFTSAICoreNodeBin",
                                                         "SinkFFTSStaManualNodeBin",
                                                         "SinkMixDyNodeBin",
                                                         "SinkMixStaticNodeBin",
                                                         "SinkNodeBinWithHandle",
                                                         "SinkNodeBinWithoutHandle",
                                                         "SinkWeightData",
                                                         "SplitConstTensor",
                                                         "SplitVariable",
                                                         "SymbolTilingParse",
                                                         "TilingParse"};
  return kTypes;
}

const std::unordered_set<std::string> &DeInitTypes() {
  static const std::unordered_set<std::string> kTypes = {"ClearAicpuCustKernel", "DavinciModelFinalizer",
                                                         "DestroyEvent", "DestroySession", "FreeFixedFeatureMemory"};
  return kTypes;
}

// 通用分配类：既能加载分配也能逐次分配，不进入固定阶段表，只用于资源配对分析。
const std::unordered_set<std::string> &AllocTypes() {
  static const std::unordered_set<std::string> kTypes = {"AllocMemHbm",
                                                         "AllocMemHost",
                                                         "AllocBatchHbm",
                                                         "AllocSpecifiedMem",
                                                         "AllocHostCpuOutputMemory",
                                                         "AllocateFftsMem",
                                                         "AllocateBatchFftsMems",
                                                         "CreateHostL2Allocator",
                                                         "SelectL1Allocator",
                                                         "SelectL2Allocator"};
  return kTypes;
}

const std::unordered_set<std::string> &FreeTypes() {
  static const std::unordered_set<std::string> kTypes = {
      "FreeMemory",  "FreeSpecifiedMem",  "FreeTensorMemory",   "FreeMemHbm",         "FreeBatchHbm",
      "FreeFftsMem", "FreeBatchFftsMems", "FreeMemoryHoldAddr", "FreeMemHbmHoldAddr", "FreeBatchHbmHoldAddr"};
  return kTypes;
}

// 逐次执行的任务下发：依赖本次 stream、执行输入或输出缓冲，不得前置到加载。
const std::unordered_set<std::string> &ExecutionTypes() {
  static const std::unordered_set<std::string> kTypes = {"LaunchKernelWithHandle",
                                                         "LaunchKernelWithFlag",
                                                         "AtomicLaunchKernelWithHandle",
                                                         "AtomicLaunchKernelWithFlag",
                                                         "LaunchMixKernelWithHandle",
                                                         "LaunchMixKernelWithFlag",
                                                         "AicpuLaunchCCKernel",
                                                         "AicpuLaunchTfKernel",
                                                         "ExecuteOpLaunch",
                                                         "ExecuteOpFunc",
                                                         "DavinciModelExecute",
                                                         "GenerateSqeAndLaunchTask",
                                                         "StarsTaskLaunchKernel",
                                                         "LaunchFFTSPlusTaskNoCopy",
                                                         "CopyFlowLaunch"};
  return kTypes;
}
}  // namespace

bool BuiltinStageTable::Lookup(const std::string &node_type, StageKind &stage) {
  if (InitTypes().count(node_type) != 0U) {
    stage = StageKind::kInit;
    return true;
  }
  if (DeInitTypes().count(node_type) != 0U) {
    stage = StageKind::kDeInit;
    return true;
  }
  return false;
}

bool BuiltinStageTable::IsBuiltinStageType(const std::string &node_type) {
  StageKind stage;
  return Lookup(node_type, stage);
}

bool BuiltinStageTable::IsAllocType(const std::string &node_type) {
  return AllocTypes().count(node_type) != 0U;
}

bool BuiltinStageTable::IsFreeType(const std::string &node_type) {
  return FreeTypes().count(node_type) != 0U;
}

bool BuiltinStageTable::IsExecutionType(const std::string &node_type) {
  return ExecutionTypes().count(node_type) != 0U;
}
}  // namespace ge
