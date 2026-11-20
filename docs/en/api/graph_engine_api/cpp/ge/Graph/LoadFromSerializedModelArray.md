# LoadFromSerializedModelArray

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:20:49.446Z pushedAt=2026-10-28T07:40:22.758Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Restores a **Graph** from the serialized data of **ModelDef**.

## Prototype

```c++
graphStatus LoadFromSerializedModelArray(const void *serialized_model, size_t size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| serialized_model | Input | Pointer to the serialized data of **ModelDef**. |
| size | Input | Length of the serialized data of **ModelDef**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS**(0): Success.<br>Other values: Failure. |

## Constraints

Only files in the air format are supported for reading.
