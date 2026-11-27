/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "common/file_constant_utils/file_constant_utils.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <iomanip>
#include <fstream>
#include <memory>
#include "securec.h"
#include "framework/common/debug/log.h"
#include "framework/common/framework_types_internal.h"
#include "common/helper/file_saver.h"
#include "common/plugin/ge_make_unique_util.h"
#include "common/math/ge_math_util.h"
#include "common/checker.h"
#include "common/thread_pool/thread_pool.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_context.h"
#include "graph/utils/attr_utils.h"
#include "graph_metadef/graph/utils/file_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/op_desc_utils_ex.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/graph_utils.h"
#include "rt_external_mem.h"
#include "mmpa/mmpa_api.h"
#include "base/err_mgr.h"
#include "graph_metadef/common/ge_common/util.h"

namespace ge {
namespace {
constexpr int64_t kBlockSize = 10485760;
constexpr int64_t kDefaultOffset = 0;
constexpr int32_t kFirstElementIndex = 0;
constexpr int32_t kIndentWidth = 2;
const std::string kBinFileValues = "value_bins";
const std::string kBinIdValue = "value_bin_id";
const std::string kBinFilePathValue = "value_bin_file";
const std::string kTmpWeightDir = "tmp_weight_";
const std::string kMetaFileName = "meta.json";
const std::string kAttrDtype = "dtype";
const std::string kAttrShape = "shape";
constexpr int32_t kIndentationLen = 4;
constexpr int64_t kDefaultThreadNum = 8;
// 合并权重文件的 512 字节对齐粒度（DMA 要求），写入与旧区间复用校验共用
constexpr size_t kCombinedWeightAlign = 512UL;
// 同目录 staging 文件前缀：仅本流程拥有，可作为失败清理依据；
// 不得以该前缀或 tmp_weight_ 前缀推断文件是否受编译目录管理
const std::string kStagingFilePrefix = ".tmp_fcst_publish_";
const std::string kMetaLockFileName = "meta.lock";

Status ReadJsonFile(const std::string &file_path, nlohmann::json &json_obj) {
  std::ifstream file_stream(file_path);
  GE_CHK_BOOL_RET_STATUS(file_stream.is_open(), FAILED, "Failed to open json file[%s]", file_path.c_str());
  try {
    file_stream >> json_obj;
  } catch (const nlohmann::json::exception &e) {
    GELOGE(FAILED, "Failed to read json file[%s], err msg: %s", file_path.c_str(), e.what());
    return FAILED;
  }
  GE_CHK_BOOL_RET_STATUS(file_stream.good(), FAILED, "Failed to read json file[%s]", file_path.c_str());
  return SUCCESS;
}

Status WriteJsonFile(const std::string &file_path, const nlohmann::json &json_obj) {
  std::ofstream out_stream(file_path);
  GE_CHK_BOOL_RET_STATUS(out_stream.is_open(), FAILED, "Failed to open json file:%s", file_path.c_str());
  try {
    out_stream << std::setw(kIndentationLen) << json_obj;
  } catch (const std::exception &e) {
    GELOGE(FAILED, "Failed to write json file[%s], err msg: %s", file_path.c_str(), e.what());
    return FAILED;
  }
  GE_CHK_BOOL_RET_STATUS(out_stream.good(), FAILED, "Failed to write json file[%s]", file_path.c_str());
  return SUCCESS;
}

// 索引文件发布：先写同目录临时文件并确认关闭，再 rename 原子替换，避免读方看到半截 json
Status WriteJsonFileAtomic(const std::string &file_path, const nlohmann::json &json_obj) {
  const auto pos = file_path.find_last_of('/');
  const std::string dir = (pos == std::string::npos) ? std::string(".") : file_path.substr(0UL, pos);
  static std::atomic<uint64_t> staging_seq{0UL};
  const std::string staging_path =
      dir + "/" + kStagingFilePrefix + std::to_string(mmGetPid()) + "_" + std::to_string(staging_seq.fetch_add(1UL));
  const ScopeGuard staging_guard([&staging_path]() { (void)remove(staging_path.c_str()); });
  GE_CHK_STATUS_RET(WriteJsonFile(staging_path, json_obj), "Failed to write staging json file[%s]",
                    staging_path.c_str());
  GE_CHK_BOOL_RET_STATUS(std::rename(staging_path.c_str(), file_path.c_str()) == 0, FAILED,
                         "Failed to rename staging json[%s] to[%s]", staging_path.c_str(), file_path.c_str());
  return SUCCESS;
}

// 带重试的 flock：信号（如 SIGCHLD/SIGALRM）会中断 flock 并返回 EINTR，须重试而非报错
Status FlockRetry(const int32_t fd, const int operation) {
  while (flock(fd, operation) != 0) {
    if (errno != EINTR) {
      GELOGE(FAILED, "flock failed, errno=%d:%s", errno, strerror(errno));
      return FAILED;
    }
  }
  return SUCCESS;
}

// 校验合并文件区间 [offset, offset + size) 与内存字节一致，分块读回，每个唯一 hash 只调用一次
bool FileRangeEqualsData(const std::string &file_path, const size_t offset, const uint8_t *const data,
                         const size_t size) {
  if ((data == nullptr) && (size > 0UL)) {
    return false;
  }
  std::ifstream ifs(file_path, std::ifstream::binary);
  if (!ifs.is_open()) {
    return false;
  }
  ifs.seekg(static_cast<int64_t>(offset), std::ifstream::beg);
  if (ifs.fail()) {
    return false;
  }
  size_t compared = 0UL;
  const auto buf = MakeUnique<char[]>(static_cast<size_t>(kBlockSize));
  while (compared < size) {
    const size_t once = std::min<size_t>(static_cast<size_t>(kBlockSize), size - compared);
    (void)ifs.read(buf.get(), static_cast<std::streamsize>(once));
    const auto read_len = ifs.gcount();
    if ((read_len <= 0) || (static_cast<size_t>(read_len) != once)) {
      return false;
    }
    if (memcmp(buf.get(), data + compared, once) != 0) {
      return false;
    }
    compared += once;
  }
  return true;
}

// 目标路径是否为已存在的常规文件；符号链接等非常规文件返回 false 并由调用方报冲突
bool IsExistingRegularFile(const std::string &path) {
  struct stat path_stat{};
  if (lstat(path.c_str(), &path_stat) != 0) {
    return false;
  }
  return S_ISREG(path_stat.st_mode) != 0;
}

// 合并文件追加失败时回退到原始 EOF；回退失败时明确报错，遗留尾部不被索引引用，下次候选仍须验证
Status RollbackCombinedFileAppend(const std::string &weight_path, const size_t original_eof) {
  if (truncate(weight_path.c_str(), static_cast<off_t>(original_eof)) != 0) {
    GELOGE(FAILED,
           "Rollback append of combined file[%s] to offset[%zu] failed, tail bytes are left but not "
           "referenced by index, next append must verify and align again",
           weight_path.c_str(), original_eof);
    REPORT_INNER_ERR_MSG("E19999", "Rollback append of combined file[%s] failed", weight_path.c_str());
    return FAILED;
  }
  GELOGI("Rollback append of combined file[%s] to offset[%zu] success", weight_path.c_str(), original_eof);
  return SUCCESS;
}

size_t GetFileSizeOrZero(const std::string &path) {
  struct stat path_stat{};
  if (stat(path.c_str(), &path_stat) != 0) {
    return 0UL;
  }
  return static_cast<size_t>(path_stat.st_size);
}

bool SameWeightBytes(const GeTensorPtr &lhs, const GeTensorPtr &rhs) {
  const auto &lhs_buffer = lhs->GetData();
  const auto &rhs_buffer = rhs->GetData();
  const size_t lhs_size = lhs_buffer.GetSize();
  const size_t rhs_size = rhs_buffer.GetSize();
  return (lhs_size == rhs_size) &&
         ((lhs_size == 0UL) || (memcmp(lhs_buffer.GetData(), rhs_buffer.GetData(), lhs_size) == 0));
}

void SortConstNodeWeightHashList(ConstNodeWeightHashList &const_to_weight_hash_list) {
  std::stable_sort(const_to_weight_hash_list.begin(), const_to_weight_hash_list.end(),
                   [](const ConstNodeWeightHashList::value_type &lhs, const ConstNodeWeightHashList::value_type &rhs) {
                     const auto &lhs_node = lhs.first;
                     const auto &rhs_node = rhs.first;
                     if (lhs_node == rhs_node) {
                       return false;
                     }
                     if (lhs_node == nullptr) {
                       return true;
                     }
                     if (rhs_node == nullptr) {
                       return false;
                     }

                     const std::string lhs_name = lhs_node->GetName();
                     const std::string rhs_name = rhs_node->GetName();
                     if (lhs_name != rhs_name) {
                       return lhs_name < rhs_name;
                     }

                     const auto lhs_graph = lhs_node->GetOwnerComputeGraph();
                     const auto rhs_graph = rhs_node->GetOwnerComputeGraph();
                     const std::string lhs_graph_name = (lhs_graph == nullptr) ? "" : lhs_graph->GetName();
                     const std::string rhs_graph_name = (rhs_graph == nullptr) ? "" : rhs_graph->GetName();
                     if (lhs_graph_name != rhs_graph_name) {
                       return lhs_graph_name < rhs_graph_name;
                     }

                     const auto lhs_op_desc = lhs_node->GetOpDesc();
                     const auto rhs_op_desc = rhs_node->GetOpDesc();
                     const int64_t lhs_id = (lhs_op_desc == nullptr) ? -1 : lhs_op_desc->GetId();
                     const int64_t rhs_id = (rhs_op_desc == nullptr) ? -1 : rhs_op_desc->GetId();
                     return lhs_id < rhs_id;
                   });
}
}  // namespace

void from_json(const nlohmann::json &j, FileIdToFilePath &info) {
  const auto id = j.find(kBinIdValue);
  if (id != j.end()) {
    info.value_bin_file_id = id->get<std::string>();
  }

  const auto file_path = j.find(kBinFilePathValue);
  if (file_path != j.end()) {
    info.value_bin_file_path = file_path->get<std::string>();
  }
}

void from_json(const nlohmann::json &j, OptionInfo &option_info) {
  const auto it = j.find(kBinFileValues);
  if (it != j.end()) {
    option_info = it->get<OptionInfo>();
  }
}

Status FileConstantUtils::GetFileIdToPathMapFromOption(std::map<std::string, std::string> &file_id_to_path_map) {
  std::string opt;
  (void)GetContext().GetOption(FILE_CONSTANT_PATH, opt);
  if (opt.empty()) {
    GELOGW("[Check][Param] Failed to get file constant path.");
    return SUCCESS;
  }
  GELOGI("source string = %s.", opt.c_str());

  nlohmann::json options;
  try {
    options = nlohmann::json::parse(opt);
  } catch (nlohmann::json::exception &ex) {
    const auto readable_name = ge::GetContext().GetReadableName(FILE_CONSTANT_PATH);
    std::string reason = "it is not a valid json string, exception: " + std::string(ex.what());
    (void)REPORT_PREDEFINED_ERR_MSG("E10001", std::vector<const char *>({"value", "parameter", "reason"}),
                                    std::vector<const char *>({opt.c_str(), readable_name.c_str(), reason.c_str()}));
    GELOGE(FAILED, "Failed to parse option FILE_CONSTANT_PATH, which [%s] is invalid, err:%s", opt.c_str(), ex.what());
    return FAILED;
  }

  for (const nlohmann::json &single_json : options) {
    GELOGD("Parsing op[%d], jsonStr = %s.", kFirstElementIndex, single_json.dump(kIndentWidth).c_str());
    std::vector<FileIdToFilePath> multi_info;
    multi_info = single_json.get<std::vector<FileIdToFilePath>>();
    for (const auto &single_info : multi_info) {
      GELOGD("get single info, file id is %s, file path is %s.", single_info.value_bin_file_id.c_str(),
             single_info.value_bin_file_path.c_str());
      (void)file_id_to_path_map.insert(
          std::pair<std::string, std::string>(single_info.value_bin_file_id, single_info.value_bin_file_path));
    }
  }
  return SUCCESS;
}

namespace {
// 按 kBlockSize 分块把文件内容拷贝到目标内存(设备或主机), 返回实际拷贝字节数
// aclrtMemcpy 的 kind 为预留参数, 系统按源/目的地址自动判定, 且文档明确支持非锁页 Host 内存(malloc/new 申请),
// 故 Host 权重缓冲也可直接走本函数, 无需像清零那样区分内存位置
Status CopyBlocksToDevice(const void *const curr_dev_ptr, const std::string &file_path, std::ifstream &ifs,
                          const size_t usable_size, const size_t left_size, size_t &used_memory) {
  // 分块上限为 kBlockSize, 小权重只分配实际需要的量; MakeUnique 会零初始化缓冲区
  const size_t buf_size = std::min<size_t>(static_cast<size_t>(kBlockSize), std::max<size_t>(usable_size, 1U));
  const std::unique_ptr<char[]> compress_nodes = MakeUnique<char[]>(buf_size);
  GE_CHECK_NOTNULL(compress_nodes);
  while ((!ifs.eof()) && (used_memory != usable_size)) {
    const size_t once_size = std::min(buf_size, usable_size - used_memory);
    (void)ifs.read(compress_nodes.get(), static_cast<std::streamsize>(once_size));
    auto copy_len_once = static_cast<size_t>(ifs.gcount());
    // read 失败(如 I/O 错误)时可能既不推进读指针也不置 eofbit, 必须显式退出避免死循环
    if (copy_len_once == 0U) {
      GELOGE(FAILED, "Read file[%s] failed or got no data at used memory[%zu].", file_path.c_str(), used_memory);
      REPORT_INNER_ERR_MSG("E19999", "Read weight file[%s] failed at used memory[%zu].", file_path.c_str(),
                           used_memory);
      return FAILED;
    }
    if ((usable_size - used_memory) < copy_len_once) {
      copy_len_once = usable_size - used_memory;
    }
    GELOGI("copy %zu bytes to memory, path = %s.", copy_len_once, file_path.c_str());
    void *const cur_dev_ptr = reinterpret_cast<void *>(PtrToValue(curr_dev_ptr) + used_memory);
    const aclError rts_error = aclrtMemcpy(cur_dev_ptr, left_size - used_memory, compress_nodes.get(), copy_len_once,
                                           ACL_MEMCPY_HOST_TO_DEVICE);
    if (rts_error != ACL_SUCCESS) {
      GELOGE(GRAPH_FAILED, "copy failed, result code = %d.", rts_error);
      REPORT_INNER_ERR_MSG("E19999", "copy failed, result code = %d.", rts_error);
      return RT_ERROR_TO_GE_STATUS(rts_error);
    }
    used_memory += copy_len_once;
  }
  return SUCCESS;
}

// 文件数据未覆盖的区域显式清零, 避免把残留内存内容暴露给算子
// Host 缓冲走 memset_s: 非 acl 接口(如 aclrtMallocHost)申请的 Host 内存传给 aclrtMemset 属未定义行为
Status ClearWeightTail(const void *const curr_ptr, const size_t used_memory, const size_t total_size,
                       const std::string &file_path, const WeightMemLocation mem_loc) {
  if (used_memory >= total_size) {
    return SUCCESS;
  }
  void *const tail_ptr = reinterpret_cast<void *>(PtrToValue(curr_ptr) + used_memory);
  const size_t tail_size = total_size - used_memory;
  GELOGI("Clear [%zu] bytes tail of file constant for file[%s].", tail_size, file_path.c_str());
  if (mem_loc == WeightMemLocation::kHost) {
    const auto memset_err = memset_s(tail_ptr, tail_size, 0, tail_size);
    if (memset_err != EOK) {
      GELOGE(FAILED, "Clear host tail of file constant failed, err = %d.", memset_err);
      return FAILED;
    }
    return SUCCESS;
  }
  const aclError rts_error = aclrtMemset(tail_ptr, tail_size, 0, tail_size);
  if (rts_error != ACL_SUCCESS) {
    GELOGE(GRAPH_FAILED, "Clear tail of file constant failed, result code = %d.", rts_error);
    return RT_ERROR_TO_GE_STATUS(rts_error);
  }
  return SUCCESS;
}
}  // namespace

Status FileConstantUtils::CheckAndSeekWeightFile(std::ifstream &ifs, const std::string &file_path, const size_t offset,
                                                 const size_t length, size_t &usable_size, const bool enforce_range) {
  usable_size = 0U;
  ifs.clear();
  (void)ifs.seekg(0, std::ifstream::end);
  const std::streamoff file_size = ifs.tellg();
  if (ifs.fail() || (file_size < 0)) {
    GELOGE(FAILED, "Failed to get size of weight file[%s].", file_path.c_str());
    (void)REPORT_PREDEFINED_ERR_MSG("E13001", std::vector<const char *>({"file", "errmsg"}),
                                    std::vector<const char *>({file_path.c_str(), "Get file size failed"}));
    return FAILED;
  }
  // 减法比较: 既避免 offset + length 溢出, 也能拦截 int64 负属性值转成 size_t 后的超大值
  const size_t act_file_len = static_cast<size_t>(file_size);
  if (offset > act_file_len) {
    GELOGE(FAILED, "Offset[%zu] exceeds the size[%zu] of weight file[%s].", offset, act_file_len, file_path.c_str());
    REPORT_INNER_ERR_MSG("E19999", "Offset[%zu] exceeds weight file size[%zu], file[%s].", offset, act_file_len,
                         file_path.c_str());
    return FAILED;
  }
  usable_size = std::min(length, act_file_len - offset);
  // length 为调用方未声明权重长度时推导出的上限(可能含对齐 padding), 允许文件偏短, 由调用方对剩余区域清零;
  // 显式声明长度时必须完整覆盖, 否则说明权重文件被截断
  if (enforce_range && (usable_size != length)) {
    GELOGE(FAILED, "Weight file[%s] is truncated: expect offset[%zu] length[%zu], but actual file size is[%zu].",
           file_path.c_str(), offset, length, act_file_len);
    REPORT_INNER_ERR_MSG("E19999", "Weight file is truncated: expect offset[%zu] length[%zu], actual size[%zu].",
                         offset, length, act_file_len);
    return FAILED;
  }
  ifs.clear();
  (void)ifs.seekg(static_cast<int64_t>(offset), ifs.beg);
  if (ifs.fail()) {
    GELOGE(FAILED, "Failed to seek weight file[%s] to offset[%zu].", file_path.c_str(), offset);
    return FAILED;
  }
  return SUCCESS;
}

Status FileConstantUtils::CopyOneWeightFromFileWithFilehandler(const void *const curr_dev_ptr,
                                                               const std::string &file_path, const size_t offset,
                                                               const size_t file_constant_size, size_t &left_size,
                                                               std::ifstream &ifs, const bool enforce_range,
                                                               const WeightMemLocation mem_loc) {
  GE_CHECK_GE(left_size, file_constant_size);
  const size_t total_size = left_size;
  size_t usable_size = 0U;
  GE_CHK_STATUS_RET(CheckAndSeekWeightFile(ifs, file_path, offset, file_constant_size, usable_size, enforce_range),
                    "Check weight file[%s] failed, offset[%zu], length[%zu].", file_path.c_str(), offset,
                    file_constant_size);
  size_t used_memory = 0U;
  GE_CHK_STATUS_RET(CopyBlocksToDevice(curr_dev_ptr, file_path, ifs, usable_size, total_size, used_memory),
                    "Copy weight from file[%s] failed.", file_path.c_str());
  // 实际读入的字节数必须与可读长度一致, 否则说明文件在读取期间被截断
  if (used_memory != usable_size) {
    GELOGE(FAILED, "Read [%zu] bytes from file[%s] but expect [%zu] bytes.", used_memory, file_path.c_str(),
           usable_size);
    REPORT_INNER_ERR_MSG("E19999", "Read weight file[%s] incomplete, read[%zu] expect[%zu].", file_path.c_str(),
                         used_memory, usable_size);
    return FAILED;
  }
  // 与前序实现保持一致: 无论清零是否成功都按实际读入量回填剩余空间
  const Status clear_ret = ClearWeightTail(curr_dev_ptr, used_memory, total_size, file_path, mem_loc);
  left_size -= used_memory;
  GELOGI("used memory is %zu.", used_memory);
  return clear_ret;
}

Status FileConstantUtils::CopyOneWeightFromFile(const void *const curr_dev_ptr, const std::string &file_path,
                                                const size_t offset, const size_t file_constant_size, size_t &left_size,
                                                const bool enforce_range, const WeightMemLocation mem_loc) {
  GE_CHECK_GE(left_size, file_constant_size);
  const std::string real_path = RealPath(file_path.c_str());
  std::ifstream ifs(real_path, std::ifstream::binary);
  if (!ifs.is_open()) {
    GELOGE(FAILED, "[Open][File] %s failed.", file_path.c_str());
    (void)REPORT_PREDEFINED_ERR_MSG("E13001", std::vector<const char *>({"file", "errmsg"}),
                                    std::vector<const char *>({file_path.c_str(), "Open file failed"}));
    return FAILED;
  }
  const Status ret = CopyOneWeightFromFileWithFilehandler(curr_dev_ptr, real_path, offset, file_constant_size,
                                                          left_size, ifs, enforce_range, mem_loc);
  ifs.close();
  return ret;
}

Status FileConstantUtils::GetFilePath(const OpDescPtr &op_desc,
                                      const std::map<std::string, std::string> &file_id_to_path_map,
                                      std::string &file_path, size_t &offset, size_t &length) {
  const auto &fileconstant_info = GetFileConstantInfo(op_desc);
  if (!fileconstant_info.weight_path.empty()) {
    file_path = fileconstant_info.weight_path;
    offset = fileconstant_info.weight_offset;
    length = fileconstant_info.weight_length;
    return SUCCESS;
  }
  offset = 0U;
  length = 0U;
  file_path = "";
  const std::string *file_path_ptr = AttrUtils::GetStr(op_desc, ATTR_NAME_FILE_PATH);
  if (file_path_ptr != nullptr && !file_path_ptr->empty()) {
    file_path = *file_path_ptr;
    return SUCCESS;
  }
  const std::string *file_id_ptr = AttrUtils::GetStr(op_desc, ATTR_NAME_FILE_CONSTANT_ID);
  GE_CHK_BOOL_RET_STATUS(file_id_ptr != nullptr, FAILED, "Failed to get filed id from attr");
  GE_CHK_BOOL_RET_STATUS(!file_id_ptr->empty(), FAILED, "The file path and file id are empty.");
  const auto it = file_id_to_path_map.find(*file_id_ptr);
  if (it == file_id_to_path_map.end()) {
    GELOGW("Failed to get file path of file id:%s", file_id_ptr->c_str());
    return SUCCESS;
  }
  GE_CHK_BOOL_RET_STATUS(!(it->second.empty()), FAILED, "File path is empty.");
  file_path = it->second;
  return SUCCESS;
}

FileConstantInfo FileConstantUtils::GetFileConstantInfo(const OpDescPtr &op_desc) {
  FileConstantInfo fileconstant_info;
  const std::string *weight_path_ptr = AttrUtils::GetStr(op_desc, ATTR_NAME_LOCATION);
  if (weight_path_ptr != nullptr) {
    fileconstant_info.weight_path = *weight_path_ptr;
  }
  int64_t attr_value = 0;
  (void)AttrUtils::GetInt(op_desc, ATTR_NAME_OFFSET, attr_value);
  fileconstant_info.weight_offset = static_cast<size_t>(attr_value);
  int64_t attr_length = 0;
  (void)AttrUtils::GetInt(op_desc, ATTR_NAME_LENGTH, attr_length);
  fileconstant_info.weight_length = static_cast<size_t>(attr_length);
  return fileconstant_info;
}

Status FileConstantUtils::GetExternalWeightDir(const ge::ModelData &model_data, string &file_constant_weight_dir) {
  if (!model_data.weight_path.empty()) {
    std::string weight_real_path = ge::RealPath(model_data.weight_path.c_str());
    GE_ASSERT_TRUE(!weight_real_path.empty());
    file_constant_weight_dir = weight_real_path + "/";
    GELOGI("Get external weight path from model_data.weight_path: %s", file_constant_weight_dir.c_str());
    return SUCCESS;
  }
  return GetExternalWeightDirFromOmPath(model_data.om_path, file_constant_weight_dir);
}

Status FileConstantUtils::GetExternalWeightDirFromOmPath(const std::string &om_path, string &file_constant_weight_dir) {
  if (om_path.empty()) {
    GELOGW("Om path is empty, thus file constant weight dir is empty.");
    return ge::SUCCESS;
  }
  const std::string om_real_path = ge::RealPath(om_path.c_str());
  GELOGD("Get OM path[%s], real path[%s].", om_path.c_str(), om_real_path.c_str());
  std::string om_dir;
  std::string om_name;
  ge::SplitFilePath(om_real_path, om_dir, om_name);
  GELOGD("OM dir is[%s], om name is[%s].", om_dir.c_str(), om_name.c_str());
  GE_ASSERT_TRUE(!om_name.empty());
  file_constant_weight_dir = om_dir.append("/weight/");
  GELOGI("Get external weight path %s from model_data.om_path: %s", file_constant_weight_dir.c_str(), om_path.c_str());
  return ge::SUCCESS;
}

Status FileConstantUtils::SetExternalPath(const OpDescPtr &op_desc, const std::string &weight_dir) {
  const auto &fileconstant_info = GetFileConstantInfo(op_desc);
  std::string file_name = fileconstant_info.weight_path;
  const size_t offset = fileconstant_info.weight_offset;
  const size_t length = fileconstant_info.weight_length;
  if (file_name.empty()) {
    return SUCCESS;
  }
  if (file_name.rfind('/') != std::string::npos) {
    file_name = file_name.substr(file_name.rfind('/') + 1UL);
  }
  const std::string file_path = weight_dir + file_name;
  // refresh file constant location to new weight_dir
  SetFileConstantPath(op_desc, file_path, static_cast<int64_t>(offset), static_cast<int64_t>(length));
  GELOGD("Set external path success, file path:%s", file_path.c_str());
  return SUCCESS;
}

Status FileConstantUtils::SetExternalPath(const ComputeGraphPtr &compute_graph, const std::string &weight_dir) {
  for (const auto &node : compute_graph->GetAllNodes()) {
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if (op_desc->GetType() != FILECONSTANT) {
      continue;
    }
    GE_ASSERT_SUCCESS(SetExternalPath(op_desc, weight_dir), "Op: %s set external path failed.",
                      op_desc->GetName().c_str());
  }
  return SUCCESS;
}

void FileConstantUtils::GetValidFullPath(const std::string &dir_name, const std::string &file_name,
                                         std::string &full_name) {
  full_name = dir_name;
  if (!dir_name.empty() && (dir_name.back() != '/')) {
    full_name = full_name.append("/");
  }
  full_name = full_name.append(file_name);
}

Status FileConstantUtils::ReadExternalWeightFromFile(const std::string &file_path, const size_t offset,
                                                     const size_t file_length, char_t *const bin_buff) {
  const std::string real_path = RealPath(file_path.c_str());
  GE_CHK_BOOL_RET_STATUS(!real_path.empty(), FAILED, "Failed to get real path of %s", file_path.c_str());
  std::ifstream ifs(real_path, std::ifstream::binary);
  GE_CHK_BOOL_RET_STATUS(ifs.is_open(), FAILED, "Read file %s failed.", real_path.c_str());
  size_t usable_size = 0U;
  GE_CHK_STATUS_RET(CheckAndSeekWeightFile(ifs, real_path, offset, file_length, usable_size),
                    "Check weight file[%s] failed, offset[%zu], length[%zu].", real_path.c_str(), offset, file_length);
  (void)ifs.read(bin_buff, static_cast<int64_t>(file_length));
  GE_CHK_BOOL_RET_STATUS(ifs.good(), FAILED, "read file %s failed.", real_path.c_str());
  ifs.close();
  return SUCCESS;
}

Status FileConstantUtils::ConvertFileConstToConst(const NodePtr &node) {
  GE_ASSERT_TRUE(node->GetType() == FILECONSTANT, "%s[%s] is not fileconstant", node->GetName().c_str(),
                 node->GetType().c_str());
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  auto output_desc = op_desc->MutableOutputDesc(0U);
  GE_CHECK_NOTNULL(output_desc);
  DataType out_type = ge::DT_UNDEFINED;
  (void)AttrUtils::GetDataType(op_desc, kAttrDtype, out_type);
  output_desc->SetDataType(out_type);
  int64_t weight_size = 0;
  GE_CHK_STATUS_RET(TensorUtils::GetTensorSizeInBytes(*output_desc, weight_size), "Failed to get weight size");
  const auto &fileconstant_info = GetFileConstantInfo(op_desc);
  std::string file_path = fileconstant_info.weight_path;
  if (file_path.empty()) {
    const std::string *file_path_ptr = AttrUtils::GetStr(op_desc, ATTR_NAME_FILE_PATH);
    if (file_path_ptr == nullptr || file_path_ptr->empty()) {
      return SUCCESS;
    }
    file_path = *file_path_ptr;
  }
  const size_t offset = fileconstant_info.weight_offset;
  const size_t length = fileconstant_info.weight_length;
  const size_t file_length = (length == 0U ? static_cast<size_t>(weight_size) : length);
  GE_CHK_STATUS_RET(CheckUint64AddOverflow(offset, file_length), "offset add length overflow uint64");
  const auto bin_buff = MakeUnique<char_t[]>(file_length);
  GE_CHECK_NOTNULL(bin_buff);
  GE_CHK_STATUS_RET(ReadExternalWeightFromFile(file_path, offset, file_length, bin_buff.get()),
                    "Failed to read external weight from file:%s", file_path.c_str());
  const auto &weight = MakeShared<GeTensor>(*output_desc, reinterpret_cast<uint8_t *>(bin_buff.get()), file_length);
  GE_CHECK_NOTNULL(weight);
  OpDescUtilsEx::SetType(op_desc, CONSTANT);
  GE_ASSERT_TRUE(AttrUtils::SetShareTensor(op_desc, ATTR_NAME_WEIGHTS, *weight));
  (void)op_desc->DelAttr(ATTR_NAME_OFFSET);
  (void)op_desc->DelAttr(ATTR_NAME_LENGTH);
  (void)op_desc->DelAttr(ATTR_NAME_LOCATION);
  GELOGI("Convert node:%s from file constant to const success.", node->GetName().c_str());
  return SUCCESS;
}

Status FileConstantUtils::ConvertFileConstToConst(const ComputeGraphPtr &compute_graph) {
  ThreadPool thread_pool("ge_fcst2cst", static_cast<uint32_t>(kDefaultThreadNum), false);
  std::vector<std::future<Status>> fut_rets;
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (node->GetType() != FILECONSTANT) {
      continue;
    }
    const auto &error_manager_context = error_message::GetErrMgrContext();
    auto fut = thread_pool.commit([node, error_manager_context]() -> Status {
      error_message::SetErrMgrContext(error_manager_context);
      GE_CHK_STATUS_RET_NOLOG(ConvertFileConstToConst(node));
      return SUCCESS;
    });
    (void)fut_rets.emplace_back(std::move(fut));
  }
  for (auto &fut : fut_rets) {
    GE_CHK_STATUS_RET(fut.get(), "Failed to convert fileconstant to const, graph:%s", compute_graph->GetName().c_str());
  }
  return SUCCESS;
}

ConstNodeWeightHashList FileConstantUtils::GetAllConstNodesAndWeightHash(const ComputeGraphPtr &compute_graph) {
  ConstNodeWeightHashList const_to_weight_hash_list;
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (!NodeUtils::IsConst(*node)) {
      continue;
    }
    std::string hash_value;
    const std::string *hash_value_ptr = AttrUtils::GetStr(node->GetOpDesc(), ATTR_NAME_WEIGHT_SHA256);
    if (hash_value_ptr == nullptr || hash_value_ptr->empty()) {
      continue;
    }
    hash_value = *hash_value_ptr;
    const auto &weights = OpDescUtils::MutableWeights(node);
    GE_ASSERT_TRUE(!weights.empty(), "Failed to get weight of const node:%s", node->GetName().c_str());
    const auto &weight = weights[0];
    GE_ASSERT_NOTNULL(weight, "Weight is null, node:%s", node->GetName().c_str());
    const_to_weight_hash_list.emplace_back(node, std::make_pair(weight, hash_value));
  }
  SortConstNodeWeightHashList(const_to_weight_hash_list);
  return const_to_weight_hash_list;
}

// 模式 1（独立文件）：不读写 meta 两张表，直接从当前目录派生 weight_<hash>；
// 批内同 hash 先校验内容一致；已有同名文件仅字节匹配时复用；新文件写同目录临时文件
// 确认关闭后无覆盖安装，避免覆盖既有不同内容的权重
Status FileConstantUtils::SaveWeightToFileWithReuse(const ConstNodeWeightHashList &const_to_weight_hash_list,
                                                    const std::string &weight_dir,
                                                    std::map<std::string, WeightLocation> &locations) {
  errno = 0;
  const char_t *value = nullptr;
  MM_SYS_GET_ENV(MM_ENV_MAX_COMPILE_CORE_NUMBER, value);
  const int64_t thread_num =
      ((value != nullptr) && (value[0U] != '\0')) ? std::strtol(value, nullptr, 10) : kDefaultThreadNum;
  GE_ASSERT_TRUE(errno == 0, "strtol failed, value: %s", value);
  GELOGI("Start to save weight to separate files, thread num[%ld]", thread_num);
  std::map<std::string, GeTensorPtr> candidates;
  GE_CHK_STATUS_RET(CollectUniqueWeightCandidates(const_to_weight_hash_list, candidates),
                    "Failed to collect unique weight candidates");
  ThreadPool thread_pool("ge_savweigh", static_cast<uint32_t>(thread_num), false);
  std::vector<std::future<Status>> fut_rets;
  static std::atomic<uint64_t> staging_seq{0UL};
  for (const auto &hash_and_tensor : candidates) {
    const auto &weight_hash = hash_and_tensor.first;
    const auto &weight = hash_and_tensor.second;
    const std::string file_name = "weight_" + weight_hash;
    std::string weight_path;
    GetValidFullPath(weight_dir, file_name, weight_path);
    const auto &data = weight->GetData().GetData();
    const auto size = weight->GetData().GetSize();
    const auto &error_manager_context = error_message::GetErrMgrContext();
    auto fut = thread_pool.commit([weight_hash, weight_path, data, size, error_manager_context]() -> Status {
      error_message::SetErrMgrContext(error_manager_context);
      if (IsExistingRegularFile(weight_path)) {
        // 已有同名文件仅在字节相同时复用，内容不同报错且不覆盖
        GE_CHK_BOOL_RET_STATUS(FileRangeEqualsData(weight_path, 0UL, data, size), FAILED,
                               "Existing weight file[%s] has different bytes with hash[%s], refuse to overwrite",
                               weight_path.c_str(), weight_hash.c_str());
        GELOGI("Reuse existing weight file: [%s] with identical bytes.", weight_path.c_str());
        return SUCCESS;
      }
      if (mmAccess(weight_path.c_str()) == EN_OK) {
        GELOGE(FAILED, "Target weight path[%s] already exists but is not a regular file", weight_path.c_str());
        REPORT_INNER_ERR_MSG("E19999", "Target weight path[%s] is not a regular file", weight_path.c_str());
        return FAILED;
      }
      const auto pos = weight_path.find_last_of('/');
      const std::string dir = (pos == std::string::npos) ? std::string(".") : weight_path.substr(0UL, pos);
      const std::string staging_path = dir + "/" + kStagingFilePrefix + std::to_string(mmGetPid()) + "_" +
                                       std::to_string(staging_seq.fetch_add(1UL));
      const ScopeGuard staging_guard([&staging_path]() { (void)remove(staging_path.c_str()); });
      std::ofstream ofs(staging_path, std::ios::binary | std::ios::trunc);
      GE_CHK_BOOL_RET_STATUS(ofs.is_open(), FAILED, "Failed to create staging weight file[%s]", staging_path.c_str());
      (void)ofs.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
      GE_CHK_BOOL_RET_STATUS(ofs.good(), FAILED, "Failed to write staging weight file[%s]", staging_path.c_str());
      ofs.flush();
      ofs.close();
      GE_CHK_BOOL_RET_STATUS(!ofs.fail() && !ofs.bad(), FAILED, "Failed to close staging weight file[%s]",
                             staging_path.c_str());
      // 同目录临时文件无覆盖安装；并发同名时回到字节比较，仅字节相同才复用
      if (link(staging_path.c_str(), weight_path.c_str()) != 0) {
        GE_CHK_BOOL_RET_STATUS(errno == EEXIST, FAILED, "Failed to install weight file[%s] from staging[%s]",
                               weight_path.c_str(), staging_path.c_str());
        GE_CHK_BOOL_RET_STATUS(FileRangeEqualsData(weight_path, 0UL, data, size), FAILED,
                               "Concurrent weight file[%s] has different bytes with hash[%s], refuse to overwrite",
                               weight_path.c_str(), weight_hash.c_str());
        GELOGI("Concurrent published weight file[%s] has identical bytes, reuse it.", weight_path.c_str());
      }
      return SUCCESS;
    });
    (void)fut_rets.emplace_back(std::move(fut));
  }
  for (auto &fut : fut_rets) {
    GE_CHK_STATUS_RET(fut.get(), "Failed to save weight to file");
  }
  for (const auto &hash_and_tensor : candidates) {
    std::string weight_path;
    GetValidFullPath(weight_dir, "weight_" + hash_and_tensor.first, weight_path);
    locations[hash_and_tensor.first] = WeightLocation{weight_path, 0UL};
  }
  return SUCCESS;
}

Status FileConstantUtils::CollectUniqueWeightCandidates(const ConstNodeWeightHashList &const_to_weight_hash_list,
                                                        std::map<std::string, GeTensorPtr> &candidates) {
  for (const auto &const_and_weight_hash : const_to_weight_hash_list) {
    const auto &weight_hash = const_and_weight_hash.second.second;
    const auto &weight = const_and_weight_hash.second.first;
    GE_CHECK_NOTNULL(weight);
    const auto iter = candidates.find(weight_hash);
    if (iter != candidates.end()) {
      // 相同 hash 的不同 tensor 先比长度和内容，不以 hash 字符串相等证明内容相同
      GE_CHK_BOOL_RET_STATUS(SameWeightBytes(iter->second, weight), FAILED,
                             "Same hash[%s] maps to different weight bytes in one batch", weight_hash.c_str());
      continue;
    }
    (void)candidates.emplace(weight_hash, weight);
  }
  return SUCCESS;
}

static std::string GetCombinedWeightFileName(const ConstNodeWeightHashList &const_to_weight_hash_list) {
  ComputeGraphPtr root_graph = nullptr;
  for (const auto &const_and_weight_hash : const_to_weight_hash_list) {
    if ((root_graph == nullptr) && (const_and_weight_hash.first != nullptr)) {
      root_graph = NodeUtils::FindRootGraph(*const_and_weight_hash.first);
    }
  }

  std::string model_file_name_prefix;
  std::string graph_name;
  if (root_graph != nullptr) {
    graph_name = root_graph->GetName();
    (void)AttrUtils::GetStr(root_graph, ATTR_MODEL_FILE_NAME_PREFIX, model_file_name_prefix);
  }

  std::string om_name = StringUtils::GetFileName(model_file_name_prefix);
  const auto pos = om_name.rfind('.');
  if (pos != std::string::npos) {
    om_name = om_name.substr(0, pos);
  }
  return om_name.empty() ? (graph_name.empty() ? "weight_combined" : (graph_name + "_weight_combined"))
                         : (om_name + "_weight_combined");
}

Status FileConstantUtils::ResolveCombinedWeightReuse(const std::string &weight_path,
                                                     const std::map<std::string, GeTensorPtr> &candidates,
                                                     const FileConstantMeta &meta,
                                                     std::map<std::string, WeightLocation> &locations,
                                                     std::map<std::string, GeTensorPtr> &to_append) {
  const size_t file_size = GetFileSizeOrZero(weight_path);
  const bool combined_file_exist = (file_size > 0UL);
  std::string real_combined_path;
  if (combined_file_exist) {
    real_combined_path = RealPath(weight_path.c_str());
    GE_CHK_BOOL_RET_STATUS(!real_combined_path.empty(), FAILED, "Failed to get real path of %s", weight_path.c_str());
  }

  // 逐 hash 决定复用或追加，每个唯一 hash 的旧区间字节最多验证一次
  for (const auto &hash_and_tensor : candidates) {
    const auto &weight_hash = hash_and_tensor.first;
    const auto &weight = hash_and_tensor.second;
    const auto size = weight->GetData().GetSize();
    bool reused = false;
    const auto offset_iter = meta.hash_to_weight_offset.find(weight_hash);
    const auto file_iter = meta.hash_to_weight_file.find(weight_hash);
    if (combined_file_exist && (offset_iter != meta.hash_to_weight_offset.end()) &&
        (file_iter != meta.hash_to_weight_file.end())) {
      const size_t old_offset = offset_iter->second;
      const std::string &old_path = file_iter->second;
      const std::string real_old_path = RealPath(old_path.c_str());
      const bool offset_ok = ((old_offset % kCombinedWeightAlign) == 0UL) &&
                             (old_offset <= (std::numeric_limits<size_t>::max() - size)) &&
                             ((old_offset + size) <= file_size);
      if (!real_old_path.empty() && (real_old_path == real_combined_path) && offset_ok &&
          FileRangeEqualsData(weight_path, old_offset, weight->GetData().data(), size)) {
        locations[weight_hash] = WeightLocation{weight_path, old_offset};
        reused = true;
        GELOGI("Reuse combined weight hash[%s] at verified offset[%zu]", weight_hash.c_str(), old_offset);
      } else if (!real_old_path.empty() && (real_old_path == real_combined_path) && offset_ok) {
        GELOGW("Recorded interval of hash[%s] in combined file[%s] does not match tensor bytes, append again",
               weight_hash.c_str(), weight_path.c_str());
      }
    }
    if (!reused) {
      (void)to_append.emplace(weight_hash, weight);
    }
  }

  return SUCCESS;
}

Status FileConstantUtils::AlignCombinedWeightEof(std::ofstream &ofs, const OneFileAppendContext &append_ctx,
                                                 size_t &offset) {
  offset = MemSizeAlign(append_ctx.original_eof, static_cast<uint32_t>(kCombinedWeightAlign));
  if (offset > append_ctx.original_eof) {
    static const std::string kAlignZeros(kCombinedWeightAlign, '\0');
    const size_t gap = offset - append_ctx.original_eof;
    (void)ofs.write(kAlignZeros.data(), static_cast<std::streamsize>(gap));
    if (!ofs.good()) {
      (void)ofs.close();
      const Status rollback_ret = RollbackCombinedFileAppend(append_ctx.weight_path, append_ctx.original_eof);
      GE_CHK_BOOL_RET_STATUS(rollback_ret == SUCCESS, FAILED, "Append failed and rollback also failed");
      GELOGE(FAILED, "Failed to align combined file[%s] to offset[%zu]", append_ctx.weight_path.c_str(), offset);
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FileConstantUtils::AppendCombinedWeights(const std::map<std::string, GeTensorPtr> &to_append,
                                                std::map<std::string, WeightLocation> &locations,
                                                OneFileAppendContext &append_ctx) {
  const auto &weight_path = append_ctx.weight_path;
  std::ofstream ofs(weight_path, std::ios::binary | std::ios::app | std::ios::out);
  GE_CHK_BOOL_RET_STATUS(ofs.is_open(), FAILED, "Failed to open weight file for writing: %s", weight_path.c_str());
  (void)ofs.seekp(0, std::ios::end);
  const std::streamoff current_eof = ofs.tellp();
  GE_CHK_BOOL_RET_STATUS(!ofs.fail() && (current_eof >= 0), FAILED, "Failed to get current eof of weight file: %s",
                         weight_path.c_str());
  append_ctx.original_eof = static_cast<size_t>(current_eof);
  size_t offset = 0UL;
  GE_CHK_STATUS_RET_NOLOG(AlignCombinedWeightEof(ofs, append_ctx, offset));
  for (const auto &hash_and_tensor : to_append) {
    const auto &weight_hash = hash_and_tensor.first;
    const auto &data = hash_and_tensor.second->GetData();
    const size_t start_offset = offset;
    if (WriteWeightWithPadding(ofs, data.data(), data.size(), offset) != SUCCESS) {
      (void)ofs.close();
      const Status rollback_ret = RollbackCombinedFileAppend(weight_path, append_ctx.original_eof);
      GE_CHK_BOOL_RET_STATUS(rollback_ret == SUCCESS, FAILED, "Append failed and rollback also failed");
      GELOGE(FAILED, "Failed to write weight[%s] to combined file[%s]", weight_hash.c_str(), weight_path.c_str());
      return FAILED;
    }
    locations[weight_hash] = WeightLocation{weight_path, start_offset};
  }
  ofs.flush();
  if (!ofs.good()) {
    (void)ofs.close();
    const Status rollback_ret = RollbackCombinedFileAppend(weight_path, append_ctx.original_eof);
    GE_CHK_BOOL_RET_STATUS(rollback_ret == SUCCESS, FAILED, "Flush failed and rollback also failed");
    return FAILED;
  }
  ofs.close();
  GE_CHK_BOOL_RET_STATUS(!ofs.fail() && !ofs.bad(), FAILED, "Failed to close combined weight file[%s]",
                         weight_path.c_str());
  append_ctx.appended = true;
  return SUCCESS;
}

Status FileConstantUtils::SaveWeightToOneFileWithReuse(const ConstNodeWeightHashList &const_to_weight_hash_list,
                                                       const std::string &weight_dir, FileConstantMeta &meta,
                                                       std::map<std::string, WeightLocation> &locations,
                                                       OneFileAppendContext &append_ctx) {
  append_ctx = {};
  const auto weight_file_name = GetCombinedWeightFileName(const_to_weight_hash_list);
  GE_CHK_BOOL_RET_STATUS(weight_file_name.find('/') == std::string::npos, FAILED,
                         "Combined weight file name[%s] must not contain path separator", weight_file_name.c_str());
  auto &weight_path = append_ctx.weight_path;
  GetValidFullPath(weight_dir, weight_file_name, weight_path);

  std::map<std::string, GeTensorPtr> candidates;
  GE_CHK_STATUS_RET(CollectUniqueWeightCandidates(const_to_weight_hash_list, candidates),
                    "Failed to collect unique weight candidates");
  std::map<std::string, GeTensorPtr> to_append;
  GE_CHK_STATUS_RET_NOLOG(ResolveCombinedWeightReuse(weight_path, candidates, meta, locations, to_append));
  const size_t reused_count = candidates.size() - to_append.size();
  GELOGI("Converting %zu const nodes (%zu unique hashes) to file constants (all-in-one mode), reused: %zu, new: %zu",
         const_to_weight_hash_list.size(), candidates.size(), reused_count, to_append.size());
  if (!to_append.empty()) {
    GE_CHK_STATUS_RET_NOLOG(AppendCombinedWeights(to_append, locations, append_ctx));
  }

  // 复用区间的 hash 同步刷新提示表，保持最近一次合并位置语义
  for (const auto &hash_and_location : locations) {
    meta.hash_to_weight_file[hash_and_location.first] = weight_path;
    meta.hash_to_weight_offset[hash_and_location.first] = hash_and_location.second.offset;
  }
  GELOGI("Saved %zu new weights to single file (reused: %zu), file size: %zu bytes", to_append.size(), reused_count,
         GetFileSizeOrZero(weight_path));
  return SUCCESS;
}

Status FileConstantUtils::WriteWeightWithPadding(std::ofstream &ofs, const uint8_t *weight_data_ptr, const size_t size,
                                                 size_t &offset) {
  // 写入权重数据
  (void)ofs.write(reinterpret_cast<const char *>(weight_data_ptr), static_cast<std::streamsize>(size));
  if (!ofs.good()) {
    GELOGE(FAILED, "Failed to write weight data to file");
    return FAILED;
  }

  // 写入填充数据以对齐512字节边界
  const size_t padding = MemSizeAlign(size, static_cast<uint32_t>(kCombinedWeightAlign)) - size;
  if (padding > 0) {
    static constexpr std::array<char, kCombinedWeightAlign> zeros = {};  // 预分配对齐粒度的零值缓冲区
    const size_t padding_blocks = padding / kCombinedWeightAlign;
    const size_t padding_remainder = padding % kCombinedWeightAlign;

    // 批量写入512字节块
    size_t i = 0;
    while (i < padding_blocks) {
      (void)ofs.write(zeros.data(), static_cast<std::streamsize>(kCombinedWeightAlign));
      ++i;
    }
    // 写入剩余字节
    if (padding_remainder > 0) {
      (void)ofs.write(zeros.data(), static_cast<std::streamsize>(padding_remainder));
    }

    if (!ofs.good()) {
      GELOGE(FAILED, "Failed to write padding data to file");
      return FAILED;
    }
  }

  // 更新偏移量（当前写入位置）
  if (offset > (std::numeric_limits<size_t>::max() - size - padding)) {
    GELOGE(FAILED, "Failed to update offset");
    return FAILED;
  }
  offset += size + padding;
  return SUCCESS;
}

// 位置表是写入 FileConstant 属性的唯一来源：先落盘并校验所有位置，全部成功后才应用节点转换，
// 避免磁盘失败后图中权重被清空；hash 无位置时报错而非抛异常
Status FileConstantUtils::ConvertToFileConstants(const ConstNodeWeightHashList &const_to_weight_hash_list,
                                                 const std::string &weight_dir, FileConstantMeta &meta,
                                                 const bool all_in_one, OneFileAppendContext &append_ctx) {
  std::map<std::string, WeightLocation> locations;
  if (all_in_one) {
    GE_CHK_STATUS_RET(SaveWeightToOneFileWithReuse(const_to_weight_hash_list, weight_dir, meta, locations, append_ctx),
                      "Failed to save weight to one file");
  } else {
    GE_CHK_STATUS_RET(SaveWeightToFileWithReuse(const_to_weight_hash_list, weight_dir, locations),
                      "Failed to save weight to file");
  }
  // 在任何 ClearData 前准备并校验全部节点转换参数（包括记录全部长度）
  struct NodeConversion {
    NodePtr node;
    const WeightLocation *location;
    int64_t length;
  };
  std::vector<NodeConversion> conversions;
  conversions.reserve(const_to_weight_hash_list.size());
  for (const auto &const_and_weight_hash : const_to_weight_hash_list) {
    const auto &const_node = const_and_weight_hash.first;
    const auto &weight = const_and_weight_hash.second.first;
    const auto &weight_hash = const_and_weight_hash.second.second;
    const auto location_iter = locations.find(weight_hash);
    GE_CHK_BOOL_RET_STATUS(location_iter != locations.end(), FAILED,
                           "No resolved location for weight hash[%s], node[%s]", weight_hash.c_str(),
                           (const_node == nullptr) ? "" : const_node->GetName().c_str());
    (void)conversions.emplace_back(
        NodeConversion{const_node, &location_iter->second, static_cast<int64_t>(weight->GetData().GetSize())});
  }
  // 落盘已成功，应用节点属性与类型，再清除内嵌权重
  for (size_t i = 0UL; i < conversions.size(); ++i) {
    const auto &conversion = conversions[i];
    const auto &weight_hash = const_to_weight_hash_list[i].second.second;
    const auto &const_node = conversion.node;
    auto const_op = const_node->GetOpDesc();
    const std::string new_name = const_node->GetName() + "_" + weight_hash;
    const_op->SetName(new_name);
    const_to_weight_hash_list[i].second.first->ClearData();
    OpDescUtilsEx::SetType(const_op, FILECONSTANT);
    (void)const_op->DelAttr(ATTR_NAME_WEIGHTS);
    SetFileConstantPath(const_op, conversion.location->path, static_cast<int64_t>(conversion.location->offset),
                        conversion.length);
    const auto &output_desc = const_op->GetOutputDesc(0U);
    (void)AttrUtils::SetDataType(const_op, kAttrDtype, output_desc.GetDataType());
    (void)AttrUtils::SetListInt(const_op, kAttrShape, output_desc.GetShape().GetDims());
    (void)AttrUtils::SetListInt(const_op, "original_shape", output_desc.GetOriginShape().GetDims());
    GELOGI("Convert node:%s from const to file constant success, save path[%s], offset[%zu]", new_name.c_str(),
           conversion.location->path.c_str(), conversion.location->offset);
  }
  return SUCCESS;
}

Status FileConstantUtils::ConvertConstToFileConst(const ComputeGraphPtr &compute_graph, bool all_in_one) {
  const auto &const_to_weight_hash_list = GetAllConstNodesAndWeightHash(compute_graph);
  if (const_to_weight_hash_list.empty()) {
    GELOGI("Cannot find valid const nodes on graph:%s, skip conversion", compute_graph->GetName().c_str());
    return SUCCESS;
  }
  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  GE_CHECK_NOTNULL(external_weight_manager);
  // 在 mutex 内按值取得路径快照，后续目录规范化、索引与临时文件都从该快照推导
  const std::string file_const_dir = external_weight_manager->GetWeightPath();
  GE_CHK_BOOL_RET_STATUS(!file_const_dir.empty(), FAILED, "file constant dir is empty.");
  GE_CHK_STATUS_RET(external_weight_manager->CreateWeightPath(), "Failed to create directory:%s.",
                    file_const_dir.c_str());
  std::string meta_path;
  GetValidFullPath(file_const_dir, kMetaFileName, meta_path);
  // 锁定独立的稳定锁文件而非 meta.json inode：meta.json 发布时会被原子替换，锁在其 inode 上无法约束后来者
  std::string lock_path;
  GetValidFullPath(file_const_dir, kMetaLockFileName, lock_path);
  int32_t fd = -1;
  bool is_file_locked = false;
  const ScopeGuard fd_guard([&fd, &is_file_locked]() {
    if (is_file_locked) {
      (void)flock(fd, LOCK_UN);
    }
    if (fd != -1) {
      (void)mmClose(fd);
      fd = -1;
    }
  });
  constexpr int32_t flags = M_RDWR | M_CREAT;
  constexpr MODE mode = static_cast<MODE>(M_UMASK_USRREAD | M_UMASK_USRWRITE);
  fd = mmOpen2(lock_path.c_str(), flags, mode);
  GE_CHK_BOOL_RET_STATUS(fd >= 0, FAILED, "open lock file[%s] failed, error=%d, error msg=%s.", lock_path.c_str(),
                         mmGetErrorCode(), GetErrorNumStr(mmGetErrorCode()).c_str());
  GE_CHK_STATUS_RET(FlockRetry(fd, LOCK_EX), "lock file[%s] failed", lock_path.c_str());
  is_file_locked = true;
  // 使用局部 meta 副本，失败时不污染 Session 内共享状态；磁盘提交成功后才写回
  FileConstantMeta meta;
  if (mmAccess(meta_path.c_str()) == EN_OK) {
    nlohmann::json meta_json;
    GE_CHK_STATUS_RET(ReadJsonFile(meta_path, meta_json), "read meta json failed.");
    meta = meta_json.get<FileConstantMeta>();
  } else {
    meta = external_weight_manager->MutableMetaFile();
  }

  OneFileAppendContext append_ctx;
  GE_CHK_STATUS_RET_NOLOG(
      ConvertToFileConstants(const_to_weight_hash_list, file_const_dir, meta, all_in_one, append_ctx));
  // 索引经同目录临时文件写入、关闭后原子替换；发布失败时回退本批追加的合并文件尾部
  const nlohmann::json out_json(meta);
  if (WriteJsonFileAtomic(meta_path, out_json) != SUCCESS) {
    GELOGE(FAILED, "save file constant meta failed.");
    if (append_ctx.appended) {
      GE_CHK_BOOL_RET_STATUS(RollbackCombinedFileAppend(append_ctx.weight_path, append_ctx.original_eof) == SUCCESS,
                             FAILED,
                             "Publish meta json failed and rollback combined file also failed, tail bytes "
                             "are left but not referenced by index");
    }
    return FAILED;
  }
  external_weight_manager->MutableMetaFile() = meta;
  return SUCCESS;
}

Status FileConstantUtils::ConvertConstToFileConst(const NodePtr &node) {
  GE_ASSERT_TRUE(NodeUtils::IsConst(*node), "%s[%s] is not fileconstant", node->GetName().c_str(),
                 node->GetType().c_str());
  const auto &weights = OpDescUtils::MutableWeights(node);
  if (weights.empty() || weights[0]->GetTensorDesc().GetShape().IsEmptyTensor()) {
    GELOGW("Node:%s weight is null or empty tensor", node->GetName().c_str());
    return SUCCESS;
  }
  const auto &tensor = weights[0];
  const auto &data = tensor->GetData().GetData();
  const auto size = tensor->GetData().GetSize();
  const auto &external_weight_manager = ExternalWeightManagerPool::Instance().GetManager(GetContext().SessionId());
  GE_CHECK_NOTNULL(external_weight_manager);
  const std::string weight_dir = external_weight_manager->GetWeightPath();
  GE_CHK_STATUS_RET(external_weight_manager->CreateWeightPath(), "Failed to create directory:%s.", weight_dir.c_str());
  std::string weight_path;
  GetValidFullPath(weight_dir, GetRegulatedName(node->GetName()), weight_path);
  GE_ASSERT_SUCCESS(FileSaver::SaveToFile(weight_path, data, size),
                    "Failed to save weight to file, node[%s], weight size:%zu", node->GetName().c_str(), size);
  tensor->ClearData();
  auto const_op = node->GetOpDesc();
  OpDescUtilsEx::SetType(const_op, FILECONSTANT);
  (void)const_op->DelAttr(ATTR_NAME_WEIGHTS);
  const auto &output_desc = const_op->GetOutputDesc(0U);
  SetFileConstantPath(const_op, weight_path, kDefaultOffset, static_cast<int64_t>(size));
  (void)AttrUtils::SetDataType(const_op, kAttrDtype, output_desc.GetDataType());
  (void)AttrUtils::SetListInt(const_op, kAttrShape, output_desc.GetShape().GetDims());
  (void)AttrUtils::SetListInt(const_op, "original_shape", output_desc.GetOriginShape().GetDims());
  GELOGI("Convert node:%s from const to file constant successfully.", node->GetName().c_str());
  return SUCCESS;
}

Status FileConstantUtils::ChangeFilePath(const ComputeGraphPtr &compute_graph, const std::string &om_path) {
  return ChangeFilePath(compute_graph, nullptr, om_path);
}

Status FileConstantUtils::ChangeFilePath(const ComputeGraphPtr &main_graph, const ComputeGraphPtr &fallback_graph,
                                         const std::string &om_path) {
  GE_ASSERT_NOTNULL(main_graph);
  std::map<std::string, std::string> old_file_to_new_file;
  GE_ASSERT_SUCCESS(ChangeFilePathAttr(main_graph, om_path, old_file_to_new_file),
                    "Change file path attribute for graph %s failed.", main_graph->GetName().c_str());
  if (fallback_graph != nullptr) {
    GE_ASSERT_SUCCESS(ChangeFilePathAttr(fallback_graph, om_path, old_file_to_new_file),
                      "Change file path attribute for fallback graph %s failed.", fallback_graph->GetName().c_str());
  }
  const Status move_ret = MoveFilePath(old_file_to_new_file);
  if (fallback_graph != nullptr) {
    GELOGI("dual-body FileConstant path change %s, moved file num=%zu.", (move_ret == SUCCESS) ? "success" : "failed",
           old_file_to_new_file.size());
  }
  return move_ret;
}

std::string FileConstantUtils::GetTmpWeightDir(const int32_t pid, const uint64_t session_id) {
  std::string dir = kTmpWeightDir + std::to_string(pid) + "_" + std::to_string(session_id) + "/";
  std::string ascend_work_path;
  (void)GetAscendWorkPath(ascend_work_path);
  if (!ascend_work_path.empty()) {
    dir = ascend_work_path + "/" + dir;
  }
  return dir;
}

void FileConstantUtils::SetFileConstantPath(const OpDescPtr &op_desc, const std::string &file_path,
                                            const int64_t offset, const int64_t length) {
  (void)AttrUtils::SetInt(op_desc, ATTR_NAME_OFFSET, offset);
  (void)AttrUtils::SetInt(op_desc, ATTR_NAME_LENGTH, length);
  (void)AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, file_path);
}

Status FileConstantUtils::ChangeFilePathAttr(const ComputeGraphPtr &compute_graph, const std::string &om_path,
                                             std::map<std::string, std::string> &old_file_to_new_file) {
  std::string origin_dir;
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (node->GetType() == FILECONSTANT) {
      const auto &op_desc = node->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      const auto &fileconstant_info = GetFileConstantInfo(op_desc);
      const auto &file_path = fileconstant_info.weight_path;
      const size_t offset = fileconstant_info.weight_offset;
      const size_t length = fileconstant_info.weight_length;
      if (file_path.empty()) {
        continue;
      }
      const std::string real_path = RealPath(file_path.c_str());
      GE_CHK_BOOL_RET_STATUS(!real_path.empty(), FAILED, "Failed to get real path of %s", file_path.c_str());
      auto pos = real_path.find(kTmpWeightDir);
      if (pos == std::string::npos) {
        continue;
      }
      if (origin_dir.empty()) {
        pos = real_path.find('/', pos);
        GE_CHK_BOOL_RET_STATUS(pos != std::string::npos, FAILED, "File path:%s is invalid.", real_path.c_str());
        origin_dir = real_path.substr(0, pos);
      }
      std::string file_name = StringUtils::GetFileName(real_path);
      GE_CHK_BOOL_RET_STATUS(!file_name.empty(), FAILED, "The file name is empty.");
      std::string path = om_path;
      const char_t *const om_dir = mmDirName(&path[0]);
      GE_CHECK_NOTNULL(om_dir);
      const std::string om_weight_path = std::string(om_dir) + "/weight/" + file_name;
      SetFileConstantPath(op_desc, file_name, static_cast<int64_t>(offset), static_cast<int64_t>(length));
      old_file_to_new_file[real_path] = om_weight_path;
      GELOGD("Node:%s changes file path attribute success.", node->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status FileConstantUtils::CollectFilePathChange(const ComputeGraphPtr &compute_graph, const std::string &om_path,
                                                std::map<std::string, std::string> &old_file_to_new_file) {
  GE_ASSERT_NOTNULL(compute_graph);
  return ChangeFilePathAttr(compute_graph, om_path, old_file_to_new_file);
}

Status FileConstantUtils::MoveExternalWeightFiles(const std::map<std::string, std::string> &old_file_to_new_file) {
  return MoveFilePath(old_file_to_new_file);
}

Status FileConstantUtils::MoveFilePath(const std::map<std::string, std::string> &old_file_to_new_file) {
  std::set<std::string> old_dirs;
  for (const auto &file_path : old_file_to_new_file) {
    const auto &old_path = file_path.first;
    const auto &new_path = file_path.second;
    GE_ASSERT_TRUE(!old_path.empty(), "Old fileconstant path cannot be null");
    GE_ASSERT_TRUE(!new_path.empty(), "New fileconstant path cannot be null");

    auto pos = old_path.rfind('/');
    GE_ASSERT_TRUE(pos != std::string::npos, "File path:%s is invalid.", old_path.c_str());
    const std::string origin_dir = old_path.substr(0, pos);
    (void)old_dirs.insert(origin_dir);
    pos = new_path.rfind('/');
    GE_ASSERT_TRUE(pos != std::string::npos, "File path:%s is invalid.", new_path.c_str());
    const std::string om_weight_dir = new_path.substr(0, pos);
    GE_ASSERT_TRUE(CreateDirectory(om_weight_dir) == 0, "Failed to create directory:%s.", om_weight_dir.c_str());
    GE_ASSERT_TRUE(std::rename(old_path.c_str(), new_path.c_str()) == 0, "Failed to change path from %s to %s.",
                   old_path.c_str(), new_path.c_str());
    GELOGD("Change file %s to %s success.", old_path.c_str(), new_path.c_str());
  }
  for (const auto &old_dir : old_dirs) {
    GE_ASSERT_TRUE(mmRmdir(old_dir.c_str()) == 0, "Failed to remove dir:%s.", old_dir.c_str());
  }
  return SUCCESS;
}

Status FileConstantUtils::RefreshRelativePath(const ComputeGraphPtr &compute_graph) {
  GE_CHECK_NOTNULL(compute_graph);
  for (const auto &node : compute_graph->GetAllNodes()) {
    if (node->GetType() == FILECONSTANT) {
      const auto &op_desc = node->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      const auto &fileconstant_info = GetFileConstantInfo(op_desc);
      if (!fileconstant_info.weight_path.empty()) {
        const std::string &file_name = StringUtils::GetFileName(fileconstant_info.weight_path);
        (void)AttrUtils::SetStr(op_desc, ATTR_NAME_LOCATION, file_name);
        GELOGI("Success to refresh relative file path:%s for fileconstant node:%s.", file_name.c_str(),
               node->GetName().c_str());
      }
    }
  }
  return SUCCESS;
}
}  // namespace ge
