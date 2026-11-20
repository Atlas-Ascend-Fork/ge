# SetStreamId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:35:15.227Z pushedAt=2026-10-28T07:42:19.871Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Sets the Stream ID of a node. Nodes with the same Stream ID are executed sequentially on the same stream.

## Prototype

```c++
graphStatus SetStreamId(const GNode &node, int64_t stream_id)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node on the graph. |
| stream_id | Input | Stream ID to be set: if it is an already allocated stream ID, set it directly; if a new Stream ID needs to be allocated, call the [AllocateNextStreamId](AllocateNextStreamId.md) API first. Otherwise, if the Stream ID exceeds the maximum Stream ID on the current graph, the API returns a failure. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

None
