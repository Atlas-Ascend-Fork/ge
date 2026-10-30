# Deserialize

<!-- md-trans-meta sourceCommit=a83d79e16734bcb373aa008e5d0dce501e2db450 translatedAt=2026-10-26T13:10:09.320Z pushedAt=2026-10-28T07:42:19.781Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

Reads the serialized binary data from the buffer and deserializes it to restore the operator's kernel state. Used together with Serialize, it enables persistent restoration of operator data and supports rebuilding the operator runtime environment during model loading.

## Prototype

```c++
virtual graphStatus Deserialize(const std::vector<uint8_t> &buffer) = 0
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| buffer | Input | Input binary data. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | graphStatus | GRAPH_SUCCESS(0): Success.<br>Other values: Failure. |

## Constraints

None
