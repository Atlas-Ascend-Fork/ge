/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_resources.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <utility>

#include "common/checker.h"
#include "common/om2/om2_kernel_binary_utils.h"
#include "framework/common/debug/ge_log.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/fast_graph/execute_graph.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/utils/attr_utils.h"

namespace gert {
namespace {
// 条目 ID 规则与来源描述一致；拼接后不会产生层级歧义。
bool IsValidEntryId(const std::string &id) {
  if (id.empty()) {
    return false;
  }
  for (const char c : id) {
    const bool ok = ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
                    (c == '_') || (c == '-');
    if (!ok) {
      return false;
    }
  }
  return true;
}

std::string FieldKey(const char *prefix, const std::string &id, const char *field) {
  return std::string(prefix) + id + "." + field;
}

// 一维 uint8 Tensor 是公共侧的字节载荷承载：长度即 Tensor 数据区大小。
bool ReadBytes(const ge::ExecuteGraph &graph, const std::string &key, std::vector<uint8_t> &bytes) {
  ge::ConstGeTensorPtr tensor;
  if (!ge::AttrUtils::GetTensor(graph, key, tensor) || (tensor == nullptr)) {
    return false;
  }
  const size_t size = tensor->GetData().size();
  const auto *data = tensor->GetData().GetData();
  if ((size != 0U) && (data == nullptr)) {
    return false;
  }
  bytes.assign(data, data + size);
  return true;
}

bool ReadFileContent(const std::string &path, const int64_t offset, const int64_t size, std::vector<uint8_t> &bytes) {
  FILE *const file = std::fopen(path.c_str(), "rb");
  if (file == nullptr) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Failed to open resource file %s", path.c_str());
    return false;
  }
  bool ok = true;
  do {
    if (std::fseek(file, static_cast<long>(offset), SEEK_SET) != 0) {
      ok = false;
      break;
    }
    bytes.resize(static_cast<size_t>(size));
    if ((size > 0) && (std::fread(bytes.data(), 1U, static_cast<size_t>(size), file) != static_cast<size_t>(size))) {
      ok = false;
      break;
    }
  } while (false);
  (void)std::fclose(file);
  if (!ok) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "Failed to read %" PRId64 " bytes at offset %" PRId64 " from %s", size, offset,
           path.c_str());
  }
  return ok;
}

bool GetFileSize(const std::string &path, int64_t &size) {
  FILE *const file = std::fopen(path.c_str(), "rb");
  if (file == nullptr) {
    return false;
  }
  (void)std::fseek(file, 0, SEEK_END);
  const long end = std::ftell(file);
  (void)std::fclose(file);
  if (end < 0) {
    return false;
  }
  size = static_cast<int64_t>(end);
  return true;
}

ge::graphStatus ReadIdList(const ge::ExecuteGraph &graph, const char *attr, std::vector<std::string> &ids) {
  if (!graph.HasAttr(attr)) {
    return ge::GRAPH_SUCCESS;
  }
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(graph, attr, ids), "Resource id list %s is not a string list", attr);
  // 属性按 ID 存取，重复 ID 必然指向同一份内容、用途和方式，按定版规范去重而非拒绝。
  std::set<std::string> seen;
  std::vector<std::string> unique_ids;
  unique_ids.reserve(ids.size());
  for (const auto &id : ids) {
    GE_ASSERT_TRUE(IsValidEntryId(id), "Resource id [%s] is not a valid entry id", id.c_str());
    if (seen.insert(id).second) {
      unique_ids.emplace_back(id);
    }
  }
  ids.swap(unique_ids);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseOneSo(const ge::ExecuteGraph &graph, const std::string &id, GertSoResource &so) {
  so.id = id;
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_SO_PREFIX, id, "name"), so.name), "SO %s has no name",
                 id.c_str());
  GE_ASSERT_TRUE(!so.name.empty(), "SO %s name is empty", id.c_str());
  std::string purpose_text;
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_SO_PREFIX, id, "purpose"), purpose_text),
                 "SO %s has no purpose", id.c_str());
  // 未知用途不能默默按 SpaceRegistry 处理：注册位置不同，注册错了运行期才暴露。
  GE_ASSERT_TRUE(ParseGertSoPurpose(purpose_text, so.purpose), "SO %s has unsupported purpose %s", id.c_str(),
                 purpose_text.c_str());

  const std::string content_key = FieldKey(GERT_ATTR_SO_PREFIX, id, "content");
  const std::string path_key = FieldKey(GERT_ATTR_SO_PREFIX, id, "path");
  const bool has_content = graph.HasAttr(content_key);
  const bool has_path = graph.HasAttr(path_key);
  // 内容与路径是二选一的来源，同时给出无法判断以哪个为准。
  GE_ASSERT_TRUE(has_content != has_path, "SO %s must provide exactly one of content or path", id.c_str());
  if (has_content) {
    GE_ASSERT_TRUE(ReadBytes(graph, content_key, so.content), "SO %s content is not a uint8 tensor", id.c_str());
  } else {
    std::string path;
    GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, path_key, path), "SO %s path is not a string", id.c_str());
    int64_t file_size = 0;
    GE_ASSERT_TRUE(GetFileSize(path, file_size), "SO %s path %s cannot be read", id.c_str(), path.c_str());
    GE_ASSERT_TRUE(ReadFileContent(path, 0, file_size, so.content), "SO %s path %s cannot be read", id.c_str(),
                   path.c_str());
  }
  // 空内容无法注册实现，提前拒绝。
  GE_ASSERT_TRUE(!so.content.empty(), "SO %s has empty content", id.c_str());

  (void)ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_SO_PREFIX, id, "vendor"), so.vendor);
  (void)ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_SO_PREFIX, id, "implementation"), so.implementation);
  (void)ge::AttrUtils::GetListStr(graph, FieldKey(GERT_ATTR_SO_PREFIX, id, "dependencies"), so.dependencies);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseOneBinary(const ge::ExecuteGraph &graph, const std::string &id, GertBinaryResource &binary) {
  binary.id = id;
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_BINARY_PREFIX, id, "name"), binary.name),
                 "Binary %s has no name", id.c_str());
  GE_ASSERT_TRUE(!binary.name.empty(), "Binary %s name is empty", id.c_str());
  const std::string content_key = FieldKey(GERT_ATTR_BINARY_PREFIX, id, "content");
  GE_ASSERT_TRUE(graph.HasAttr(content_key), "Binary %s has no content", id.c_str());
  GE_ASSERT_TRUE(ReadBytes(graph, content_key, binary.content), "Binary %s content is not a uint8 tensor", id.c_str());
  GE_ASSERT_TRUE(!binary.content.empty(), "Binary %s has empty content", id.c_str());
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ReadNonNegativeInt(const ge::ExecuteGraph &graph, const std::string &key, const char *label,
                                   int64_t &value, const bool required) {
  if (!graph.HasAttr(key)) {
    GE_ASSERT_TRUE(!required, "Resource %s is required", key.c_str());
    return ge::GRAPH_SUCCESS;
  }
  GE_ASSERT_TRUE(ge::AttrUtils::GetInt(graph, key, value), "Resource %s is not an int64", key.c_str());
  GE_ASSERT_TRUE(value >= 0, "Resource %s (%s) is negative: %" PRId64, key.c_str(), label, value);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ParseOneWeight(const ge::ExecuteGraph &graph, const std::string &id, GertWeightResource &weight) {
  weight.id = id;
  std::string mode_text;
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "mode"), mode_text),
                 "Weight %s has no mode", id.c_str());
  // 方式必须声明：不能从节点阶段或是否存在文件反推。
  GE_ASSERT_TRUE(ParseGertWeightMode(mode_text, weight.mode), "Weight %s has unsupported mode %s", id.c_str(),
                 mode_text.c_str());

  std::string placement;
  GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "placement"), placement),
                 "Weight %s has no placement", id.c_str());
  GE_ASSERT_TRUE((placement == "host") || (placement == "device"), "Weight %s placement %s is unsupported", id.c_str(),
                 placement.c_str());
  weight.device_placement = (placement == "device");

  GE_ASSERT_SUCCESS(
      ReadNonNegativeInt(graph, FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "size"), "size", weight.size, true));
  GE_ASSERT_SUCCESS(ReadNonNegativeInt(graph, FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "file_offset"), "file_offset",
                                       weight.file_offset, false));

  const std::string content_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "content");
  const std::string path_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "path");
  const std::string address_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, id, "address");
  if (weight.mode == GertWeightMode::kEmbedded) {
    // 内置：内容、Host 文件、Host 内存地址三选一，编译期全部转成 content。
    const bool has_content = graph.HasAttr(content_key);
    const bool has_path = graph.HasAttr(path_key);
    const bool has_address = graph.HasAttr(address_key);
    const size_t provided =
        static_cast<size_t>(has_content) + static_cast<size_t>(has_path) + static_cast<size_t>(has_address);
    GE_ASSERT_TRUE(provided == 1U, "Embedded weight %s must provide exactly one of content/path/address", id.c_str());
    if (has_content) {
      GE_ASSERT_TRUE(ReadBytes(graph, content_key, weight.content), "Weight %s content is not a uint8 tensor",
                     id.c_str());
    } else if (has_path) {
      GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, path_key, weight.path), "Weight %s path is not a string", id.c_str());
      // Device 地址不能当 Host 字节解引用；文件来源本身就是 Host 可读的。
      GE_ASSERT_TRUE(ReadFileContent(weight.path, weight.file_offset, weight.size, weight.content),
                     "Weight %s cannot read %" PRId64 " bytes from %s", id.c_str(), weight.size, weight.path.c_str());
    } else {
      GE_ASSERT_TRUE(ge::AttrUtils::GetInt(graph, address_key, weight.address), "Weight %s address is not a uint64",
                     id.c_str());
      GE_ASSERT_TRUE(!weight.device_placement, "Embedded weight %s cannot read a device address as host bytes",
                     id.c_str());
      weight.content.assign(reinterpret_cast<const uint8_t *>(weight.address),
                            reinterpret_cast<const uint8_t *>(weight.address) + weight.size);
    }
    // 声明的大小必须与实际内容一致，否则紧凑索引会错位。
    GE_ASSERT_TRUE(static_cast<int64_t>(weight.content.size()) == weight.size,
                   "Weight %s declares %" PRId64 " bytes but provides %zu", id.c_str(), weight.size,
                   weight.content.size());
  } else if (weight.mode == GertWeightMode::kExternalFile) {
    (void)ge::AttrUtils::GetStr(graph, path_key, weight.path);
  } else {
    // 外置内存：编译期只保留声明，实际地址在加载期提供，不能在此处要求可读。
    (void)content_key;
    (void)address_key;
  }
  return ge::GRAPH_SUCCESS;
}
}  // namespace

bool ParseGertSoPurpose(const std::string &text, GertSoPurpose &purpose) {
  if (text == "space_registry") {
    purpose = GertSoPurpose::kSpaceRegistry;
    return true;
  }
  if (text == "kernel") {
    purpose = GertSoPurpose::kKernel;
    return true;
  }
  if (text == "tiling") {
    purpose = GertSoPurpose::kTiling;
    return true;
  }
  if (text == "custom") {
    purpose = GertSoPurpose::kCustom;
    return true;
  }
  return false;
}

bool ParseGertWeightMode(const std::string &text, GertWeightMode &mode) {
  if (text == "embedded") {
    mode = GertWeightMode::kEmbedded;
    return true;
  }
  if (text == "external_file") {
    mode = GertWeightMode::kExternalFile;
    return true;
  }
  if (text == "external_memory") {
    mode = GertWeightMode::kExternalMemory;
    return true;
  }
  return false;
}

ge::graphStatus ParseGertResources(const ge::ExecuteGraph &graph, GertResourceTable &table) {
  GertResourceTable parsed;
  std::vector<std::string> so_ids;
  GE_ASSERT_SUCCESS(ReadIdList(graph, gert::attr::kSoIds, so_ids));
  for (const auto &id : so_ids) {
    GertSoResource so;
    GE_ASSERT_SUCCESS(ParseOneSo(graph, id, so), "Failed to parse SO %s", id.c_str());
    parsed.sos.emplace_back(std::move(so));
  }

  std::vector<std::string> binary_ids;
  GE_ASSERT_SUCCESS(ReadIdList(graph, gert::attr::kBinaryIds, binary_ids));
  for (const auto &id : binary_ids) {
    GertBinaryResource binary;
    GE_ASSERT_SUCCESS(ParseOneBinary(graph, id, binary), "Failed to parse binary %s", id.c_str());
    parsed.binaries.emplace_back(std::move(binary));
  }

  std::vector<std::string> weight_ids;
  GE_ASSERT_SUCCESS(ReadIdList(graph, gert::attr::kWeightIds, weight_ids));
  for (const auto &id : weight_ids) {
    GertWeightResource weight;
    GE_ASSERT_SUCCESS(ParseOneWeight(graph, id, weight), "Failed to parse weight %s", id.c_str());
    parsed.weights.emplace_back(std::move(weight));
  }
  // SO 依赖必须指向本模型提供的 SO，跨模型依赖无法保证加载期可用。
  for (const auto &so : parsed.sos) {
    for (const auto &dep : so.dependencies) {
      const bool found = std::any_of(parsed.sos.cbegin(), parsed.sos.cend(),
                                     [&dep](const GertSoResource &candidate) { return candidate.id == dep; });
      GE_ASSERT_TRUE(found, "SO %s depends on %s which is not provided", so.id.c_str(), dep.c_str());
    }
  }
  table = std::move(parsed);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ApplyGertLoadBindings(const ge::ExecuteGraph &graph, GertResourceTable &table) {
  for (auto &weight : table.weights) {
    const std::string address_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, weight.id, "address");
    const std::string path_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, weight.id, "path");
    const std::string size_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, weight.id, "size");
    const std::string placement_key = FieldKey(GERT_ATTR_WEIGHT_PREFIX, weight.id, "placement");
    if (weight.mode == GertWeightMode::kExternalMemory) {
      // 外置内存必须在 Init 消费前给出地址，大小与 placement 必须满足声明。
      GE_ASSERT_TRUE(graph.HasAttr(address_key), "External weight %s has no load address", weight.id.c_str());
      GE_ASSERT_TRUE(ge::AttrUtils::GetInt(graph, address_key, weight.load_address),
                     "External weight %s load address is not a uint64", weight.id.c_str());
      weight.load_size = weight.size;
      if (graph.HasAttr(size_key)) {
        GE_ASSERT_SUCCESS(ReadNonNegativeInt(graph, size_key, "size", weight.load_size, true));
        GE_ASSERT_TRUE(weight.load_size >= weight.size, "External weight %s load size is smaller than declared",
                       weight.id.c_str());
      }
      if (graph.HasAttr(placement_key)) {
        std::string placement;
        GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, placement_key, placement),
                       "External weight %s load placement is not a string", weight.id.c_str());
        GE_ASSERT_TRUE((placement == "host") || (placement == "device"),
                       "External weight %s load placement %s is unsupported", weight.id.c_str(), placement.c_str());
        weight.has_load_placement = true;
        weight.load_device_placement = (placement == "device");
        // 编译期声明的 placement 是借用地址的属性，加载期覆盖不能与它矛盾。
        GE_ASSERT_TRUE(weight.load_device_placement == weight.device_placement,
                       "External weight %s load placement conflicts with the declared placement", weight.id.c_str());
      }
    } else if (weight.mode == GertWeightMode::kExternalFile) {
      // 加载配置可覆盖路径；未覆盖时用图上的路径，两者都没有才算缺失。
      if (graph.HasAttr(path_key)) {
        GE_ASSERT_TRUE(ge::AttrUtils::GetStr(graph, path_key, weight.load_path),
                       "External weight %s load path is not a string", weight.id.c_str());
      } else {
        weight.load_path = weight.path;
      }
      GE_ASSERT_TRUE(!weight.load_path.empty(), "External file weight %s has no path", weight.id.c_str());
      int64_t file_size = 0;
      GE_ASSERT_TRUE(GetFileSize(weight.load_path, file_size), "External weight %s path %s cannot be read",
                     weight.id.c_str(), weight.load_path.c_str());
      GE_ASSERT_TRUE(weight.file_offset + weight.size <= file_size,
                     "External weight %s range [%" PRId64 ", %" PRId64 ") exceeds file size %" PRId64,
                     weight.id.c_str(), weight.file_offset, weight.file_offset + weight.size, file_size);
    } else {
      // 内置权重在编译期已经持有内容，不参与加载期绑定。
    }
  }
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus BuildGertResourceImages(const GertResourceTable &table, GertResourceImages &images) {
  GertResourceImages built;
  // SO 按名称去重：加载期按名称落盘注册，同名只能是同一份内容。
  std::map<std::string, size_t> so_index_by_name;
  for (const auto &so : table.sos) {
    const auto [iter, inserted] = so_index_by_name.emplace(so.name, built.sos.size());
    if (inserted) {
      built.sos.push_back({so.name, so.purpose, so.content});
    } else {
      auto &existing = built.sos[iter->second];
      GE_ASSERT_TRUE(existing.content == so.content, "SO %s has conflicting content", so.name.c_str());
      GE_ASSERT_TRUE(existing.purpose == so.purpose, "SO %s is declared with conflicting purposes", so.name.c_str());
    }
    (void)built.so_ids.insert(so.id);
  }

  std::map<std::string, size_t> binary_index_by_name;
  for (const auto &binary : table.binaries) {
    const auto [iter, inserted] = binary_index_by_name.emplace(binary.name, built.binaries.size());
    if (inserted) {
      // key 必须由内容算出，生成代码按同一规则引用，名字与内容不一致运行期才会暴露。
      built.binaries.push_back(
          {binary.name, om2::GetKernelBinaryKey(binary.content.data(), binary.content.size()), binary.content});
    } else {
      GE_ASSERT_TRUE(built.binaries[iter->second].content == binary.content, "Binary %s has conflicting content",
                     binary.name.c_str());
    }
    (void)built.binary_ids.insert(binary.id);
  }

  // 权重按声明顺序占用紧凑下标；外置权重不进紧凑表，加载期按 ID 绑定。
  for (const auto &weight : table.weights) {
    (void)built.weight_ids.insert(weight.id);
    if (weight.mode != GertWeightMode::kEmbedded) {
      continue;
    }
    const auto index = static_cast<uint32_t>(built.weights.entries.size());
    GertWeightIndex::Entry entry;
    entry.id = weight.id;
    entry.mode = weight.mode;
    entry.device_placement = weight.device_placement;
    entry.size = weight.size;
    entry.data = weight.content;
    built.weights.entries.emplace_back(std::move(entry));
    (void)built.weights.index_by_id.emplace(weight.id, index);
  }

  images = std::move(built);
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus ResolveWeightSlice(const GertNodeWeightRef &ref, const GertWeightIndex &index, GertWeightSlice &slice) {
  const auto iter = index.index_by_id.find(ref.id);
  GE_ASSERT_TRUE(iter != index.index_by_id.cend(), "Node references weight %s which is not an embedded resource",
                 ref.id.c_str());
  const auto &entry = index.entries[iter->second];
  GE_ASSERT_TRUE(ref.offset >= 0, "Weight %s offset is negative", ref.id.c_str());
  GE_ASSERT_TRUE(ref.length >= 0, "Weight %s length is negative", ref.id.c_str());
  // 先判溢出再判范围，避免 offset + length 回绕后通过检查。
  GE_ASSERT_TRUE(ref.offset <= entry.size, "Weight %s offset %" PRId64 " exceeds size %" PRId64, ref.id.c_str(),
                 ref.offset, entry.size);
  GE_ASSERT_TRUE(ref.length <= entry.size - ref.offset,
                 "Weight %s slice [%" PRId64 ", %" PRId64 ") exceeds size %" PRId64, ref.id.c_str(), ref.offset,
                 ref.offset + ref.length, entry.size);
  slice.weight_index = iter->second;
  slice.offset = ref.offset;
  slice.length = ref.length;
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetNodeSoIds(const ge::FastNode &node, std::vector<std::string> &so_ids) {
  so_ids.clear();
  const auto desc = node.GetOpDescPtr();
  GE_ASSERT_NOTNULL(desc);
  if (!desc->HasAttr(gert::attr::kSoIds)) {
    return ge::GRAPH_SUCCESS;
  }
  GE_ASSERT_TRUE(ge::AttrUtils::GetListStr(desc, gert::attr::kSoIds, so_ids), "Node %s SO id list is not a string list",
                 node.GetNamePtr());
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetNodeBinaryId(const ge::FastNode &node, std::string &binary_id) {
  binary_id.clear();
  const auto desc = node.GetOpDescPtr();
  GE_ASSERT_NOTNULL(desc);
  const auto *value = ge::AttrUtils::GetStr(desc, gert::attr::kNodeBinaryId);
  if (value == nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  GE_ASSERT_TRUE(!value->empty(), "Node %s declares an empty binary id", node.GetNamePtr());
  binary_id = *value;
  return ge::GRAPH_SUCCESS;
}

ge::graphStatus GetNodeWeightRef(const ge::FastNode &node, GertNodeWeightRef &weight_ref) {
  const auto desc = node.GetOpDescPtr();
  GE_ASSERT_NOTNULL(desc);
  const auto *id = ge::AttrUtils::GetStr(desc, gert::attr::kNodeWeightId);
  if (id == nullptr) {
    return ge::GRAPH_SUCCESS;
  }
  GE_ASSERT_TRUE(!id->empty(), "Node %s declares an empty weight id", node.GetNamePtr());
  weight_ref.id = *id;
  int64_t offset = 0;
  int64_t length = 0;
  GE_ASSERT_TRUE(ge::AttrUtils::GetInt(desc, "gert.weight.offset", offset), "Node %s weight %s has no offset",
                 node.GetNamePtr(), id->c_str());
  GE_ASSERT_TRUE(ge::AttrUtils::GetInt(desc, "gert.weight.length", length), "Node %s weight %s has no length",
                 node.GetNamePtr(), id->c_str());
  GE_ASSERT_TRUE(offset >= 0, "Node %s weight %s offset is negative", node.GetNamePtr(), id->c_str());
  GE_ASSERT_TRUE(length >= 0, "Node %s weight %s length is negative", node.GetNamePtr(), id->c_str());
  weight_ref.offset = offset;
  weight_ref.length = length;
  return ge::GRAPH_SUCCESS;
}
}  // namespace gert
