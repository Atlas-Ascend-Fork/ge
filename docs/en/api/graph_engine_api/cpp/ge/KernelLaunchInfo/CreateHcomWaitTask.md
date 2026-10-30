# CreateHcomWaitTask

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:37:06.981Z pushedAt=2026-10-28T07:40:22.826Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Creates a Wait Task that blocks a stream. When a Record Task with the same **group\_name** is executed, the block is released.

## Prototype

```c++
static KernelLaunchInfo CreateHcomWaitTask(const gert::ExeResGenerationContext *context, const char *group_name = "group")
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| context | Input | Input parameter of the GenerateTask function, which stores the basic information of the operator. |
| group_name | Input | Group name of the Wait Task, defaulting to group, used to pair with the Record Task. |

## Return Value

Returns the information of the created Wait Task.

## Constraints

**group\_name** must be consistent with the attributes defined in the operator prototype. For example, if an mc2 operator defines an attribute **group\_ep**, you can use **group\_name** to create a Record task and a Wait task for **group\_ep**.

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  // Create the WaitTask.
  auto wait_task = KernelLaunchInfo::CreateHcomWaitTask(context);
  wait_task.SetStreamId(attach_stream_id);
  tasks.insert(tasks.begin() + aicore_index, wait_task.Serialize());
  aicore_index++;
  ...
}
```
