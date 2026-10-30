# SetSubgraphInstanceName

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:29:38.483Z pushedAt=2026-10-28T07:42:19.707Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Sets the name of the subgraph instance.

## Prototype

```c++
graphStatus SetSubgraphInstanceName(const uint32_t index, const char_t *name);
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| index | Input | Subgraph index. |
| name | Input | Name of the subgraph instance. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS on success; otherwise, returns GRAPH\_FAILED.

## Constraints

None
