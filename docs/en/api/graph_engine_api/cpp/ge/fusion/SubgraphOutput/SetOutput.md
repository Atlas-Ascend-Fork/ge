# SetOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:44:28.727Z pushedAt=2026-10-28T07:40:22.628Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler.so

## Description

Sets the node output.

## Prototype

```c++
Status SetOutput(const NodeIo &node_output)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_output | Output | Node and one of its output indexes. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success<br>**FAILED**: Failure |

## Constraints

None
