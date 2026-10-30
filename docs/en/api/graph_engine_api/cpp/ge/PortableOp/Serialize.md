# Serialize

<!-- md-trans-meta sourceCommit=a83d79e16734bcb373aa008e5d0dce501e2db450 translatedAt=2026-10-26T13:11:08.209Z pushedAt=2026-10-28T07:42:19.784Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

Serializes the custom operator's kernel binary data into a buffer. The format is defined by the operator; GE does not parse the data but only passes it through. This is used for the persistent storage and restoration of operator data.

## Prototype

```c++
virtual graphStatus Serialize(std::vector<uint8_t> &buffer) = 0
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| buffer | Output | Output binary data. The format is defined by the operator, and GE does not parse it but only passes it through. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
