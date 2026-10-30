# GetCapturedTensors

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:32:09.449Z pushedAt=2026-10-28T07:40:22.572Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern.h\>
- Library file: libge\_compiler.so

## Description

Obtains all tensors captured in the Pattern. The order in the vector is the capture order.

## Prototype

```c++
Status GetCapturedTensors(std::vector<NodeIo> &node_outputs) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_outputs | Input | All tensors captured in the Pattern. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Obtained successfully.<br>**FAILED**: Failed to obtain. |

## Constraints

None
