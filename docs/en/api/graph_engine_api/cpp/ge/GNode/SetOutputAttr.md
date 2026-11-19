# SetOutputAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:09:42.591Z pushedAt=2026-10-28T07:40:22.720Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: **libgraph.so**

## Description

Sets the output attributes of a **Graph**. This is a generic attribute API whose attribute type is **attr\_value**.

## Prototype

```c++
graphStatus SetOutputAttr(const AscendString &name, uint32_t output_index, const AttrValue &attr_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output attribute name. |
| output_index | Input | Output node index. |
| attr_value | Input | Output attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
