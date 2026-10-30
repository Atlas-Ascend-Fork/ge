# SetHiddenInputSubType

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T12:11:50.381Z pushedAt=2026-10-28T07:40:22.242Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Sets the type of the hidden input address of **ArgDescInfo**. The setting succeeds only when the type is **kHiddenInput**.

## Prototype

```c++
graphStatus SetHiddenInputSubType(HiddenInputSubType hidden_type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| hidden_type | Input | Type of the hidden input. |

## Return Value

Returns “ge::GRAPH\_SUCCESS” on success.

For the definition of graphStatus, see [ge::graphStatus](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

## Constraints

None

## Examples

```c++
graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
...
  // Set the AI CPU task.
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context,
      "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  std::vector<ArgDescInfo> aicpu_args_format;
  ArgDescInfo args_info(ArgDescType::kHiddenInput);
  args_info.SetHiddenInputSubType(HiddenInputSubType::kHcom);
  aicpu_args_format.emplace_back(args_info);
...
}
```
