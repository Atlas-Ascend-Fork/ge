# Overview

<!-- md-trans-meta sourceCommit=0dbcd6a7f5e0903d3b1a7edd19591c135723d9d6 translatedAt=2026-10-28T08:11:27.158Z pushedAt=2026-10-28T08:50:45.478Z nl=2 -->

Context environment for executing operators.

Provides runtime support such as input/output management and memory allocation for operators running on the host.

## Header File to Include

```c++
#include <exe_graph/runtime/host_cpu_op_execution_context.h>
```

## Public Member Functions

```c++
const Tensor *GetInputTensor(size_t index) const
const Tensor *GetRequiredInputTensor(size_t ir_index) const
const Tensor *GetOptionalInputTensor(size_t ir_index) const
const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const
const Tensor *GetOutputTensor(size_t index) const
Tensor *MallocOutputTensor(size_t index, const StorageShape &shape, const StorageFormat &format, ge::DataType dtype)
Tensor *MakeOutputRefInput(size_t output_index, size_t input_index)
```
