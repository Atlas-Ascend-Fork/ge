/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "register/register_custom_pass.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "common/checker.h"

#include "register/custom_pass_helper.h"
#include "register/pass_option_utils.h"
#include "framework/common/debug/ge_log.h"
#include "graph_metadef/graph/debug/ge_util.h"
#include "graph_metadef/common/plugin/plugin_manager.h"
#include "register/custom_pass_context_impl.h"
#include "graph/utils/graph_utils_ex.h"
#include "graph/utils/graph_utils.h"
#include "graph/ge_context.h"

namespace ge {
const std::set<CustomPassStage> kConstGraphStages = {CustomPassStage::kAfterAssignLogicStream};
const std::map<CustomPassStage, std::string> kCustomPassStageToStringMap = {
    {CustomPassStage::kBeforeInferShape, "BeforeInferShape"},
    {CustomPassStage::kAfterInferShape, "AfterInferShape"},
    {CustomPassStage::kAfterAssignLogicStream, "AfterAssignLogicStream"},
    {CustomPassStage::kAfterBuiltinFusionPass, "AfterBuiltinFusionPass"},
    {CustomPassStage::kAfterOriginGraphOptimize, "AfterOriginGraphOptimize"},
    {CustomPassStage::kCompatibleInherited, "CompatibleInherited"},
    {CustomPassStage::kInvalid, "InvalidStage"}};

namespace {
constexpr const char_t *const kMiniDagStreamPass = "MiniDAGStreamPass";
constexpr const char_t *const kAutoMultistreamParallelModeOption = "ge.autoMultistreamParallelMode";

std::string CustomPassStageToString(CustomPassStage stage) {
  GE_ASSERT_TRUE(stage <= CustomPassStage::kInvalid);
  return kCustomPassStageToStringMap.find(stage)->second;
}

bool GetAutoMultistreamModeFromGraph(const GraphPtr &graph, std::string &multi_stream_mode) {
  if (graph == nullptr) {
    return false;
  }
  AttrValue attr_value;
  if (graph->GetAttr(AscendString(kAutoMultistreamParallelModeOption), attr_value) != GRAPH_SUCCESS) {
    return false;
  }
  AscendString value;
  if (attr_value.GetAttrValue(value) != GRAPH_SUCCESS) {
    return false;
  }
  const char_t *const value_str = value.GetString();
  multi_stream_mode = (value_str == nullptr) ? "" : value_str;
  return true;
}

bool ShouldSkipMiniDagStreamPass(const GraphPtr &graph) {
  std::string multi_stream_mode;
  const bool from_graph = GetAutoMultistreamModeFromGraph(graph, multi_stream_mode);
  if ((!from_graph) &&
      (GetContext().GetOption(kAutoMultistreamParallelModeOption, multi_stream_mode) != GRAPH_SUCCESS)) {
    GELOGI("MiniDAGStreamPass skipped: auto multistream parallel mode not set.");
    return true;
  }

  const bool should_skip =
      multi_stream_mode.empty() || (multi_stream_mode == "cv") || (from_graph && (multi_stream_mode == "default"));
  if (should_skip) {
    GELOGI("MiniDAGStreamPass skipped for auto multistream parallel mode %s.", multi_stream_mode.c_str());
  }
  return should_skip;
}

bool ShouldSkipMiniDagStreamPass(const PassRegistrationData &reg_data, const GraphPtr &graph) {
  return (reg_data.GetPassName() == kMiniDagStreamPass) && ShouldSkipMiniDagStreamPass(graph);
}

Status RunAllocateStreamPass(const PassRegistrationData &reg_data, const GraphPtr &graph,
                             CustomPassContext &custom_pass_context) {
  GE_ASSERT_NOTNULL(graph);
  const auto allocate_stream_pass_func = reg_data.GetCustomAllocateStreamPass();
  if (allocate_stream_pass_func == nullptr) {
    GE_LOGE("[Check][Param] It is required CustomAllocateStreamPassFunc of [%s] at stage[%s] but got nullptr.",
            reg_data.GetPassName().c_str(), CustomPassStageToString(reg_data.GetStage()).c_str());
    std::stringstream reason;
    reason << "Custom stream allocation pass function is required in stage "
           << CustomPassStageToString(reg_data.GetStage()) << ", but got nullptr";
    (void)REPORT_PREDEFINED_ERR_MSG(
        "E13030", std::vector<const char_t *>({"passname", "reason"}),
        std::vector<const char_t *>({reg_data.GetPassName().c_str(), reason.str().c_str()}));
    return FAILED;
  }

  if (ShouldSkipMiniDagStreamPass(reg_data, graph)) {
    return SUCCESS;
  }

  const auto compute_graph = GraphUtilsEx::GetComputeGraph(*graph);
  GE_ASSERT_NOTNULL(compute_graph);
  const auto root_graph = GraphUtils::FindRootGraph(compute_graph);
  GE_ASSERT_NOTNULL(root_graph);

  GE_DUMP(root_graph, "RunCustomPass_BeforeAssignLogicStream" + reg_data.GetPassName());
  // 此处框架保证传入的custom_pass_context实例为stream_pass_context
  custom_pass_context.SetPassName(reg_data.GetPassName().c_str());
  auto *stream_pass_context = dynamic_cast<StreamPassContext *>(&custom_pass_context);
  GE_ASSERT_NOTNULL(stream_pass_context, "Failed to transfer CustomPassContext to StreamPassContext");
  const auto ret = allocate_stream_pass_func(graph, *stream_pass_context);
  GE_DUMP(root_graph, "RunCustomPass_AfterAssignLogicStream" + reg_data.GetPassName());
  if (ret != SUCCESS) {
    GE_LOGE("Execution of custom pass [%s] failed! Reason: %s.", reg_data.GetPassName().c_str(),
            custom_pass_context.GetErrorMessage().GetString());
    (void)REPORT_PREDEFINED_ERR_MSG(
        "E13028", std::vector<const char_t *>({"passname", "retcode", "reason"}),
        std::vector<const char_t *>({reg_data.GetPassName().c_str(), std::to_string(ret).c_str(),
                                     std::string(custom_pass_context.GetErrorMessage().GetString()).c_str()}));
    return FAILED;
  }
  return SUCCESS;
}

Status RunCustomPass(const PassRegistrationData &reg_data, GraphPtr &graph, CustomPassContext &custom_pass_context) {
  const auto custom_pass_fn = reg_data.GetCustomPassFn();
  if (custom_pass_fn == nullptr) {
    GELOGW("[Check][Param] Failed to retrieve custom_pass_fn for custom pass %s failed",
           reg_data.GetPassName().c_str());
    return SUCCESS;
  }
  custom_pass_context.SetPassName(reg_data.GetPassName().c_str());
  const auto ret = custom_pass_fn(graph, custom_pass_context);
  if (ret != SUCCESS) {
    GE_LOGE("Execution of custom pass [%s] failed! Reason: %s.", reg_data.GetPassName().c_str(),
            custom_pass_context.GetErrorMessage().GetString());
    (void)REPORT_PREDEFINED_ERR_MSG(
        "E13028", std::vector<const char_t *>({"passname", "retcode", "reason"}),
        std::vector<const char_t *>({reg_data.GetPassName().c_str(), std::to_string(ret).c_str(),
                                     std::string(custom_pass_context.GetErrorMessage().GetString()).c_str()}));
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace
PassReceiver::PassReceiver(PassRegistrationData &reg_data) {
  CustomPassHelper::Instance().Insert(reg_data);
}

class PassRegistrationDataImpl {
 public:
  PassRegistrationDataImpl() = default;
  ~PassRegistrationDataImpl() = default;

  explicit PassRegistrationDataImpl(const std::string &pass_name);

 private:
  friend class PassRegistrationData;
  std::string pass_name_;
  CustomPassFunc custom_pass_;
  CustomAllocateStreamPassFunc allocate_stream_pass_;
  CustomPassStage stage_ = CustomPassStage::kBeforeInferShape;
  PassSwitch default_switch_ = PassSwitch::kOn;
};

PassRegistrationDataImpl::PassRegistrationDataImpl(const std::string &pass_name)
    : pass_name_(pass_name), custom_pass_(nullptr) {}

PassRegistrationData::PassRegistrationData(std::string pass_name) {
  impl_ = ge::ComGraphMakeShared<PassRegistrationDataImpl>(pass_name);
  if (impl_ == nullptr) {
    GELOGW("[Check][Param] make impl failed, pass_name:%s", pass_name.c_str());
  }
}

std::string PassRegistrationData::GetPassName() const {
  if (impl_ == nullptr) {
    return "";
  }
  return impl_->pass_name_;
}

PassRegistrationData &PassRegistrationData::CustomPassFn(const CustomPassFunc &custom_pass_fn) {
  if (impl_ != nullptr) {
    impl_->custom_pass_ = custom_pass_fn;
  }
  return *this;
}

PassRegistrationData &PassRegistrationData::CustomAllocateStreamPassFn(
    const CustomAllocateStreamPassFunc &allocate_stream_pass_fn) {
  if (impl_ != nullptr) {
    impl_->allocate_stream_pass_ = allocate_stream_pass_fn;
    impl_->stage_ = CustomPassStage::kAfterAssignLogicStream;
  }
  return *this;
}

CustomPassFunc PassRegistrationData::GetCustomPassFn() const {
  if (impl_ == nullptr) {
    return nullptr;
  }
  return impl_->custom_pass_;
}

CustomAllocateStreamPassFunc PassRegistrationData::GetCustomAllocateStreamPass() const {
  if (impl_ == nullptr) {
    return nullptr;
  }
  return impl_->allocate_stream_pass_;
}

PassRegistrationData &PassRegistrationData::DefaultSwitch(PassSwitch pass_switch) {
  if (impl_ != nullptr) {
    impl_->default_switch_ = pass_switch;
  }
  return *this;
}

PassSwitch PassRegistrationData::GetDefaultSwitch() const {
  if (impl_ == nullptr) {
    return PassSwitch::kOn;
  }
  return impl_->default_switch_;
}

PassRegistrationData &PassRegistrationData::Stage(const CustomPassStage stage) {
  if ((impl_ != nullptr) && (stage < CustomPassStage::kInvalid)) {
    impl_->stage_ = stage;
    GELOGD("Setting pass [%s] stage to [%s]", impl_->pass_name_.c_str(), CustomPassStageToString(stage).c_str());
  }
  return *this;
}

CustomPassStage PassRegistrationData::GetStage() const {
  if (impl_ == nullptr) {
    return CustomPassStage::kInvalid;
  }
  return impl_->stage_;
}

CustomPassContext::CustomPassContext() {
  impl_ = ComGraphMakeUnique<CustomPassContextImpl>();
  if (impl_ == nullptr) {
    GELOGW("[Check][Param] make impl failed");
  }
}

CustomPassContext::~CustomPassContext() = default;

void CustomPassContext::SetErrorMessage(const AscendString &error_message) {
  if (impl_ != nullptr) {
    impl_->SetErrorMessage(error_message);
  }
}

void CustomPassContext::SetPassName(const AscendString &pass_name) {
  if (impl_ != nullptr) {
    impl_->SetPassName(pass_name);
  }
}

AscendString CustomPassContext::GetErrorMessage() const {
  if (impl_ != nullptr) {
    return impl_->GetErrorMessage();
  }
  return "";
}

AscendString CustomPassContext::GetPassName() const {
  if (impl_ != nullptr) {
    return impl_->GetPassName();
  }
  return "";
}

graphStatus CustomPassContext::GetOptionValue(const AscendString &option_key, AscendString &option_value) const {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->GetOptionValue(option_key, option_value);
}

StreamPassContext::StreamPassContext(int64_t current_max_stream_id) {
  impl_ = ge::ComGraphMakeUnique<StreamPassContextImpl>(current_max_stream_id);
  if (impl_ == nullptr) {
    GELOGW("[Check][Param] make StreamPassContextImpl failed");
  }
}

StreamPassContext::~StreamPassContext() = default;

graphStatus StreamPassContext::SetStreamId(const GNode &node, int64_t stream_id) {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->SetStreamId(node, stream_id);
}

int64_t StreamPassContext::GetStreamId(const GNode &node) const {
  int64_t stream_id;
  if ((impl_ == nullptr) || impl_->GetStreamId(node, stream_id) != SUCCESS) {
    return INVALID_STREAM_ID;
  }
  return stream_id;
}

int64_t StreamPassContext::GetCurrMaxStreamId() const {
  GE_ASSERT_NOTNULL(impl_);
  return impl_->GetCurrentMaxStreamId();
}

int64_t StreamPassContext::AllocateNextStreamId() {
  if (impl_ != nullptr) {
    return impl_->AllocateNextStreamId();
  }
  return INT64_MAX;
}

CustomPassHelper &CustomPassHelper::Instance() {
  static CustomPassHelper instance;
  return instance;
}

void CustomPassHelper::Insert(const PassRegistrationData &reg_data) {
  // Insert is only called from PassReceiver constructor during dlopen in Load(),
  // which already holds the unique_lock. Using a lock here would cause deadlock
  // since std::shared_mutex is not recursive.
  (void)registration_datas_.emplace_back(reg_data);
}

bool CustomPassHelper::RegisterPythonResourcePlanPass(const PassRegistrationData &reg_data) {
  const auto pass_name = reg_data.GetPassName();
  const auto stage = reg_data.GetStage();
  if (pass_name.empty() || stage >= CustomPassStage::kInvalid || reg_data.GetCustomAllocateStreamPass() == nullptr) {
    GELOGW("Reject malformed Python resource-plan registration: name[%s], stage[%u].", pass_name.c_str(),
           static_cast<uint32_t>(stage));
    return false;
  }

  std::unique_lock<std::shared_mutex> lock(mutex_);
  const std::pair<std::string, CustomPassStage> key{pass_name, stage};

  for (const auto &item : registration_datas_) {
    if (item.GetPassName() == pass_name && item.GetStage() == stage) {
      GELOGW("Reject duplicate custom resource-plan registration: name[%s], stage[%u].", pass_name.c_str(),
             static_cast<uint32_t>(stage));
      return false;
    }
  }
  if (python_resource_plan_passes_.find(key) != python_resource_plan_passes_.cend()) {
    return false;
  }
  const auto registration_index = registration_datas_.size();
  registration_datas_.emplace_back(reg_data);
  python_resource_plan_passes_.emplace(key);
  python_resource_plan_pass_indices_.emplace(registration_index);
  return true;
}

void CustomPassHelper::ClearPythonResourcePlanPasses() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  if (python_resource_plan_pass_indices_.empty()) {
    python_resource_plan_passes_.clear();
    return;
  }

  for (auto iter = python_resource_plan_pass_indices_.rbegin(); iter != python_resource_plan_pass_indices_.rend();
       ++iter) {
    if (*iter < registration_datas_.size()) {
      registration_datas_.erase(registration_datas_.begin() + static_cast<std::ptrdiff_t>(*iter));
    }
  }
  python_resource_plan_pass_indices_.clear();
  python_resource_plan_passes_.clear();
}

Status CustomPassHelper::Load() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  GELOGD("[Load][CustomPassLibs] Start to load custom pass libs");
  const size_t initial_handle_count = handles_.size();
  const size_t initial_registration_count = registration_datas_.size();
  const auto initial_python_resource_plan_passes = python_resource_plan_passes_;
  const auto initial_python_resource_plan_pass_indices = python_resource_plan_pass_indices_;

  const auto rollback = [this, initial_handle_count, initial_registration_count, initial_python_resource_plan_passes,
                         initial_python_resource_plan_pass_indices]() {
    if (registration_datas_.size() > initial_registration_count) {
      registration_datas_.erase(registration_datas_.begin() + static_cast<std::ptrdiff_t>(initial_registration_count),
                                registration_datas_.end());
    }
    python_resource_plan_passes_ = initial_python_resource_plan_passes;
    python_resource_plan_pass_indices_ = initial_python_resource_plan_pass_indices;
    Status rollback_status = SUCCESS;
    for (size_t index = handles_.size(); index > initial_handle_count; --index) {
      auto *handle = handles_[index - 1U];
      if ((handle != nullptr) && (dlclose(handle) != 0)) {
        GELOGE(FAILED, "[Load][CustomPassLibs] Failed to rollback custom pass lib: %s", dlerror());
        rollback_status = FAILED;
      }
    }
    handles_.resize(initial_handle_count);
    return rollback_status;
  };

  std::string opp_path;
  GE_ASSERT_SUCCESS(ge::PluginManager::GetOppPath(opp_path));
  std::string vendors_path = opp_path + "/vendors";

  // 存储所有的 .so 文件路径
  std::vector<std::string> so_files;
  ge::PluginManager::FindSoFilesInCustomPassDirs(vendors_path, so_files);

  if (so_files.empty()) {
    GELOGD("No custom pass libs found in %s, skip loading custom pass libs", vendors_path.c_str());
    return ge::SUCCESS;
  }

  // 逐个 dlopen
  for (const auto &so_file : so_files) {
    void *handle = dlopen(so_file.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
      const char *error = dlerror();
      (void)REPORT_PREDEFINED_ERR_MSG("E13029", std::vector<const char_t *>({"passlibname", "reason"}),
                                      std::vector<const char_t *>({so_file.c_str(), error}));
      GELOGE(ge::FAILED, "Failed to load %s: %s", so_file.c_str(), error);
      (void)rollback();
      return ge::FAILED;
    }
    (void)handles_.emplace_back(handle);
    GELOGI("Load custom pass lib %s success", so_file.c_str());
  }
  return ge::SUCCESS;
}

Status CustomPassHelper::Unload() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  python_resource_plan_passes_.clear();
  python_resource_plan_pass_indices_.clear();
  registration_datas_.clear();
  Status unload_status = ge::SUCCESS;
  for (auto &handle : handles_) {
    if (handle != nullptr && dlclose(handle) != 0) {
      GELOGE(ge::FAILED, "[Unload][CustomPassLibs] Failed to unload custom pass lib: %s", dlerror());
      unload_status = ge::FAILED;
      continue;
    }
    GELOGI("Unload custom pass lib success");
  }
  handles_.clear();
  return unload_status;
}

Status CustomPassHelper::Run(GraphPtr &graph, CustomPassContext &custom_pass_context, const CustomPassStage stage,
                             const std::map<std::string, bool> &pass_name_to_switches,
                             const std::map<std::string, int32_t> &pass_name_to_priorities) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  // 按FE解析的op_base Priority配置（进程级缓存快照，由编译层调用方传入）排序：
  // 有条目按数值升序，无条目视为最末。同值（含无条目）时兜底：流分配stage按pass名字序
  // （保持多流组件化的确定性语义），其余stage保持注册序（dlopen序，与历史行为一致）
  constexpr int32_t kNoEntryPriority = INT32_MAX;
  bool has_mini_dag = false;
  std::vector<std::pair<int32_t, const PassRegistrationData *>> keyed_pass_regs;
  keyed_pass_regs.reserve(registration_datas_.size());
  for (const auto &item : registration_datas_) {
    if (item.GetStage() != stage) {
      continue;
    }
    has_mini_dag = has_mini_dag || (item.GetPassName() == kMiniDagStreamPass);
    if (!PassOptionUtils::IsPassEnable(pass_name_to_switches, item.GetPassName(), item.GetDefaultSwitch())) {
      GELOGI("[CustomPass][SKIP] Pass [%s] is disabled.", item.GetPassName().c_str());
      continue;
    }
    const auto iter = pass_name_to_priorities.find(item.GetPassName());
    const int32_t priority = (iter != pass_name_to_priorities.end()) ? iter->second : kNoEntryPriority;
    (void)keyed_pass_regs.emplace_back(priority, &item);
  }
  // MiniDAG校验基于注册集合（不受开关过滤影响）：auto multistream模式需要但内置pass未注册时显式报错
  if ((stage == CustomPassStage::kAfterAssignLogicStream) && (!has_mini_dag) && (!ShouldSkipMiniDagStreamPass(graph))) {
    GELOGE(FAILED,
           "ge.autoMultistreamParallelMode requires the built-in %s, but it is not registered. The Python pass "
           "bridge may have failed to load during initialization, see the earlier warning in the log.",
           kMiniDagStreamPass);
    return FAILED;
  }
  std::stable_sort(keyed_pass_regs.begin(), keyed_pass_regs.end(),
                   [stage](const std::pair<int32_t, const PassRegistrationData *> &lhs,
                           const std::pair<int32_t, const PassRegistrationData *> &rhs) {
                     if (lhs.first != rhs.first) {
                       return lhs.first < rhs.first;
                     }
                     return (stage == CustomPassStage::kAfterAssignLogicStream) &&
                            (lhs.second->GetPassName() < rhs.second->GetPassName());
                   });
  for (const auto &keyed_reg : keyed_pass_regs) {
    const auto &item = *(keyed_reg.second);
    GELOGD("[CustomPass][ADD] %s, priority: %d", item.GetPassName().c_str(), keyed_reg.first);
    GELOGD("Starting custom pass [%s] in stage [%s]!", item.GetPassName().c_str(),
           CustomPassStageToString(stage).c_str());
    if (stage == CustomPassStage::kAfterAssignLogicStream) {
      GE_ASSERT_SUCCESS(RunAllocateStreamPass(item, graph, custom_pass_context));
    } else {
      GE_ASSERT_SUCCESS(RunCustomPass(item, graph, custom_pass_context));
    }
    GELOGD("Run custom pass [%s] successfully!", item.GetPassName().c_str());
  }
  return SUCCESS;
}
}  // namespace ge
