# AddOutput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:46:32.559Z pushedAt=2026-10-28T07:40:22.861Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: libgraph.so, libgraph\_static.a

## Description

Adds an output instance port. Users add output instances in the order defined by the IR. Regular outputs use the IR name, while dynamic outputs use consecutive instance names in the form of ir\_name0, ir\_name1, and so on.

## Prototype

```c++
NamedIoNodeBuilder &AddOutput(const char_t *name)
NamedIoNodeBuilder &AddOutput(const char_t *name, const TensorDesc &desc)
```

- First prototype: Adds an output instance port using the default TensorDesc (DT\_FLOAT + FORMAT\_ND).
- Second prototype: Adds an output instance port with a description, allowing the data type, format, and shape to be specified.

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output instance name. Regular outputs use the IR name, while dynamic outputs use consecutive instance names in the form of ir_name0, ir_name1, and so on. |
| desc | Input | Output tensor description, containing the data type, format, and shape information. Used only in the second prototype. |

## Return Value

Returns a reference to the builder, supporting chained calls.

## Constraints

- Output instances must be added in the order defined by the IR.
- If dynamic output instances are passed, the instance names must be numbered consecutively starting from ir\_name0, without skipping any intermediate sequence number.
- Required outputs must be added; otherwise, **Build** returns nullptr.
- If nullptr is passed as **name**, the output is not added.

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
