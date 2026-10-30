# Type

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:49:59.537Z pushedAt=2026-10-28T07:40:22.873Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: libgraph.so, libgraph\_static.a

## Description

Sets the node type (required). The operator IR corresponding to the node type must have been registered through REG\_OP; otherwise, **Build** returns **nullptr**.

## Prototype

```c++
NamedIoNodeBuilder &Type(const char_t *type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Input | Operator type, such as "Add" and "MatMul". The corresponding operator IR must have been registered through **REG_OP**. |

## Return Value

Returns the builder reference, which supports chained calls.

## Constraints

- This method is required. If **Type** is not called, **Build** returns **nullptr**.
- If **nullptr** is passed in, **Type** is not set, and **Build** reports the error "Type must be set before Build\(\)".

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
