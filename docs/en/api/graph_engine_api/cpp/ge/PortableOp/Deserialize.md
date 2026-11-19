# Deserialize

<!-- md-trans-meta sourceCommit=a83d79e16734bcb373aa008e5d0dce501e2db450 translatedAt=2026-10-26T13:10:09.320Z pushedAt=2026-10-28T07:42:19.781Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: libgraph.so

## Description

GE calls this API during model loading to read the serialized binary data from the buffer saved with the offline model (OM) and deserialize it to restore the operator data. It is used together with Serialize to restore operator data persistence, supporting the reconstruction of the operator runtime environment during model loading.

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

Serialize and Deserialize must be used together. A custom operator that does not implement this API can still be used to generate an offline model: if the operator implements the AnnotatedArgsOp API, it can submit kernel data to GE through DeclareLaunchArgs to be saved with the model and used for launch during the model loading and execution phase; otherwise, its data (such as kernel bin) is not saved or restored with the OM, and the Compile API of the custom operator is not called again during model loading, so you must manage the saving and restoration of this data yourself.
