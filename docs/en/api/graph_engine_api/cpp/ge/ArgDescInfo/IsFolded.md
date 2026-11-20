# IsFolded

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:10:21.741Z pushedAt=2026-10-28T07:40:22.238Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Determines whether the current **Args** address needs to be folded into a secondary pointer.

## Prototype

```c++
bool IsFolded() const
```

## Parameters

None

## Return Value

Returns whether the current **Args** address needs to be folded into a secondary pointer.

true: yes, false: no.

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
  aicpu_args_format.emplace_back(ArgDescInfo(ArgDescType::kIrOutputDesc, 0, true));
  // Determine whether this address needs to be folded into a secondary pointer. Here, true is returned.
  auto is_folded = aicpu_args_format.back().IsFolded();
...
}
```
