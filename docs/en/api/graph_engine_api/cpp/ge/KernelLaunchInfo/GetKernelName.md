# GetKernelName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:38:01.341Z pushedAt=2026-10-28T07:40:22.829Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/kernel\_launch\_info.h\>
- Library file: libregister.so

## Description

Obtains the name of the Kernel entrypoint function of an AI CPU task.

## Prototype

```c++
const char *GetKernelName() const
```

## Parameters

None

## Return Value

Returns the name of the Kernel entrypoint function of the operator on success.

Returns nullptr on failure.

## Constraints

Only AI CPU tasks can obtain the name of the Kernel entrypoint function.

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
  ...
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context,
      "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  // Obtain RunAicpuKfcSrvLaunch.
  auto so_name = aicpu_task.GetKernelName();
  ...
}
```
