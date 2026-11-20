# CreateHcomRecordTask

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:36:49.344Z pushedAt=2026-10-28T07:40:22.823Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Creates a Record task, which is used to wake up Wait Tasks on other streams that have the same **group\_name**.

## Prototype

```c++
static KernelLaunchInfo CreateHcomRecordTask(const gert::ExeResGenerationContext *context, const char *group_name = "group")
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| context | Input | Input parameter of the **GenerateTask** function, which stores the basic information of the operator. |
| group_name | Input | Group name of the Record task, defaulting to **group**, used to pair with the Wait Task. |

## Return Value

Created Record Task information.

## Constraints

**group\_name** must be consistent with the attributes defined in the operator prototype. For example, if an mc2 operator defines an attribute **group\_ep**, you can use **group\_name** to create Record tasks and Wait tasks for **group\_ep**.

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  // Create the RecordTask.
  auto record_task = KernelLaunchInfo::CreateHcomRecordTask(context);
  // Set the stream information.
  record_task.SetStreamId(stream_id);
  tasks.insert(tasks.begin() + aicore_index, record_task.Serialize());
  aicore_index++;
  ...
}
```
