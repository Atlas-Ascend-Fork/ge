# SetAttrs

<!-- md-trans-meta sourceCommit=8e1d9780ef8207a0aa36f04312a187ddccd776b9 translatedAt=2026-10-16T14:09:10.356Z pushedAt=2026-10-28T07:40:22.714Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: **libgraph.so**

## Description

Sets the attributes and attribute values of a **Node** in batch.

## Prototype

```c++
graphStatus SetAttrs(const std::map<AscendString, AttrValue> &attr_values) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **attr_values** | Input | Attribute names and attribute values to be set. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | **graphStatus** | **GRAPH_SUCCESS(0)**: Success.<br>**GRAPH_PARAM_INVALID**: The attribute name is empty.<br>Other values: Failure. |

## Constraints

None

## Examples

```c++
std::map<AscendString, AttrValue> attr_values;
AttrValue attr_value;
attr_value.SetAttrValue(int64_t{1});
attr_values.emplace(AscendString("attr_name"), attr_value);
node.SetAttrs(attr_values);
```
