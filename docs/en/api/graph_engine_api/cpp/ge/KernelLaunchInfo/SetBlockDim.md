# SetBlockDim

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T14:41:49.277Z pushedAt=2026-10-28T07:40:22.841Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Sets the operator **BlockDim**.

## Prototype

```c++
graphStatus SetBlockDim(uint32_t block_dim)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| block_dim | Input | Operator **BlockDim**. |

## Return Value

Returns “ge::GRAPH\_SUCCESS” on success.

For the definition of **graphStatus**, see [ge::graphStatus](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context,
      "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  aicpu_task.SetBlockDim(4);
  ...
}
```
