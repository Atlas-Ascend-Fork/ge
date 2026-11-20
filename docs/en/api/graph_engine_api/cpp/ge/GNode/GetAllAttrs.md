# GetAllAttrs

<!-- md-trans-meta sourceCommit=8e1d9780ef8207a0aa36f04312a187ddccd776b9 translatedAt=2026-10-16T13:59:25.782Z pushedAt=2026-10-28T07:40:22.680Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: **libgraph.so**

## Description

Obtains all attributes and attribute values of a Node, including predefined IR attributes and common attributes, but excluding input/output port attributes and internal attributes whose names start with an underscore (`_`).
Attributes that are not set are not written to the output map.

## Prototype

```c++
graphStatus GetAllAttrs(std::map<AscendString, AttrValue> &attr_values) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_values | Output | Returns all attributes and attribute values of the Node. The original content is cleared upon invocation. |

## Return Value

| Type | Description |
| --- | --- |
| graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None

## Examples

```c++
std::map<AscendString, AttrValue> attr_values;
node.GetAllAttrs(attr_values);
```
