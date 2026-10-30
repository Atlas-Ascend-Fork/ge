# SetSubgraphBuilder

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:29:34.090Z pushedAt=2026-10-28T07:42:19.706Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Sets the function object for building the specified subgraph.

## Prototype

```c++
void SetSubgraphBuilder(const char_t *ir_name, uint32_t index, const SubgraphBuilder &builder)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_name | Input | Subgraph name. |
| index | Input | In a scenario with a dynamic number of subgraphs (where the number of subgraphs is not fixed), identifies the sequence number of the subgraph. |
| builder | Input | Function object for building the subgraph. |

## Return Value

None

## Constraints

None
