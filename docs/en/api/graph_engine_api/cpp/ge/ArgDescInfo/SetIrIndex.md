# SetIrIndex

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:12:04.515Z pushedAt=2026-10-28T07:40:22.245Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Sets the IR index of the current **ArgDescInfo**. The setting succeeds only when **type** is **kIrInput**, **kIrOutput**, **kIrInputDesc**, or **kIrOutputDesc**.

## Prototype

```c++
void SetIrIndex(int32_t ir_index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ir_index | Input | IR index of the input/output. |

## Return Value

None

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
  aicpu_args_format.emplace_back(ArgDescInfo(ArgDescType::kIrOutput));
  // Set the IR index to 0.
  aicpu_args_format.back().SetIrIndex(0);
...
}
```
