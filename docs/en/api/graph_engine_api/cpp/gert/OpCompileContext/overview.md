# Overview

<!-- md-trans-meta sourceCommit=7519835ba162ed60f7faf4cd52d6c65ad2a468ff translatedAt=2026-11-17T02:44:13.993Z pushedAt=2026-11-18T02:41:08.880Z nl=2 -->

A context class for the operator compilation phase that inherits from ExtendedKernelContext. It provides capabilities required at compile time, including input tensor retrieval (supporting REQUIRED/OPTIONAL/DYNAMIC types), compilation configuration option query, and platform information retrieval, and is used to pass the compilation context to the Compile API of a custom operator.

## Header File to Include

```c++
#include <exe_graph/runtime/op_compile_context.h
```

## Public Member Functions

```c++
const Tensor *GetInputTensor(size_t index) const
const Tensor *GetRequiredInputTensor(size_t ir_index) const
const Tensor *GetOptionalInputTensor(size_t ir_index) const
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
ge::graphStatus GetOption(const ge::AscendString &option_key, ge::AscendString &option) const
ge::graphStatus GetPlatformInfos(fe::PlatFormInfos &platform_info, fe::OptionalInfos& optional_infos) const
bool GetContextValue(ContextParam key, uint64_t &value) const
```
