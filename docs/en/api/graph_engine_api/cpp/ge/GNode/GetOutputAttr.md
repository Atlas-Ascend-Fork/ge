# GetOutputAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:05:11.742Z pushedAt=2026-10-28T07:40:22.702Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the output attribute of **Graph** through the generic attribute API, with the attribute type being **attr\_value**.

## Prototype

```c++
graphStatus GetOutputAttr(const AscendString &name, uint32_t output_index, AttrValue &attr_value) const;
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Output | Output attribute name. |
| output_index | Output | Output node index. |
| attr_value | Output | Output attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
