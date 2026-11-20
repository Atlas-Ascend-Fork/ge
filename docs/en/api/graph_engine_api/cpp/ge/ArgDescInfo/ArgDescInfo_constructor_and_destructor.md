# ArgDescInfo Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:04:21.349Z pushedAt=2026-10-28T07:40:22.225Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Constructor and destructor of **ArgDescInfo**.

## Prototype

```c++
explicit ArgDescInfo(ArgDescType arg_type, int32_t ir_index = -1, bool is_folded = false)
~ArgDescInfo()
ArgDescInfo(const ArgDescInfo &other)
ArgDescInfo(ArgDescInfo &&other) noexcept
ArgDescInfo &operator=(const ArgDescInfo &other)
ArgDescInfo &operator=(ArgDescInfo &&other) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| arg_type | Input | Type of the current Args address. For the specific type definition, see [ArgDescType](../ArgDescType.md). |
| ir_index | Input | Operator IR index corresponding to the current Args address. |
| is_folded | Input | Whether the current address needs to be folded into a secondary pointer. |

## Return Value

None

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context, std::vector<std::vector<uint8_t>> &tasks) {
...
  // Set the AI CPU task.
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context, "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  std::vector<ArgDescInfo> aicpu_args_format;
  // Construct an address description of type kIrOutputDesc with ir_index 0 that needs to be folded into a secondary pointer.
  aicpu_args_format.emplace_back(ArgDescInfo(ArgDescType::kIrOutputDesc, 0, true));
...
}
```
