# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:14:31.569Z pushedAt=2026-10-28T08:50:45.493Z nl=2 -->

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
```
