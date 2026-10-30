# GetBlockDim

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:37:52.050Z pushedAt=2026-10-28T07:40:22.827Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Obtains the operator **BlockDim**.

## Prototype

```c++
uint32_t GetBlockDim() const
```

## Parameters

None

## Return Value

Returns the **BlockDim** value of this operator's **Task**, with a default value of 0.

Returns **int32\_max** on exception.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  auto aicore_task = KernelLaunchInfo::LoadFromData(context, tasks.back());
  auto block_dim = aicore_task.GetBlockDim();
  ...
}
```
