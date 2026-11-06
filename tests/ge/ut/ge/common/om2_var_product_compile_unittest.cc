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
#include <cstdlib>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/helper/om2/om2_utils.h"
#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/ast/ast_nodes.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"
#include "common/om2/codegen/file_code_generator/var_mgr_file_code_generator.h"
#include "common/om2/codegen/om2_codegen_types.h"
#include "framework/common/gert_model_data_utils.h"
#include "framework/om2/model_data/gert_model_data.h"
#include "graph/ge_local_context.h"

// <model>_var_mgr.cpp 的产物编译验证。
//
// 为什么需要它：生成器自身的 UT 只做子串断言，而子串断言证明不了产物是合法 C++。拼接产物
// 一旦缺某个声明（如 ge::PtrAdd），任何带格式转换的模型 SO 都编译不过，这类缺陷只有真编一遍
// 才能暴露——既有的独立编译 UT 在 x86 上 GTEST_SKIP，覆盖不到。
//
// 覆盖的门控组合以"链接是否自洽"为主轴：var_mgr 的 TransAllVarData 恒定引用 ::gert::var_trans
// 的两个入口，BuildSegments 恒定定义二者（能力缺失的那个是桩），故每种能力组合都必须能编译
// 并链接成一个 SO。
namespace ge {
std::string_view GetOm2ModelApiHeader();

namespace {
constexpr char const *kModelName = "vtc";

// 在 main 之前求值：om2_package_helper_unittest.cc 会 unsetenv("ASCEND_HOME_PATH")
// 且不恢复，全量跑时本套件排在其后就会误跳过。故先把进程启动时的值存下来，SetUp 时恢复。
// 不修那个文件：它是既有缺陷、与本特性无关，且改它可能影响其自身断言。
const std::string kSavedAscendHome = [] {
  const char *value = getenv("ASCEND_HOME_PATH");
  return (value != nullptr) ? std::string(value) : std::string();
}();

// 与 ProgramGenerator::GenerateMakeFile 的 CPPFLAGS/CXXFLAGS/LDFLAGS/LDLIBS 保持一致——那才是
// 产物真实的编译参数，换成测试专用的宽松参数就验不出问题。TARGET 与 SRC_FILES 会被
// BuildCompileMakefileData 改写成 memfd 路径，故此处必须保留这两个变量名。
const char *const kMakefile = R"(CANN_ROOT ?= $(ASCEND_HOME_PATH)
USE_STUB_LIB ?= 1

ifeq ($(origin CXX),default)
CXX := c++
endif
CXX ?= c++

TARGET := ../libvtc_om2.so
SRC_FILES := vtc_var_mgr.cpp

CPPFLAGS := \
  -I$(CANN_ROOT)/include \
  -I$(CANN_ROOT)/include/external \
  -I$(CANN_ROOT)/pkg_inc \
  -I$(CANN_ROOT)/pkg_inc/base \
  -I$(CANN_ROOT)/pkg_inc/runtime \
  -I$(CANN_ROOT)/pkg_inc/profiling \
  -I$(CURDIR)/include

CXXFLAGS := -std=c++17 -O2 -fPIC -fstack-protector-all -D_FORTIFY_SOURCE=2

ifeq ($(USE_STUB_LIB),1)
LIB_PATH ?= $(CANN_ROOT)/devlib
else
LIB_PATH ?= $(CANN_ROOT)/lib64
endif

LDFLAGS := -shared -L$(LIB_PATH) -Wl,--no-as-needed -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -s
LDLIBS := -lacl_rt -Wl,--as-needed

.PHONY: all

all: $(TARGET)

$(TARGET): $(SRC_FILES)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
)";

// 桩 _internal.h。真实产物由 InterfaceFileCodeGenerator 生成、体量很大，而 var_mgr/var_trans
// 只需要它传递来的 om2_model_api.h（GertModelVarContext 等）与两组宏：
//   - OM2_LOGE：真实实现走 DlogRecord，依赖 dlog_pub.h——该头不在工具包对外可见路径内，故以
//     snprintf 复刻。用 "" __VA_ARGS__ 而非 fmt 形参，既容忍无消息调用，又让编译器对格式串做
//     实参个数/类型校验，var_mgr 的失败日志因此能在本用例中被静态检查。
//   - OM2_CHK_STATUS / OM2_CHK_NOTNULL / OM2_CHK_TRUE：逐字取自 stable_part_provider.cc 的
//     kInterfaceMacros，语义必须与真实产物一致，否则验不出 return 类型与错误码传播的问题。
// 内存编译路径只改写 _internal.h 内对 om2_model_api.h 的 include，故桩必须保留这一行。
const char *const kStubInternalHeader = R"STUB(#pragma once
#include "om2_model_api.h"
#include <cstdio>

#define OM2_LOGE(...) \
  do { \
    (void)std::snprintf(nullptr, 0, "" __VA_ARGS__); \
  } while (false)

#define OM2_CHK_STATUS(expr, ...)            \
  do {                                       \
    const aclError _chk_status = (expr);     \
    if (_chk_status != ACL_SUCCESS) {        \
      OM2_LOGE(__VA_ARGS__);                 \
      return _chk_status;                    \
    }                                        \
  } while (false)

#define OM2_CHK_NOTNULL(ptr, ...)            \
  do {                                       \
    if ((ptr) == nullptr) {                  \
      OM2_LOGE(__VA_ARGS__);                 \
      return ACL_ERROR_FAILURE;              \
    }                                        \
  } while (false)

#define OM2_CHK_TRUE(expr, ...)              \
  do {                                       \
    if (!(expr)) {                           \
      OM2_LOGE(__VA_ARGS__);                 \
      return ACL_ERROR_FAILURE;              \
    }                                        \
  } while (false)
)STUB";

gert::GertModelDataFile MakeArtifact(const std::string &file_name, const std::string &content) {
  gert::GertModelDataFile artifact;
  artifact.file_name = gert::GertMakeStr(file_name);
  artifact.data = gert::GertMakeFileData(content.data(), content.size());
  artifact.data_size = content.size();
  return artifact;
}

VarFixupEntry MakeEntry(const uint32_t slot, const std::string &var_key, const uint64_t size,
                        const uint64_t init_offset, const uint64_t init_size) {
  VarFixupEntry entry;
  entry.slot = slot;
  entry.var_key = var_key;
  entry.size = size;
  entry.memory_type = 2U;  // RT_MEMORY_HBM
  entry.data_type = 0U;    // DT_FLOAT
  entry.logic_addr = 0xC000000000ULL;
  entry.init_data_offset = init_offset;
  entry.init_data_size = init_size;
  return entry;
}

VarTransNodeFixup MakeNode(const VarTransKind kind) {
  VarTransNodeFixup node;
  node.kind = kind;
  node.src_format = static_cast<uint32_t>(FORMAT_NCHW);
  node.dst_format = static_cast<uint32_t>(FORMAT_NHWC);
  node.src_data_type = 0U;
  node.dst_data_type = 0U;
  node.src_dims = {1, 2, 3, 4};
  node.dst_dims = {1, 3, 4, 2};
  node.src_dim_num = 4U;
  node.dst_dim_num = 4U;
  return node;
}

// 指定主格式对的格式转换节点。格式转换单元按转换链里出现的格式对选择，格式对决定内嵌哪些单元。
VarTransNodeFixup MakeFormatNode(const Format src, const Format dst) {
  auto node = MakeNode(VarTransKind::kFormat);
  node.src_format = static_cast<uint32_t>(src);
  node.dst_format = static_cast<uint32_t>(dst);
  return node;
}

// ---- 六个场景的固化表构造。每个函数对应下面一个 TEST_F，注释即该场景的存在理由。----

// 有变量、无转换需求：两个入口退化为桩、FixedUnits 与两类转换单元一个都不内嵌，
// 但 var_mgr 的 TransAllVarData 仍恒定引用两个入口 ⇒ 桩的签名必须与调用点逐字一致，
// 否则链接失败。这是桩路径唯一的编译验证。
VarFixupTable MakeStubOnlyTable() {
  VarFixupTable table;
  table.graph_id = 11U;
  table.entries.push_back(MakeEntry(0U, "w1_0", 96U, 0U, 96U));
  table.weight_data.assign(96U, 0xA5U);
  return table;
}

// format-only：TransFormat 是真实体（依赖 FixedUnits 与格式转换单元），
// TransDataType 是桩（数据类型转换单元未内嵌）。
// 两个混合形态之一，专门验证"一侧真实体 + 一侧桩"仍能链接。
VarFixupTable MakeFormatOnlyTable() {
  VarFixupTable table;
  table.graph_id = 22U;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeNode(VarTransKind::kFormat));
  entry.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(entry));
  return table;
}

// cast-only：与上条对称，TransDataType 是真实体（依赖 FixedUnits 与数据类型转换单元），
// TransFormat 是桩。
VarFixupTable MakeCastOnlyTable() {
  VarFixupTable table;
  table.graph_id = 33U;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeNode(VarTransKind::kCast));
  entry.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(entry));
  return table;
}

// 全能力 + 全部数据形态：FixedUnits 与两类转换单元都内嵌，两个入口都是真实体，
// 转换、拷贝、填表三条路径都有活干。
// 顺带覆盖转义键（含引号与反斜杠）、无槽位拷贝源、CONSTPLACEHOLDER 的外部设备地址。
VarFixupTable MakeBothCapabilitiesTable() {
  VarFixupTable table;
  table.graph_id = 44U;
  auto trans = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  trans.trans_road.push_back(MakeNode(VarTransKind::kFormat));
  trans.trans_road.push_back(MakeNode(VarTransKind::kCast));
  trans.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(trans));
  table.entries.push_back(MakeEntry(kVarNoSlot, "src0_0", 96U, 0U, 96U));
  auto copy_dst = MakeEntry(1U, "we\"ird\\key", 96U, 0U, 0U);
  copy_dst.copy_src_var_key = "src0_0";
  table.entries.push_back(std::move(copy_dst));
  auto placeholder = MakeEntry(2U, "ph4_0", 16U, 0U, 0U);
  placeholder.extern_dev_addr = 0x7F0000001000ULL;
  table.entries.push_back(std::move(placeholder));
  table.weight_data.assign(96U, 0x5AU);
  return table;
}

// 异 dtype 的变量间拷贝，且全表无 kCast 节点：数据类型转换单元仅因 NeedDataTypeCapability 的
// copy_src_data_type != data_type 判据而内嵌，唯一消费者是 CopyVarData 的 CAST 分支。
// 缺这条 CAST 时这类模型会白背整批数据类型转换单元却从不使用（能力判据与消费点脱节）。
VarFixupTable MakeCopyAcrossDataTypeTable() {
  VarFixupTable table;
  table.graph_id = 66U;
  auto copy_src = MakeEntry(kVarNoSlot, "src0_0", 48U, 0U, 48U);
  copy_src.data_type = 1U;  // DT_FLOAT16，2 字节
  table.entries.push_back(std::move(copy_src));
  auto copy_dst = MakeEntry(0U, "dst0_0", 96U, 0U, 0U);
  copy_dst.copy_src_var_key = "src0_0";
  copy_dst.data_type = 0U;           // DT_FLOAT，4 字节
  copy_dst.copy_src_data_type = 1U;  // 与目标不同 ⇒ CopyVarData 须 CAST
  table.entries.push_back(std::move(copy_dst));
  table.weight_data.assign(48U, 0x5AU);
  return table;
}

// fz_c04 是唯一有跨单元依赖的格式转换单元：om2_format_transfer_fz_c04.cc include 了
// transpose 的头并直调 gert::formats::Transpose，运行期还会经注册表回查 HWCN -> NCHW。
// 依赖闭包若漏掉 transpose，拼接产物会因符号缺失编译/链接失败。
VarFixupTable MakeFzC04ClosureTable() {
  VarFixupTable table;
  table.graph_id = 55U;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeFormatNode(FORMAT_HWCN, FORMAT_FRACTAL_Z_C04));
  entry.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(entry));
  return table;
}

// 场景下标，与 kScenarioMakers 一一对应；TEST_F 用它取自己那份编译结果。
enum ScenarioIndex : size_t {
  kStubOnly = 0U,
  kFormatOnly,
  kCastOnly,
  kBothCapabilities,
  kCopyAcrossDataType,
  kFzC04Closure,
  kScenarioCount
};

using TableMaker = VarFixupTable (*)();
constexpr TableMaker kScenarioMakers[kScenarioCount] = {MakeStubOnlyTable,           MakeFormatOnlyTable,
                                                        MakeCastOnlyTable,           MakeBothCapabilitiesTable,
                                                        MakeCopyAcrossDataTypeTable, MakeFzC04ClosureTable};

// 单个场景的编译结果。SetUpTestSuite 的工作线程只填自己那一份，TEST_F 各自断言，
// 故一个场景坏掉只挂对应用例、不会牵连其余五个。
struct CompileOutcome {
  Status mgr_build = SUCCESS;
  Status compile = FAILED;
  std::string so_name;
  bool so_data_non_null = false;
  size_t so_data_size = 0U;
};

// 与 base/common/om2/codegen/program_generator.cc 的 EmitFile 一致：atc 经 /proc/self/fd/N
// 传源码给编译器，__FILE__ 会展开为 fd 路径，故插 #line 指令纠正。
// 不用 gtest 宏：本函数在工作线程内执行，断言归属不明确，故以返回码传递失败。
Status EmitToString(const DeclNode *node, std::string &code) {
  CppEmitter emitter;
  code = "#line 1 \"" + std::string(kModelName) + "_var.cpp\"\n";
  return node->Accept(emitter, code);
}

// 跑生成器、发射、连同 om2_model_api.h / 桩 _internal.h / Makefile 一起交给生产路径
// Om2Utils::CompileGeneratedCppToSo 编成 SO。转换部分与变量准备同处 <model>_var_mgr.cpp，
// 故只有一个 .cpp 产物、只编一个 TU。产物以 HasVariables() 门控：空表时不生成、无 cpp 可编，
// 故只用于有变量的场景（真出现空表会撞上 CompileGeneratedCppToSo 的 !cpp_files.empty()）。
// 全程不使用 gtest 宏，也不触碰跨线程共享的可变状态：AstContext / AstBuildContext 都是本函数
// 局部量，产物缓冲区经 GertMakeFileData 各自堆分配。
CompileOutcome CompileOne(const VarFixupTable &table) {
  CompileOutcome outcome;
  AstContext ctx;
  AstBuildContext ast{ctx};
  std::vector<gert::GertModelDataFile> artifacts;
  artifacts.push_back(MakeArtifact("om2_model_api.h", std::string(GetOm2ModelApiHeader())));
  // 桩 _internal.h：内容见 kStubInternalHeader 的注释
  artifacts.push_back(MakeArtifact(std::string(kModelName) + "_internal.h", std::string(kStubInternalHeader)));
  artifacts.push_back(MakeArtifact("Makefile", kMakefile));

  const VarMgrFileCodeGenerator mgr(ast, kModelName, table);
  if (mgr.HasVariables()) {
    DeclNode *mgr_file = nullptr;
    outcome.mgr_build = mgr.BuildFile(mgr_file);
    if ((outcome.mgr_build != SUCCESS) || (mgr_file == nullptr)) {
      return outcome;
    }
    std::string code;
    outcome.mgr_build = EmitToString(mgr_file, code);
    if (outcome.mgr_build != SUCCESS) {
      return outcome;
    }
    artifacts.push_back(MakeArtifact(std::string(kModelName) + "_var_mgr.cpp", code));
  }

  gert::GertModelDataFile so_artifact;
  outcome.compile =
      Om2Utils::CompileGeneratedCppToSo(artifacts, Om2CompileInfo{kModelName, kModelName, 0U}, so_artifact, false);
  outcome.so_name = gert::GertGetStr(so_artifact.file_name);
  outcome.so_data_non_null = (so_artifact.data != nullptr);
  outcome.so_data_size = so_artifact.data_size;
  return outcome;
}

class VarProductCompileUt : public testing::Test {
 protected:
  // 六个场景各自真编一个 SO，单个 g++ 就要秒级（地板是 om2_thread_pool 正当需要的 STL 头，
  // 裁剪 include 或降优化级别都动不了它）。串行跑会让每个用例都撞上 CI 的 check_slow_tests
  // （阈值 1000 ms），故在此并行编译、用例体只做断言。并发度 6 = 场景数。
  static void SetUpTestSuite() {
    // 产物要 include acl/acl.h 等工具包公开头，经 Makefile 的 -I$(CANN_ROOT)/… 解析，而
    // CANN_ROOT ?= $(ASCEND_HOME_PATH)。两者都没设时无从编译，按既有先例跳过。不在此处
    // GTEST_SKIP：那会整套件跳过，改为置标志、由各用例自行 skip。
    if (!kSavedAscendHome.empty()) {
      (void)setenv("ASCEND_HOME_PATH", kSavedAscendHome.c_str(), 1);
    }
    if ((getenv("ASCEND_HOME_PATH") == nullptr) && (getenv("CANN_ROOT") == nullptr)) {
      toolkit_available_ = false;
      return;
    }
    // 抑制子进程（make/g++）的 LSAN 检测，避免 g++ 编译器内部泄漏触发 LeakSanitizer 报错导致
    // so 编译失败。本套件经生产路径 CompileGeneratedCppToSo 在运行期真编 SO 故必须处理；
    // 非 ASan 构建下这两个变量本就无消费者，设置无副作用。
    (void)setenv("ASAN_OPTIONS", "detect_leaks=0:halt_on_error=0", 1);
    (void)setenv("LSAN_OPTIONS", "exitcode=0", 1);
    toolkit_available_ = true;

    outcomes_.assign(kScenarioCount, CompileOutcome{});
    // GEContext::GetOption 实际读的是 static thread_local GEThreadLocalContext，裸 std::thread 的
    // 工作线程会拿到空上下文，使 kOptionBuildConfig 与 OPTION_HOST_ENV_OS/CPU 全部落空、与串行版
    // 行为不一致。故按 thread_pool.cc 的既有范式，在父线程捕获、进 worker 后先恢复。
    const auto parent_ctx = GetThreadLocalContext();
    std::vector<std::thread> workers;
    workers.reserve(kScenarioCount);
    for (size_t i = 0U; i < kScenarioCount; ++i) {
      workers.emplace_back([i, &parent_ctx]() {
        GetThreadLocalContext() = parent_ctx;
        // 各 worker 只写 outcomes_[i]：vector 已预分配到位、元素互不相同，无竞争。
        outcomes_[i] = CompileOne(kScenarioMakers[i]());
      });
    }
    for (auto &worker : workers) {
      worker.join();
    }
  }

  static void TearDownTestSuite() {
    outcomes_.clear();
    // 与 tests/ge/ut/ge/graph/load/om2_package_helper_unittest.cc 的先例一致，不把抑制项留给后续套件。
    unsetenv("ASAN_OPTIONS");
    unsetenv("LSAN_OPTIONS");
  }

  // 断言第 idx 个场景的编译结果。工具包不可达时跳过而非失败。
  void AssertOutcome(const size_t idx) const {
    if (!toolkit_available_) {
      GTEST_SKIP() << "toolkit headers are unreachable without ASCEND_HOME_PATH or CANN_ROOT";
    }
    ASSERT_LT(idx, outcomes_.size());
    const auto &outcome = outcomes_[idx];
    ASSERT_EQ(outcome.mgr_build, SUCCESS) << "var_mgr BuildFile/Emit failed";
    ASSERT_EQ(outcome.compile, SUCCESS) << "CompileGeneratedCppToSo failed";
    EXPECT_EQ(outcome.so_name, std::string("lib") + kModelName + "_om2.so");
    EXPECT_TRUE(outcome.so_data_non_null);
    EXPECT_GT(outcome.so_data_size, 0U);
  }

  static inline bool toolkit_available_ = false;
  static inline std::vector<CompileOutcome> outcomes_;
};

TEST_F(VarProductCompileUt, VariablesWithoutTransCompilesAgainstStubEntries) {
  AssertOutcome(kStubOnly);
}

TEST_F(VarProductCompileUt, FormatOnlyCompilesWithDataTypeStub) {
  AssertOutcome(kFormatOnly);
}

TEST_F(VarProductCompileUt, CastOnlyCompilesWithFormatStub) {
  AssertOutcome(kCastOnly);
}

TEST_F(VarProductCompileUt, BothCapabilitiesWithCopyAndPlaceholderCompile) {
  AssertOutcome(kBothCapabilities);
}

TEST_F(VarProductCompileUt, CopyAcrossDataTypeCompiles) {
  AssertOutcome(kCopyAcrossDataType);
}

TEST_F(VarProductCompileUt, FzC04ClosureCompiles) {
  AssertOutcome(kFzC04Closure);
}
}  // namespace
}  // namespace ge
