# RequestAttachedStream

<!-- md-trans-meta sourceCommit=6c0f2edd2793cdeb0ad99aa92d61e13b5213c8ef translatedAt=2026-10-28T07:59:09.961Z pushedAt=2026-10-28T08:50:45.433Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Applies for a logical auxiliary stream by key. The scope of the key is the subgraph where the operator resides (corresponding to a compiled submodel): within the same subgraph, the same key reuses the same logical stream ID; across different subgraphs, keys are independent of each other, and even if the keys are the same, each is assigned its own stream ID, corresponding to its own independent physical stream at runtime. The returned logical stream ID can be used in the `stream_id` field of [`AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md) to submit a kernel launch on the auxiliary stream.

## Prototype

```c++
uint32_t RequestAttachedStream(const ge::AscendString &key)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| key | Input | Auxiliary stream identifier, scoped to the subgraph where the operator resides. It cannot be empty and cannot use the `CANN-FMK-` reserved prefix. |

## Return Value

Returns the logical auxiliary stream ID corresponding to the key on success; returns UINT32_MAX if the key is empty or the application fails.

## Constraints

- The returned compile-time logical ID can only be used in `AnnotatedKernelLaunchInfo::stream_id` of the current Context.
- After the Context is destroyed, this ID cannot be used as a runtime handle.
- The key is not reused across subgraphs. This API does not support sharing the same physical auxiliary stream among multiple subgraphs.

## Examples

```c++
const auto aux_stream_id = ctx.RequestAttachedStream(ge::AscendString("aux"));
if (aux_stream_id == UINT32_MAX) {
  return ge::GRAPH_FAILED;
}
// Fill aux_stream_id into AnnotatedKernelLaunchInfo::stream_id,
// and then call the [`AddLaunch`](AddLaunch.md) overload with the predecessors parameter to submit a launch on the auxiliary stream.
```
