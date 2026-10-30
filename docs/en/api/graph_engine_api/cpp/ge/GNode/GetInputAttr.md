# GetInputAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:01:54.889Z pushedAt=2026-10-28T07:40:22.689Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the input attributes of **Graph**. This is a generic attribute API whose attribute type is **attr\_value**.

## Prototype

```c++
graphStatus GetInputAttr(const AscendString &name, uint32_t input_index, AttrValue &attr_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Output | Input attribute name. |
| input_index | Output | Input node index. |
| attr_value | Output | Input attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
