# SetArgsFormat

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:07:44.341Z pushedAt=2026-11-18T02:41:08.943Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header File: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Sets the Args address layout description of the task, corresponding to the args\_format field of KernelContext. args\_format is obtained by serializing ArgDescInfo through [ArgsFormatSerializer::Serialize](../ArgsFormatSerializer/Serialize.md). The layout order is defined by the args\_format string itself, and this API does not validate it.

## Prototype

```c++
KernelContextBuilder &SetArgsFormat(const ge::AscendString &args_format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| args_format | Input | Serialized Args layout description string. |

## Return Value

Returns a reference to the KernelContextBuilder object, supporting chained calls; the return value can be ignored.

## Constraints

None

## Examples

```c++
// Construct the Args layout: the hidden input (HCCL superkernel) comes first, followed by inputs and outputs.
std::vector<ge::ArgDescInfo> argDescs;
argDescs.emplace_back(ge::ArgDescInfo::CreateHiddenInput(ge::HiddenInputSubType::kHcclSuperKernel));
argDescs.emplace_back(ge::ArgDescType::kIrInput, 0);
argDescs.emplace_back(ge::ArgDescType::kIrOutput, 0);
gert::TaskDefBuilder task;
task.GetKernelBuilder().GetContextBuilder()
    .SetArgsFormat(ge::ArgsFormatSerializer::Serialize(argDescs));
```
