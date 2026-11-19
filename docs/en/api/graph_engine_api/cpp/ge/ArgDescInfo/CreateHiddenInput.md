# CreateHiddenInput

<!-- md-trans-meta sourceCommit=8ea012521aa90988463e5b39d4aa4835f83b0893 translatedAt=2026-10-16T12:06:48.237Z pushedAt=2026-10-28T07:40:22.228Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Creates an **ArgDescInfo** object of the [kHiddenInput](../ArgDescType.md) type (an input address that does not exist in the IR prototype definition), indicating that the Args address of this block is a HiddenInput address.

## Prototype

```c++
static ArgDescInfo CreateHiddenInput(HiddenInputSubType hidden_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| hidden_type | Input | Type of the Hidden input. For details, see [HiddenInputSubType](../HiddenInputSubType.md). |

## Return Value

Returns an **ArgDescInfo** object whose type is **kHiddenInput**.

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
  // Create a HiddenInput address of the Hcom type.
  aicpu_args_format.emplace_back(ArgDescInfo::CreateHiddenInput(HiddenInputSubType::kHcom));
...
}
```
