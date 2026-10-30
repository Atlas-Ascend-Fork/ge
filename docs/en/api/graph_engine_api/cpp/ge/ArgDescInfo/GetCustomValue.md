# GetCustomValue

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:06:50.585Z pushedAt=2026-10-28T07:40:22.230Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/arg\_desc\_info.h\>
- Library file: libgraph.so

## Description

Obtains the custom value of **ArgDescInfo**.

The correct value can be obtained only when **type** is **kCustomValue**.

## Prototype

```c++
uint64_t GetCustomValue() const
```

## Parameters

None

## Return Value

Returns the value of the custom content, with a default value of 0.

On exception, the obtained value is **uint64\_max**.

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
  // Convert the content of this structure into a uint64_t number and save it to ArgsFormat.
  aicpu_args_format.emplace_back(ArgDescInfo::CreateCustomValue(desc.custom_value));
  // The value in custom_value here is the result of concatenating the HcclCommParamDesc content.
  uint64_t custom_value = aicpu_args_format.back().GetCustomValue();
...
}
```
