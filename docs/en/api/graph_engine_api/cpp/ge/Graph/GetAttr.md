# GetAttr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:17:46.979Z pushedAt=2026-10-28T07:40:22.745Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains the attributes of **Graph**. This is a generic attribute API, and the attribute type is attr\_value.

## Prototype

```c++
graphStatus GetAttr(const AscendString &name, AttrValue &attr_value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Output | Attribute name of **Graph**. |
| attr_value | Output | Attribute value of **Graph**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None

## Examples

```c++
AttrValue av_get;
graph.GetAttr("int_attr", av_get);
int64_t int_attr_get{};
av_get.GetAttrValue(int_attr_get);
ASSERT_EQ(int_attr_get, 100);
```
