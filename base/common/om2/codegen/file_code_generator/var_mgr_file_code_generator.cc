/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/om2/codegen/file_code_generator/var_mgr_file_code_generator.h"

#include <cinttypes>
#include <map>
#include <set>
#include <string_view>

#include "common/ge_common/debug/ge_log.h"
#include "common/om2/codegen/file_code_generator/var_trans_file_code_generator.h"
#include "common/om2/codegen/source_embed_util.h"

namespace ge {
// 由 base/CMakeLists.txt 在 configure 阶段生成的资源文件提供：
// om2_thread_pool.{h,cc} 的全文，键为相对仓库根的路径。
const std::map<std::string, std::string_view> &GetOm2ThreadPoolSources();

namespace {
// ast_.Str 的契约是"已转义好的字面量内容"（见 AstBuildContext::Str 注释），发射时只补引号，
// 故转义必须在此完成。变量键取自图节点名，可能含引号或反斜杠。
std::string EscapeStringLiteral(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (const char c : text) {
    if ((c == '"') || (c == '\\')) {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

const char *TransKindLiteral(VarTransKind kind) {
  switch (kind) {
    case VarTransKind::kNoOp:
      return "kVarTransNoOp";
    case VarTransKind::kFormat:
      return "kVarTransFormat";
    case VarTransKind::kCast:
      return "kVarTransCast";
    default:
      return "kVarTransUnsupported";
  }
}

// 内嵌的 om2_thread_pool 必须换到产物私有命名空间下，否则与 libom2_executor.so 的同名副本
// 冲突：std::thread 的 _State_impl<...>::_M_run() 是弱符号，dlopen 时会被先加载的运行时副本
// 抢占，形成"产物 ctor 构造对象 + 运行时 ThreadFunc 读对象"。而产物按默认 CXXFLAGS 编译
// （std::string 是 32 字节 SSO）、运行时侧是 ABI 0（8 字节 COW）⇒ thread_name_prefix_ 的长度
// 字段被当成栈地址，触发巨额分配后 abort。换名后抢占在结构上不可能发生。
// 本隔离层不可移除：om2_thread_pool.{h,cc} 是产物与 libom2_executor.so 共用的同一份源文件，
// 两侧声明同类 ⇒ _State_impl 的模板实参完全一致，只有 kPoolNamespace 前缀能使 mangled 名相异。
constexpr char kPoolNamespace[] = "var_mgr_embed";

// 产物内变量转换线程池的并行度上限，发射为产物侧的同名 constexpr
constexpr uint32_t kVarTransThreadNum = 16U;

// 池文本里不含任何 #include：EmbedUnits 已剥掉并上提到文件头。这是必须的——<queue> 等一旦被包进
// kPoolNamespace，整份 libstdc++ 就会在该命名空间内展开而报 _Deque_iterator 无此成员。
// 池内跨命名空间的引用只有两处：GELOGE 的实参 ge::FAILED，与 GELOGE 内部 GE_GET_ERRORNO_STR
// 展开出的 ge::StatusFactory。两者都是限定名，而 kPoolNamespace 内已无 ge 子命名空间遮蔽
// （池文本现声明 namespace gert），故直接解析到 ::ge，无需 using 桥接。
std::string WrapPoolUnits(const std::string &text) {
  return std::string("namespace ") + kPoolNamespace + " {\n" + text + "}  // namespace " + kPoolNamespace + "\n";
}

}  // namespace

std::vector<DeclNode *> VarMgrFileCodeGenerator::BuildIncludes(const std::set<std::string> &hoisted) const {
  // 只能 include <model>_internal.h：内存编译路径把 om2_model_api.h 做成 memfd，只改写 _internal.h
  // 内对它的 include 与各 .cpp 内对 _internal.h 的 include（均在 om2_utils.cc）。故 .cpp 直接
  // include om2_model_api.h 会找不到文件，相关类型经 _internal.h 传递取得，且它必须排在最前。
  //
  // 内嵌转换单元与变量准备同处一个 TU，include 在此取并集一次发射：散在各单元里会让产物出现
  // 十几处站点、同一头重复多达 8 次。三组布局（内部头 → angle → quote）组内字典序 ⇒ 同一模型
  // 多次生成的产物逐字节相同。
  std::vector<DeclNode *> items = {ast_.Include(artifact_name_ + "_internal.h")};

  // quote 组：工具包公开头，在模型 SO 的 CPPFLAGS 下可达。graph/def_types.h 提供 ge::PtrAdd，
  // 内嵌单元原本经 om2_formats_trans_utils.h 的 quote include 传递取得、已被剥掉，故显式补回；
  // common/checker.h 经 ge_common/debug/ge_log.h 供给内嵌线程池用的 GELOGD/GELOGE/GELOGW。
  static const std::vector<std::string> kQuoteIncludes = {
      "acl/acl.h",         "base/err_msg.h", "common/checker.h", "ge/ge_error_codes.h",
      "graph/def_types.h", "graph/types.h",  "securec.h"};
  // angle 组 = 产物自身直接依赖的 ∪ 内嵌单元收集来的。自身依赖里 <algorithm> 供
  // TransAllVarData 的 std::min<uint32_t>、<future> 供其 std::vector<std::future<aclError>>；
  // std::atomic / std::thread / pthread_* 只由内嵌线程池使用，经 hoisted 自动带入 ⇒ 不硬编码。
  static const std::set<std::string> kOwnAngleIncludes = {"algorithm", "cstdint", "cstring", "future",
                                                          "map",       "memory",  "string",  "vector"};
  std::set<std::string> angle_includes = kOwnAngleIncludes;
  angle_includes.insert(hoisted.begin(), hoisted.end());
  // 同一头不得两种写法并存（内嵌单元带 <securec.h>、本函数发 "securec.h"），
  // 否则读者会误以为是两个不同的头。quote 形式为准，因为它同时服务于非内嵌代码。
  for (const auto &header : kQuoteIncludes) {
    angle_includes.erase(header);
  }
  for (const auto &header : angle_includes) {
    items.push_back(ast_.Include(header, IncludeDecl::Kind::kAngle));
  }
  for (const auto &header : kQuoteIncludes) {
    items.push_back(ast_.Include(header));
  }
  return items;
}

Status VarMgrFileCodeGenerator::BuildThreadPoolUnits(std::string &text, std::set<std::string> &angle_includes) const {
  // 内嵌而非重写：om2_thread_pool 是在产、已测的实现（file_const_loader.cc 在用）。逐字内嵌 ⇒
  // 零转写风险、模板 commit 原样保留（AST 无模板支持）、上游修复经 CMAKE_CONFIGURE_DEPENDS 跟随。
  // 无需适配 StablePart：它只用 GELOGD/GELOGE/GELOGW 与 ge::FAILED，经 BuildIncludes 已可达。
  static const std::vector<std::string> kUnits = {"runtime/om2/om2_thread_pool.h", "runtime/om2/om2_thread_pool.cc"};
  return EmbedUnits(GetOm2ThreadPoolSources(), kUnits, artifact_name_, "VarMgr", text, angle_includes);
}

DeclNode *VarMgrFileCodeGenerator::BuildVarTransKindEnum() const {
  // 枚举取值与"只对枚举 switch、不做字符串比较"的理由见 om2_codegen_types.h 的 VarTransKind。
  return ast_.Enum("VarTransKind", "uint8_t",
                   {
                       {"kVarTransNoOp", Arg(0)},
                       {"kVarTransFormat", Arg(1)},
                       {"kVarTransCast", Arg(2)},
                       {"kVarTransUnsupported", Arg(3)},
                   });
}

StructDecl *VarMgrFileCodeGenerator::BuildTransNodeStruct() const {
  const std::string dims_decl = "[" + std::to_string(kVarDimsMax) + "]";
  return ast_.Struct("TransNode", {
                                      ast_.Field("uint8_t", "trans_kind"),
                                      ast_.Field("ge::Format", "src_format"),
                                      ast_.Field("ge::Format", "dst_format"),
                                      ast_.Field("ge::DataType", "src_data_type"),
                                      ast_.Field("ge::DataType", "dst_data_type"),
                                      ast_.Field("int64_t", "src_dims" + dims_decl),
                                      ast_.Field("int64_t", "dst_dims" + dims_decl),
                                      // src 与 dst 各一个维数：格式转换会改变维数（NCHW 4 维 → NC1HWC0 5 维），
                                      // 共用一个会让 dst_shape 被截断（少算 C0 倍）或多读到补齐位（乘积归零）。
                                      ast_.Field("uint32_t", "src_dim_num"),
                                      ast_.Field("uint32_t", "dst_dim_num"),
                                  });
}

StructDecl *VarMgrFileCodeGenerator::BuildVarMetaStruct() const {
  return ast_.Struct("VarMeta", {
                                    ast_.Field("uint32_t", "slot"),
                                    ast_.Field("const char *", "var_key"),
                                    ast_.Field("uint64_t", "size"),
                                    ast_.Field("uint32_t", "memory_type"),
                                    ast_.Field("uint64_t", "logic_addr"),
                                    ast_.Field("uint64_t", "extern_dev_addr"),
                                    ast_.Field("uint64_t", "init_data_offset"),
                                    ast_.Field("uint64_t", "init_data_size"),
                                    ast_.Field("const TransNode *", "trans_road"),
                                    ast_.Field("uint32_t", "trans_road_num"),
                                    ast_.Field("const char *", "trans_src_var_key"),
                                    ast_.Field("const char *", "copy_src_var_key"),
                                    // 条目自身的数据类型，与 copy_src_data_type 成对，
                                    // 供 CopyVarData 判定是否需要 CAST。
                                    ast_.Field("ge::DataType", "data_type"),
                                    ast_.Field("ge::DataType", "copy_src_data_type"),
                                });
}

StructDecl *VarMgrFileCodeGenerator::BuildVarAddrStruct() const {
  // GetVarDevAddr 的出参，每次调用的栈上局部量而非共享缓存：地址与加载状态一律现查容器，
  // RESOLVE 幂等，故 SO 侧无需留副本。size 承载 QUERY_ONLY 的容器回填值（见
  // GertVarDesc::size 的 @note）供随后的 D2H 定长；RESOLVE 下容器不改写 size。
  return ast_.Struct("VarAddr", {
                                    ast_.Field("void *", "addr", Arg("nullptr")),
                                    ast_.Field("uint64_t", "size", ast_.ULongLong(0)),
                                    ast_.Field("uint32_t", "is_loaded", ast_.UInt(0)),
                                });
}

std::vector<DeclNode *> VarMgrFileCodeGenerator::BuildTransRoadTables() const {
  std::vector<DeclNode *> tables;
  for (size_t i = 0U; i < table_.entries.size(); ++i) {
    const auto &road = table_.entries[i].trans_road;
    if (road.empty()) {
      continue;
    }
    std::vector<Arg> nodes;
    nodes.reserve(road.size());
    for (const auto &node : road) {
      nodes.push_back(ast_.InitList({
          Arg(TransKindLiteral(node.kind)),
          ast_.StaticCast("ge::Format", Arg(static_cast<int64_t>(node.src_format))),
          ast_.StaticCast("ge::Format", Arg(static_cast<int64_t>(node.dst_format))),
          ast_.StaticCast("ge::DataType", Arg(static_cast<int64_t>(node.src_data_type))),
          ast_.StaticCast("ge::DataType", Arg(static_cast<int64_t>(node.dst_data_type))),
          DimsArg(node.src_dims),
          DimsArg(node.dst_dims),
          ast_.UInt(node.src_dim_num),
          ast_.UInt(node.dst_dim_num),
      }));
    }
    tables.push_back(
        ast_.Field("const TransNode", "kTransRoad" + std::to_string(i) + "[]", ast_.InitList(nodes, true)));
  }
  return tables;
}

// 是否有条目需要转换。三条静态判据已在构表期折叠进 trans_road（见 var_fixup_table_builder.cc 的
// BuildFixupEntry），故此处只问"有没有非空的链"，不再复算判据、也不再需要候选下标数组。
// 唯一用途是决定要不要发射 TransAllVarData 的遍历循环：全表都无链时循环体恒被
// trans_road_num == 0 跳过，发射它只会让纯桩产物白背一段空转。
bool VarMgrFileCodeGenerator::HasAnyTransRoad() const {
  for (const auto &entry : table_.entries) {
    if (!entry.trans_road.empty()) {
      return true;
    }
  }
  return false;
}

std::vector<DeclNode *> VarMgrFileCodeGenerator::BuildConstants() const {
  // 产物内的 kNoSlot 是字面量文本（AstBuildContext 无十六进制助手，走 ast_.UInt 会发射成
  // 4294967295U，可读性更差），故用 static_assert 把它与 kVarNoSlot 钉在一起。
  static_assert(kVarNoSlot == 0xFFFFFFFFU, "emitted kNoSlot literal must match kVarNoSlot");
  uint32_t slot_num = 0U;
  for (const auto &entry : table_.entries) {
    if (entry.slot != kVarNoSlot) {
      ++slot_num;
    }
  }
  // 不再有候选下标数组：判据已在构表期折叠进 trans_road，产物侧 trans_road_num 即唯一判据，
  // 再发一份下标数组就是同一谓词的第二种编码，两者一旦走偏即静默漏转换。
  return {
      ast_.Field("constexpr uint64_t", "kModelGraphId", ast_.ULongLong(table_.graph_id)),
      ast_.Field("constexpr uint32_t", "kVarNum", ast_.UInt(table_.entries.size())),
      ast_.Field("constexpr uint32_t", "kSlotNum", ast_.UInt(slot_num)),
      ast_.Field("constexpr uint32_t", "kNoSlot", Arg("0xFFFFFFFFU")),
  };
}

Arg VarMgrFileCodeGenerator::DimsArg(const std::vector<int64_t> &dims) const {
  // 补齐到定长 kVarDimsMax：SO 侧的 src_dims/dst_dims 是定长数组，真实维数另由
  // src_dim_num / dst_dim_num 承载（补齐值不能当维数用，见 VarTransNodeFixup 同名成员）
  std::vector<Arg> values;
  values.reserve(kVarDimsMax);
  for (size_t i = 0U; i < kVarDimsMax; ++i) {
    values.push_back(Arg((i < dims.size()) ? dims[i] : static_cast<int64_t>(0)));
  }
  return ast_.InitList(values);
}

Arg VarMgrFileCodeGenerator::OptionalKeyArg(const std::string &key) const {
  return key.empty() ? Arg(nullptr) : Arg(ast_.Str(EscapeStringLiteral(key)));
}

Arg VarMgrFileCodeGenerator::BuildEntryRow(const VarFixupEntry &entry, size_t road_index) const {
  const Arg slot = (entry.slot == kVarNoSlot) ? Arg("kNoSlot") : Arg(ast_.UInt(entry.slot));
  const Arg road_ptr = entry.trans_road.empty() ? Arg(nullptr) : Arg("kTransRoad" + std::to_string(road_index));
  return ast_.InitList({
      slot,
      ast_.Str(EscapeStringLiteral(entry.var_key)),
      ast_.ULongLong(entry.size),
      ast_.UInt(entry.memory_type),
      ast_.Hex(entry.logic_addr, LiteralExpr::IntSuffix::kULL),
      ast_.Hex(entry.extern_dev_addr, LiteralExpr::IntSuffix::kULL),
      ast_.ULongLong(entry.init_data_offset),
      ast_.ULongLong(entry.init_data_size),
      road_ptr,
      ast_.UInt(entry.trans_road.size()),
      OptionalKeyArg(entry.trans_src_var_key),
      OptionalKeyArg(entry.copy_src_var_key),
      ast_.StaticCast("ge::DataType", Arg(static_cast<int64_t>(entry.data_type))),
      ast_.StaticCast("ge::DataType", Arg(static_cast<int64_t>(entry.copy_src_data_type))),
  });
}

DeclNode *VarMgrFileCodeGenerator::BuildVarMetaTable() const {
  std::vector<Arg> rows;
  rows.reserve(table_.entries.size());
  for (size_t i = 0U; i < table_.entries.size(); ++i) {
    rows.push_back(BuildEntryRow(table_.entries[i], i));
  }
  return ast_.Field("const VarMeta", "kVarMetas[kVarNum]", ast_.InitList(rows, true));
}

FunctionDef *VarMgrFileCodeGenerator::BuildGetSlotNum() const {
  // InitResources 要按槽位数 resize Om2Model 自有的 var_addrs_，而 kSlotNum 只在本文件内
  // 可见，故导出一个查询函数。
  return ast_.DefineFunction("GetSlotNum", std::vector<VarRef>{}, "uint32_t", ast_.Body({ast_.Return("kSlotNum")}));
}

// 产物内唯一与容器对话的地址入口，转换、拷贝与地址表填充三条路径共用。
FunctionDef *VarMgrFileCodeGenerator::BuildGetVarDevAddr() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto mode = ast_.Var("uint32_t", "mode");
  auto meta = ast_.Var("const VarMeta &", "meta");
  auto var_key = ast_.Var("const char *", "var_key");
  auto out = ast_.Var("VarAddr &", "out");
  auto desc = ast_.Var("GertVarDesc", "desc");
  // RESOLVE 才需要 meta 的分配参数；QUERY_ONLY 下容器忽略 memory_type 与 logic_addr、
  // 且要求 host_data 为空（见 GetVarAddrFunc 的 @note），故整段以 mode 门控。
  // 转换源走 QUERY_ONLY 时传入的 meta 是目标的，其 size/init_data 一律不得用于源。
  const std::vector<BodyItem> resolve_fields = {
      ast_.Assign(desc.Attr("size"), meta.Attr("size")),
      ast_.Assign(desc.Attr("memory_type"), meta.Attr("memory_type")),
      ast_.Assign(desc.Attr("logic_addr"), meta.Attr("logic_addr")),
      ast_.If(meta.Attr("init_data_size") > ast_.ULongLong(0),
              {
                  ast_.Call("OM2_CHK_NOTNULL",
                            {ctx.Arrow("var_weight_data"), ast_.Str("[Var] weight block is null, var=%s"), var_key}),
                  ast_.Call("OM2_CHK_TRUE",
                            {(meta.Attr("init_data_offset") + meta.Attr("init_data_size")) <=
                                 ctx.Arrow("var_weight_data_size"),
                             ast_.Str("[Var] init data exceeds weight block, var=%s, offset=%zu, need=%zu, block=%zu"),
                             var_key, ast_.StaticCast("size_t", meta.Attr("init_data_offset")),
                             ast_.StaticCast("size_t", meta.Attr("init_data_size")),
                             ast_.StaticCast("size_t", ctx.Arrow("var_weight_data_size"))}),
                  ast_.Assign(desc.Attr("host_data"), ast_.StaticCast("const uint8_t *", ctx.Arrow("var_weight_data")) +
                                                          meta.Attr("init_data_offset")),
                  ast_.Assign(desc.Attr("host_data_size"), meta.Attr("init_data_size")),
              }),
  };
  return ast_.DefineFunction(
      "GetVarDevAddr", {ctx, mode, meta, var_key, out}, "static aclError",
      ast_.Body({
          ast_.Comment("RESOLVE：申请地址，meta 带初值则一并提交，容器首次分配时写入并置 is_loaded"),
          ast_.Comment("QUERY_ONLY：只查容器内既有地址，不分配不写入，容器回填 out.size 供随后的 D2H 定长"),
          ast_.Assign(out.Attr("addr"), Arg("nullptr")),
          ast_.Assign(out.Attr("size"), ast_.ULongLong(0)),
          ast_.Assign(out.Attr("is_loaded"), ast_.UInt(0)),
          // 编译期已取得的外部设备地址（CONSTPLACEHOLDER），直接用、不调回调，与容器零交互
          ast_.If((mode == "GERT_VAR_ADDR_RESOLVE") && (meta.Attr("extern_dev_addr") != ast_.ULongLong(0)),
                  {
                      ast_.Assign(out.Attr("addr"), ast_.ReinterpretCast("void *", meta.Attr("extern_dev_addr"))),
                      ast_.Assign(out.Attr("size"), meta.Attr("size")),
                      ast_.Assign(out.Attr("is_loaded"), ast_.UInt(1)),
                      ast_.Return("ACL_SUCCESS"),
                  }),
          ast_.VarDecl(desc, ast_.InitList({}), VarDeclStmt::InitStyle::kDirect),
          ast_.Assign(desc.Attr("var_key"), var_key),
          ast_.Assign(desc.Attr("device_id"), ctx.Arrow("device_id")),
          ast_.Assign(desc.Attr("mode"), mode),
          ast_.If(mode == "GERT_VAR_ADDR_RESOLVE", resolve_fields, std::vector<BodyItem>{}),
          ast_.Call("OM2_CHK_TRUE",
                    {ast_.Call("ctx->get_var_addr", {ctx.Arrow("var_manager_handle"), desc.Addr()}) == Arg(0),
                     ast_.Str("[Var] get_var_addr failed, var=%s, mode=%u"), var_key, mode}),
          ast_.Assign(out.Attr("addr"), desc.Attr("dev_addr")),
          // RESOLVE 下容器不改写 size，等于传入的 meta.size；QUERY_ONLY 下是容器回填的源字节数
          ast_.Assign(out.Attr("size"), desc.Attr("size")),
          ast_.Assign(out.Attr("is_loaded"), desc.Attr("is_loaded")),
          ast_.Return("ACL_SUCCESS"),
      }));
}

// 转换与拷贝两条路径共用，故独立成函数。
FunctionDef *VarMgrFileCodeGenerator::BuildCopyVarFromDevice() const {
  auto src = ast_.Var("const VarAddr &", "src");
  auto buf = ast_.Var("std::vector<uint8_t> &", "buf");
  return ast_.DefineFunction(
      "CopyVarFromDevice", {src, buf}, "static aclError",
      ast_.Body({
          ast_.Call("OM2_CHK_TRUE", {(src.Attr("addr") != "nullptr") && (src.Attr("size") != ast_.ULongLong(0)),
                                     ast_.Str("[Var] copy source has no device addr, size=%zu"),
                                     ast_.StaticCast("size_t", src.Attr("size"))}),
          ast_.IgnoreOutput(buf.Resize(ast_.StaticCast("size_t", src.Attr("size")))),
          ast_.Call(
              "OM2_CHK_STATUS",
              {AclrtMemcpy(buf.Data(), buf.Size(), src.Attr("addr"), buf.Size(), "ACL_MEMCPY_DEVICE_TO_HOST"),
               ast_.Str("[Var] D2H failed, size=%zu, ret=%d"), buf.Size(), ast_.StaticCast("int", Arg("_chk_status"))}),
          ast_.Return("ACL_SUCCESS"),
      }));
}

// 转换与拷贝两条路径共用，故独立成函数。名字刻意镜像它包装的容器回调 CommitVarDataFunc，
// 故不与 CopyVarFromDevice 凑成对称的 CopyHostToDevice。容器侧带 is_loaded 守卫：目标已就绪
// 则幂等丢弃本次数据。回调返回的 dev_addr 不取用——地址一律由 GetVarDevAddr 现查。
FunctionDef *VarMgrFileCodeGenerator::BuildCommitVarData() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto var_key = ast_.Var("const char *", "var_key");
  auto data = ast_.Var("const uint8_t *", "data");
  auto len = ast_.Var("size_t", "len");
  auto commit = ast_.Var("GertVarCommit", "commit");
  return ast_.DefineFunction(
      "CommitVarData", {ctx, var_key, data, len}, "static aclError",
      ast_.Body({
          ast_.VarDecl(commit, ast_.InitList({}), VarDeclStmt::InitStyle::kDirect),
          ast_.Assign(commit.Attr("var_key"), var_key),
          ast_.Assign(commit.Attr("host_data"), data),
          ast_.Assign(commit.Attr("host_data_size"), len),
          ast_.Assign(commit.Attr("device_id"), ctx.Arrow("device_id")),
          ast_.Call("OM2_CHK_TRUE",
                    {ast_.Call("ctx->commit_var_data", {ctx.Arrow("var_manager_handle"), commit.Addr()}) == Arg(0),
                     ast_.Str("[Var] commit_var_data failed, var=%s, size=%zu"), var_key, len}),
          ast_.Return("ACL_SUCCESS"),
      }));
}

FunctionDef *VarMgrFileCodeGenerator::BuildTransVarOnHost() const {
  auto meta = ast_.Var("const VarMeta &", "meta");
  auto data = ast_.Var("std::vector<uint8_t> &", "data");
  auto cur_data = ast_.Var("const uint8_t *", "cur_data");
  auto cur_len = ast_.Var("size_t", "cur_len");
  auto cur_dtype = ast_.Var("ge::DataType", "cur_dtype");
  auto held = ast_.Var("std::shared_ptr<uint8_t>", "held");
  auto k = ast_.Var("uint32_t", "k");
  auto d = ast_.Var("uint32_t", "d");
  auto node = ast_.Var("const TransNode &", "node");
  auto out = ast_.Var("std::shared_ptr<uint8_t>", "out");
  auto out_len = ast_.Var("size_t", "out_len");
  auto src_shape = ast_.Var("const std::vector<int64_t>", "src_shape");
  auto dst_shape = ast_.Var("const std::vector<int64_t>", "dst_shape");
  auto elem_num = ast_.Var("size_t", "elem_num");
  auto type_size = ast_.Var("const int32_t", "type_size");

  const std::vector<BodyItem> cast_body = {
      // TransDataType 的 src_data_size 是源元素数而非字节数：消费侧按 src_data_size *
      // GetSizeByDataType(dst) 分配目标缓冲，TransDataSrc2Dst 亦按元素数读源。元素数取自
      // shape（与 V1 的 TransTensor 同源），不由 cur_len 反推。
      ast_.VarDecl(elem_num, ast_.UInt(1)),
      ast_.For(ast_.VarDecl(d, ast_.UInt(0)), d < node.Attr("src_dim_num"), ast_.PostInc(d),
               {ast_.Assign(elem_num, elem_num * ast_.StaticCast("size_t", node.Attr("src_dims")[d]))}),
      ast_.VarDecl(type_size, ast_.Call("ge::GetSizeByDataType", {cur_dtype})),
      // 守卫测元素数而非维数：标量 dim_num == 0 时上面的累乘不执行、elem_num 保持初值 1，
      // 属合法输入（V1 CalcVarSizeInBytes、GetItemNumByShape 对空 shape 同按 1 元素算）；
      // 若改测 dim_num，既误拒标量，又漏放 dims 含 0（如 [0,5]）这种 elem_num 真为 0 的形状。
      // 元素数为 0 时不得兜底成 1：TransDataType 会因 total_size == 0 提前返回、
      // result.length 为 0，随后 commit 因 host_data_size == 0 被容器拒（PARAM_INVALID）
      // ⇒ 响亮失败；兜底成 1 只会转换 1 个元素并成功提交 ⇒ 静默错误。
      ast_.Call("OM2_CHK_TRUE",
                {(elem_num > ast_.UInt(0)) && (type_size > Arg(0)),
                 ast_.Str("[Var][B] invalid cast src, var=%s, node=%u, elem=%zu, dim_num=%u, dtype=%d"),
                 meta.Attr("var_key"), k, elem_num, node.Attr("src_dim_num"), ast_.StaticCast("int", cur_dtype)}),
      // CastKernel 按元素数读源缓冲，越界即堆越界读。多数转换单元的 result.length 由
      // GetItemNumByShape(args.dst_shape) 算出、与 shape 自洽；但 om2_format_transfer_fz_c04.cc
      // 一类自算内部 padded shape、不看 args.dst_shape，故此处显式校验而不假定二者恒等。
      ast_.Call("OM2_CHK_TRUE",
                {(elem_num * ast_.StaticCast("size_t", type_size)) <= cur_len,
                 ast_.Str("[Var][B] cast elem num exceeds src buffer, var=%s, node=%u, elem=%zu, buf=%zu"),
                 meta.Attr("var_key"), k, elem_num, cur_len}),
      ast_.Call("OM2_CHK_TRUE",
                {ast_.Call("::gert::var_trans::TransDataType",
                           {cur_data, elem_num, ast_.StaticCast("uint32_t", cur_dtype),
                            ast_.StaticCast("uint32_t", node.Attr("dst_data_type")), out, out_len}) == "ge::SUCCESS",
                 ast_.Str("[Var][B] TransDataType failed, var=%s, node=%u"), meta.Attr("var_key"), k}),
  };
  const std::vector<BodyItem> unsupported_body = {
      // kVarTransUnsupported：编译期本应已拦下，此处防御
      ast_.Call("OM2_LOGE", {ast_.Str("[Var][B] unsupported trans kind, var=%s, node=%u"), meta.Attr("var_key"), k}),
      ast_.Return("ACL_ERROR_FAILURE"),
  };

  // held 持有上一节点产物，保证 cur_data 存活。末尾只在链上确有节点跑过时回写 data：
  // 全 NoOp 时 cur_data 仍指向 data 自身，自我赋值是未定义行为。
  return ast_.DefineFunction(
      "TransVarOnHost", {meta, data}, "static aclError",
      ast_.Body({
          ast_.VarDecl(cur_data, data.Data()),
          ast_.VarDecl(cur_len, data.Size()),
          ast_.VarDecl(cur_dtype, meta.Attr("trans_road")[Arg(0)].Attr("src_data_type")),
          ast_.VarDecl(held),
          ast_.For(
              ast_.VarDecl(k, ast_.UInt(0)), k < meta.Attr("trans_road_num"), ast_.PostInc(k),
              {
                  ast_.VarDecl(node, meta.Attr("trans_road")[k]),
                  ast_.If(node.Attr("trans_kind") == "kVarTransNoOp", {ast_.Continue()}),
                  ast_.VarDecl(out),
                  ast_.VarDecl(out_len, ast_.UInt(0)),
                  // 两个分支恒定发射：var_trans 侧两个入口也恒定定义（能力缺失的是桩），
                  // 故无需按能力裁剪，也就不存在两侧判据漂移导致链接失败的可能。
                  // src_shape / dst_shape 各用自己的维数，理由见 VarTransNodeFixup 同名成员。
                  ast_.If(node.Attr("trans_kind") == "kVarTransFormat",
                          {
                              ast_.VarDecl(src_shape,
                                           ast_.InitList({node.Attr("src_dims"),
                                                          node.Attr("src_dims") + node.Attr("src_dim_num")},
                                                         false, InitListExpr::Delimiter::kParen),
                                           VarDeclStmt::InitStyle::kDirect),
                              ast_.VarDecl(dst_shape,
                                           ast_.InitList({node.Attr("dst_dims"),
                                                          node.Attr("dst_dims") + node.Attr("dst_dim_num")},
                                                         false, InitListExpr::Delimiter::kParen),
                                           VarDeclStmt::InitStyle::kDirect),
                              ast_.Call("OM2_CHK_TRUE",
                                        {ast_.Call("::gert::var_trans::TransFormat",
                                                   {cur_data, ast_.StaticCast("uint32_t", node.Attr("src_format")),
                                                    ast_.StaticCast("uint32_t", node.Attr("dst_format")),
                                                    ast_.StaticCast("uint32_t", cur_dtype), src_shape, dst_shape, out,
                                                    out_len}) == "ge::SUCCESS",
                                         ast_.Str("[Var][B] TransFormat failed, var=%s, node=%u, fmt %u->%u"),
                                         meta.Attr("var_key"), k, ast_.StaticCast("uint32_t", node.Attr("src_format")),
                                         ast_.StaticCast("uint32_t", node.Attr("dst_format"))}),
                          },
                          {ast_.If(node.Attr("trans_kind") == "kVarTransCast", cast_body, unsupported_body)}),
                  ast_.Assign(cur_dtype, node.Attr("dst_data_type")),
                  ast_.Assign(held, out),
                  ast_.Assign(cur_data, out.GetPtr()),
                  ast_.Assign(cur_len, out_len),
              }),
          ast_.If(held != "nullptr", {ast_.IgnoreOutput(data.Attr("assign")({cur_data, cur_data + cur_len}))}),
          ast_.Return("ACL_SUCCESS"),
      }));
}

FunctionDef *VarMgrFileCodeGenerator::BuildTransSingleVarData() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto i = ast_.Var("uint32_t", "i");
  auto meta = ast_.Var("const VarMeta &", "meta");
  auto src = ast_.Var("VarAddr", "src");
  auto dst = ast_.Var("VarAddr", "dst");
  auto data = ast_.Var("std::vector<uint8_t>", "data");

  const std::vector<BodyItem> body = {
      ast_.VarDecl(meta, Arg("kVarMetas[i]")),
      // 转换源不在本模型固化表内（属先加载的另一个模型），只能凭编译期固化的键向容器只查询。
      // 只查询不分配，故源必须已由它所属模型的准备流程解析过；固化表是编译期快照、
      // 运行期不会凭空多出新键，查不到只能是源所属模型未先加载，即加载失败。
      ast_.VarDecl(src),
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("GetVarDevAddr", {ctx, Arg("GERT_VAR_ADDR_QUERY_ONLY"), meta,
                                                               meta.Attr("trans_src_var_key"), src}),
                                   ast_.Str("[Var][B] query trans source failed, var=%s, src=%s, i=%u"),
                                   meta.Attr("var_key"), meta.Attr("trans_src_var_key"), i}),
      ast_.Call("OM2_CHK_TRUE", {(src.Attr("addr") != "nullptr") && (src.Attr("size") != ast_.ULongLong(0)),
                                 ast_.Str("[Var][B] trans source missing, var=%s, src=%s, i=%u"), meta.Attr("var_key"),
                                 meta.Attr("trans_src_var_key"), i}),
      ast_.VarDecl(data),
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("CopyVarFromDevice", {src, data}),
                                   ast_.Str("[Var][B] D2H of trans source failed, var=%s, i=%u, size=%zu"),
                                   meta.Attr("var_key"), i, ast_.StaticCast("size_t", src.Attr("size"))}),
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("TransVarOnHost", {meta, data}),
                                   ast_.Str("[Var][B] trans on host failed, var=%s, i=%u"), meta.Attr("var_key"), i}),
      // 目标是转换后形式的新变量，此处新申请内存。它没有初值（初值在源变体首次申请时已赋予），
      // 故 is_loaded 为假，随后的 commit 不会被容器的幂等守卫丢弃。
      ast_.VarDecl(dst),
      ast_.Call("OM2_CHK_STATUS",
                {ast_.Call("GetVarDevAddr", {ctx, Arg("GERT_VAR_ADDR_RESOLVE"), meta, meta.Attr("var_key"), dst}),
                 ast_.Str("[Var][B] resolve trans target failed, var=%s, i=%u"), meta.Attr("var_key"), i}),
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("CommitVarData", {ctx, meta.Attr("var_key"), data.Data(), data.Size()}),
                                   ast_.Str("[Var][B] commit trans result failed, var=%s, i=%u, size=%zu"),
                                   meta.Attr("var_key"), i, data.Size()}),
      ast_.Return("ACL_SUCCESS"),
  };
  // try/catch 必需：异常不得跨 SO 边界传播。串行路径异常经 TransAllVarData → PrepareVars（导出
  // 入口，无外层 try/catch）逃出 SO；并行路径被 packaged_task 捕进共享状态、在 future.get() 处
  // 重抛，落点相同。catch 块与 try 平级，try 内声明的 meta 不在作用域，故只能报入参 i。
  return ast_.DefineFunction(
      "TransSingleVarData", {ctx, i}, "static aclError",
      ast_.Body({ast_.TryCatchAll(
          body, {ast_.Call("OM2_LOGE", {ast_.Str("[Var][B] exception escaped in trans task, i=%u"), i}),
                 ast_.Return("ACL_ERROR_FAILURE")})}));
}

std::vector<BodyItem> VarMgrFileCodeGenerator::BuildTransDispatch() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto trans_tasks = ast_.Var("std::vector<uint32_t> &", "trans_tasks");
  auto parent_ctx = ast_.Var("aclrtContext", "parent_ctx");
  auto thread_num = ast_.Var("const uint32_t", "thread_num");
  auto pool = ast_.Var(std::string("::") + kPoolNamespace + "::gert::ThreadPool", "pool");
  auto futures = ast_.Var("std::vector<std::future<aclError>>", "futures");
  auto future = ast_.Var("std::future<aclError> &", "future");
  // 数组 declarator 只属于声明处：VarRef 的名字会原样出现在每个引用点，
  // 故声明走 VarDecl 的字符串重载（名字带 [16]），引用走这个不带下标的 VarRef。
  auto name_prefix = ast_.Var("char", "name_prefix");
  auto task_i = ast_.Var("const uint32_t", "i");

  // 转换任务体。aclrt 上下文在任务内绑定而非池的工作线程里：池保持纯泛型原语（不认识 aclrt），
  // 与 file_const_loader.cc 的 LoadUniqueIndividualConsts 同一范式。绑定本身必需——容器回调与
  // aclrtMemcpy 都依赖调用方当前上下文。绑定失败只是把错误码写进 future、工作线程不提前退出。
  const auto trans_task = ast_.Lambda(
      {LambdaCaptureSpec{"ctx", LambdaCaptureSpec::Kind::kByValue},
       LambdaCaptureSpec{"parent_ctx", LambdaCaptureSpec::Kind::kByValue},
       LambdaCaptureSpec{"i", LambdaCaptureSpec::Kind::kByValue}},
      {}, "aclError",
      {
          ast_.If(ast_.Call("aclrtSetCurrentContext", {parent_ctx}) != "ACL_SUCCESS",
                  {
                      ast_.Call("OM2_LOGE",
                                {ast_.Str("[Var][B] aclrtSetCurrentContext failed in trans task, i=%u"), task_i}),
                      ast_.Return("ACL_ERROR_FAILURE"),
                  }),
          ast_.Return(ast_.Call("TransSingleVarData", {ctx, task_i})),
      });

  return {
      // 串/并行分派。并行度上限为 kVarTransThreadNum；任务数不足时按任务数收窄，
      // 单条目与空集直接走串行——起线程的开销大于收益。
      ast_.If(trans_tasks.Size() <= ast_.UInt(1),
              {ast_.RangeFor(task_i, trans_tasks,
                             {ast_.Call("OM2_CHK_STATUS", {ast_.Call("TransSingleVarData", {ctx, task_i}),
                                                           ast_.Str("[Var][B] serial trans task failed, i=%u, ret=%d"),
                                                           task_i, Arg("_chk_status")})})},
              {
                  ast_.VarDecl(parent_ctx, ast_.InitList({}), VarDeclStmt::InitStyle::kDirect),
                  ast_.Call("OM2_CHK_STATUS",
                            {ast_.Call("aclrtGetCurrentContext", {parent_ctx.Addr()}),
                             ast_.Str("[Var][B] aclrtGetCurrentContext failed, ret=%d"), Arg("_chk_status")}),
                  // 线程名前缀含模型图 ID 以区分多模型，池内 ThreadFunc 再追加线程序号。
                  // 前缀由 snprintf_s 界定在 15 字符内；图 ID 收窄到 32 位后实际最长 12 字符
                  // （'v' + 10 位 + '_'），加 2 位序号共 14 字符，不触 Linux 的 15 字符上限。
                  ast_.VarDecl("char", "name_prefix[16]", ast_.InitList({Arg(0)})),
                  ast_.IgnoreOutput(ast_.Call(
                      "snprintf_s", {name_prefix, ast_.Sizeof(name_prefix), ast_.Sizeof(name_prefix) - Arg(1U),
                                     ast_.Str("v%u_"), ast_.StaticCast("uint32_t", Arg("kModelGraphId"))})),
                  ast_.VarDecl(thread_num,
                               ast_.Call("std::min<uint32_t>",
                                         {Arg("kVarTransThreadNum"), ast_.StaticCast("uint32_t", trans_tasks.Size())})),
                  // 池必须活过下面全部 future.get()：Destroy() 置 is_stoped_ 后每个工作线程最多
                  // 再跑一个任务就退出，队列余量作废、future 永不就绪 ⇒ get() 死锁。故池与
                  // get() 循环同作用域；om2_thread_pool 自身对此无断言，由 UT 的 ScopeSpans 守住。
                  ast_.VarDecl(pool, ast_.InitList({name_prefix, thread_num}, false, InitListExpr::Delimiter::kParen),
                               VarDeclStmt::InitStyle::kDirect),
                  ast_.VarDecl(futures),
                  futures.Attr("reserve")(trans_tasks.Size()),
                  ast_.RangeFor(task_i, trans_tasks, {futures.Attr("emplace_back")(pool.Attr("commit")(trans_task))}),
                  // 提前返回是安全的：packaged_task 得来的 std::future 析构不阻塞
                  // （只有 std::async(launch::async) 的才阻塞），池析构 join 掉正在跑的那个任务即可，
                  // 队列余量直接丢弃。
                  ast_.RangeFor(future, futures,
                                {ast_.Call("OM2_CHK_STATUS", {future.Attr("get")(),
                                                              ast_.Str("[Var][B] parallel trans task failed, ret=%d"),
                                                              Arg("_chk_status")})}),
              }),
  };
}

// 格式转换。静态判据已在构表期折叠进 trans_road，产物侧 trans_road_num 即唯一判据，故运行期
// 遍历全表、按该字段跳过无链条目即可，不必再携带一份候选下标数组。运行期只保留唯一真正动态
// 的那条判据——向容器只查询目标的加载状态。产物内不存本地状态表：容器状态才是 Bundle 下
// 跨子模型的唯一真相。转换本体（D2H → 转换链 → H2D）才是 CPU 密集段，是并行的收益所在。
FunctionDef *VarMgrFileCodeGenerator::BuildTransAllVarData() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto meta = ast_.Var("const VarMeta &", "meta");
  auto probe = ast_.Var("VarAddr", "probe");
  auto trans_tasks = ast_.Var("std::vector<uint32_t>", "trans_tasks");
  std::vector<BodyItem> items = {ast_.VarDecl(trans_tasks)};
  // 全表都无转换链时（有变量但无一条需要转换）不发射循环：
  // trans_tasks 恒空，由下面的 empty() 早返回短路。
  if (HasAnyTransRoad()) {
    auto i = ast_.Var("uint32_t", "i");
    items.push_back(ast_.For(
        ast_.VarDecl(i, ast_.UInt(0)), i < "kVarNum", ast_.PostInc(i),
        {
            ast_.VarDecl(meta, Arg("kVarMetas[i]")),
            ast_.If(meta.Attr("trans_road_num") == ast_.UInt(0), {ast_.Continue()}),
            ast_.Comment("容器内该目标已就绪"),
            ast_.VarDecl(probe),
            ast_.Call(
                "OM2_CHK_STATUS",
                {ast_.Call("GetVarDevAddr", {ctx, Arg("GERT_VAR_ADDR_QUERY_ONLY"), meta, meta.Attr("var_key"), probe}),
                 ast_.Str("[Var][B] probe trans target failed, var=%s, i=%u"), meta.Attr("var_key"), i}),
            ast_.If(probe.Attr("is_loaded") != ast_.UInt(0), {ast_.Continue()}),
            trans_tasks.PushBack(i),
        }));
  }
  items.push_back(ast_.If(trans_tasks.Empty(), {ast_.Return("ACL_SUCCESS")}));
  items.push_back(ast_.BlankLine());
  auto dispatch = BuildTransDispatch();
  items.insert(items.end(), dispatch.begin(), dispatch.end());
  items.push_back(ast_.Return("ACL_SUCCESS"));
  return ast_.DefineFunction("TransAllVarData", {ctx}, "static aclError", ast_.Body(items));
}

FunctionDef *VarMgrFileCodeGenerator::BuildCopyVarData() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto i = ast_.Var("uint32_t", "i");
  auto j = ast_.Var("uint32_t", "j");
  auto k = ast_.Var("uint32_t", "k");
  auto meta = ast_.Var("const VarMeta &", "meta");
  auto dst = ast_.Var("VarAddr", "dst");
  auto src = ast_.Var("VarAddr", "src");
  auto buf = ast_.Var("std::vector<uint8_t>", "buf");
  auto src_bytes = ast_.Var("const size_t", "src_bytes");
  auto need_cast = ast_.Var("const bool", "need_cast");
  auto elem_num = ast_.Var("const size_t", "elem_num");
  auto src_type_size = ast_.Var("const int32_t", "src_type_size");
  auto host_data = ast_.Var("const uint8_t *", "host_data");
  auto host_len = ast_.Var("size_t", "host_len");
  auto cast_out = ast_.Var("std::shared_ptr<uint8_t>", "cast_out");
  auto cast_len = ast_.Var("size_t", "cast_len");

  std::vector<BodyItem> loop_body = {
      ast_.VarDecl(meta, Arg("kVarMetas[i]")),
      ast_.If(meta.Attr("copy_src_var_key") == "nullptr", {ast_.Continue()}),
      // 目标（拷贝而来的新变量）先申请内存；它没有初值，故 is_loaded 为假。
      // 已为真说明 Bundle 下别的子模型抢先完成，跳过。
      // 判据不可用"地址已分配"代替：无初值的拷贝目标会是"已分配但无数据"。
      ast_.VarDecl(dst),
      ast_.Call("OM2_CHK_STATUS",
                {ast_.Call("GetVarDevAddr", {ctx, Arg("GERT_VAR_ADDR_RESOLVE"), meta, meta.Attr("var_key"), dst}),
                 ast_.Str("[Var][C] resolve copy target failed, var=%s, i=%u"), meta.Attr("var_key"), i}),
      ast_.If(dst.Attr("is_loaded") != ast_.UInt(0), {ast_.Continue()}),
      // 拷贝源在本表内（builder 沿 copy_src_var_name 传递展开），编译期未固化下标，故按键线性定位
      ast_.VarDecl(j, Arg("kVarNum")),
      ast_.For(
          ast_.VarDecl(k, ast_.UInt(0)), k < "kVarNum", ast_.PostInc(k),
          {
              ast_.If(ast_.Call("std::strcmp", {Arg("kVarMetas[k].var_key"), meta.Attr("copy_src_var_key")}) == Arg(0),
                      {
                          ast_.Assign(j, k),
                          ast_.Break(),
                      }),
          }),
      // 源缺失即加载失败，不降级成告警后跳过：跳过会让目标变量留在"已分配但无数据"状态，
      // 后续执行读到未初始化内存。与 V1 的 CopyVarData（GE_CHECK_NOTNULL(src_node)）一致。
      ast_.Call("OM2_CHK_TRUE", {j < "kVarNum", ast_.Str("[Var][C] copy source not in table, var=%s, src=%s, i=%u"),
                                 meta.Attr("var_key"), meta.Attr("copy_src_var_key"), i}),
      // 源用 RESOLVE 而非 QUERY_ONLY：源虽在本表内，但此刻可能还没人申请过它，只查询会落空。
      // RESOLVE 幂等（容器命中 dev_addrs 即返回既有地址），已申请过时等价于查询，
      // 未申请时顺带把源的初值灌进去——D2H 才有意义。
      ast_.VarDecl(src),
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("GetVarDevAddr", {ctx, Arg("GERT_VAR_ADDR_RESOLVE"), Arg("kVarMetas[j]"),
                                                               Arg("kVarMetas[j].var_key"), src}),
                                   ast_.Str("[Var][C] resolve copy source failed, var=%s, src=%s, i=%u"),
                                   meta.Attr("var_key"), meta.Attr("copy_src_var_key"), i}),
      // D2H 长度与 CAST 元素数都取自源条目自身的尺寸，不从目标尺寸反推：源与目标的 padding
      // 可以不同，按目标尺寸读源会越界读设备内存（源更小时）或截断（源更大时）。
      ast_.VarDecl(src_bytes, ast_.StaticCast("size_t", src.Attr("size"))),
      ast_.VarDecl(buf),
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("CopyVarFromDevice", {src, buf}),
                                   ast_.Str("[Var][C] D2H of copy source failed, var=%s, i=%u, size=%zu"),
                                   meta.Attr("var_key"), i, src_bytes}),
      ast_.VarDecl(host_data, buf.Data()),
      ast_.VarDecl(host_len, buf.Size()),
  };

  // ::gert::var_trans::TransDataType 由 BuildSegments 拼在本文件更靠前的位置（同一 TU、
  // 定义在前），故 CAST 分支可恒定发射、无需再裁剪。
  loop_body.push_back(ast_.VarDecl(need_cast, meta.Attr("copy_src_data_type") != meta.Attr("data_type")));
  loop_body.push_back(ast_.VarDecl(cast_out));
  loop_body.push_back(ast_.If(
      need_cast,
      {
          // 源与目标数据类型不同则须 CAST。守恒元素数，故元素数由源尺寸与源 dtype 得出
          ast_.VarDecl(src_type_size, ast_.Call("ge::GetSizeByDataType", {meta.Attr("copy_src_data_type")})),
          ast_.Call("OM2_CHK_TRUE",
                    {src_type_size > Arg(0), ast_.Str("[Var][C] unsupported src data type, var=%s, i=%u, src_type=%d"),
                     meta.Attr("var_key"), i, ast_.StaticCast("int", meta.Attr("copy_src_data_type"))}),
          // 源尺寸不能被源 dtype 尺寸整除说明固化表自相矛盾；截断会静默丢数据，故直接失败
          ast_.Call("OM2_CHK_TRUE",
                    {(src_bytes % ast_.StaticCast("size_t", src_type_size)) == Arg(0),
                     ast_.Str("[Var][C] src size not divisible by src data type, var=%s, i=%u, size=%zu, type_size=%d"),
                     meta.Attr("var_key"), i, src_bytes, src_type_size}),
          ast_.VarDecl(elem_num, src_bytes / ast_.StaticCast("size_t", src_type_size)),
          ast_.VarDecl(cast_len, ast_.UInt(0)),
          ast_.Call(
              "OM2_CHK_TRUE",
              {ast_.Call("::gert::var_trans::TransDataType",
                         {buf.Data(), elem_num, ast_.StaticCast("uint32_t", meta.Attr("copy_src_data_type")),
                          ast_.StaticCast("uint32_t", meta.Attr("data_type")), cast_out, cast_len}) == "ge::SUCCESS",
               ast_.Str("[Var][C] TransDataType failed, var=%s, i=%u, src_type=%d, dst_type=%d, elem=%zu"),
               meta.Attr("var_key"), i, ast_.StaticCast("int", meta.Attr("copy_src_data_type")),
               ast_.StaticCast("int", meta.Attr("data_type")), elem_num}),
          ast_.Assign(host_data, cast_out.GetPtr()),
          ast_.Assign(host_len, cast_len),
      }));

  loop_body.push_back(ast_.Call(
      "OM2_CHK_STATUS",
      {ast_.Call("CommitVarData", {ctx, meta.Attr("var_key"), host_data, host_len}),
       ast_.Str("[Var][C] commit copy result failed, var=%s, i=%u, size=%zu"), meta.Attr("var_key"), i, host_len}));

  // 变量间拷贝。串行：拷贝源可能同时是转换目标，故本函数必须在 TransAllVarData 之后跑，
  // 否则读到的是转换前的旧数据。
  return ast_.DefineFunction("CopyVarData", {ctx}, "static aclError",
                             ast_.Body({
                                 ast_.For(ast_.VarDecl(i, ast_.UInt(0)), i < "kVarNum", ast_.PostInc(i), loop_body),
                                 ast_.Return("ACL_SUCCESS"),
                             }));
}

// 填出执行器传入的运行期地址表。名字与导出入口 PrepareVars 刻意拉开距离——两者仅差中段
// 5 个字符且一个是 static、一个是 ABI 导出符号，混淆会让内部函数被误当接口调用。
// 前两个函数没碰过的变量在此第一次申请（有初值则一并灌入）；kNoSlot 条目（如拷贝源）跳过。
FunctionDef *VarMgrFileCodeGenerator::BuildFillVarAddrTable() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto var_addrs = ast_.Var("void **", "var_addrs");
  auto slot_num = ast_.Var("uint32_t", "slot_num");
  auto i = ast_.Var("uint32_t", "i");
  auto meta = ast_.Var("const VarMeta &", "meta");
  auto v = ast_.Var("VarAddr", "v");
  return ast_.DefineFunction(
      "FillVarAddrTable", {ctx, var_addrs, slot_num}, "static aclError",
      ast_.Body({
          ast_.For(
              ast_.VarDecl(i, ast_.UInt(0)), i < "kVarNum", ast_.PostInc(i),
              {
                  ast_.VarDecl(meta, Arg("kVarMetas[i]")),
                  ast_.If(meta.Attr("slot") == "kNoSlot", {ast_.Continue()}),
                  ast_.VarDecl(v),
                  ast_.Call(
                      "OM2_CHK_STATUS",
                      {ast_.Call("GetVarDevAddr", {ctx, Arg("GERT_VAR_ADDR_RESOLVE"), meta, meta.Attr("var_key"), v}),
                       ast_.Str("[Var][D] resolve failed, var=%s, i=%u"), meta.Attr("var_key"), i}),
                  ast_.Call("OM2_CHK_TRUE", {(var_addrs != "nullptr") && (meta.Attr("slot") < slot_num),
                                             ast_.Str("[Var][D] slot out of range, var=%s, i=%u, slot=%u, slot_num=%u"),
                                             meta.Attr("var_key"), i, meta.Attr("slot"), slot_num}),
                  ast_.Assign(var_addrs[meta.Attr("slot")], v.Attr("addr")),
              }),
          ast_.Return("ACL_SUCCESS"),
      }));
}

FunctionDef *VarMgrFileCodeGenerator::BuildPrepareVars() const {
  auto ctx = ast_.Var("const GertModelVarContext *", "ctx");
  auto var_addrs = ast_.Var("void **", "var_addrs");
  auto slot_num = ast_.Var("uint32_t", "slot_num");
  // 两级判空：ctx 整体为空表示无变量模型；ctx 非空但句柄或回调缺失而固化表非空是执行器侧
  // 编程错误。三个函数严格串行：拷贝源可能同时是转换目标故拷贝在转换之后，地址表依赖前两者
  // 的最终地址故在最后。地址申请全部惰性发生在各消费点（GetVarDevAddr 幂等）。
  std::vector<BodyItem> body = {
      ast_.If(ctx == "nullptr", {ast_.Return("ACL_SUCCESS")}),
      ast_.Call("OM2_CHK_TRUE",
                {(ctx.Arrow("var_manager_handle") != "nullptr") && (ctx.Arrow("get_var_addr") != "nullptr") &&
                     (ctx.Arrow("commit_var_data") != "nullptr"),
                 ast_.Str("[Var] var context is incomplete while kVarNum=%u, executor side programming error"),
                 Arg("kVarNum")}),
      ast_.BlankLine(),
  };
  // TransVarOnHost 与 CopyVarData 引用 ::gert::var_trans 的两个内部入口（同一 TU、定义在前），
  // 故这三个函数可恒定发射、无需按转换能力裁剪。
  body.push_back(ast_.Call(
      "OM2_CHK_STATUS",
      {ast_.Call("TransAllVarData", {ctx}), ast_.Str("[Var] trans all var data failed, ret=%d"), Arg("_chk_status")}));
  body.push_back(ast_.Call("OM2_CHK_STATUS", {ast_.Call("CopyVarData", {ctx}),
                                              ast_.Str("[Var] copy var data failed, ret=%d"), Arg("_chk_status")}));
  body.push_back(
      ast_.Call("OM2_CHK_STATUS", {ast_.Call("FillVarAddrTable", {ctx, var_addrs, slot_num}),
                                   ast_.Str("[Var] fill var addr table failed, ret=%d"), Arg("_chk_status")}));
  // 任一阶段失败经 OM2_CHK_* 记日志后直接返回错误码。SO 不持有需自行释放的设备内存，
  // 已解析的地址归容器、由其 Finalize 释放。
  body.push_back(ast_.Return("ACL_SUCCESS"));
  return ast_.DefineFunction("PrepareVars", {ctx, var_addrs, slot_num}, "aclError", ast_.Body(body));
}

Status VarMgrFileCodeGenerator::BuildFile(DeclNode *&file) const {
  // 两阶段：先产出两段内嵌文本并收集它们被剥掉的 angle include，才能把并集一次发射在文件头。
  // 反过来（先发射 include 再内嵌）拿不到并集，就只能让每个单元各带一份 include。
  std::set<std::string> hoisted_includes;
  // 转换部分（适配头 → 内嵌单元原文 → gert::var_trans 两个入口）整体前置于 gert::var_mgr，同 TU
  // 内先定义后使用即可（理由见类文档）。适配头必须先于内嵌单元——单元正文用到它复刻的
  // GE_CHECK_GE / MakeShared / TypeUtilsInner。
  VarTransFileCodeGenerator var_trans(ast_, artifact_name_, table_);
  std::vector<DeclNode *> segments;
  GE_CHK_STATUS_RET_NOLOG(var_trans.BuildSegments(segments, hoisted_includes));
  // 线程池本体必须在全局作用域：内嵌源自带 namespace gert {，放进 body 会嵌套成
  // gert::var_mgr::gert。转换单元同理在 items 顶层，只有生成的入口函数在命名空间内。
  // 位置只须先于唯一使用者 gert::var_mgr，与转换部分之间无依赖。
  std::string pool_text;
  GE_CHK_STATUS_RET_NOLOG(BuildThreadPoolUnits(pool_text, hoisted_includes));

  std::vector<DeclNode *> items = BuildIncludes(hoisted_includes);
  items.push_back(ast_.Space());
  items.insert(items.end(), segments.begin(), segments.end());
  items.push_back(ast_.Space());
  items.push_back(ast_.RawText(WrapPoolUnits(pool_text)));
  items.push_back(ast_.Space());
  // 以下各项的物理顺序即依赖顺序：转换链常量被元数据表以指针指向故在前，变量准备各函数的
  // 定义顺序即调用顺序。
  std::vector<DeclNode *> body = {
      BuildVarTransKindEnum(),
      BuildTransNodeStruct(),
  };
  for (auto *road_table : BuildTransRoadTables()) {
    body.push_back(road_table);
  }
  body.push_back(ast_.Space());
  body.push_back(BuildVarMetaStruct());
  for (auto *constant : BuildConstants()) {
    body.push_back(constant);
  }
  body.push_back(BuildVarMetaTable());
  body.push_back(ast_.Space());
  body.push_back(BuildVarAddrStruct());
  body.push_back(ast_.Space());
  // 池本体已在文件头内嵌，此处只定并行度；池的生命周期为单次 PrepareVars 调用
  body.push_back(ast_.Field("constexpr uint32_t", "kVarTransThreadNum", ast_.UInt(kVarTransThreadNum)));
  body.push_back(ast_.Space());
  body.push_back(BuildGetVarDevAddr());
  body.push_back(BuildCopyVarFromDevice());
  body.push_back(BuildCommitVarData());
  body.push_back(BuildTransVarOnHost());
  body.push_back(BuildTransSingleVarData());
  body.push_back(BuildTransAllVarData());
  body.push_back(BuildCopyVarData());
  body.push_back(BuildFillVarAddrTable());
  body.push_back(BuildGetSlotNum());
  body.push_back(BuildPrepareVars());
  items.push_back(ast_.Namespace("gert", {ast_.Namespace("var_mgr", body)}));
  file = ast_.File(items);
  return SUCCESS;
}
}  // namespace ge
