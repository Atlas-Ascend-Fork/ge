# Overview

<!-- md-trans-meta sourceCommit=ff42e70ef4dd6e3da4c079832d1983d4ff97dea4 translatedAt=2026-10-28T08:17:05.125Z pushedAt=2026-10-28T08:50:45.503Z nl=2 -->

`gert::ResourceUsageContext` inherits from `gert::ExtendedKernelContext` and serves as the resource declaration context of [`ge::ResourceUsageReporter::DeclareResourceUsage`](../../ge/ResourceUsageReporter/DeclareResourceUsage.md).

## Header File to Include

```c++
#include <exe_graph/runtime/resource_usage_context.h>
```

Library file: liblowering.so

## Public Member Functions

| Function | Description |
| --- | --- |
| [`ge::graphStatus ReportAttachedStream(const std::vector<ge::AscendString> &keys)`](ReportAttachedStream.md) | Reports the list of auxiliary stream keys that this node will request at runtime. |
| [`const Tensor *GetInputTensor(size_t index) const`](GetInputTensor.md) | Obtains the input tensor by flat instance index. |
| [`const Tensor *GetOutputTensor(size_t index) const`](GetOutputTensor.md) | Obtains the output tensor by flat instance index. |
| [`const Tensor *GetRequiredInputTensor(size_t ir_index) const`](GetRequiredInputTensor.md) | Obtains the required input tensor by IR prototype index. |
| [`const Tensor *GetOptionalInputTensor(size_t ir_index) const`](GetOptionalInputTensor.md) | Obtains the optional input tensor by IR prototype index. |
| [`const Tensor *GetDynamicInputTensor(size_t ir_index, size_t relative_index) const`](GetDynamicInputTensor.md) | Obtains the dynamic input tensor by IR prototype index and relative index. |
| [`const Tensor *GetRequiredOutputTensor(size_t ir_index) const`](GetRequiredOutputTensor.md) | Obtains the required output tensor by IR prototype index. |
| [`const Tensor *GetDynamicOutputTensor(size_t ir_index, size_t relative_index) const`](GetDynamicOutputTensor.md) | Obtains the dynamic output tensor by IR prototype index and relative index. |

## Related APIs

- [`ge::ResourceUsageReporter`](../../ge/ResourceUsageReporter/overview.md)
- [`gert::EagerOpExecutionContext::RequestAttachedStream`](../EagerOpExecutionContext/RequestAttachedStream.md)
- [`CompiledGraphSummary::GetStreamNum`](../../ge/CompiledGraphSummary/GetStreamNum.md)
