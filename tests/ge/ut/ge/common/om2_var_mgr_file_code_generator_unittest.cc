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
#include <set>
#include <string>

#include "common/om2/codegen/ast/ast_build_context.h"
#include "common/om2/codegen/ast/ast_nodes.h"
#include "common/om2/codegen/emitter/cpp_emitter.h"
#include "common/om2/codegen/file_code_generator/var_mgr_file_code_generator.h"
#include "common/om2/codegen/om2_codegen_types.h"

namespace ge {
namespace {
// 内联转换单元的边界注释前缀。转换部分与变量准备同处一个产物，
// "include 必须先于首个内联单元"这类跨部分次序断言在此锚定。
constexpr char const *kUnitMark = "/* ===== unit: ";
// 文件头固定 kHeaderIncludeNum 个引号 include。内嵌源码（转换单元与线程池）的引号 include
// 全部被整行删除，故产物中引号 include 总数恒定，多一个即说明有单元把自己的带了进来。
constexpr size_t kHeaderIncludeNum = 8U;

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

VarTransNodeFixup MakeTransNode(VarTransKind kind) {
  VarTransNodeFixup node;
  node.kind = kind;
  // 格式对必须是注册表里真有的：BuildFile 会经 BuildSegments → SelectUnits →
  // CollectFormatUnits 校验每个 kFormat 节点，未注册的对直接编译期失败。NCHW → NHWC 已注册。
  node.src_format = 0U;  // FORMAT_NCHW
  node.dst_format = 1U;  // FORMAT_NHWC
  node.src_data_type = 0U;
  node.dst_data_type = 0U;
  node.src_dims = {1, 2, 3, 4};
  node.dst_dims = {1, 3, 4, 2};
  node.src_dim_num = 4U;
  node.dst_dim_num = 4U;
  return node;
}

// AstContext 是节点的竞技场，必须活到发射结束；AstBuildContext 不可拷贝，故由 fixture 持有。
// 成员声明次序即初始化次序：ctx 在前，ast{ctx} 才有效。
class VarMgrFileCodeGeneratorTest : public testing::Test {
 protected:
  // BuildFile 后发射为产物文本。调用点只关心文本内容，
  // 逐个写 CppEmitter + std::string + Accept 会淹没断言主体。
  std::string GenerateOk(const VarMgrFileCodeGenerator &generator) const {
    CppEmitter emitter;
    std::string code;
    DeclNode *file = nullptr;
    EXPECT_EQ(generator.BuildFile(file), SUCCESS);
    EXPECT_EQ(file->Accept(emitter, code), SUCCESS);
    return code;
  }

  // 出现次数。用于钉住"回调只在一处被调用""调用点个数"这类结构性不变量，
  // 单靠 find != npos 无法表达。
  static size_t Count(const std::string &haystack, const std::string &needle) {
    size_t total = 0U;
    size_t pos = 0U;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
      ++total;
      pos += needle.size();
    }
    return total;
  }

  // decl_pos 处的作用域是否一直未闭合到 use_pos，用于钉住"池与 get() 循环同作用域"这类作用域
  // 不变量——find != npos 证明不了嵌套关系，"深度相等"也不行（正确代码里 get() 在 for 体内，
  // 深度本就大 1）。判据只能是"从池声明扫到 get()，深度从未跌破池声明处的深度"。
  // 前提：产物内注释与字符串字面量里 0 个花括号，故逐字符计数不受干扰。
  static bool ScopeSpans(const std::string &code, size_t decl_pos, size_t use_pos) {
    size_t base = 0U;
    for (size_t i = 0U; i < decl_pos; ++i) {
      if (code[i] == '{') {
        ++base;
      } else if (code[i] == '}') {
        --base;
      }
    }
    size_t depth = base;
    for (size_t i = decl_pos; i < use_pos; ++i) {
      if (code[i] == '{') {
        ++depth;
      } else if (code[i] == '}') {
        --depth;
      }
      if (depth < base) {
        return false;
      }
    }
    return true;
  }

  // 池的机制全部收进内嵌类后，分派点不得再残留任何协调量。这五个名字是同一条不变量
  // （"串/并行分派不再自己管线程"）的五个侧面，故用循环表达而非五份拷贝。
  static void ExpectNoDispatchLeftovers(const std::string &code) {
    for (const auto *sym : {"next_task", "task_status", "spawn_failed", "ctx_failed", "workers"}) {
      EXPECT_EQ(Count(code, sym), 0U) << "分派点残留协调量: " << sym;
    }
  }

  AstContext ctx;
  AstBuildContext ast{ctx};
};

// 有变量 ⇒ 发射完整骨架：三步准备、线程池、转换部分（适配头 + 内嵌单元 + 两个入口）全在，
// 与是否需要转换无关。能力差异只体现在内嵌哪些单元、入口是真实体还是桩。
TEST_F(VarMgrFileCodeGeneratorTest, HasVariablesGeneratesCompleteSkeleton) {
  VarFixupTable table;
  table.graph_id = 11U;
  table.entries.push_back(MakeEntry(0U, "w1_0", 96U, 0U, 96U));
  table.weight_data.assign(96U, 0xA5U);

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  EXPECT_TRUE(generator.HasVariables());

  const auto code = GenerateOk(generator);
  // 唯一可直接 include 的项目头：om2_model_api.h 只能经它传递取得，
  // .cpp 直接 include 会 fatal error（内存编译路径只改写对 _internal.h 的 include）
  EXPECT_NE(code.find("#include \"g1_internal.h\""), std::string::npos);
  EXPECT_EQ(code.find("#include \"om2_model_api.h\""), std::string::npos);
  // 固化表与三步准备入口。表长直接用 kVarNum：本文件只在有变量时生成，kVarNum 恒 >= 1，
  // 无需 (kVarNum == 0U) ? 1U : kVarNum 的零长数组占位
  EXPECT_NE(code.find("const VarMeta kVarMetas[kVarNum]"), std::string::npos);
  EXPECT_EQ(code.find("kVarTableSize"), std::string::npos);
  EXPECT_NE(code.find("constexpr uint32_t kVarNum = 1U;"), std::string::npos);
  EXPECT_NE(code.find("constexpr uint32_t kSlotNum = 1U;"), std::string::npos);
  EXPECT_NE(code.find("constexpr uint64_t kModelGraphId = 11ULL;"), std::string::npos);
  EXPECT_NE(code.find("aclError PrepareVars(const GertModelVarContext *ctx, void **var_addrs, uint32_t slot_num)"),
            std::string::npos);
  EXPECT_NE(code.find("\"w1_0\", 96ULL, 2U, 0xc000000000ULL"), std::string::npos);
  // 转换动作枚举必须发射：SO 侧只对枚举 switch，不做字符串比较
  EXPECT_NE(code.find("enum VarTransKind : uint8_t"), std::string::npos);
  EXPECT_NE(code.find("kVarTransNoOp = 0"), std::string::npos);
  EXPECT_NE(code.find("kVarTransFormat = 1"), std::string::npos);
  EXPECT_NE(code.find("kVarTransCast = 2"), std::string::npos);
  EXPECT_NE(code.find("kVarTransUnsupported = 3"), std::string::npos);
  // 无转换需求也照发转换/拷贝路径与线程池：门控只有 HasVariables 一个
  EXPECT_NE(code.find("ACL_MEMCPY_DEVICE_TO_HOST"), std::string::npos);
  EXPECT_NE(code.find("kVarTransThreadNum"), std::string::npos);
  EXPECT_NE(code.find("::gert::var_trans::TransFormat("), std::string::npos);

  // 适配头随转换部分进入本产物：它复刻的 UNSUPPORTED 正是桩入口的返回值。
  EXPECT_NE(code.find("static_cast<ge::Status>(0x50100064U)"), std::string::npos);
  // gert::var_trans 两个入口的【定义】必须先于 gert::var_mgr：跨 TU 的 extern 前置声明在 -shared
  // 下允许未定义符号 ⇒ 声明与定义不一致要拖到 dlopen 期才暴露；同 TU 内则退化为编译错误。
  const auto trans_def_pos = code.find("ge::Status TransFormat(");
  const auto mgr_ns_pos = code.find("namespace var_mgr");
  ASSERT_NE(trans_def_pos, std::string::npos);
  ASSERT_NE(mgr_ns_pos, std::string::npos);
  EXPECT_LT(trans_def_pos, mgr_ns_pos);
}

// 守 graph/def_types.h 必须先于首个内联单元：它提供 ge::PtrAdd，内嵌单元普遍调用，缺了产物报
// "'PtrAdd' was not declared in this scope"。为何要显式补回见 BuildIncludes 处注释。
TEST_F(VarMgrFileCodeGeneratorTest, HeaderRestoresDefTypesBeforeFirstUnit) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  const auto def_types_pos = code.find("#include \"graph/def_types.h\"");
  const auto first_unit_pos = code.find(std::string(kUnitMark) + "runtime/om2/formats/");
  ASSERT_NE(def_types_pos, std::string::npos);
  ASSERT_NE(first_unit_pos, std::string::npos);
  // 必须出现在首个内联单元之前，否则单元正文里的 PtrAdd 调用点仍先于声明
  EXPECT_LT(def_types_pos, first_unit_pos);
  // 内联单元的引号 include 必须全部被整行剥除，只余文件头那 kHeaderIncludeNum 个：多一个即说明
  // 有单元把自己的 quote include 带进了产物，那会是工具包里不存在的路径。
  EXPECT_EQ(Count(code, "#include \""), kHeaderIncludeNum);
  // om2_model_api.h 只能经 _internal.h 传递取得，任何形式都不得直接 include
  EXPECT_EQ(code.find("#include \"om2_model_api.h\""), std::string::npos);
}

// 产物只允许一处 include 站点：全部 #include 都在首个内联单元之前，且同一头不重复发射。
// 这条守住"剥掉+收集+统一发射"的链路：漏收会让产物编不过，漏剥会让站点数回涨。
TEST_F(VarMgrFileCodeGeneratorTest, IncludesHoistedToSingleSiteWithoutDuplicates) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  const auto first_unit_pos = code.find(kUnitMark);
  ASSERT_NE(first_unit_pos, std::string::npos);

  constexpr size_t kIncludePrefixLen = 8U;  // strlen("#include")
  std::set<std::string> seen;
  size_t total = 0U;
  size_t duplicated = 0U;
  size_t after_first_unit = 0U;
  size_t securec_forms = 0U;
  size_t line_start = 0U;
  while (line_start < code.size()) {
    size_t line_end = code.find('\n', line_start);
    if (line_end == std::string::npos) {
      line_end = code.size();
    }
    const auto line = code.substr(line_start, line_end - line_start);
    if (line.compare(0U, kIncludePrefixLen, "#include") == 0) {
      ++total;
      if (!seen.insert(line).second) {
        ++duplicated;
      }
      if (line_start >= first_unit_pos) {
        ++after_first_unit;
      }
      if (line.find("securec.h") != std::string::npos) {
        ++securec_forms;
      }
    }
    line_start = line_end + 1U;
  }
  // 站点唯一：首个内联单元之后不得再有任何 #include
  EXPECT_EQ(after_first_unit, 0U);
  // 无重复：行数即唯一行数
  EXPECT_EQ(duplicated, 0U);
  EXPECT_EQ(total, seen.size());
  // 同一头不得 angle/quote 两种写法并存：内嵌单元带 <securec.h>、BuildIncludes 发 "securec.h"
  EXPECT_EQ(securec_forms, 1U);
  // 确有内联单元，才能证明上面几条是剥除+上提的结果而非根本没内嵌
  EXPECT_GT(Count(code, kUnitMark), 1U);
}

// 转换的并行形态：内嵌 om2_thread_pool + 串/并行分派。本文件只在有变量时生成（门控在
// ProgramGenerator::GenerateVarMgrSource），故二者恒定发射；这条守住"不会悄悄退回串行"。
TEST_F(VarMgrFileCodeGeneratorTest, TransUsesThreadPoolAndParallelDispatch) {
  VarFixupTable table;
  table.graph_id = 22U;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  entry.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(generator.HasVariables());

  const auto code = GenerateOk(generator);
  // 线程池并行度常量
  EXPECT_NE(code.find("constexpr uint32_t kVarTransThreadNum = 16U;"), std::string::npos);
  // 池本体是逐字内嵌的上游源码，不是重写：两个单元各带一条边界注释（产物内唯一的上游溯源信息），
  // .h 必须在 .cc 之前（类定义先于类外定义）
  const auto hdr_pos = code.find("/* ===== unit: runtime/om2/om2_thread_pool.h ===== */");
  const auto src_pos = code.find("/* ===== unit: runtime/om2/om2_thread_pool.cc ===== */");
  ASSERT_NE(hdr_pos, std::string::npos);
  ASSERT_NE(src_pos, std::string::npos);
  EXPECT_LT(hdr_pos, src_pos);
  EXPECT_NE(code.find("class ThreadPool {"), std::string::npos);
  // 版权块与守卫宏名都由 EmbedUnits 剥掉（理由见 source_embed_util.h）。
  // 不按 "#ifndef 总数为 0" 断言：kVarTransAdapter 自带 GE_CHECK_GE / GE_CHECK_NOTNULL 两对宏
  // 重定义守卫，那是 StablePart 而非内嵌单元，与本剥离无关。
  EXPECT_EQ(code.find("Copyright"), std::string::npos);
  EXPECT_EQ(Count(code, "#ifndef GE_OM2_"), 0U);
  EXPECT_EQ(Count(code, "#ifndef GE_RUNTIME_OM2_"), 0U);
  EXPECT_EQ(Count(code, "#ifndef INC_OM2_"), 0U);
  // 不得半剥：#ifndef 与 #endif 必须配平，否则说明只剥掉了配对的一半
  EXPECT_EQ(Count(code, "#ifndef"), Count(code, "#endif"));
  // 内嵌单元的 angle include 被 EmbedUnits 剥掉并收集，由 BuildIncludes 在文件头统一发射并集。
  // <thread>/<atomic>/<pthread.h> 只由内嵌线程池使用、产物自身不直接依赖，故它们出现在文件头
  // 即证明"收集回传"这条链路是通的——漏收会让产物编不过。
  EXPECT_NE(code.find("#include <thread>"), std::string::npos);
  EXPECT_NE(code.find("#include <atomic>"), std::string::npos);
  EXPECT_NE(code.find("#include <pthread.h>"), std::string::npos);
  // 产物自身直接依赖的两个：std::min<uint32_t> 要 <algorithm>，std::vector<std::future> 要 <future>
  EXPECT_NE(code.find("#include <algorithm>"), std::string::npos);
  EXPECT_NE(code.find("#include <future>"), std::string::npos);
  // 单条目转换是独立函数，串行与并行两条路径共用同一份逻辑，避免日后走偏
  EXPECT_NE(code.find("static aclError TransSingleVarData(const GertModelVarContext *ctx, uint32_t i)"),
            std::string::npos);
  // 并行路径经 commit 提交任务，任务体的最后一步就是调 TransSingleVarData
  EXPECT_NE(code.find("futures.emplace_back(pool.commit("), std::string::npos);
  EXPECT_NE(code.find("return TransSingleVarData(ctx, i);"), std::string::npos);
  // 任务数 <= 1 走串行：起线程的开销大于收益
  EXPECT_NE(code.find("if (trans_tasks.size() <= 1U) {"), std::string::npos);
  EXPECT_NE(code.find("OM2_CHK_STATUS(TransSingleVarData(ctx, i),"), std::string::npos);
  // 空任务集直接返回，既不取 aclrt 上下文也不建池
  EXPECT_NE(code.find("if (trans_tasks.empty()) {"), std::string::npos);
  // aclrt 上下文必须从主线程传到工作线程：回调与 aclrtMemcpy 都依赖调用方当前上下文。
  // 绑定在任务内而非池的工作线程里，池因此保持纯泛型原语（理由见 BuildTransDispatch）。
  EXPECT_NE(code.find("aclrtGetCurrentContext(&parent_ctx)"), std::string::npos);
  EXPECT_NE(code.find("aclrtSetCurrentContext(parent_ctx)"), std::string::npos);
  // 线程名前缀含模型图 ID 以区分多模型，池内 ThreadFunc 再追加线程序号
  EXPECT_NE(code.find("\"v%u_\", static_cast<uint32_t>(kModelGraphId)"), std::string::npos);
  EXPECT_NE(code.find("pthread_setname_np(pthread_self(), thread_name.c_str())"), std::string::npos);
  // 池与 get() 循环必须同作用域：Destroy() 置 is_stoped_ 后每个工作线程最多再跑一个任务就退出，
  // 队列余量作废、future 永不就绪 ⇒ get() 死锁。om2_thread_pool 自身对此无断言，产物侧只能由
  // 本断言守住——把池挪进更窄的作用域，两段文本都还在，只有作用域跨度能揭穿。
  const auto pool_pos = code.find("::gert::ThreadPool pool(name_prefix, thread_num);");
  const auto get_pos = code.find("OM2_CHK_STATUS(future.get(),");
  ASSERT_NE(pool_pos, std::string::npos);
  ASSERT_NE(get_pos, std::string::npos);
  EXPECT_LT(pool_pos, get_pos);
  EXPECT_TRUE(ScopeSpans(code, pool_pos, get_pos));
  EXPECT_NE(code.find("for (std::future<aclError> &future : futures) {"), std::string::npos);
  // 异常不得跨 SO 边界传播，两处都要兜住：TransSingleVarData 自身（串行路径直接在主线程调，
  // 并行路径由 packaged_task 捕获后在 get() 处重抛，落点都在导出入口 PrepareVars 之下），
  // 以及内嵌池 Destroy() 里的 join
  EXPECT_EQ(Count(code, "} catch (...) {"), 2U);
  // 建线程失败也必须 join 已建成的，否则 pool_ 析构时 std::terminate（内嵌池的 Destroy 负责）
  EXPECT_NE(code.find("if (thd.joinable()) {"), std::string::npos);
  ExpectNoDispatchLeftovers(code);
  // 转换调用点确实接通本文件内 gert::var_trans 的两个内部入口
  EXPECT_NE(code.find("::gert::var_trans::TransFormat("), std::string::npos);
  EXPECT_NE(code.find("::gert::var_trans::TransDataType("), std::string::npos);
}

// 转换链逐节点固化：种类走枚举、格式与数据类型走 static_cast、维度补齐到定长 8。
// src_dim_num / dst_dim_num 各自承载真实维数，补齐值不能当维数用（理由见 VarTransNodeFixup）。
TEST_F(VarMgrFileCodeGeneratorTest, TransRoadIsFixatedWithEnumAndPaddedDims) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kNoOp));
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("kVarTransNoOp, static_cast<ge::Format>(0)"), std::string::npos);
  EXPECT_NE(code.find("kTransRoad0"), std::string::npos);
  // src_dims 补齐到 8 个，两个维数字段记真实维数 4
  EXPECT_NE(code.find("uint32_t src_dim_num;\n  uint32_t dst_dim_num;"), std::string::npos);
  EXPECT_NE(code.find("{1, 2, 3, 4, 0, 0, 0, 0}, {1, 3, 4, 2, 0, 0, 0, 0}, 4U, 4U}"), std::string::npos);
}

// src_shape 与 dst_shape 必须各用自己的维数切片。格式转换会改变维数（NCHW 4 维 →
// NC1HWC0 5 维），共用一个会让 dst_shape 被截断（目标缓冲少 16 倍）或多读补齐位（乘积归零）。
TEST_F(VarMgrFileCodeGeneratorTest, TransShapesUseTheirOwnDimNum) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  auto node = MakeTransNode(VarTransKind::kFormat);
  // 显式取 NCHW → NC1HWC0：本用例要的就是"转换改变维数"这一形态，与下面 5 维 dst_dims
  // 及用例注释保持一致（MakeTransNode 的默认对是 4 维 → 4 维的 NCHW → NHWC）。
  node.dst_format = 3U;              // FORMAT_NC1HWC0
  node.dst_dims = {1, 2, 3, 4, 16};  // NC1HWC0 的 5 维存储形状，C0 = 16
  node.dst_dim_num = 5U;
  entry.trans_road.push_back(node);
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // 固化表里两个维数分别是 4 与 5，C0 必须被固化
  EXPECT_NE(code.find("{1, 2, 3, 4, 0, 0, 0, 0}, {1, 2, 3, 4, 16, 0, 0, 0}, 4U, 5U}"), std::string::npos);
  // 两个 shape 各用自己的维数切片
  EXPECT_NE(code.find("const std::vector<int64_t> src_shape(node.src_dims, (node.src_dims + node.src_dim_num));"),
            std::string::npos);
  EXPECT_NE(code.find("const std::vector<int64_t> dst_shape(node.dst_dims, (node.dst_dims + node.dst_dim_num));"),
            std::string::npos);
  // kCast 的元素数循环也用 src 的维数
  EXPECT_NE(code.find("for (uint32_t d = 0U; (d < node.src_dim_num); d++) {"), std::string::npos);
}

// 拷贝恒定发射：无拷贝源时每次迭代都在 copy_src_var_key == nullptr 处 continue，
// 运行期是一次 O(kVarNum) 空转，换来的是门控只剩 HasVariables 一个。
TEST_F(VarMgrFileCodeGeneratorTest, CopyVarDataIsEmittedEvenWithoutCopySource) {
  VarFixupTable table;
  table.entries.push_back(MakeEntry(0U, "w1_0", 96U, 0U, 96U));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("if (meta.copy_src_var_key == nullptr) {"), std::string::npos);
  EXPECT_NE(code.find("std::strcmp(kVarMetas[k].var_key, meta.copy_src_var_key)"), std::string::npos);
}

// 变量间拷贝：拷贝源无槽位下标但仍必须在表内，否则拷贝阶段找不到源
TEST_F(VarMgrFileCodeGeneratorTest, CopySourceWithoutSlotIsFixated) {
  VarFixupTable table;
  table.entries.push_back(MakeEntry(kVarNoSlot, "src0_0", 96U, 0U, 96U));
  auto dst = MakeEntry(0U, "dst0_0", 96U, 0U, 0U);
  dst.copy_src_var_key = "src0_0";
  dst.copy_src_data_type = 0U;
  table.entries.push_back(std::move(dst));
  table.weight_data.assign(96U, 0x5AU);

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("ACL_MEMCPY_DEVICE_TO_HOST"), std::string::npos);
  EXPECT_NE(code.find("{kNoSlot, \"src0_0\""), std::string::npos);
  EXPECT_NE(code.find("constexpr uint32_t kVarNum = 2U;"), std::string::npos);
  // 拷贝源不占槽位
  EXPECT_NE(code.find("constexpr uint32_t kSlotNum = 1U;"), std::string::npos);
  // 源缺失即加载失败，不得静默跳过：跳过会让目标变量留在"已分配但无数据"状态。
  // 与 V1 的 CopyVarData（GE_CHECK_NOTNULL(src_node)）一致。走 OM2_CHK_TRUE 记日志后返错误码。
  EXPECT_NE(code.find("OM2_CHK_TRUE((j < kVarNum)"), std::string::npos);
  EXPECT_NE(code.find("[Var][C] copy source not in table, var=%s, src=%s, i=%u"), std::string::npos);
  // 目标（新变量）先申请内存，源随后申请：源虽在本表内，但此刻可能还没人申请过，
  // 只查询会落空；RESOLVE 幂等，已申请过时等价于查询
  const auto pos_dst = code.find("GetVarDevAddr(ctx, GERT_VAR_ADDR_RESOLVE, meta, meta.var_key, dst)");
  const auto pos_src = code.find("GetVarDevAddr(ctx, GERT_VAR_ADDR_RESOLVE, kVarMetas[j], kVarMetas[j].var_key, src)");
  ASSERT_NE(pos_dst, std::string::npos);
  ASSERT_NE(pos_src, std::string::npos);
  EXPECT_LT(pos_dst, pos_src);
  // D2H 长度取自源条目自身尺寸，不从目标尺寸反推
  EXPECT_NE(code.find("const size_t src_bytes = static_cast<size_t>(src.size);"), std::string::npos);
}

// 转换链的 kCast 分支必须传源元素数而非字节数，且元素数取自 shape（理由见
// BuildTransSingleVarData 处注释）。传字节数会让目标缓冲按 dst 类型再乘一次而过大。
TEST_F(VarMgrFileCodeGeneratorTest, TransCastElemNumComesFromShape) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_src_var_key = "ext0_0";
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kCast));
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("size_t elem_num = 1U;"), std::string::npos);
  EXPECT_NE(code.find("for (uint32_t d = 0U; (d < node.src_dim_num); d++) {"), std::string::npos);
  EXPECT_NE(code.find("elem_num = (elem_num * static_cast<size_t>(node.src_dims[d]));"), std::string::npos);
  EXPECT_NE(code.find("const int32_t type_size = ge::GetSizeByDataType(cur_dtype);"), std::string::npos);
  EXPECT_NE(code.find("::gert::var_trans::TransDataType(cur_data, elem_num,"), std::string::npos);
  // 不得再把字节数直接当元素数传，也不得由运行中字节数反推
  EXPECT_EQ(code.find("::gert::var_trans::TransDataType(cur_data, cur_len,"), std::string::npos);
  EXPECT_EQ(code.find("cur_len / static_cast<size_t>"), std::string::npos);
  // dtype 尺寸非法（保留位返回 -1）或元素数为 0 时失败，不得继续乘除。
  // 判据是 elem_num 而非 src_dim_num：标量 dim_num == 0 但 elem_num == 1，属合法输入。
  EXPECT_NE(code.find("OM2_CHK_TRUE(((elem_num > 0U) && (type_size > 0))"), std::string::npos);
  // CastKernel 按元素数读源缓冲；fz_c04 一类转换单元自算内部 padded shape、不看 args.dst_shape，
  // 故显式校验元素数不超出实际缓冲，防越界读
  EXPECT_NE(code.find("OM2_CHK_TRUE(((elem_num * static_cast<size_t>(type_size)) <= cur_len)"), std::string::npos);
  // 元素数为 0 时不得兜底成 1：那会把"只转换 1 个元素"当成功提交，
  // 静默错误；置 0 则 TransDataType 返回 length 0、commit 因 host_data_size == 0 被拒，响亮失败
  EXPECT_EQ(code.find("elem_num == 0U"), std::string::npos);
}

// 标量变量（dim_num == 0）：累乘循环不执行、elem_num 保持初值 1，属合法输入，与 V1
// CalcVarSizeInBytes 及 GetItemNumByShape 对空 shape 同按 1 元素算一致。守卫若以维数为
// 判据会在运行期把标量误判为非法转换源，故此处锁定判据是 elem_num。
TEST_F(VarMgrFileCodeGeneratorTest, TransCastAcceptsScalarWithZeroDimNum) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "w1_0", 4U, 0U, 0U);
  entry.trans_src_var_key = "ext0_0";
  auto node = MakeTransNode(VarTransKind::kCast);
  // 标量 shape 经 ClampDims 补齐为全 0、ClampDimNum 得 0
  node.src_dims = {0, 0, 0, 0, 0, 0, 0, 0};
  node.dst_dims = {0, 0, 0, 0, 0, 0, 0, 0};
  node.src_dim_num = 0U;
  node.dst_dim_num = 0U;
  entry.trans_road.push_back(std::move(node));
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // 判据不得是维数，否则标量在运行期被误拒
  EXPECT_EQ(code.find("node.src_dim_num > 0U"), std::string::npos);
  EXPECT_NE(code.find("OM2_CHK_TRUE(((elem_num > 0U) && (type_size > 0))"), std::string::npos);
  // 累乘循环仍按 src_dim_num 界定：标量时不执行，elem_num 留在初值 1
  EXPECT_NE(code.find("size_t elem_num = 1U;"), std::string::npos);
  EXPECT_NE(code.find("for (uint32_t d = 0U; (d < node.src_dim_num); d++) {"), std::string::npos);
}

// 变量间拷贝：源与目标数据类型不同则须 CAST。缺 CAST 时目标变量会收到按源布局重解释的
// 裸字节（静默数值错误）。V1 的 TransTensor 同样在拷贝路径上做必要时的数据类型转换。
TEST_F(VarMgrFileCodeGeneratorTest, CopyCastsWhenDataTypeDiffers) {
  VarFixupTable table;
  table.entries.push_back(MakeEntry(kVarNoSlot, "src0_0", 48U, 0U, 48U));
  auto dst = MakeEntry(0U, "dst0_0", 96U, 0U, 0U);
  dst.copy_src_var_key = "src0_0";
  dst.data_type = 0U;           // DT_FLOAT，4 字节
  dst.copy_src_data_type = 1U;  // DT_FLOAT16，2 字节
  table.entries.push_back(std::move(dst));
  table.weight_data.assign(48U, 0x5AU);

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // data_type 必须固化进产物，否则运行期无从判定是否需要 CAST
  EXPECT_NE(code.find("ge::DataType data_type;"), std::string::npos);
  EXPECT_NE(code.find("static_cast<ge::DataType>(0), static_cast<ge::DataType>(1)"), std::string::npos);
  EXPECT_NE(code.find("const bool need_cast = (meta.copy_src_data_type != meta.data_type);"), std::string::npos);
  // 元素数由【源】尺寸与【源】dtype 得出：源 48 字节 / 2 = 24 个元素，转换后 24 * 4 = 96 恰好是目标尺寸。
  // 不从目标尺寸反推——源与目标的 padding 可以不同，按目标尺寸读源会越界读设备内存
  EXPECT_NE(code.find("const int32_t src_type_size = ge::GetSizeByDataType(meta.copy_src_data_type);"),
            std::string::npos);
  EXPECT_NE(code.find("const size_t elem_num = (src_bytes / static_cast<size_t>(src_type_size));"), std::string::npos);
  EXPECT_NE(code.find("::gert::var_trans::TransDataType(buf.data(), elem_num,"), std::string::npos);
  // 不得再从目标尺寸反推元素数与源字节数
  EXPECT_EQ(code.find("static_cast<size_t>(meta.size) / static_cast<size_t>(dst_type_size)"), std::string::npos);
  EXPECT_EQ(code.find("dst_type_size"), std::string::npos);
  // 提交的是转换产物而非裸 buf
  EXPECT_NE(code.find("host_data = cast_out.get();"), std::string::npos);
  EXPECT_NE(code.find("host_len = cast_len;"), std::string::npos);
  EXPECT_NE(code.find("CommitVarData(ctx, meta.var_key, host_data, host_len)"), std::string::npos);
  // 源尺寸不能被源 dtype 尺寸整除说明固化表自相矛盾，截断会静默丢数据，故直接失败
  EXPECT_NE(code.find("% static_cast<size_t>(src_type_size)) == 0"), std::string::npos);
}

// 运行期转换循环的形态：三条静态判据已在构表期折叠进 trans_road（覆盖见构表 UT），产物侧只剩
// trans_road_num 这一个判据，故循环遍历全表、按该字段跳过无链条目，不再携带候选下标数组。
// is_loaded 探测必须留在运行期——查容器而非 SO 侧缓存，容器状态才是 Bundle 下跨子模型的唯一真相。
TEST_F(VarMgrFileCodeGeneratorTest, TransLoopSkipsEntriesWithoutRoad) {
  VarFixupTable table;
  table.graph_id = 33U;
  table.entries.push_back(MakeEntry(0U, "w_plain", 96U, 0U, 0U));
  auto real_entry = MakeEntry(1U, "w_real", 96U, 0U, 0U);
  real_entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  real_entry.trans_src_var_key = "src1_0";
  table.entries.push_back(std::move(real_entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(generator.HasVariables());
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("constexpr uint32_t kVarNum = 2U;"), std::string::npos);
  // 遍历全表 + 按 trans_road_num 跳过。少跳会让无链条目去只查询转换源，而源不在容器内
  // （dev_addr == nullptr）即被误判为加载失败；多跳则漏转换。两者都不编译失败，故须逐字钉住。
  EXPECT_NE(code.find("for (uint32_t i = 0U; (i < kVarNum); i++) {"), std::string::npos);
  EXPECT_NE(code.find("if (meta.trans_road_num == 0U) {\n      continue;\n    }"), std::string::npos);
  // 候选下标数组已删：同一谓词只允许 trans_road_num 一种编码，两份编码一旦走偏即静默漏转换
  EXPECT_EQ(code.find("kTransCandidates"), std::string::npos);
  EXPECT_EQ(code.find("kTransCandidateNum"), std::string::npos);
  // is_loaded 只查询不分配，未命中即为假，正是要转换的情形
  EXPECT_NE(code.find("GetVarDevAddr(ctx, GERT_VAR_ADDR_QUERY_ONLY, meta, meta.var_key, probe)"), std::string::npos);
  EXPECT_NE(code.find("if (probe.is_loaded != 0U) {\n      continue;\n    }"), std::string::npos);
  // 三条静态判据在构表期求值 ⇒ 产物内不得留下这三个字段的任何痕迹
  for (const auto *folded : {"changed_graph_id", "allocated_graph_id", "need_real_trans"}) {
    EXPECT_EQ(code.find(folded), std::string::npos) << "已在构表期求值的判据字段仍留在产物里: " << folded;
  }
  // VarMeta 尾部两个字段是 data_type / copy_src_data_type（共 14 个字段）
  EXPECT_NE(code.find("ge::DataType data_type;\n  ge::DataType copy_src_data_type;\n};"), std::string::npos);
}

// 全表都无转换链（有变量但无一条需要转换）⇒ 不发射遍历循环：trans_tasks 恒空，靠既有的
// empty() 早返回短路。线程池与分派块照旧发射，不新增"按转换有无"的裁剪轴。
// 三条静态判据的折叠覆盖在构表 UT（VarFixupTableBuilderTest），本例只钉发射侧的裁剪。
TEST_F(VarMgrFileCodeGeneratorTest, NoTransRoadEmitsNoLoop) {
  VarFixupTable table;
  table.graph_id = 99U;
  table.entries.push_back(MakeEntry(0U, "w0", 96U, 0U, 0U));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(generator.HasVariables());
  const auto code = GenerateOk(generator);
  // meta.trans_road_num == 0U 只由 TransAllVarData 的跳过分支发射（BuildTransSingleVarData 用的是
  // k < meta.trans_road_num），故它缺席即循环未发射。不能用 "(i < kVarNum)" 判空：拷贝与填表也遍历全表。
  EXPECT_EQ(code.find("meta.trans_road_num == 0U"), std::string::npos);
  EXPECT_EQ(code.find("kTransCandidates"), std::string::npos);
  EXPECT_EQ(code.find("kTransCandidateNum"), std::string::npos);
  // 函数与早返回仍在：PrepareVars 里的调用点 OM2_CHK_STATUS(TransAllVarData(ctx), ...) 恒定发射
  EXPECT_NE(code.find("static aclError TransAllVarData(const GertModelVarContext *ctx) {"), std::string::npos);
  EXPECT_NE(code.find("if (trans_tasks.empty()) {\n    return ACL_SUCCESS;\n  }"), std::string::npos);
  EXPECT_NE(code.find("class ThreadPool {"), std::string::npos);
  EXPECT_NE(code.find("kVarTransThreadNum"), std::string::npos);
}

// CONSTPLACEHOLDER 变量：外部设备地址非 0 时直接用固化值、不调回调
TEST_F(VarMgrFileCodeGeneratorTest, ExternDevAddrIsFixated) {
  VarFixupTable table;
  auto entry = MakeEntry(0U, "ph4_0", 16U, 0U, 0U);
  entry.extern_dev_addr = 0x7F0000001000ULL;
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("0x7f0000001000ULL"), std::string::npos);
  // 短路只在 RESOLVE 下生效：QUERY_ONLY 不分配，也不该被固化地址拦下
  EXPECT_NE(code.find("if ((mode == GERT_VAR_ADDR_RESOLVE) && (meta.extern_dev_addr != 0ULL)) {"), std::string::npos);
  // 该分支直接用地址、不调回调，故 is_loaded 就地置 1
  EXPECT_NE(code.find("out.addr = reinterpret_cast<void *>(meta.extern_dev_addr);"), std::string::npos);
  EXPECT_NE(code.find("out.is_loaded = 1U;"), std::string::npos);
}

// 三步编排与次序：转换 → 拷贝 → 全量地址表。拷贝源可能同时是转换目标故拷贝在转换之后，
// 地址表依赖前两步的最终地址故在最后。地址申请全部惰性发生在各消费点，无前置全量解析。
TEST_F(VarMgrFileCodeGeneratorTest, PrepareVarsOrchestratesThreePhasesInOrder) {
  VarFixupTable table;
  table.entries.push_back(MakeEntry(0U, "w1_0", 96U, 0U, 96U));
  table.weight_data.assign(96U, 0xA5U);

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);

  const auto pos_trans = code.find("OM2_CHK_STATUS(TransAllVarData(ctx)");
  const auto pos_copy = code.find("OM2_CHK_STATUS(CopyVarData(ctx)");
  const auto pos_addr = code.find("OM2_CHK_STATUS(FillVarAddrTable(ctx, var_addrs, slot_num)");
  ASSERT_NE(pos_trans, std::string::npos);
  ASSERT_NE(pos_copy, std::string::npos);
  ASSERT_NE(pos_addr, std::string::npos);
  // 拷贝源可能同时是转换目标，故拷贝在转换之后；地址表依赖前两阶段的最终地址，故在最后
  EXPECT_LT(pos_trans, pos_copy);
  EXPECT_LT(pos_copy, pos_addr);
  // 本地表整体删除：地址与加载状态一律现查容器
  EXPECT_EQ(code.find("LocalEntry"), std::string::npos);
  EXPECT_EQ(code.find("local["), std::string::npos);
  EXPECT_EQ(code.find("std::vector<VarAddr>"), std::string::npos);
  // VarAddr 是栈上出参，多一个 size 字段承载 QUERY_ONLY 的容器回填值
  EXPECT_NE(code.find("struct VarAddr {"), std::string::npos);
  EXPECT_NE(code.find("uint64_t size;"), std::string::npos);
}

// GetVarDevAddr 是产物内唯一与容器对话的地址入口，两种模式共用一份实现。
// 调用点个数钉住"惰性申请"这一结构：一旦有人加回前置的全量解析循环，计数就会变。
TEST_F(VarMgrFileCodeGeneratorTest, GetVarDevAddrIsTheOnlyContainerAddressEntry) {
  VarFixupTable table;
  table.graph_id = 44U;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  entry.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  ASSERT_TRUE(generator.HasVariables());
  const auto code = GenerateOk(generator);

  EXPECT_NE(code.find("static aclError GetVarDevAddr(const GertModelVarContext *ctx, uint32_t mode, "
                      "const VarMeta &meta, const char *var_key, VarAddr &out)"),
            std::string::npos);
  // 两个回调各自只在一处被调用：get_var_addr 在 GetVarDevAddr 内，commit_var_data 在 CommitVarData 内
  EXPECT_EQ(Count(code, "ctx->get_var_addr("), 1U);
  EXPECT_EQ(Count(code, "ctx->commit_var_data("), 1U);
  // 6 个调用点：转换源只查询、筛选探测只查询、转换目标申请、拷贝目标申请、拷贝源申请、地址表申请。
  // 没有前置的全量解析循环——地址一律在消费点惰性申请，容器侧幂等
  EXPECT_EQ(Count(code, "GetVarDevAddr(ctx, "), 6U);
  EXPECT_EQ(Count(code, "GERT_VAR_ADDR_QUERY_ONLY"), 2U);
  // D2H 与 H2D 各只有一份实现，转换与拷贝两条路径共用。
  // SO 侧只有 1 处 aclrtMemcpy：D2H。H2D 由容器在 commit_var_data 内完成，SO 从不直接写设备内存。
  EXPECT_EQ(Count(code, "static aclError CopyVarFromDevice("), 1U);
  EXPECT_EQ(Count(code, "static aclError CommitVarData("), 1U);
  EXPECT_EQ(Count(code, "aclrtMemcpy("), 1U);
  EXPECT_EQ(Count(code, "ACL_MEMCPY_HOST_TO_DEVICE"), 0U);
}

// 变量键含引号或反斜杠时必须转义，否则生成的源码语法错误。
// ast_.Str 的契约是"已转义好的字面量内容"，发射时只补引号，故转义在生成器侧完成。
TEST_F(VarMgrFileCodeGeneratorTest, VarKeyIsEscaped) {
  VarFixupTable table;
  table.entries.push_back(MakeEntry(0U, "we\"ird\\key", 96U, 0U, 0U));
  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("\"we\\\"ird\\\\key\""), std::string::npos);
}

// 空的转换源键与拷贝源键固化为 nullptr，而非空串字面量：
// 转换/拷贝的筛选靠 == nullptr 判定，空串会让筛选失效
TEST_F(VarMgrFileCodeGeneratorTest, EmptyKeysAreFixatedAsNullptr) {
  VarFixupTable table;
  table.entries.push_back(MakeEntry(0U, "w1_0", 96U, 0U, 0U));
  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  EXPECT_NE(code.find("nullptr, 0U, nullptr, nullptr, static_cast<ge::DataType>(0)"), std::string::npos);
  EXPECT_EQ(code.find("\"\""), std::string::npos);
}

// 与 var_trans 侧的同名用例对称。产物随 OM 交付给客户，仓库内部的分段编号、阶段编号
// 与使用处数统计对客户不可解析，必须留在生成器侧注释里。适配头是 StablePart 字符串
// 而非 AST，两侧都要各自守住。
TEST_F(VarMgrFileCodeGeneratorTest, ProductCarriesNoInternalVocabulary) {
  VarFixupTable table;
  table.graph_id = 22U;
  auto entry = MakeEntry(0U, "w1_0", 96U, 0U, 0U);
  entry.trans_road.push_back(MakeTransNode(VarTransKind::kFormat));
  entry.trans_src_var_key = "w0_0";
  table.entries.push_back(std::move(entry));

  const VarMgrFileCodeGenerator generator(ast, "g1", table);
  const auto code = GenerateOk(generator);
  // 正向对照：确认取到的是含转换的最大产物形态，否则下面那组探针是空转
  ASSERT_NE(code.find("class ThreadPool {"), std::string::npos);
  ASSERT_NE(code.find("aclError PrepareVars(const GertModelVarContext *ctx,"), std::string::npos);
  // 前一组是仓库内部的分段/阶段/判据编号与使用处数统计，后一组是改动史词汇：产物描述的是
  // 当前形态，客户拿到既无法解析编号、也无从核对改前改后。刻意不收"折叠""不再""不保留"
  // ——它们在产物里可能是正当的技术或运行期陈述（如"编译期折叠"），收进来会误伤。
  for (const auto *leak :
       {"见风险", "风险 R", "段①",   "段②",    "段③",    "段④",        "段⑤",      "阶段 A",  "阶段 B",
        "阶段 C", "阶段 D", "守卫①", "守卫②",  "守卫③",  "守卫④",      "集合 P",   "集合 D",  "级别 1",
        "级别 2", "级别 3", "读点 ", "处使用", "令牌",   "三件套",     "转换段",   "下沉前",  "下沉后",
        "改造前", "改动前", "原先",  "此前",   "已删除", "合并为单一", "早已备好", "设计文档"}) {
    EXPECT_EQ(code.find(leak), std::string::npos) << "内部词汇泄漏进客户产物: " << leak;
  }
}

}  // namespace
}  // namespace ge
