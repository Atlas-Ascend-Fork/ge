# SetCustomValue

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-16T12:11:12.747Z pushedAt=2026-10-28T07:40:22.239Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: **libgraph.so**

## Description

Sets the custom value of **ArgDescInfo**. The value can be set successfully only when the type is **kCustomValue**.

## Prototype

```c++
graphStatus SetCustomValue(uint64_t custom_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| custom_value | Input | Custom value. |

## Return Value

Returns “ge::GRAPH\_SUCCESS” on success.

For the definition of **graphStatus**, see [ge::graphStatus](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

## Constraints

None

## Examples

```c++
// Structure to be stored in Args.
struct HcclCommParamDesc {
  uint64_t version : 4;
  uint64_t group_num : 4;
  uint64_t has_ffts : 1;
  uint64_t tiling_off : 7;
  uint64_t is_dyn : 48;
};

graphStatus Mc2GenTaskCallback(const gert::ExeResGenerationContext *context,
    std::vector<std::vector<uint8_t>> &tasks) {
...
  // Set the AI CPU task.
  auto aicpu_task = KernelLaunchInfo::CreateAicpuKfcTask(context,
      "libccl_kernel.so", "RunAicpuKfcSrvLaunch");
  size_t input_size = context->GetComputeNodeInfo()->GetIrInputsNum();
  size_t output_size = context->GetComputeNodeInfo()->GetIrOutputsNum();
  const size_t offset = 3UL;
  union {
    HcclCommParamDesc hccl_desc;
    uint64_t custom_value;
  } desc;
  // Assign the value.
  desc.hccl_desc.version = 1;
  desc.hccl_desc.group_num = 1;
  desc.hccl_desc.has_ffts = 0;
  desc.hccl_desc.tiling_off = offset + input_size + output_size;
  desc.hccl_desc.is_dyn = 0;
  std::vector<ArgDescInfo> aicpu_args_format;
  ArgDescInfo custom_value_arg(ArgDescType::kCustomValue);
  // Set the custom value.
  custom_value_arg.SetCustomValue(desc.custom_value);
  // Convert the content of this structure into a uint64_t number and save it to ArgsFormat.
  aicpu_args_format.emplace_back(custom_value_arg);
...
}
```
