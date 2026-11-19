# GetKernelBuilder

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:06:08.219Z pushedAt=2026-11-18T02:41:08.937Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Obtains the view object of the KernelDef message ([KernelDefBuilder](overview.md)), which is used to set fields at the KernelDef level. Internal sub-messages are created lazily to ensure that the object pointed to by the view is non-null.

## Prototype

```c++
KernelDefBuilder GetKernelBuilder() const
```

## Parameters

None

## Return Value

Returns the KernelDefBuilder view object (by value, holding a raw pointer to the sub-message without owning it).

## Constraints

The view lifetime must be covered by the parent TaskDefBuilder object. The view must not be used after the parent object is destroyed.

## Examples

```c++
// In the GenTask callback: blockDim is obtained from node attributes (for example, hcom_block_dim),
// and argsFormat is obtained by serializing std::vector<ge::ArgDescInfo> through ge::ArgsFormatSerializer::Serialize.
gert::TaskDefBuilder task;
task.GetKernelBuilder().SetBlockDim(blockDim).GetContextBuilder().SetArgsFormat(argsFormat);
```
