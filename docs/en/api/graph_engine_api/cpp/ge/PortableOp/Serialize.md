# Serialize

<!-- md-trans-meta sourceCommit=a83d79e16734bcb373aa008e5d0dce501e2db450 translatedAt=2026-10-26T13:11:08.209Z pushedAt=2026-10-28T07:42:19.784Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

GE calls this API back during the model saving phase. The custom operator serializes the data that needs to be persistently stored in the offline model into the buffer. The format is defined by the operator, and GE does not parse it but only passes it through.

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

Serialize and Deserialize must be implemented together. A custom operator that does not implement this API can still be used to generate an offline model: if the operator implements the AnnotatedArgsOp API, it can submit kernel data to GE through DeclareLaunchArgs so that the data is saved with the model and used for launch during the model loading and execution phase; otherwise, its data (such as the kernel bin) is not saved and restored with the OM, and the Compile API of the custom operator is not called back again during the model loading phase. You need to manage the saving and restoration of this data on your own.
