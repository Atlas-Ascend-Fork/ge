# GetIrIndex

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:08:38.808Z pushedAt=2026-10-28T07:40:22.234Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: **libgraph.so**

## Description

Obtains the operator IR index of the current **ArgDescInfo**. The index can be obtained only when **type** is **kIrInput**, **kIrOutput**, **kIrInputDesc**, or **kIrOutputDesc**.

## Prototype

```c++
int32_t GetIrIndex() const
```

## Parameters

None

## Return Value

Returns the IR index of the input/output corresponding to the current Args address. The default value is **-1** when it is not set.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
...
  // Set the AI CPU task.
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context,
      "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  std::vector<ArgDescInfo> aicpu_args_format;
  aicpu_args_format.emplace_back(ArgDescInfo(ArgDescType::kIrOutput, 0));
  // Obtain the IR index of the ArgDescInfo of the kIrOutput type. The obtained value is 0.
  auto ir_index = aicpu_args_format.back().GetIrIndex();
...
}
```
