# SetAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:23:34.649Z pushedAt=2026-10-28T07:40:22.767Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Sets the attribute of **Graph**. This is a generic attribute API, and the attribute type is **attr\_value**.

## Prototype

```c++
graphStatus SetAttr(const AscendString &name, const AttrValue &attr_value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| attr_value | Input | Attribute value. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None

## Examples

```c++
AttrValue av;
av.SetAttrValue(static_cast<int64_t>(100));
Graph graph("test");
graph.SetAttr("int_attr", av);
```
