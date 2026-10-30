# AddControlEdge

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:10:14.086Z pushedAt=2026-10-28T07:40:22.480Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Connects control edges.

## Prototype

```c++
Status AddControlEdge(const std::vector<EsTensorHolder> &ctrl_ins) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ctrl_ins | Input | Control edge input, supporting multiple inputs. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GRAPH_SUCCESS(0)**: Success<br>Other values: Failure |

## Constraints

None

## Examples

```c++
  auto builder = std::make_unique<ge::EsGraphBuilder>("graph");
  auto tensor0 = builder->CreateInput(0);
  auto tensor1 = builder->CreateInput(1);
  auto tensor2 = builder->CreateInput(2);
  std::vector<EsTensorHolder> ctrl_ins = {tensor1, tensor2};
  (void)tensor0.AddControlEdge(ctrl_ins); // The producer node of tensor0 uses the producer nodes of tensor1 and tensor2 as control inputs.
```
