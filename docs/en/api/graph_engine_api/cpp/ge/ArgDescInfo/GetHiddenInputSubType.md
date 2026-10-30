# GetHiddenInputSubType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:07:43.760Z pushedAt=2026-10-28T07:40:22.231Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Obtains the type of the hidden input address of **ArgDescInfo**. The correct value can be obtained only when **type** is **kHiddenInput**.

## Prototype

```c++
HiddenInputSubType GetHiddenInputSubType() const
```

## Parameters

None

## Return Value

Obtains the type of the hidden input address. The default value is **kEnd**.

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
  aicpu_args_format.emplace_back(ArgDescInfo::CreateHiddenInput(HiddenInputSubType::kHcom));
  // Obtain kHcom here.
  uint64_t hidden_type = aicpu_args_format.back().GetHiddenInputSubType();
...
}
```
