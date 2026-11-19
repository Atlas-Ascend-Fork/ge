# AddInput

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:46:36.623Z pushedAt=2026-10-28T07:40:22.863Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: libgraph.so, libgraph\_static.a

## Description

Adds an input instance port. Users add input instances in the order defined by the IR. Regular/optional inputs use the IR name, while dynamic inputs use consecutive instance names in the form of ir\_name0, ir\_name1, and so on.

## Prototype

```c++
NamedIoNodeBuilder &AddInput(const char_t *name)
NamedIoNodeBuilder &AddInput(const char_t *name, const TensorDesc &desc)
```

- First prototype: Adds an input instance port using the default TensorDesc (DT\_FLOAT + FORMAT\_ND).
- Second prototype: Adds an input instance port with a description, allowing the data type, format, and shape to be specified.

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Input instance name. Regular/optional inputs use the IR name, while dynamic inputs use consecutive instance names in the form of ir_name0, ir_name1, and so on. |
| desc | Input | Input tensor description, containing data type, format, and shape information. Used only in the second prototype. |

## Return Value

Returns a builder reference that supports chained calls.

## Constraints

- Input instances must be added in the order defined by the IR.
- Usage of optional inputs (OPTIONAL\_INPUT):
    - Not passing an optional input: Add only the required inputs and skip this optional input.
    - Passing a valid optional input: Add it at the corresponding position in IR order, using the default TensorDesc.
    - Passing a placeholder optional input: Use the overload with TensorDesc and pass in DT\_UNDEFINED and FORMAT\_RESERVED.
    - An optional input can provide at most one instance. Adding it repeatedly causes validation to fail.

- If a dynamic input instance is passed, the instance names must be numbered consecutively starting from ir\_name0, without skipping any intermediate sequence number.
- If nullptr is passed as the name, the input is not added.

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
    // Build failed. Check error_msg for the error information.
}
```
