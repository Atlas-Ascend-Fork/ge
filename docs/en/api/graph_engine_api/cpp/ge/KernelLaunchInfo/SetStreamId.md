# SetStreamId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:42:10.704Z pushedAt=2026-10-28T07:40:22.843Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Sets the stream ID of a **Task**.

## Prototype

```c++
void SetStreamId(uint32_t stream_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| stream_id | Input | Stream ID. |

## Return Value

None

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  auto wait_task = KernelLaunchInfo::CreateHcomWaitTask(context);
  wait_task.SetStreamId(attach_stream_id);
  ...
}
```
