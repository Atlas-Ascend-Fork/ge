# NamedIoNodeBuilder Constructor and Destructor

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:49:08.247Z pushedAt=2026-10-28T07:40:22.870Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/named\_io\_node\_builder.h\>
- Library file: **libgraph.so**, libgraph\_static.a

## Description

**NamedIoNodeBuilder** constructor, which creates a Builder object for constructing graph nodes based on input, output, and attribute names.

**NamedIoNodeBuilder** is intended for scenarios where graph nodes are constructed directly by node input/output names. The caller can specify the input names, output names, attribute names, and related description information of a node instance. During construction, the operator IR definition registered through REG_\__OP is matched based on the node type, and the input/output instance names and their order are verified for compatibility with that definition, ultimately creating the corresponding graph node.

## Prototype

```c++
explicit NamedIoNodeBuilder(Graph &graph)
~NamedIoNodeBuilder()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | The graph object to which the constructed node will be added. |

## Return Value

Returns an object of type **NamedIoNodeBuilder**.

## Constraints

- A **NamedIoNodeBuilder** object cannot be copy-constructed or assigned.
- The builder object should not be used again after a successful **Build**.
