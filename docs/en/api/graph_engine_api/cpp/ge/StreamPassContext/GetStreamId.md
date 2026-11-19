# GetStreamId

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:34:29.988Z pushedAt=2026-10-28T07:42:19.868Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Obtains the current Stream ID of a node. The result indicates the Stream ID assigned to the node by the built-in stream allocation algorithm.

## Prototype

```c++
int64_t GetStreamId(const GNode &node) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node in the graph. |

## Return Value

Returns the Stream ID to which the node belongs.

## Constraints

None
