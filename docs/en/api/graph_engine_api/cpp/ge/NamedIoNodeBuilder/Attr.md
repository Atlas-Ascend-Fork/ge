# Attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:46:57.491Z pushedAt=2026-10-28T07:40:22.862Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: libgraph.so, libgraph\_static.a

## Description

Sets node attributes. Attributes set by the user during **Build** take precedence; default attributes in the registered IR definition are used only for completion and do not override user-set values.

## Prototype

```c++
NamedIoNodeBuilder &Attr(const char_t *name, const AttrValue &value)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Attribute name. |
| value | Input | Attribute value, constructed through `AttrValue::SetAttrValue`. |

## Return Value

Returns a reference to the builder, supporting chained calls.

## Constraints

- If `nullptr` is passed as name, the attribute is not set.
- If `AttrValue` has no value set (default construction), `Build` may return `nullptr`.
- Attribute values set by the user are not overridden by default attributes in the registered IR definition.

## Examples

```c++
ge::Graph graph("test_graph");
ge::AscendString error_msg;

// Build an Add operator node.
auto node = ge::NamedIoNodeBuilder(graph)
    .Type("Add")
    .Name("add_node")
    .AddInput("x1")
    .AddInput("x2")
    .AddOutput("y")
    .Build(error_msg);

if (node != nullptr) {
    // Build succeeded. The node has been added to the graph.
} else {
    // Build failed. Check error_msg for error information.
}
```
