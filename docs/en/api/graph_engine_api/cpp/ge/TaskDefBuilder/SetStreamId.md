# SetStreamId

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:08:39.044Z pushedAt=2026-11-18T02:41:08.946Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Sets the stream ID of the task, corresponding to the stream\_id field of TaskDef.

## Prototype

```c++
TaskDefBuilder &SetStreamId(uint32_t stream_id)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| stream_id | Input | Stream ID for task execution, generally obtained from ExeResGenerationContext::GetStreamId(). |

## Return Value

Returns a reference to the TaskDefBuilder object, supporting chained calls; the return value can be ignored.

## Constraints

None

## Examples

```c++
// In the GenTask callback: context is the callback input parameter (gert::ExeResGenerationContext*),
// RT_MODEL_TASK_KERNEL is the operator-side local constant 0 (Kernel-type task)
gert::TaskDefBuilder task;
task.SetStreamId(context->GetStreamId()).SetType(RT_MODEL_TASK_KERNEL);
```
