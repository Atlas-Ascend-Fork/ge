/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <deque>
#include <memory>
#include <vector>

#include "securec.h"
#include "kernel/memory/mem_block.h"
#include "aicore/launch_kernel/rt_kernel_launch_args_ex.h"
#include "faker/kernel_run_context_facker.h"
#include "faker/space_registry_faker.h"
#include "register/kernel_registry.h"
#include "common/share_graph.h"
#include "faker/global_data_faker.h"
#include "register/node_converter_registry.h"
#include "graph/load/model_manager/davinci_model.h"
#include "kernel/known_subgraph/davinci_model_kernel.h"
#include "kernel/known_subgraph/davinci_model_tracing.h"
#include "common/ge_inner_attrs.h"
#include "graph/manager/graph_var_manager.h"
#include "exe_graph/runtime/gert_tensor_data.h"
#include "graph_builder/bg_reusable_stream_allocator.h"
#include "graph/custom_op_registry.h"

using namespace ge;
namespace gert {
namespace {
const void *kInputAddressBase = reinterpret_cast<void *>(0x110);
constexpr const char *kInputFrozenIndex = "0,1\0";

enum InputsCommon { kDavinciModel, kInputsCommonEnd };
enum ModelExecute { kRtStream = InputsCommon::kInputsCommonEnd, kInputNum, kOutputNum, kModelExecuteEnd };
}  // namespace

// A nullptr stream is a valid input after the stream null check was removed from
// DavinciModelUpdateArgs / DavinciModelExecute. Exercise the whole kernel path with
// a nullptr stream so both warning branches are covered by the ST suite.
class DavinciModelNullStreamStest : public testing::Test {
 public:
  void SetUp() override {
    mmSetEnv("GE_USE_STATIC_MEMORY", "4", 1);
    custom_op_registry_ = std::make_shared<ge::CustomOpRegistry>();
    auto space_registry_array = SpaceRegistryFaker().BuildRegistryArray();
    ASSERT_NE(space_registry_array, nullptr);
    context_holder_with_davinci_model_ = BuildContextForCreateDavinciModel(space_registry_array);
    auto ret = registry_.FindKernelFuncs("DavinciModelCreate")->run_func(context_holder_with_davinci_model_);
    ASSERT_EQ(ret, ge::SUCCESS);
    davinci_model_ =
        context_holder_with_davinci_model_
            .value_holder[static_cast<size_t>(kernel::DavinciModelCreateInput::kDavinciModelCreateInputEnd)]
            .GetPointer<ge::DavinciModel>();
    ASSERT_NE(davinci_model_, nullptr);
  }

  void TearDown() override {
    mmSetEnv("GE_USE_STATIC_MEMORY", "0", 1);
    if (davinci_model_ == nullptr) {
      return;
    }
    ge::VarManager::Instance(davinci_model_->GetSessionId())->FreeVarMemory();
    for (auto &mem_info : davinci_model_->GetRuntimeParam().memory_infos) {
      const_cast<MemInfo &>(mem_info.second).memory_base = nullptr;
    }
    auto list = davinci_model_->GetStreamList();
    const_cast<std::vector<rtStream_t> &>(list).clear();
  }

  void FillFileConstantVec(KernelRunContextHolder &context) {
    std::vector<kernel::FileConstantNameAndMem> file_constant_weight_mems;
    {
      kernel::FileConstantNameAndMem file1{};
      auto ret = strcpy_s(file1.name, sizeof(file1.name), "file1");
      (void)ret;
      file1.mem = &file_constant_mem;
      file1.size = 100;
      file_constant_weight_mems.emplace_back(file1);
    }
    {
      kernel::FileConstantNameAndMem file1{};
      auto ret = strcpy_s(file1.name, sizeof(file1.name), "file2");
      (void)ret;
      file1.mem = &file_constant_mem;
      file1.size = 100;
      file_constant_weight_mems.emplace_back(file1);
    }
    size_t total_size = 0U;
    size_t vec_size = file_constant_weight_mems.size();
    file_constant_mem_data_ = ContinuousVector::Create<kernel::FileConstantNameAndMem>(vec_size, total_size);
    auto vec = reinterpret_cast<ContinuousVector *>(file_constant_mem_data_.get());
    vec->SetSize(vec_size);
    for (size_t i = 0U; i < vec_size; ++i) {
      *(reinterpret_cast<kernel::FileConstantNameAndMem *>(vec->MutableData()) + i) = file_constant_weight_mems[i];
    }
    context.value_holder[15].Set((void *)file_constant_mem_data_.get(), nullptr);
  }

  KernelRunContextHolder BuildContextForCreateDavinciModel(
      const std::shared_ptr<OpImplSpaceRegistryV2Array> &space_registry_array) {
    (void)bg::ValueHolder::PushGraphFrame();
    auto graph = ShareGraph::BuildWithKnownSubgraph();
    auto parent_node = graph->FindNode(ge::PARTITIONEDCALL);
    auto root_model = GeModelBuilder(graph).BuildGeRootModel();
    faker_ = GlobalDataFaker(root_model);
    auto global_data = faker_.FakeWithoutHandleAiCore("Conv2d", false).Build();
    LowerInput lower_input = {{}, {}, &global_data};
    auto ge_model = reinterpret_cast<ge::GeModel *>(
        lower_input.global_data->GetGraphStaticCompiledModel(parent_node->GetOpDesc()->GetSubgraphInstanceName(0)));
    ge::AttrUtils::SetInt(ge_model, ge::ATTR_MODEL_STREAM_NUM, 1);
    size_t weight_size = ge_model->GetWeightSize();
    weight_data_ = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[weight_size]);
    weight_info_ = {weight_data_.get(), weight_size, kTensorPlacementEnd, -1};

    KernelRunContextHolder context =
        BuildKernelRunContext(static_cast<size_t>(kernel::DavinciModelCreateInput::kDavinciModelCreateInputEnd), 1);
    context.value_holder[0].Set(ge_model, nullptr);
    context.value_holder[1].Set((void *)session_id_, nullptr);
    context.value_holder[2].Set(&weight_info_, nullptr);
    context.value_holder[3].Set((void *)1, nullptr);
    context.value_holder[4].Set((void *)1, nullptr);
    context.value_holder[5].Set(space_registry_array.get(), nullptr);
    std::string file_constant_weight_dir;
    auto file_constant_weight_dir_data = static_cast<void *>(const_cast<char *>(file_constant_weight_dir.c_str()));
    context.value_holder[6].Set(file_constant_weight_dir_data, nullptr);
    reusable_stream_allocator_.emplace_back(ge::ReusableStreamAllocator::Create());
    context.value_holder[7].Set((void *)reusable_stream_allocator_.back().get(), nullptr);
    context.value_holder[8].Set((void *)1, nullptr);
    context.value_holder[9].Set((void *)1, nullptr);
    context.value_holder[10].Set((void *)1, nullptr);
    context.value_holder[11].Set((void *)1, nullptr);
    context.value_holder[12].Set((void *)1, nullptr);
    context.value_holder[13].Set((void *)1, nullptr);
    auto frozen_inputs_data = static_cast<void *>(const_cast<char *>(kInputFrozenIndex));
    context.value_holder[14].Set(frozen_inputs_data, nullptr);
    FillFileConstantVec(context);
    context.value_holder[static_cast<size_t>(kernel::DavinciModelCreateInput::kCustomOpRegistry)].Set(
        custom_op_registry_.get(), nullptr);
    (void)bg::ValueHolder::PopGraphFrame();
    return context;
  }

  KernelContext *BuildContextForDavinciModelExecute() {
    const size_t inputs_num = 1U;
    const size_t outputs_num = 3U;
    context_ = BuildKernelRunContext(kModelExecuteEnd + inputs_num + outputs_num, 1U);
    context_.value_holder[kDavinciModel].Set(davinci_model_, nullptr);
    stream_ = reinterpret_cast<void *>(0x123);
    context_.value_holder[kRtStream].Set(stream_, nullptr);
    context_.value_holder[kInputNum].Set(reinterpret_cast<void *>(inputs_num), nullptr);
    context_.value_holder[kOutputNum].Set(reinterpret_cast<void *>(outputs_num), nullptr);

    for (size_t i = 0U; i < inputs_num; ++i) {
      inputs_tensor_data[i] = {reinterpret_cast<void *>(PtrAdd<uint32_t>((uint32_t *)kInputAddressBase, 100, i)), 0U,
                               kTensorPlacementEnd, -1};
      context_.value_holder[kModelExecuteEnd + i].Set(&inputs_tensor_data[i], nullptr);
    }
    for (size_t i = 0U; i < outputs_num; ++i) {
      outputs_tensor_data[i] = {0, 16, kTensorPlacementEnd, -1};
      context_.value_holder[kModelExecuteEnd + inputs_num + i].Set(&outputs_tensor_data[i], nullptr);
    }
    return context_;
  }

  KernelContext *BuildContextForDavinciModelExecuteNullStream() {
    (void)BuildContextForDavinciModelExecute();
    stream_ = nullptr;
    context_.value_holder[kRtStream].Set(stream_, nullptr);
    return context_;
  }

 protected:
  KernelRegistry &registry_ = KernelRegistry::GetInstance();
  std::deque<std::unique_ptr<ge::ReusableStreamAllocator>> reusable_stream_allocator_;
  ge::DavinciModel *davinci_model_ = nullptr;
  KernelRunContextHolder context_holder_with_davinci_model_;
  KernelRunContextHolder context_;
  GlobalDataFaker faker_;
  GertTensorData inputs_tensor_data[10];
  GertTensorData outputs_tensor_data[10];
  rtStream_t stream_ = nullptr;
  uint64_t session_id_ = 1U;
  std::unique_ptr<uint8_t[]> weight_data_;
  GertTensorData weight_info_;
  uint32_t file_constant_mem[256];
  std::unique_ptr<uint8_t[]> file_constant_mem_data_;
  std::shared_ptr<ge::CustomOpRegistry> custom_op_registry_;
};

TEST_F(DavinciModelNullStreamStest, ExecuteWithNullStreamIsNotBlocked) {
  KernelContext *context = BuildContextForDavinciModelExecuteNullStream();
  const auto *funcs = registry_.FindKernelFuncs("DavinciModelExecute");
  ASSERT_NE(funcs, nullptr);
  // The nullptr stream must only produce warnings in DavinciModelUpdateArgs and
  // DavinciModelExecute, and the model must still be executed successfully.
  EXPECT_EQ(funcs->run_func(context), ge::SUCCESS);
}

TEST_F(DavinciModelNullStreamStest, ExecuteWithValidStreamStillWorks) {
  KernelContext *context = BuildContextForDavinciModelExecute();
  const auto *funcs = registry_.FindKernelFuncs("DavinciModelExecute");
  ASSERT_NE(funcs, nullptr);
  EXPECT_EQ(funcs->run_func(context), ge::SUCCESS);
}
}  // namespace gert
