# CreateCustomValue

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:05:52.973Z pushedAt=2026-10-28T07:40:22.227Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Creates an **ArgDescInfo** object of type [kCustomValue](../ArgDescType.md) (custom parameter type), indicating that this block of **Args** address is used to store custom content.

## Prototype

```c++
static ArgDescInfo CreateCustomValue(uint64_t custom_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| custom_value | Input | Custom data stored in this block of **Args** memory. |

## Return Value

Returns an **ArgDescInfo** object whose type is **kCustomValue**.

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
  // Assign values.
  desc.hccl_desc.version = 1;
  desc.hccl_desc.group_num = 1;
  desc.hccl_desc.has_ffts = 0;
  desc.hccl_desc.tiling_off = offset + input_size + output_size;
  desc.hccl_desc.is_dyn = 0;
  std::vector<ArgDescInfo> aicpu_args_format;
  // Convert the content of this structure into a uint64_t value and store it in ArgsFormat.
  aicpu_args_format.emplace_back(ArgDescInfo::CreateCustomValue(desc.custom_value));
...
}
```
