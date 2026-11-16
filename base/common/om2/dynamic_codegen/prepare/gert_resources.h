/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_V2_LOWERING_GERT_RESOURCES_H_
#define AIR_CXX_RUNTIME_V2_LOWERING_GERT_RESOURCES_H_

#include "framework/common/ge_visibility.h"
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "graph/error_codes.h"

namespace ge {
class ExecuteGraph;
class FastNode;
}  // namespace ge

namespace gert {
// SO 的用途。内部加载路径按用途分派，未知用途明确报错而不是统一当 SpaceRegistry 注册。
enum class GertSoPurpose : uint32_t { kSpaceRegistry = 0, kKernel = 1, kTiling = 2, kCustom = 3 };

bool ParseGertSoPurpose(const std::string &text, GertSoPurpose &purpose);

// 权重存放方式，与节点所处的 Init/Main 阶段完全独立。
enum class GertWeightMode : uint32_t { kEmbedded = 0, kExternalFile = 1, kExternalMemory = 2 };

bool ParseGertWeightMode(const std::string &text, GertWeightMode &mode);

struct GertSoResource {
  std::string id;
  std::string name;
  GertSoPurpose purpose{GertSoPurpose::kSpaceRegistry};
  // 内容二选一：content 直接提供，path 在编译期读取后复制。复制后不再依赖来源。
  std::vector<uint8_t> content;
  std::string vendor;
  std::string implementation;
  std::vector<std::string> dependencies;
};

struct GertBinaryResource {
  std::string id;
  std::string name;
  std::vector<uint8_t> content;
};

struct GertWeightResource {
  std::string id;
  GertWeightMode mode{GertWeightMode::kEmbedded};
  bool device_placement{false};
  int64_t size{0};
  // 内置权重的来源：内容 / 文件（可带 file_offset）/ Host 地址，编译期全部转成 content。
  std::vector<uint8_t> content;
  std::string path;
  int64_t file_offset{0};
  uint64_t address{0};
  // 加载期外置绑定：external_memory 必须提供地址，external_file 必须提供路径。
  uint64_t load_address{0};
  int64_t load_size{0};
  bool has_load_placement{false};
  bool load_device_placement{false};
  std::string load_path;
};

// 节点维度的资源引用：权重按「资源 ID + 资源内偏移 + 长度」表达，内部再转紧凑索引。
struct GertNodeWeightRef {
  std::string id;
  int64_t offset{0};
  int64_t length{0};
};

// 图属性解析出的全部资源。ID 在不同类别表内各自唯一。
struct GertResourceTable {
  std::vector<GertSoResource> sos;
  std::vector<GertBinaryResource> binaries;
  std::vector<GertWeightResource> weights;
};

// 归一化后的权重切片：内部只认紧凑下标，不再携带资源 ID。
struct GertWeightSlice {
  uint32_t weight_index{0U};
  int64_t offset{0};
  int64_t length{0};
};

// 装配后的设备二进制。key 由内容算出，生成代码按同一个 key 引用，因此名字与内容必须一致。
struct GertBinaryImage {
  std::string name;
  std::string key;
  std::vector<uint8_t> content;
};

// 装配后的 SO。同名的按内容去重，同名不同内容拒绝。
struct GertSoImage {
  std::string name;
  GertSoPurpose purpose{GertSoPurpose::kSpaceRegistry};
  std::vector<uint8_t> content;
};

// 紧凑权重表。每个权重资源占一个下标，顺序即资源声明顺序，与节点引用顺序无关，
// 因此同一资源被多个节点共享时只保存一份。外置权重的 data 在加载期填充或直接指向借用地址。
struct GertWeightIndex {
  struct Entry {
    std::string id;
    GertWeightMode mode{GertWeightMode::kEmbedded};
    bool device_placement{false};
    int64_t size{0};
    std::vector<uint8_t> data;
    // 外置内存：借用调用方地址，不拥有、不释放。
    const void *external_address{nullptr};
  };
  std::vector<Entry> entries;
  // 资源 ID → 紧凑下标。
  std::map<std::string, uint32_t> index_by_id;
};

// 装配结果：编译期只保留内容、名字和用途，不再引用原图属性。
struct GertResourceImages {
  std::vector<GertSoImage> sos;
  std::vector<GertBinaryImage> binaries;
  GertWeightIndex weights;
  // 节点引用校验用：资源 ID 集合，用于发现节点引用了未声明的资源。
  std::set<std::string> so_ids;
  std::set<std::string> binary_ids;
  std::set<std::string> weight_ids;
};

// 解析 gert.so.* / gert.binary.* / gert.weight.* 描述。
// 校验：ID 合法且唯一、字段完整性、内容/路径二选一、偏移长度非负且不越界。
VISIBILITY_EXPORT ge::graphStatus ParseGertResources(const ge::ExecuteGraph &graph, GertResourceTable &table);

// 加载期配置覆盖：合并到已解析资源上，仅对 external_file / external_memory 有意义。
ge::graphStatus ApplyGertLoadBindings(const ge::ExecuteGraph &graph, GertResourceTable &table);

// 把解析结果装配成编译期资源镜像：SO / 二进制按名称去重、权重转为紧凑下标。
// 装配后不再依赖原图属性或临时缓冲区。
VISIBILITY_EXPORT ge::graphStatus BuildGertResourceImages(const GertResourceTable &table, GertResourceImages &images);

// 节点引用的资源切片转紧凑下标。offset/length 必须落在资源范围内，越界拒绝。
VISIBILITY_EXPORT ge::graphStatus ResolveWeightSlice(const GertNodeWeightRef &ref, const GertWeightIndex &index,
                                                     GertWeightSlice &slice);

// 节点上的资源引用。三个查询都返回是否声明；声明了非法值即失败。
VISIBILITY_EXPORT ge::graphStatus GetNodeSoIds(const ge::FastNode &node, std::vector<std::string> &so_ids);
VISIBILITY_EXPORT ge::graphStatus GetNodeBinaryId(const ge::FastNode &node, std::string &binary_id);
VISIBILITY_EXPORT ge::graphStatus GetNodeWeightRef(const ge::FastNode &node, GertNodeWeightRef &weight_ref);
}  // namespace gert

#endif  // AIR_CXX_RUNTIME_V2_LOWERING_GERT_RESOURCES_H_
