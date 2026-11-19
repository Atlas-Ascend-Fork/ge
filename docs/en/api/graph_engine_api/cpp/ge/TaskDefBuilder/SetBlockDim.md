# SetBlockDim

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:07:54.660Z pushedAt=2026-11-18T02:41:08.944Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header File: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Sets the multi-core parallelism of a Kernel task, corresponding to the block\_dim field of KernelDef. It is generally taken from node hcom\_block\_dim and other attributes or from the tiling result.

## Prototype

```c++
KernelDefBuilder &SetBlockDim(uint32_t block_dim)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| block_dim | Input | Core parallelism. |

## Return Value

Returns a reference to the KernelDefBuilder object, which supports chained calls. The return value can be ignored.

## Constraints

None

## Examples

```c++
// In the GenTask callback, blockDim is taken from a node attribute (for example, hcom_block_dim).
// argsFormat is obtained by serializing std::vector<ge::ArgDescInfo> through ge::ArgsFormatSerializer::Serialize.
gert::TaskDefBuilder task;
task.GetKernelBuilder().SetBlockDim(blockDim).GetContextBuilder().SetArgsFormat(argsFormat);
```
