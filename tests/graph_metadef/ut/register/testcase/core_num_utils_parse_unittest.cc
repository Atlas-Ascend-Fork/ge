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

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/ge_common/error_codes_define.h"
#include "depends/mmpa/src/mmpa_stub.h"
#include "depends/slog/src/slog_stub.h"
#include "ge_common/ge_common_api_types.h"
#include "graph/ge_local_context.h"
#include "register/core_num_utils.h"
#include "securec.h"

namespace ge {
namespace {
constexpr const char *kSocSpecLabel = "SoCInfo";
constexpr const char *kAiCoreCntKey = "ai_core_cnt";
constexpr const char *kCubeCoreCntKey = "cube_core_cnt";
constexpr const char *kVectorCoreCntKey = "vector_core_cnt";
constexpr int32_t kSocKeyNotFound = 0x71A0001;  // 与真机 310P 无 cube_core_cnt 键的返回码一致
const std::string kAbsent = "<absent>";

// 注入 rtGetSocSpec 的规格查询桩：spec 里没有的键按真机行为返回 kSocKeyNotFound。
class SocSpecMockMmpa : public MmpaStubApiGe {
 public:
  int32_t RtGetSocSpec(const char *label, const char *key, char *value, const uint32_t size) override {
    if (ret_code != 0) {
      return ret_code;
    }
    const std::string spec_key =
        std::string((label == nullptr) ? "" : label) + "." + std::string((key == nullptr) ? "" : key);
    const auto iter = spec_values.find(spec_key);
    if (iter == spec_values.end()) {
      return kSocKeyNotFound;
    }
    if (iter->second.size() >= static_cast<size_t>(size)) {
      return 1;
    }
    (void)memcpy_s(value, size, iter->second.c_str(), iter->second.size() + 1U);
    return 0;
  }

  void SetCores(uint32_t ai_core_cnt, uint32_t cube_core_cnt, uint32_t vector_core_cnt) {
    Set(kAiCoreCntKey, std::to_string(ai_core_cnt));
    Set(kCubeCoreCntKey, std::to_string(cube_core_cnt));
    Set(kVectorCoreCntKey, std::to_string(vector_core_cnt));
  }

  void Set(const std::string &key, const std::string &value) {
    spec_values[std::string(kSocSpecLabel) + "." + key] = value;
  }

  int32_t ret_code = 0;
  std::map<std::string, std::string> spec_values;
};

class RecordingSlog : public SlogStub {
 public:
  RecordingSlog() {
    SetLevelDebug();
  }

  void Log(int, int level, const char *format, va_list args) override {
    if ((level != DLOG_INFO) && (level != DLOG_WARN) && (level != DLOG_ERROR)) {
      return;
    }
    char buffer[2048] = {0};
    if (vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1U, format, args) < 0) {
      return;
    }
    const std::lock_guard<std::mutex> lock(mutex_);
    logs_.emplace_back(buffer);
  }

  size_t CountContains(const std::string &keyword) const {
    const std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0U;
    for (const auto &log : logs_) {
      if (log.find(keyword) != std::string::npos) {
        ++count;
      }
    }
    return count;
  }

  bool Contains(const std::string &keyword) const {
    return CountContains(keyword) > 0U;
  }

  void Clear() {
    const std::lock_guard<std::mutex> lock(mutex_);
    logs_.clear();
  }

 private:
  mutable std::mutex mutex_;
  std::vector<std::string> logs_;
};

class ParseAicoreNumFromOptionUT : public testing::Test {
 protected:
  void SetUp() override {
    log_ = std::make_shared<RecordingSlog>();
    SlogStub::SetInstance(log_);
    mmpa_ = std::make_shared<SocSpecMockMmpa>();
    MmpaStub::GetInstance().SetImpl(mmpa_);
    ClearContextOptions();
  }

  void TearDown() override {
    MmpaStub::GetInstance().Reset();
    SlogStub::SetInstance(nullptr);
    ClearContextOptions();
  }

  static void ClearContextOptions() {
    GetThreadLocalContext().SetGlobalOption({});
    GetThreadLocalContext().SetSessionOption({});
    GetThreadLocalContext().SetGraphOption({});
  }

  static std::map<std::string, std::string> SingleValueOptions(const std::string &value) {
    return {{AICORE_NUM, value}};
  }

  static std::string GetVectorCoreNum(const std::map<std::string, std::string> &options) {
    const auto iter = options.find(kVectorCoreNum);
    return (iter == options.end()) ? kAbsent : iter->second;
  }

  std::shared_ptr<RecordingSlog> log_;
  std::shared_ptr<SocSpecMockMmpa> mmpa_;
};

// ------------------------------------------------------------------ 单值派生
TEST_F(ParseAicoreNumFromOptionUT, SingleValueRatioOneToTwoDerivesDouble) {
  mmpa_->SetCores(20U, 20U, 40U);
  for (const auto &[input, expect] :
       std::vector<std::pair<std::string, std::string>>{{"1", "2"}, {"6", "12"}, {"20", "40"}}) {
    SCOPED_TRACE(input);
    log_->Clear();
    auto options = SingleValueOptions(input);
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(options[AICORE_NUM], input);  // ge.aicoreNum 保持原值
    EXPECT_EQ(GetVectorCoreNum(options), expect);
    EXPECT_EQ(log_->CountContains("Derive ge.vectorcoreNum"), 1U);
    EXPECT_FALSE(log_->Contains("Invalid format for ge.aicoreNum"));
  }
}

TEST_F(ParseAicoreNumFromOptionUT, SingleValueRatioOneToOneDerivesSame) {
  mmpa_->SetCores(1U, 1U, 1U);
  auto options = SingleValueOptions("1");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), "1");
  EXPECT_EQ(log_->CountContains("Derive ge.vectorcoreNum"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, NoAicoreNumOptionKeepsOptionsUntouched) {
  std::map<std::string, std::string> options{{SOC_VERSION, "Ascend910_9362"}};
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(options[SOC_VERSION], "Ascend910_9362");
  EXPECT_FALSE(log_->Contains("Derive ge.vectorcoreNum"));
}

TEST_F(ParseAicoreNumFromOptionUT, EmptyAicoreNumOptionIsIgnored) {
  auto options = SingleValueOptions("");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_FALSE(log_->Contains("Invalid format for ge.aicoreNum"));
}

// ------------------------------------------------------------------ 单值非法
TEST_F(ParseAicoreNumFromOptionUT, InvalidSingleValuesWarnAndKeepUnchanged) {
  mmpa_->SetCores(20U, 20U, 40U);
  for (const std::string &input : {"abc", "6a", "+6", "-6", "06", "0006", " 6", "6 ", "6.0", ".6", "6e1", "0x6", "０６",
                                   "六", "2147483648", "99999999999"}) {
    SCOPED_TRACE(input);
    log_->Clear();
    auto options = SingleValueOptions(input);
    // 解析阶段只静默校验：返回成功、只告警、不派生，E10001 仍由 platform info 产生。
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
    EXPECT_EQ(options[AICORE_NUM], input);
    EXPECT_EQ(log_->CountContains("Invalid format for ge.aicoreNum"), 1U);
    EXPECT_FALSE(log_->Contains("Derive ge.vectorcoreNum"));
  }
}

TEST_F(ParseAicoreNumFromOptionUT, ZeroSingleValueWarnsAndDoesNotDerive) {
  mmpa_->SetCores(20U, 20U, 40U);
  auto options = SingleValueOptions("0");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(log_->CountContains("Invalid format for ge.aicoreNum"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, SingleValueOutOfPlatformRangeDoesNotDerive) {
  mmpa_->SetCores(20U, 20U, 40U);
  auto options = SingleValueOptions("21");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(log_->CountContains("Invalid format for ge.aicoreNum"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, SingleValueOutOfOneToOneRangeDoesNotDerive) {
  mmpa_->SetCores(1U, 1U, 1U);
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(log_->CountContains("Invalid format for ge.aicoreNum"), 1U);
}

// ------------------------------------------------------------------ 成对写法
TEST_F(ParseAicoreNumFromOptionUT, PairCompatibleWritesBothKeys) {
  mmpa_->SetCores(20U, 20U, 40U);
  struct PairCase {
    std::string input;
    std::string expect_aicore;
    std::string expect_vectorcore;
  };
  for (const auto &item : std::vector<PairCase>{{"6|6", "6", "6"},
                                                {"6|12", "6", "12"},
                                                {"6|", "6", ""},
                                                {"|6", "", "6"},
                                                {"6|0", "6", "0"},
                                                {"|", "", ""}}) {
    SCOPED_TRACE(item.input);
    log_->Clear();
    auto options = SingleValueOptions(item.input);
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(options[AICORE_NUM], item.expect_aicore);
    EXPECT_EQ(GetVectorCoreNum(options), item.expect_vectorcore);
    EXPECT_FALSE(log_->Contains("Invalid format for ge.aicoreNum"));
    EXPECT_FALSE(log_->Contains("Derive ge.vectorcoreNum"));
  }
}

TEST_F(ParseAicoreNumFromOptionUT, InvalidPairOnlySplitsWithoutParseWarning) {
  mmpa_->SetCores(20U, 20U, 40U);
  struct PairCase {
    std::string input;
    std::string expect_aicore;
    std::string expect_vectorcore;
  };
  for (const auto &item : std::vector<PairCase>{{"||", "", "|"},
                                                {"6|6|6", "6", "6|6"},
                                                {"6||12", "6", "|12"},
                                                {"6| 12", "6", " 12"},
                                                {"6|abc", "6", "abc"},
                                                {"6|-1", "6", "-1"}}) {
    SCOPED_TRACE(item.input);
    log_->Clear();
    auto options = SingleValueOptions(item.input);
    // 成对分支不做解析期校验：只拆分、不告警、不派生，失败仍由 platform info/FE 报出。
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(options[AICORE_NUM], item.expect_aicore);
    EXPECT_EQ(GetVectorCoreNum(options), item.expect_vectorcore);
    EXPECT_FALSE(log_->Contains("Invalid format for ge.aicoreNum"));
    EXPECT_FALSE(log_->Contains("Derive ge.vectorcoreNum"));
  }
}

// ------------------------------------------------------------------ 显式优先
TEST_F(ParseAicoreNumFromOptionUT, ExplicitVectorCoreNumWinsOverSingleValue) {
  mmpa_->SetCores(20U, 20U, 40U);
  auto options = SingleValueOptions("6");
  options[kVectorCoreNum] = "8";
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), "8");
  EXPECT_EQ(options[AICORE_NUM], "6");
  EXPECT_EQ(log_->CountContains("configured explicitly"), 1U);
  EXPECT_FALSE(log_->Contains("Derive ge.vectorcoreNum"));
}

TEST_F(ParseAicoreNumFromOptionUT, ExplicitEmptyVectorCoreNumIsTreatedAsUnconfigured) {
  mmpa_->SetCores(20U, 20U, 40U);
  for (const std::string &explicit_value : {"", "0"}) {
    SCOPED_TRACE(explicit_value);
    log_->Clear();
    auto options = SingleValueOptions("6");
    options[kVectorCoreNum] = explicit_value;
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(GetVectorCoreNum(options), "12");
    EXPECT_EQ(log_->CountContains("treated as unconfigured"), 1U);
  }
}

TEST_F(ParseAicoreNumFromOptionUT, ExplicitInvalidVectorCoreNumSkipsDeriving) {
  mmpa_->SetCores(20U, 20U, 40U);
  for (const std::string &explicit_value : {"abc", "-1", "99999999999"}) {
    SCOPED_TRACE(explicit_value);
    log_->Clear();
    auto options = SingleValueOptions("6");
    options[kVectorCoreNum] = explicit_value;
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(GetVectorCoreNum(options), explicit_value);  // 不覆盖，交由 platform info 报 E10001
    EXPECT_FALSE(log_->Contains("Derive ge.vectorcoreNum"));
    EXPECT_FALSE(log_->Contains("Invalid format for ge.aicoreNum"));
  }
}

TEST_F(ParseAicoreNumFromOptionUT, ExplicitVectorCoreNumFromThreadLocalContextWins) {
  mmpa_->SetCores(20U, 20U, 40U);
  GetThreadLocalContext().SetGlobalOption({{kVectorCoreNum, "8"}});
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(log_->CountContains("configured explicitly"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, ExplicitEmptyVectorCoreNumInContextStillDerives) {
  mmpa_->SetCores(20U, 20U, 40U);
  GetThreadLocalContext().SetGlobalOption({{kVectorCoreNum, ""}});
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), "12");
  EXPECT_EQ(log_->CountContains("treated as unconfigured"), 1U);
}

// ------------------------------------------------------------------ 冲突
TEST_F(ParseAicoreNumFromOptionUT, PairConflictWithExplicitValueReturnsParamInvalid) {
  mmpa_->SetCores(20U, 20U, 40U);
  auto options = SingleValueOptions("6|12");
  options[kVectorCoreNum] = "8";
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), PARAM_INVALID);
  EXPECT_EQ(log_->CountContains("Conflict: ge.vectorcoreNum is set to"), 1U);
  EXPECT_EQ(log_->CountContains("E10001"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, PairConflictWithContextExplicitValueReturnsParamInvalid) {
  mmpa_->SetCores(20U, 20U, 40U);
  GetThreadLocalContext().SetSessionOption({{kVectorCoreNum, "8"}});
  auto options = SingleValueOptions("6|12");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), PARAM_INVALID);
  EXPECT_EQ(log_->CountContains("Conflict: ge.vectorcoreNum is set to"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, PairAgreeingWithExplicitValueSucceeds) {
  mmpa_->SetCores(20U, 20U, 40U);
  auto options = SingleValueOptions("6|12");
  options[kVectorCoreNum] = "12";
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(options[AICORE_NUM], "6");
  EXPECT_EQ(GetVectorCoreNum(options), "12");
  EXPECT_FALSE(log_->Contains("Conflict: ge.vectorcoreNum is set to"));
}

TEST_F(ParseAicoreNumFromOptionUT, EmptyPairSideKeepsExplicitVectorCoreNum) {
  mmpa_->SetCores(20U, 20U, 40U);
  for (const std::string &pair_value : {"6|", "6|0"}) {
    SCOPED_TRACE(pair_value);
    auto options = SingleValueOptions(pair_value);
    options[kVectorCoreNum] = "8";
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(options[AICORE_NUM], "6");
    EXPECT_EQ(GetVectorCoreNum(options), "8");  // 空侧不覆盖显式值
  }
}

// ------------------------------------------------------------------ 能力缺失降级
TEST_F(ParseAicoreNumFromOptionUT, UnsupportedRatioWarnsAndDoesNotDerive) {
  mmpa_->SetCores(10U, 10U, 8U);  // 10:8 产品
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(log_->CountContains("skip deriving ge.vectorcoreNum"), 1U);
  EXPECT_TRUE(log_->Contains("ratio of vector_core_cnt(8) to cube_core_cnt(10) is not supported"));
}

TEST_F(ParseAicoreNumFromOptionUT, NoVectorCoreWarnsAndDoesNotDerive) {
  mmpa_->SetCores(32U, 32U, 0U);  // 无独立 Vector 产品
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_TRUE(log_->Contains("vector_core_cnt is 0 (no independent vector core)"));
}

TEST_F(ParseAicoreNumFromOptionUT, SocSpecQueryFailureWarnsAndDoesNotDerive) {
  mmpa_->ret_code = kSocKeyNotFound;
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
  EXPECT_EQ(log_->CountContains("failed to get SoC spec"), 1U);
}

TEST_F(ParseAicoreNumFromOptionUT, SocSpecEmptyOrNonNumericValueWarnsAndDoesNotDerive) {
  // 后三个值用于覆盖 stoull 宽松解析的绕过场景：前导空白、正号、空格+负号回绕（旧实现会被解析成 1）
  for (const std::string &vector_value : {"", "abc", "-1", "6.5", " 6", "+6", " -18446744073709551615"}) {
    SCOPED_TRACE(vector_value);
    log_->Clear();
    mmpa_->spec_values.clear();
    mmpa_->Set(kAiCoreCntKey, "20");
    mmpa_->Set(kCubeCoreCntKey, "20");
    mmpa_->Set(kVectorCoreCntKey, vector_value);
    auto options = SingleValueOptions("6");
    EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
    EXPECT_EQ(GetVectorCoreNum(options), kAbsent);
    EXPECT_EQ(log_->CountContains("failed to get SoC spec"), 1U);
  }
}

TEST_F(ParseAicoreNumFromOptionUT, MissingCubeCoreCntFallsBackToAiCoreCnt) {
  mmpa_->spec_values.clear();
  mmpa_->Set(kAiCoreCntKey, "20");
  mmpa_->Set(kVectorCoreCntKey, "40");  // 无 cube 键，回退 ai_core_cnt = 20，比例仍为 1:2
  auto options = SingleValueOptions("6");
  EXPECT_EQ(CoreNumUtils::ParseAicoreNumFromOption(options), GRAPH_SUCCESS);
  EXPECT_EQ(GetVectorCoreNum(options), "12");
  EXPECT_EQ(log_->CountContains("Derive ge.vectorcoreNum"), 1U);
}
}  // namespace
}  // namespace ge
