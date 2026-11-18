/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef AIR_CXX_RUNTIME_OM2_OM2_ARGS_HANDLER_H_
#define AIR_CXX_RUNTIME_OM2_OM2_ARGS_HANDLER_H_

#include <cstddef>
#include <cstdint>
#include <deque>

#include "framework/om2/model_api/om2_model_api.h"
#include "framework/runtime/args_handler.h"

namespace ge {
/**
 * @brief 自定义算子 Callback 模式的参数服务 Handler。
 *
 * 算子在 Execute 期经 MallocReadOnlyDevArgs 注册私有参数，框架在每轮地址刷新周期改写 host 侧字节，
 * 并由模型 SO 统一上传到已下沉任务引用的 device 位置。两端登记表用 deque 承载：push_back
 * 不失效已有元素指针，故可安全把元素地址交给算子长期持有。
 *
 * @note 本头文件属 GE 运行库内部接口，不进入模型 SO 的包含路径。SO 只经不透明 void * 句柄
 *       与 gert::ArgsHandler 基类接口访问。
 */
class Om2ArgsHandler final : public gert::ArgsHandler {
 public:
  Om2ArgsHandler(void *args_table, GertOm2ArgsAllocFunc allocate_args_buffer)
      : args_table_(args_table), allocate_args_buffer_(allocate_args_buffer) {}
  ~Om2ArgsHandler() override = default;

  Om2ArgsHandler(const Om2ArgsHandler &) = delete;
  Om2ArgsHandler &operator=(const Om2ArgsHandler &) = delete;

  const gert::KernelArgs *MallocReadOnlyDevArgs(void *host_args, size_t args_size) override;
  const std::deque<gert::KernelArgs> &GetKernelArgs(gert::Placement placement) const override;

 private:
  void *args_table_{nullptr};                           // SO 参数池对象，GE 不解释其内容，仅原样回传
  GertOm2ArgsAllocFunc allocate_args_buffer_{nullptr};  // SO 侧分配回调
  std::deque<gert::KernelArgs> host_kernel_args_;       // 与 device 端按注册序一一对应
  std::deque<gert::KernelArgs> device_kernel_args_;
};
}  // namespace ge

extern "C" {
/**
 * @brief 创建 Om2ArgsHandler，输出 gert::ArgsHandler 基类地址形式的不透明句柄。
 * @return ACL_SUCCESS 成功；入参非法或分配失败返回非 0，此时 *handler 保持 nullptr。
 */
int32_t GertOm2ArgsHandlerCreate(void *args_table, GertOm2ArgsAllocFunc allocate_args_buffer, void **handler);

/**
 * @brief 销毁 Handler 与其两端登记表；不释放 SO 参数池中已划出的字节。
 * @note handler 为 nullptr 时是空操作。
 */
void GertOm2ArgsHandlerDestroy(void *handler);
}

#endif  // AIR_CXX_RUNTIME_OM2_OM2_ARGS_HANDLER_H_
