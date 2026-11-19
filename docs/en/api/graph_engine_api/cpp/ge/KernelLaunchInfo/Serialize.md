# Serialize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:40:42.349Z pushedAt=2026-10-28T07:40:22.837Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Serializes **KernelLaunchInfo** into a data stream.

## Prototype

```c++
std::vector<uint8_t> Serialize()
```

## Parameters

None

## Return Value

Returns the serialized data stream.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  // Create WaitTask.
  auto wait_task = KernelLaunchInfo::CreateHcomWaitTask(context);
  wait_task.SetStreamId(attach_stream_id);
  // Serialize.
  tasks.insert(tasks.begin() + aicore_index, wait_task.Serialize());
  aicore_index++;
  ...
}
```
