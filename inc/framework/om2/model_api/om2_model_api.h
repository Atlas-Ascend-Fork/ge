/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef OM2_MODEL_API_H_
#define OM2_MODEL_API_H_

#include <cstddef>
#include <cstdint>

#include "acl/acl_rt.h"

namespace gert {
class Tensor;
}  // namespace gert

/**
 * @brief OM2 算子单个输入或输出 Tensor 与 args buffer 地址槽位的映射信息。
 */
struct GertModelTaskIoEntry {
  uint64_t struct_size = sizeof(GertModelTaskIoEntry);  // 布局变化时更新
  gert::Tensor *tensor = nullptr;                       // 输入，Tensor 基础信息指针，不允许为空。
  uint64_t offset = 0;                                  // 输入，Tensor 地址在 args buffer 中的偏移，单位为字节。
};

/**
 * @brief L0 异常 dump 的 kernel 参数槽位类型。
 */
enum GertModelArgKind : uint64_t {
  GERT_MODEL_ARG_INPUT = 0,           // 输入 Tensor 地址槽位。
  GERT_MODEL_ARG_OUTPUT = 1,          // 输出 Tensor 地址槽位。
  GERT_MODEL_ARG_WORKSPACE = 2,       // Workspace 地址槽位。
  GERT_MODEL_ARG_TILING = 3,          // Tiling 数据地址槽位。
  GERT_MODEL_ARG_SHAPE_INFO = 4,      // Shape 信息地址槽位。
  GERT_MODEL_ARG_LEVEL1_DESC = 5,     // 一级描述信息地址槽位。
  GERT_MODEL_ARG_PLACEHOLDER = 6,     // 占位地址槽位。
  GERT_MODEL_ARG_CUSTOM_VALUE = 7,    // 自定义立即数槽位。
  GERT_MODEL_ARG_FFTS_ADDR = 8,       // FFTS 地址槽位。
  GERT_MODEL_ARG_EVENT_ADDR = 9,      // Event 地址槽位。
  GERT_MODEL_ARG_OVERFLOW_ADDR = 10,  // Overflow 地址槽位。
  GERT_MODEL_ARG_EMPTY_ADDR = 11,     // 空地址槽位。
  GERT_MODEL_ARG_INVALID_KIND = 0xFFFFU
};

/**
 * @brief L0 异常 dump 的单个 kernel 参数槽位原始信息。
 */
struct GertModelArgSlotInfo {
  uint64_t struct_size = sizeof(GertModelArgSlotInfo);  // 结构体版本号，布局变化时更新
  GertModelArgKind kind = GERT_MODEL_ARG_INVALID_KIND;  // 输入，槽位类型，取值见 GertModelArgKind。
  uint64_t flags = 0;                                   // 输入，预留标志位，当前填 0。
  uint64_t args_offset = 0;                             // 输入，槽位在 args buffer 中的偏移，单位为字节。
  uint64_t value = 0;                 // 输入，槽位附加值。Tiling 或 Shape 信息场景表示数据大小，其他场景按 kind 解释。
  uint64_t related_index = 0;         // 输入，关联索引。输入、输出或 Workspace 场景表示对应数组下标。
  uint64_t event_id = 0;              // 输入，Event 场景的事件 ID，非 Event 场景填 0。
  uint64_t level1_target_offset = 0;  // 输入，一级描述信息指向的目标槽位偏移，单位为字节。
};

/**
 * @brief L0 异常 dump 的 kernel 参数原始信息列表。
 */
struct GertModelTaskRawInfo {
  uint64_t struct_size = sizeof(GertModelTaskRawInfo);  // 结构体版本号，布局变化时更新
  uint64_t need_assert_or_printf = 0;  // 输入，是否需要 assert 或 printf 相关异常 dump，0 表示不需要，非 0 表示需要。
  uint64_t arg_num = 0;                // 输入，args 指向的槽位个数。
  const GertModelArgSlotInfo *args = nullptr;  // 输入，槽位信息数组首地址。arg_num 为 0 时可以为空指针。
};

/**
 * @brief OM2 算子任务 dump 信息。
 */
struct GertModelTaskDesc {
  uint64_t struct_size = sizeof(GertModelTaskDesc);  // 结构体版本号，布局变化时更新
  const char *op_name = nullptr;                     // 输入，算子名称，不允许为空。
  const char *op_type = nullptr;                     // 输入，算子类型，不允许为空。
  uint64_t task_id = 0;                              // 输入，运行时 task ID。
  uint64_t stream_id = 0;                            // 输入，运行时 stream ID。
  uint64_t context_id = 0;                           // 输入，运行时 context ID。
  uint64_t thread_id = 0;                            // 输入，运行时 thread ID。
  uint64_t block_dim = 0;                            // 输入，Kernel launch block dim，非 Kernel 任务填 0。
  uint64_t op_desc_id = 0;                           // 输入，OpDesc ID。
  uintptr_t args_base = 0;                           // 输入，args buffer 基地址。
  uint64_t args_size = 0;                            // 输入，args buffer 大小，单位为字节。
  uint64_t input_num = 0;                            // 输入，inputs 指向的输入 Tensor 个数。
  const GertModelTaskIoEntry *inputs = nullptr;   // 输入，输入 Tensor 映射数组首地址。input_num 为 0 时可以为空指针。
  uint64_t output_num = 0;                        // 输入，outputs 指向的输出 Tensor 个数。
  const GertModelTaskIoEntry *outputs = nullptr;  // 输入，输出 Tensor 映射数组首地址。output_num 为 0 时可以为空指针。
  uint64_t workspace_num = 0;                     // 输入，Workspace 个数。
  const uint64_t *workspace_addrs = nullptr;      // 输入，Workspace 地址数组首地址。workspace_num 为 0 时可以为空指针。
  const uint64_t *workspace_sizes =
      nullptr;                    // 输入，Workspace 大小数组首地址，单位为字节。workspace_num 为 0 时可以为空指针。
  uint64_t task_type = 0;         // 输入，任务类型，取值与 ModelTaskType 保持一致。
  uint64_t kernel_type = 10000U;  // 输入，kernel 类型，取值与 ge::ccKernelType 保持一致，默认 INVALID。
  void *stream = nullptr;         // 输入，rtStream_t 运行时流句柄。
  uint64_t is_raw_address = 0;    // 输入，是否为 raw address 模式，0 表示否，非 0 表示是。
  const GertModelTaskRawInfo *task_raw_info =
      nullptr;                              // 输入，L0 异常 dump 原始信息指针。不需要 L0 异常 dump 时可以为空指针。
  uint64_t launch_begin = 0;                // 输入，kernel launch 开始时间戳。
  const char *original_op_names = nullptr;  // 输入，分号分隔的原始算子名称列表。非融合算子为 nullptr。
  uint64_t input_mem_size = 0;              // 输入，输入内存大小，单位为字节。
  uint64_t output_mem_size = 0;             // 输入，输出内存大小，单位为字节。
  uint64_t workspace_mem_size = 0;          // 输入，workspace 内存大小，单位为字节。
  uint64_t weight_mem_size = 0;             // 输入，权重内存大小，单位为字节。
  uint64_t op_impl_mode = 0;  // 输入，算子实现模式，取值与 ge 的 _op_impl_mode_enum 属性一致，0 表示默认模式。
  uint64_t prof_ge_task_type =
      0;  // 输入，profiling 上报任务类型，取值与 MsprofGeTaskType(aprof_pub.h)一致，0 表示 AI_CORE。
  uint64_t prof_block_dim =
      0;  // 输入，profiling 上报口径的 Block Dim(未归一/mix 编码/tiling_sink 占位)，与 block_dim(launch 口径)分离。
};

// GertModelBaseInfo: report_model_base_info 回调入参（codegen → executor 传递 rt_model_handle）
struct GertModelBaseInfo {
  uint64_t struct_size = sizeof(GertModelBaseInfo);  // 布局变化时更新
  void *rt_model_handle = nullptr;                   // 输入：codegen 创建的 aclmdlRI*（InitResources 后即可获得）
};

using ReportModelBaseInfoFunc = int32_t (*)(void *instance_handle, const GertModelBaseInfo *info);

enum GertModelTaskLaunchType : uint64_t {
  ACL_RT_LAUNCH_KERNEL_V2 = 0,         // 通过 aclrtLaunchKernelV2 下发。
  RT_STARS_TASK_LAUNCH_WITH_FLAG = 1,  // 通过 rtStarsTaskLaunchWithFlag 下发。
  ACL_RT_LAUNCH_CUSTOM_KERNEL = 2,     // 通过 自定义算子的回调函数 下发。
  ACL_RT_LAUNCH_HCCL = 3,              // 通过 Executor 侧 HCCL 适配层下发（静态离线通信任务）。
};

/**
 * @brief 普通 Kernel 下发参数。
 */
struct GertModelLaunchKernelV2Params {
  uint64_t struct_size = sizeof(GertModelLaunchKernelV2Params);  // 布局变化时更新。
  aclrtFuncHandle func_handle = nullptr;                         // 输入，待下发的 Kernel 函数。
  uint32_t block_dim = 0;                                        // 输入，Kernel block dim。
  uint32_t abi_pad_1 = 0;                                        // ABI 填充，保持与 ACL 接口布局一致。
  const void *args_data = nullptr;                               // 输入，Kernel 参数地址。
  size_t args_size = 0;                                          // 输入，Kernel 参数大小，单位为字节。
  aclrtLaunchKernelCfg *config = nullptr;                        // 输入，Kernel 下发配置。
  aclrtStream stream = nullptr;                                  // 输入，下发流。
};

/**
 * @brief Stars Task 下发参数。
 */
struct GertModelLaunchStarsTaskWithFlagParams {
  uint64_t struct_size = sizeof(GertModelLaunchStarsTaskWithFlagParams);  // 布局变化时更新。
  const void *task_sqe = nullptr;                                         // 输入，Task SQE 地址。
  uint32_t sqe_len = 0;                                                   // 输入，SQE 长度，单位为字节。
  uint32_t abi_pad_1 = 0;                                                 // ABI 填充，保持与 ACL 接口布局一致。
  aclrtStream stream = nullptr;                                           // 输入，下发流。
  uint32_t flag = 0;                                                      // 输入，下发标志。
  uint32_t abi_pad_2 = 0;                                                 // ABI 填充，保持与 ACL 接口布局一致。
};

/**
 * @brief 自定义算子下发参数。
 */
struct GertModelLaunchCustomKernelParams {
  uint64_t struct_size = sizeof(GertModelLaunchCustomKernelParams);  // 布局变化时更新。
  uint32_t (*func_launch_custom_kernel)(void * /* eager op */,
                                        void * /* eager op context */) = nullptr;  // 输入，自定义算子下发回调函数
  void *eager_op = nullptr;                                                        // 输入，自定义算子op
  void *eager_op_context = nullptr;                                                // 输入，自定义算子op执行入参
};

/**
 * @brief HVD（Horovod）通信算子每输入元数据。
 */
struct HcclHorovodMeta {
  uint64_t struct_size = sizeof(HcclHorovodMeta);  // 布局变化时更新。
  const char *input_name = nullptr;  // 输入，TF tensor 名（op_desc->GetInputNameByIndex(i)，回调对接用）。
  const int64_t *dims = nullptr;     // 输入，该输入维度数组（SO 内静态字节）。
  uint32_t dims_num = 0U;            // 输入，维度个数。
  uint32_t abi_pad_1 = 0U;           // ABI 填充。
  int32_t hccl_data_type = 0;        // 输入，该输入 HCCL 数据类型枚举数值（编译期转换，每输入独立）。
  uint32_t abi_pad_2 = 0U;           // ABI 填充。
  uint64_t count = 0U;               // 输入，该输入通信元素数（编译期按 GetHorovodCount 规则）。
};

/**
 * @brief HCCL 通信任务扁平参数。
 */
struct HcclTaskParams {
  uint64_t struct_size = sizeof(HcclTaskParams);  // 布局变化时更新。
  const char *hccl_type = nullptr;                // 输入，通信算子类型字符串（KernelHcclDef.hccl_type）。
  const void *private_def = nullptr;              // 输入，HCCL 私有定义（编译期静态字节）。
  uint32_t private_def_len = 0U;                  // 输入，私有定义长度，单位为字节。
  uint32_t abi_pad_1 = 0U;                        // ABI 填充。
  void *main_stream = nullptr;                    // 输入，主流句柄。
  void *const *input_addrs = nullptr;             // 输入，输入地址数组（录制期解析）。
  uint32_t input_num = 0U;                        // 输入，输入地址数量。
  uint32_t abi_pad_2 = 0U;                        // ABI 填充。
  void *const *output_addrs = nullptr;            // 输入，输出地址数组（录制期解析）。
  uint32_t output_num = 0U;                       // 输入，输出地址数量。
  uint32_t abi_pad_3 = 0U;                        // ABI 填充。
  void *workspace_addr = nullptr;                 // 输入，workspace 地址，可为空。
  uint64_t workspace_size = 0U;                   // 输入，workspace 字节数。
  int32_t hccl_data_type = 0;                     // 输入，HCCL 数据类型枚举数值（编译期转换）。
  int32_t hccl_reduce_op = 0;                     // 输入，HCCL 归约类型枚举数值（编译期转换）。
  uint64_t count = 0U;                            // 输入，通信元素数。
  int32_t root_id = 0;                            // 输入，Broadcast/Gather/Reduce root rank。
  uint32_t abi_pad_4 = 0U;                        // ABI 填充。
  void *const *follow_streams = nullptr;          // 输入，follow 从流句柄数组（InitResources 按主流预创建）。
  uint32_t follow_stream_num = 0U;                // 输入，follow 从流数量。
  uint32_t abi_pad_5 = 0U;                        // ABI 填充。
  void *const *attached_streams = nullptr;        // 输入，attached 从流句柄数组，可为空。
  uint32_t attached_stream_num = 0U;              // 输入，attached 从流数量。
  uint8_t need_refresh = 0U;                      // 输入，是否地址刷新模式（args 槽位间接寻址）。
  uint8_t abi_pad_6[3] = {0U};                    // ABI 填充。
  void *args_base = nullptr;                      // 输入，args 基址（need_refresh 间接寻址模式，可为空）。
  uint32_t abi_pad_7 = 0U;                        // ABI 填充。
  uint32_t abi_pad_8 = 0U;                        // ABI 填充（尾部显式对齐）。
  // ---- HVD / 零拷贝 / 溢出扩展（尾部追加，多 info/零拷贝/溢出协议）----
  const HcclHorovodMeta *hvd_metas = nullptr;  // 输入，非空 = HVD 多 info 模式（每输入一条 meta）；HCOM 为 nullptr。
  uint8_t hvd_wait = 0U;                       // 输入，HVDWAIT：仅 1 条 info（只记 hccl_type，无 IO）。
  uint8_t is_collective = 0U;   // 输入，LoadTask 成功后缓存通信任务扩展信息（aclrtCacheLastTaskExtendInfo）。
  uint8_t abi_pad_9[6] = {0U};  // ABI 填充。
  const int32_t *input_zero_copy_flags = nullptr;   // 输入，每输入零拷贝标志（槽位化；长度 = input_num）。
  const int32_t *output_zero_copy_flags = nullptr;  // 输入，每输出零拷贝标志（长度 = output_num）。
  void *overflow_addr = nullptr;                    // 输入，溢出检测地址（op 含 GLOBALWORKSPACE_TYPE 时非空，L3）。
};

/**
 * @brief Executor 提供给模型 SO 的 HCCL 插件宿主服务函数。
 */
using HcclLoadTaskFunc = int32_t (*)(void *instance_handle, const HcclTaskParams *params);

/**
 * @brief HCCL 通信任务下发参数。
 */
struct GertModelLaunchHcclParams {
  uint64_t struct_size = sizeof(GertModelLaunchHcclParams);  // 布局变化时更新。
  HcclLoadTaskFunc func_load_hccl = nullptr;                 // 输入，任务下发函数（宿主 hccl_load_task）。
  const HcclTaskParams *task_params = nullptr;               // 输入，扁平任务参数。
};

/**
 * @brief 任务下发参数联合体，根据 GertModelTaskLaunchInfo::launch_type 选择成员。
 */
union GertModelTaskLaunchParams {
  GertModelLaunchKernelV2Params launch_kernel_v2_params;
  GertModelLaunchStarsTaskWithFlagParams launch_stars_task_params;
  GertModelLaunchCustomKernelParams launch_custom_kernel_params;
  GertModelLaunchHcclParams launch_hccl_params;
};

/**
 * @brief 运行时任务下发回调入参。
 */
struct GertModelTaskLaunchInfo {
  uint64_t struct_size = sizeof(GertModelTaskLaunchInfo);         // 布局变化时更新。
  GertModelTaskLaunchType launch_type = ACL_RT_LAUNCH_KERNEL_V2;  // 输入，下发类型。
  GertModelTaskDesc *task_info = nullptr;                         // 输入，任务 DFX 信息；调用方可补充运行时 task ID。
  const GertModelTaskLaunchParams *launch_params = nullptr;       // 输入，与下发类型对应的参数。
};

using GertModelLaunchFunc = int32_t (*)(void *instance_handle, GertModelTaskLaunchInfo *launch_info);
using LockBinHandleStoreFunc = int32_t (*)();
using UnlockBinHandleStoreFunc = int32_t (*)();
using QueryBinHandleFromStoreFunc = int32_t (*)(const char *bin_id, aclrtBinHandle *bin_handle);
using SaveBinHandleToStoreFunc = int32_t (*)(const char *bin_id, aclrtBinHandle bin_handle);
using ReleaseBinHandleFromStoreFunc = int32_t (*)(const char *bin_id, uint8_t *need_unload);

/**
 * @brief 模型加载期由执行器提供给模型 SO 的回调集合。
 * @note 变量相关的回调不在此处，见 GertModelVarContext。
 */
struct GertModelLoadCallbacks {
  uint64_t struct_size = sizeof(GertModelLoadCallbacks);                  // 布局变化时更新。
  ReportModelBaseInfoFunc report_model_base_info = nullptr;               // 上报模型运行时句柄。
  GertModelLaunchFunc launch_func = nullptr;                              // 下发单个任务。
  LockBinHandleStoreFunc lock_bin_handle_store = nullptr;                 // 锁定二进制句柄缓存。
  UnlockBinHandleStoreFunc unlock_bin_handle_store = nullptr;             // 解锁二进制句柄缓存。
  QueryBinHandleFromStoreFunc query_bin_handle_from_store = nullptr;      // 查询缓存。
  SaveBinHandleToStoreFunc save_bin_handle_to_store = nullptr;            // 写入缓存。
  ReleaseBinHandleFromStoreFunc release_bin_handle_from_store = nullptr;  // 释放缓存。
  HcclLoadTaskFunc hccl_load_task = nullptr;  // 下发 HCCL 任务（store 获取内化 + Prepare+LoadTask）。
};

using ReportModelRunFunc = int32_t (*)(void *instance_handle, const struct GertModelRunReportInfo *info);

/**
 * @brief 模型执行前后 DFX 回调入参。
 */
struct GertModelRunReportInfo {
  uint64_t struct_size = sizeof(GertModelRunReportInfo);  // 布局变化时更新。
  uint64_t model_id = 0;                                  // 输入，模型 ID。
  aclrtStream stream = nullptr;                           // 输入，执行流。
  uint64_t is_async = 0;                                  // 输入，是否异步执行。
};

/**
 * @brief 模型执行期 DFX 回调集合。
 */
struct GertModelRunCallbacks {
  uint64_t struct_size = sizeof(GertModelRunCallbacks);      // 布局变化时更新。
  ReportModelRunFunc report_run_info_preprocess = nullptr;   // 执行前回调。
  ReportModelRunFunc report_run_info_postprocess = nullptr;  // 执行后回调。
};

/**
 * @brief 变量地址解析模式。数值一经发布不可改变，新增值只能追加。
 */
enum GertVarAddrMode : uint32_t {
  GERT_VAR_ADDR_RESOLVE = 0,     // 解析或分配地址，可选写入首次使用的初始数据。
  GERT_VAR_ADDR_QUERY_ONLY = 1,  // 只查询容器内既有地址，不分配、不写入。
};

/**
 * @brief 变量地址解析的请求与应答。
 * @note 由模型 SO 填写输入字段，容器填写输出字段。容器只在调用期间读取 var_key 与
 *       host_data，不持有其所有权。
 */
struct GertVarDesc {
  uint64_t struct_size = sizeof(GertVarDesc);  // 布局变化时更新。
  const char *var_key = nullptr;               // 输入，变量键，UTF-8、以 \0 结束。
  uint64_t size = 0;                           // 输入，变量字节数，用于分配，必须大于 0；
                                               // QUERY_ONLY 模式下为输出，由容器回填。
  uint64_t memory_type = 0;                    // 输入，分配策略，取值同 RT_MEMORY_HBM 等内存类型常量。
  uint64_t logic_addr = 0;                     // 输入，编译期逻辑地址原值，用于内存区分支判定。
  const void *host_data = nullptr;             // 输入，首次使用的初始数据；为空表示只解析地址。
  uint64_t host_data_size = 0;                 // 输入，host_data 字节数；非空时必须等于 size。
  uint32_t device_id = 0;                      // 输入，变量所在 device。
  uint32_t mode = GERT_VAR_ADDR_RESOLVE;       // 输入，取值见 GertVarAddrMode。
  void *dev_addr = nullptr;                    // 输出，该变量在此设备上的地址；
                                               // QUERY_ONLY 未命中时为 nullptr。
  uint32_t is_loaded = 0;                      // 输出，该变量在此设备上是否已有有效数据。
  uint32_t abi_pad_0 = 0;                      // 显式填充，无业务含义。
};

/**
 * @brief 解析变量设备地址，并可选写入该变量首次使用时期的初始数据。
 * @param[in]     var_manager_handle 会话级变量容器句柄。
 * @param[in,out] desc 请求与应答。
 * @return 0 表示调用成功，含 QUERY_ONLY 未命中（此时 desc->dev_addr 为 nullptr）；
 *         非 0 表示入参非法或内部失败。不跨边界抛异常。
 * @note 未命中用出参 nullptr 表达而非独立返回码，与 QueryBinHandleFromStoreFunc 同一约定。
 * @note mode 为 GERT_VAR_ADDR_RESOLVE 时：host_data 非空且该设备尚未加载则写入并置
 *       is_loaded = 1，已加载则不重复写入；host_data 为空则只分配、不写入。
 * @note mode 为 GERT_VAR_ADDR_QUERY_ONLY 时：返回容器内既有地址、加载状态与该变量的字节数，
 *       不分配、不写入；此时 memory_type 与 logic_addr 被忽略，host_data 必须为空。
 *       回填 size 是必需的，调用方随后要对该地址做设备到主机拷贝，而它没有源条目的元数据。
 * @note 在调用方线程上同步执行，使用调用方当前的 aclrt 上下文；不切换上下文、不创建线程、
 *       不跨线程转交任务。
 * @note 变量设备内存的分配与释放全部由容器负责，调用方不得释放 dev_addr，也不得向其写入数据。
 * @note 线程安全：可被多个线程并发调用，包括不同变量之间以及同一变量的重复调用。
 */
using GetVarAddrFunc = int32_t (*)(void *var_manager_handle, struct GertVarDesc *desc);

/**
 * @brief 提交派生数据（格式转换或变量间拷贝的结果）的请求与应答。
 * @note 由模型 SO 填写输入字段，容器填写输出字段。容器只在调用期间读取 var_key 与
 *       host_data，不持有其所有权。
 */
struct GertVarCommit {
  uint64_t struct_size = sizeof(GertVarCommit);  // 布局变化时更新。
  const char *var_key = nullptr;                 // 输入，目标变量键，UTF-8、以 \0 结束。
  const void *host_data = nullptr;               // 输入，派生数据，不可为空。
  uint64_t host_data_size = 0;                   // 输入，字节数，不得超过容器记下的变量字节数。
  uint32_t device_id = 0;                        // 输入，变量所在 device。
  uint32_t is_loaded = 0;                        // 输出，返回时该变量在此设备上是否已有有效数据。
  void *dev_addr = nullptr;                      // 输出，返回时该变量在此设备上的地址。
};

/**
 * @brief 把调用方准备好的派生数据写入变量设备内存，并置加载状态。
 * @param[in]     var_manager_handle 会话级变量容器句柄。
 * @param[in,out] commit 请求与应答。
 * @return 0 表示成功，含"已加载故幂等跳过"；非 0 表示失败。不跨边界抛异常。
 * @note 不分配内存：该变量必须已由 GetVarAddrFunc 解析出地址，否则返回错误。
 * @note 该设备 is_loaded 已为 1 时不写入，幂等丢弃本次数据，返回既有地址与 is_loaded = 1，
 *       不视为错误。
 * @note host_data_size 超过容器记下的变量字节数时返回错误、不写入。
 * @note 执行线程、aclrt 上下文、内存归属与线程安全语义同 GetVarAddrFunc。
 */
using CommitVarDataFunc = int32_t (*)(void *var_manager_handle, struct GertVarCommit *commit);

/**
 * @brief 变量容器句柄、变量初始数据通道与变量回调集合。
 * @note 变量相关的全部边界内容集中于此。GertModelLoadCallbacks 保持不变，
 *       变量回调不追加到那里。
 */
struct GertModelVarContext {
  uint64_t struct_size = sizeof(GertModelVarContext);  // 布局变化时更新。
  void *var_manager_handle = nullptr;                  // 输入，会话级变量容器句柄。
  uint32_t device_id = 0;                              // 输入，变量所在 device。
  uint8_t abi_pad_0[4] = {0};                          // 显式填充，无业务含义。
  const void *var_weight_data = nullptr;               // 输入，变量初始数据块基址，可为空。
  uint64_t var_weight_data_size = 0;                   // 输入，数据块字节数。
  GetVarAddrFunc get_var_addr = nullptr;               // 输入，解析变量地址。
  CommitVarDataFunc commit_var_data = nullptr;         // 输入，提交派生数据。
};

/**
 * @brief GertModelLoad 的加载参数。
 */
struct GertModelLoadConfig {
  uint64_t struct_size = sizeof(GertModelLoadConfig);  // 布局变化时更新。
  const char **bin_files = nullptr;                    // 输入，二进制文件路径数组。
  const void **bin_data = nullptr;                     // 输入，二进制内存地址数组。
  uint64_t *bin_size = nullptr;                        // 输入，二进制大小数组，单位为字节。
  uint64_t bin_num = 0;                                // 输入，二进制个数。
  void **constants = nullptr;                          // 输入，常量地址数组。
  void *var_context = nullptr;                         // 输入，变量容器上下文，实际指向 GertModelVarContext。
  void *work_ptr = nullptr;                            // 输入，工作空间地址。
  uint64_t *session_id = nullptr;                      // 输入，Session ID。
  uint64_t model_id = 0;                               // 输入，用于日志和 DFX 的模型 ID。
  void *instance_handle = nullptr;                     // 输入，执行器实例句柄。
  const GertModelLoadCallbacks *callbacks = nullptr;   // 输入，加载期回调集合。
  int64_t priority = 0;                                // 输入，模型优先级。
  uint64_t reuse_zero_copy = 0;                        // 输入，是否复用零拷贝资源。
  aclmdlRI external_rt_model = nullptr;                // 输入，外部运行时模型句柄。
  aclrtStream *external_streams = nullptr;             // 输入，外部流数组。
  uint64_t external_stream_num = 0;                    // 输入，外部流个数。
  aclrtEvent *external_events = nullptr;               // 输入，外部 Event 数组。
  uint64_t external_event_num = 0;                     // 输入，外部 Event 个数。
  aclrtLabel *external_labels = nullptr;               // 输入，外部 Label 数组。
  uint64_t external_label_num = 0;                     // 输入，外部 Label 个数。
  aclrtNotify *external_notifies = nullptr;            // 输入，外部 Notify 数组。
  uint64_t external_notify_num = 0;                    // 输入，外部 Notify 个数。
  // 可选尾字段：借用的 gert::Allocators*；旧尺寸或 nullptr 表示未提供。
  // 集合及 allocator 须存活至关联流任务完成、相关内存块释放；不转移所有权。
  void *external_allocator = nullptr;
};

/**
 * @brief GertModelRun 和 GertModelRunAsync 的执行参数。
 */
struct GertModelRunConfig {
  uint64_t struct_size = sizeof(GertModelRunConfig);     // 布局变化时更新。
  uint64_t input_count = 0;                              // 输入，模型输入个数。
  gert::Tensor **input_data = nullptr;                   // 输入，模型输入 Tensor 数组。
  uint64_t output_count = 0;                             // 输入，模型输出个数。
  gert::Tensor **output_data = nullptr;                  // 输入，模型输出 Tensor 数组。
  uint64_t stream_sync_timeout_ms = 0;                   // 输入，同步执行的流同步超时，单位为毫秒。
  const GertModelRunCallbacks *run_callbacks = nullptr;  // 输入，执行期 DFX 回调集合。
  // 借用的 gert::Allocators*；nullptr 沿用 Load 的 allocator 集合，不转移所有权。
  // 调用方保证集合及 allocator 在关联流任务完成、相关内存块释放前有效。
  void *external_allocator = nullptr;
};

/** @brief GertModelUnload 的预留参数。 */
struct GertModelUnloadConfig {
  uint64_t struct_size = sizeof(GertModelUnloadConfig);
};

/** @brief GertModelLoad 的预留输出。 */
struct GertModelLoadOutput {
  uint64_t struct_size = sizeof(GertModelLoadOutput);
};

/** @brief GertModelRun 和 GertModelRunAsync 的预留输出。 */
struct GertModelRunOutput {
  uint64_t struct_size = sizeof(GertModelRunOutput);
};

/** @brief GertModelUnload 的预留输出。 */
struct GertModelUnloadOutput {
  uint64_t struct_size = sizeof(GertModelUnloadOutput);
};

#ifdef __cplusplus
extern "C" {
#endif

typedef void *GertModelHandle;

int32_t GertModelLoad(const GertModelLoadConfig *config, GertModelHandle *model_handle, GertModelLoadOutput *output);
int32_t GertModelRunAsync(GertModelHandle model_handle, aclrtStream stream, const GertModelRunConfig *config,
                          GertModelRunOutput *output);
int32_t GertModelRun(GertModelHandle model_handle, const GertModelRunConfig *config, GertModelRunOutput *output);
int32_t GertModelUnload(GertModelHandle model_handle, const GertModelUnloadConfig *config,
                        GertModelUnloadOutput *output);
uint64_t GertModelGetStreamNum();
int32_t GertModelGetStreamDesc(uint32_t *stream_flags, uint64_t stream_num, void *extended_attrs);
uint64_t GertModelGetEventNum();
int32_t GertModelGetEventDesc(uint32_t *event_flags, uint64_t event_num, void *extended_attrs);
uint64_t GertModelGetLabelNum();
uint64_t GertModelGetNotifyNum();
int32_t GertModelGetNotifyDesc(uint64_t *notify_flags, uint64_t notify_num, void *extended_attrs);
int32_t GertModelRefreshFeatureMap(GertModelHandle model_handle, uintptr_t base_addr);

#ifdef __cplusplus
}
#endif

/** @brief GertModelGetStreamDesc 的预留参数  */
struct GertModelStreamDesc {
  uint64_t struct_size = sizeof(GertModelStreamDesc);  // 输入，结构体版本号，布局变化时更新。
  GertModelHandle model_handle = nullptr;              // 输入，GertModelLoad 成功返回、Unload 前的有效句柄。
  aclrtStream *streams = nullptr;  // 输出，SO 回填，指向模型内部流数组，只读，Load 后至 Unload 前有效。
  uint64_t stream_num;             // 输出，SO 回填的实际流个数；旧版本 SO 不回填，保持调用前值。
};

#endif  // OM2_MODEL_API_H_
