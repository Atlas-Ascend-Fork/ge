# SetType

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:09:00.560Z pushedAt=2026-11-18T02:41:08.947Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Sets the task type, which corresponds to the type field of TaskDef. Pass 0 for a Kernel type task (corresponding to RT\_MODEL\_TASK\_KERNEL).

## Prototype

```c++
TaskDefBuilder &SetType(uint32_t type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Input | Task type. Set to 0 when constructing a Kernel type task. |

## Return Value

Returns a TaskDefBuilder object reference, which supports chained calls. The return value can be ignored.

## Constraints

None

## Examples

```c++
// In the GenTask callback: context is the callback input parameter (gert::ExeResGenerationContext*),
// RT_MODEL_TASK_KERNEL is the operator-side local constant 0 (Kernel type task).
gert::TaskDefBuilder task;
task.SetStreamId(context->GetStreamId()).SetType(RT_MODEL_TASK_KERNEL);
```
