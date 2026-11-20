# Build

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:31:23.014Z pushedAt=2026-10-28T07:40:22.325Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/compliant\_node\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Builds and returns a graph node.

## Prototype

```c++
ge::GNode Build() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | GNode | The completed graph node object. |

## Constraints

None

## Examples

```c++
auto ge_graph = std::make_unique<ge::Graph>("graph");
auto add_node = ge::es::CompliantNodeBuilder(ge_graph.get()).OpType("Add")
      .Name("add_0")
      .IrDefInputsV2({
          {"x1", ge::es::CompliantNodeBuilder::kEsIrInputRequired, ""},
          {"x2", ge::es::CompliantNodeBuilder::kEsIrInputRequired, ""},
      })
      .IrDefOutputsV2({
          {"y", ge::es::CompliantNodeBuilder::kEsIrOutputRequired, ""},
      })
      .IrDefAttrsV2({
      })
      .Build();
// In the example, create an instance add_0 of the Add operator in the graph through chained calls based on the IR prototype of Add.

```
