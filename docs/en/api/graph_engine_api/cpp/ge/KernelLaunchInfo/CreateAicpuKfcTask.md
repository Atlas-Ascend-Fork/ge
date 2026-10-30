# CreateAicpuKfcTask

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:36:25.498Z pushedAt=2026-10-28T07:40:22.822Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Creates a KFC Task for the AI CPU.

## Prototype

```c++
static KernelLaunchInfo CreateAicpuKfcTask(const gert::ExeResGenerationContext *context, const char *so_name, const char *kernel_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| context | Input | Input parameter of the GenerateTask function, which stores the basic information of the operator. |
| so_name | Input | Name of the .so file in which the AI CPU operator is packaged. |
| kernel_name | Input | Name of the entry function of the AI CPU operator. |

## Return Value

Information of the created AI CPU KFC Task.

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  // Create the AI CPU task.
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context,
      "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  // Obtain the attach stream.
  auto stream_infos = context->GetAttachedStreamInfos();
  GE_ASSERT_TRUE(!stream_infos.empty());
  const int64_t attach_stream_id = stream_infos[0].stream_id;
  // Add information to the AI CPU KFC task.
  aicpu_task->SetStreamId(attach_stream_id);
  ...
}
```
