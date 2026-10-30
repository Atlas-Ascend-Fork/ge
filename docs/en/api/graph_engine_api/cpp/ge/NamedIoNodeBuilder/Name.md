# Name

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:48:08.504Z pushedAt=2026-10-28T07:40:22.864Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: libgraph.so, libgraph\_static.a

## Description

Sets the node name (optional). If this method is not called, the node name is an empty string, and Build still succeeds.

## Prototype

```c++
NamedIoNodeBuilder &Name(const char_t *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Node Name, used to identify the node in the graph. |

## Return Value

Returns the builder reference, which supports chained calls.

## Constraints

- This method is optional. If **Name** is not called, the node name is empty, which does not affect **Build**.
- If **nullptr** is passed, **Name** is not set, the node name is an empty string, and **Build** still succeeds.

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
    // Build succeeds, and the node has been added to the graph.
} else {
    // Build fails. Check error_msg for error information.
}
```
