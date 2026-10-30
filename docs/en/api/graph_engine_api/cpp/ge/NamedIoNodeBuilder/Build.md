# Build

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:47:55.837Z pushedAt=2026-10-28T07:40:22.867Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: libgraph.so, libgraph\_static.a

## Description

Builds a graph node and adds it to the Graph. Build creates a GNode based on the configured input, output, and attribute information, and verifies whether the input/output instance names and their order are compatible with the registered operator IR definition. On success, the returned GNode has already been added to the Graph.

Build performs the following externally visible checks and operations:

1. Verifies that Type is set and that the operator IR corresponding to the node type has been registered through REG\_OP.
2. Verifies whether the input/output instance names and their order are compatible with the registered operator IR definition.
3. Creates a GNode and adds it to the Graph.
4. Returns unique\_ptr<GNode\>, and clears error\_message on success.

## Prototype

```c++
std::unique_ptr<GNode> Build(AscendString &error_message)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| error_message | Output | Error message when the build fails. On success, error_message is cleared. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<GNode> | Returns a **unique_ptr** to the **GNode** on success, which has been added to the **Graph**. Returns **nullptr** on failure. |

## Constraints

- After a successful **Build**, the **Builder** object should not be used again. Calling **Build** again after a successful build returns **nullptr** and outputs the error message "Build\(\) has already been called on this builder".
- The operator IR corresponding to the node type must have been registered through REG\_OP; otherwise, **Build** returns **nullptr**.
- Input/output instances must be added in the order defined by the IR; otherwise, **Build** returns **nullptr**.
- **Data** and **NetOutput** nodes do not perform input/output instance compatibility validation.

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
    // Build failed. Check error_msg for the error message.
}
```
