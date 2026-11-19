# SetFolded

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:11:26.379Z pushedAt=2026-10-28T07:40:22.241Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Sets whether the current address needs to be folded into a secondary pointer.

## Prototype

```c++
void SetFolded(bool is_folded)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| is_folded | Input | Whether this address needs to be folded into a secondary pointer. |

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
  aicpu_args_format.emplace_back(ArgDescInfo(ArgDescType::kIrOutputDesc, 0));
  // Set the address to be folded into a secondary pointer.
  aicpu_args_format.back().SetFolded(true);
...
}
```
