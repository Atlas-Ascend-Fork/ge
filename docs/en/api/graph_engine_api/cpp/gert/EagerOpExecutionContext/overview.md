# Overview

<!-- md-trans-meta sourceCommit=7519835ba162ed60f7faf4cd52d6c65ad2a468ff translatedAt=2026-11-17T02:42:53.700Z pushedAt=2026-11-18T02:41:08.874Z nl=2 -->

Context for executing operators.

Provides runtime support such as input/output management, memory allocation, and stream control for operators running on devices.

## Header File to Include

```c++
#include <exe_graph/runtime/eager_op_execution_context.h>
```

## Public Member Functions

```c++
rtStream GetStream() const
const Tensor *GetInputTensor(size_t index) const
const Tensor *GetRequiredInputTensor(size_t ir_index) const
const Tensor *GetOptionalInputTensor(size_t ir_index) const
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
Tensor *MallocOutputTensor(size_t index, const StorageShape &shape, const StorageFormat &format, ge::DataType dtype)
Tensor *MakeOutputRefInput(size_t output_index, size_t input_index) const
const KernelArgs* MallocReadOnlyDevArgs(void *host_args, size_t args_size) const
rtStream RequestAttachedStream(const ge::AscendString &key, uint32_t flags)
bool GetContextValue(ContextParam key, uint64_t &value) const
ge::graphStatus GetOption(const ge::AscendString &option_key, ge::AscendString &option_value) const
void *MallocWorkSpace(size_t size)
const Tensor *GetOutputTensor(size_t index) const
```
