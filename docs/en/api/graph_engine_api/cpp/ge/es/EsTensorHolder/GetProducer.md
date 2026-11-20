# GetProducer

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:12:02.849Z pushedAt=2026-10-28T07:40:22.486Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the graph node that produces this tensor.

## Prototype

```c++
GNode *GetProducer() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | GNode * | Returns the graph node pointer. |

## Constraints

None

## Examples

```c++
  auto builder = std::make_unique<ge::EsGraphBuilder>("graph");
  auto tensor0 = builder->CreateInput(0);
  GNode *data0 = tensor0.GetProducer(); // Get the node data0 of input tensor0.
```
