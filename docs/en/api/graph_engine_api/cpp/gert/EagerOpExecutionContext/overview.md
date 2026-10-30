# Overview

<!-- md-trans-meta sourceCommit=b88d828e7fb7e4bea48dcc8f4c5e5a7e6ca23f9d translatedAt=2026-10-28T08:07:30.826Z pushedAt=2026-10-28T08:50:45.466Z nl=2 -->

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
void *MallocWorkSpace(size_t size)
const Tensor *GetOutputTensor(size_t index) const
```
