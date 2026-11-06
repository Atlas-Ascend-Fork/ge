/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_ADAPTER_H_
#define CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_ADAPTER_H_

#include <memory>
#include <string>
#include <vector>

#include "graph/custom_op/artifact_provider.h"
#include "graph/custom_op/cast.h"
#include "graph/custom_op/framework_hosted_op.h"
#include "graph/custom_op/infer_meta.h"
#include "runtime/custom_op/python_custom_op_bridge_types.h"

namespace ge {
namespace custom_op {
struct PythonCustomOpAdapterDescriptor {
  std::string op_type;
  std::string impl_descriptor_key;
  CustomOpCapabilityMask capabilities{0U};
  PythonCustomOpInferMetaFn infer_meta{nullptr};
  // 编译期冻结的 Python 部署 artifact（1~2 项），仅声明 portable 的实现类非空。
  // 共享同一定义文件的多个 adapter 复用同一 immutable 对象；分文件原型时额外带上
  // 承载 @register_op 声明的原型 artifact，否则 proto.py 不进 OM、加载后无 infer_func。
  std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>> deployment_artifacts;
};

class PythonCustomOpImplRuntimeRegistry {
 public:
  static PythonCustomOpImplRuntimeRegistry &GetInstance();

  static bool Register(const PythonCustomOpAdapterDescriptor &desc, const PythonCustomOpAdapterCallbacks &callbacks);
  static bool Unregister(const std::string &descriptor_key);
  static bool Acquire(const PythonCustomOpAdapterDescriptor &desc, PythonCustomOpAdapterCallbacks &callbacks);
  static void Release(const PythonCustomOpAdapterDescriptor &desc);
  static void Clear();

 private:
  PythonCustomOpImplRuntimeRegistry() = default;
  ~PythonCustomOpImplRuntimeRegistry() = default;
};

class PythonCustomOpImplHolder {
 public:
  explicit PythonCustomOpImplHolder(const PythonCustomOpAdapterDescriptor &desc);
  ~PythonCustomOpImplHolder();

  PythonCustomOpImplHolder(const PythonCustomOpImplHolder &) = delete;
  PythonCustomOpImplHolder &operator=(const PythonCustomOpImplHolder &) = delete;

  bool IsValid() const;
  void *GetHolder() const;
  const PythonCustomOpAdapterCallbacks &GetCallbacks() const;
  const PythonCustomOpAdapterDescriptor &GetDescriptor() const;

 private:
  PythonCustomOpAdapterDescriptor desc_;
  PythonCustomOpAdapterCallbacks callbacks_;
  void *holder_{nullptr};
  bool valid_{false};
};

class PythonCustomOpAdapter final : public EagerExecuteOp,
                                    public HostCpuExecuteOp,
                                    public AnnotatedArgsOp,
                                    public CompilableOp,
                                    public ShapeInferOp,
                                    public PortableOp,
                                    public ArgsUpdater,
                                    public CustomOpCapabilityProvider,
                                    public CustomOpInferMetaProvider,
                                    public FrameworkHostedOp,
                                    public CustomOpArtifactProvider {
 public:
  explicit PythonCustomOpAdapter(PythonCustomOpAdapterDescriptor desc);
  ~PythonCustomOpAdapter() override;

  bool IsValid() const;
  bool HasCapability(CustomOpCapability capability) const override;

  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override;
  graphStatus Execute(gert::HostCpuOpExecutionContext *ctx) override;
  graphStatus DeclareLaunchArgs(gert::AnnotatedArgsContext &ctx) override;
  graphStatus Compile(gert::OpCompileContext *ctx) override;
  graphStatus InferShape(gert::InferShapeContext *ctx) override;
  graphStatus InferDataType(gert::InferDataTypeContext *ctx) override;
  graphStatus Serialize(std::vector<uint8_t> &buffer) override;
  graphStatus Deserialize(const std::vector<uint8_t> &buffer) override;
  graphStatus UpdateHostArgs(gert::UpdateArgsContext *ctx) override;
  graphStatus InferMeta(gert::InferShapeContext *ctx, CustomOpInferMetaResult *result) override;
  std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>> GetDeploymentArtifacts() const override;

 private:
  graphStatus ReportUnsupported(CustomOpCapability capability, const char *method_name) const;

  std::string op_type_;
  std::string impl_descriptor_key_;
  CustomOpCapabilityMask capabilities_{0U};
  PythonCustomOpInferMetaFn infer_meta_{nullptr};
  std::vector<std::shared_ptr<const PythonCustomOpDeploymentArtifact>> deployment_artifacts_;
  std::unique_ptr<PythonCustomOpImplHolder> holder_;
};

void ClearPythonCustomOpRuntimeRegistry();
}  // namespace custom_op
}  // namespace ge

#endif  // CANN_GRAPH_ENGINE_RUNTIME_CUSTOM_OP_PYTHON_CUSTOM_OP_ADAPTER_H_
