/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/ge_common/ge_types.h"

namespace ge {
const std::map<std::string, std::string> kFwkTypeToStr = {
    {"0", "Caffe"}, {"1", "MindSpore"}, {"3", "TensorFlow"}, {"4", "Android_NN"}, {"5", "Onnx"}};

// profiling data
const std::string kTaskTypeAicore = "AI_CORE";
const std::string kTaskTypeAiv = "AIV";
const std::string kTaskTypeMixAic = "MIX_AIC";
const std::string kTaskTypeMixAiv = "MIX_AIV";
const std::string kTaskTypeAicpu = "AI_CPU";
const std::string kTaskTypeDsa = "DSA";
const std::string kTaskTypeWriteBackData = "WRITE_BACK";
const std::string kTaskTypeInvalidData = "INVALID";
const std::string kTaskTypeInvalid = "TASK_TYPE_INVALID";
const std::string kTaskTypeFftsPlus = "FFTS_PLUS";
const std::string kEngineNameVectorCore = "VectorEngine";

const std::string kEngineNameHccl = "ops_kernel_info_hccl";
const std::string kEngineNameRts = "DNN_VM_RTS_OP_STORE";
const std::string kEngineNameHostCpu = "DNN_VM_HOST_CPU_OP_STORE";
const std::string kEngineNameGeLocal = "DNN_VM_GE_LOCAL_OP_STORE";
const std::string kEngineNameCustom = "DNN_VM_CUSTOM";
const std::string kCustomOpKernelLibName = "DNN_VM_CUSTOM_OP_STORE";
const std::string kCustomGraphOptimizer = "custom_graph_optimizer";
const std::string kEngineNameAiCpu = "aicpu_ascend_kernel";
const std::string kEngineNameAiCpuTf = "aicpu_tf_kernel";
const std::string kEngineNameAiCore = "AIcoreEngine";
const std::string kEngineNameDvpp = "dvpp_ops_kernel";
const std::string kEngineNameDsa = "DSAEngine";
const std::string kAtomicOpType = "DynamicAtomicAddrClean";

// runtime2.0 lowering func
const std::string kAttrLowingFunc = "_ge_attr_lowering_func";
const std::string kFFTSAiCoreLowerFunc = "ffts_ai_core_lower_func";
const std::string kFFTSGraphLowerFunc = "ffts_graph_lower_func";
const std::string kFFTSStaticGraphLowerFunc = "ffts_static_graph_lower_func";
const std::string kFFTSMixL2LowerFunc = "ffts_mix_l2_lower_func";
const std::string kHostCpuCustomOpLowerFunc = "host_cpu_custom_op_lower_func";
// runtime2.0 calculate func
const std::string kAttrCalcArgsSizeFunc = "_ge_attr_calculate_func";
const std::string kFFTSMixL2CalcFunc = "ffts_mix_l2_calc_func";

const std::string kInputTensorIndexs = "input_tensor_indexs";
const std::string kOutputTensorIndexs = "output_tensor_indexs";
const std::string kShapeTypeStatic = "static";
const std::string kShapeTypeDynamic = "dynamic";
const std::string kAtomicPrefix = "_atomic";

// memory policy
const std::string kBalanceMode = "BalanceMode";
const std::string kMemoryPriority = "MemoryPriority";
const std::set<std::string> kValidValues = {"", kBalanceMode, kMemoryPriority};

// model deploy mode
const std::string kModelDeployModeSpmd = "SPMD";

// Configure Attribute Compression flag(true or false, default true for backward compatibility)
const std::string ENABLE_ATTR_COMPRESSION = "ge.enableAttrCompression";

const std::set<std::string> ir_builder_suppported_options_inner = {
    EVALUATE_GRAPH_RESOURCE_MODE, RESOURCE_CONFIG_PATH, RECOMPUTE, OPTION_TOPOSORTING_MODE, GRAPH_SLICE_MODE};

}  // namespace ge
