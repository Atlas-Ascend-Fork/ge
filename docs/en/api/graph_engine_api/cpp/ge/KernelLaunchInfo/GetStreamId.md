# GetStreamId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:38:52.046Z pushedAt=2026-10-28T07:40:22.832Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Obtains the ID of the stream where the current **Task** resides.

## Prototype

```c++
uint32_t GetStreamId() const
```

## Parameters

None

## Return Value

Returns the ID of the stream where the current **Task** resides, with a default value of 0.

On exception, returns **int32\_max**.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  auto aicore_task = KernelLaunchInfo::LoadFromData(context, tasks.back());
  auto stream_id = aicore_task.GetStreamId();
  ...
}
```
