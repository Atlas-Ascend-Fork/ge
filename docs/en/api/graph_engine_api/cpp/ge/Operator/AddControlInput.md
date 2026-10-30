# AddControlInput

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:14:07.818Z pushedAt=2026-10-28T07:42:19.654Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Adds a control edge to the operator. Currently, a control edge only controls the execution order of operators.

## Prototype

```c++
Operator &AddControlInput(const Operator &src_oprt)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| src_oprt | Input | Source operator corresponding to the control edge. |

## Return Value

The operator object itself.

## Constraints

None
