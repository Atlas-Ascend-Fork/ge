# GetOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:44:15.910Z pushedAt=2026-10-28T07:40:22.625Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/subgraph\_boundary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the node output.

## Prototype

```c++
Status GetOutput(NodeIo &node_output) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_output | Output | Node and its output index. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Obtained successfully.<br>**FAILED**: Failed to obtain. |

## Constraints

None
