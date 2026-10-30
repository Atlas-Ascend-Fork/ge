# SetInputAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:09:36.169Z pushedAt=2026-10-28T07:40:22.716Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Sets the input attribute of a Graph. This is a generic attribute API, and the attribute type is attr\_value.

## Prototype

```c++
graphStatus SetInputAttr(const AscendString &name, uint32_t input_index, const AttrValue &attr_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Input attribute name. |
| input_index | Input | Input node index. |
| attr_value | Input | Input attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
