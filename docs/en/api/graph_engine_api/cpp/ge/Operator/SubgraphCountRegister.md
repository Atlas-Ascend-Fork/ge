# SubgraphCountRegister

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:30:44.729Z pushedAt=2026-10-28T07:42:19.709Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Registers subgraphs.

## Prototype

```c++
void SubgraphCountRegister(const char_t *ir_name, uint32_t count)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| ir_name | Input | Subgraph name. |
| count | Input | In a dynamic subgraph count scenario (where the number of subgraphs is not fixed), registers count subgraphs named ir_name_0 through ir_name_n, where n < count. |

## Return Value

None

## Constraints

None
